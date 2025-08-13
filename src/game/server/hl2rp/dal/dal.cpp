// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "dal.h"
#include "idao.h"
#include "keyvalues_driver.h"
#include <hl2rp_shareddefs.h>
#include <filesystem.h>
#include <smartptr.h>

// This should approximately match a realistic max. amount of concurrent
// most-derived DAO classes that may be instantiated, for input hash map
#define DAL_PENDING_DAOS_HASH_BUCKETS 16

#define DAL_KEYVALUES_DRIVER_NAME "keyvalues"
#define DAL_SQLITE_DRIVER_NAME    "sqlite"
#define DAL_MYSQL_DRIVER_NAME     "mysql"

#define DAL_DEFAULT_DATABASE_DRIVER     DAL_KEYVALUES_DRIVER_NAME
#define DAL_DEFAULT_DATABASE_NAME       "save"
#define DAL_DEFAULT_MYSQL_DATABASE_HOST "localhost"
#define DAL_DEFAULT_MYSQL_DATABASE_USER "root"
#define DAL_DEFAULT_MYSQL_PORT          3306

CDAL& DAL()
{
	static CDAL sDAL;
	return sDAL;
}

static CDAL& sDAL = DAL(); // Instantiate DAL early at a proper place

abstract_class IDatabaseConnection
{
public:
	IDatabaseConnection(IDatabaseDriver * pDriver) : mpDriver(pDriver) {}

	virtual ~IDatabaseConnection() = 0;

	IDatabaseDriver* mpDriver;
};

IDatabaseConnection::~IDatabaseConnection()
{

}

class CKeyValuesConnection : public IDatabaseConnection
{
	using IDatabaseConnection::IDatabaseConnection;

	~CKeyValuesConnection() OVERRIDE
	{
		delete mpDriver;
	}
};

class CSQLConnection : public IDatabaseConnection
{
	~CSQLConnection() OVERRIDE
	{
		// Close connection immediately, since module may be unloaded later and not close it now
		static_cast<ISQLDriver*>(mpDriver)->Close();

		Sys_UnloadModule(mpModule); // NOTE: The module will delete the driver when unloaded
	}

	CSysModule* mpModule;

public:
	CSQLConnection(ISQLDriver* pDriver, CSysModule* pModule) : IDatabaseConnection(pDriver), mpModule(pModule) {}
};

CDAL::CDAOTypeRef::CDAOTypeRef(IDAO* pDAO) : mType(typeid(*pDAO))
{

}

bool CDAL::CDAOTypeRef::operator==(const CDAOTypeRef& other) const
{
	return (other.mType == mType);
}

const char* CDAL::CDAOTypeRef::Name()
{
	return mType.name();
}

unsigned CDAL::CDAOHashFunctor::operator()(const CDAOTypeRef& typeRef) const
{
	return typeRef.mType.hash_code();
}

CDAL::CHashedDAOList::CHashedDAOList() : mIndicesByType(DAL_PENDING_DAOS_HASH_BUCKETS)
{

}

CDAL::CDAL()
{
	SetName("DALThread");
}

void CDAL::LevelInitPostEntity()
{
	for (auto pLevelInitDAOFactory : mLevelInitDAOFactories)
	{
		AddDAO(pLevelInitDAOFactory->Create());
	}
}

void CDAL::LevelShutdownPostEntity()
{
	AUTO_LOCK(mPendingDAOs.mMutex);

	// Remove eligible pending DAOs, in order to prevent problems like double loads on next level
	FOR_EACH_HASHTABLE(mPendingDAOs.mIndicesByType, i)
	{
		auto& indices = mPendingDAOs.mIndicesByType[i];

		for (int current = indices.Head(), next; indices.IsValidIndex(current); current = next)
		{
			next = indices.Next(current);

			if (mPendingDAOs[indices[current]]->MustBeRemovedOnLevelShutdown())
			{
				delete mPendingDAOs[indices[current]];
				mPendingDAOs.Remove(indices[current]);
				indices.Remove(current);
			}
		}
	}

	if (mPendingDAOs.IsEmpty())
	{
		mMustIOThreadRun = false;
	}
	else if (!mPendingDAOs.IsValidIndex(mOldLevelPendingDAOIndex))
	{
		mOldLevelPendingDAOIndex = mPendingDAOs.Tail();
	}

	// Remove all completed DAOs, which we don't want anymore
	AUTO_LOCK(mCompletedDAOs.mMutex);
	mCompletedDAOs.PurgeAndDeleteElements();
}

void CDAL::FrameUpdatePostEntityThink()
{
	// Check thread doesn't exist, or, in case previous thread is finalizing, wait for it before creating new
	if (!IsAlive() && Join())
	{
		mMustIOThreadRun = true;
		Start();
	}

	// Handle completed DAOs, minimizing extract contention to prevent possible dependency
	// deadlocks with the pending DAO list mutex generated within self completion handling
	for (IDAO* pDAO;;)
	{
		if (!mCompletedDAOs.IsEmpty())
		{
			AUTO_LOCK(mCompletedDAOs.mMutex);
			pDAO = mCompletedDAOs[mCompletedDAOs.Head()];
			mCompletedDAOs.Remove(mCompletedDAOs.Head());
		}
		else
		{
			break;
		}

		pDAO->HandleCompletion();
		delete pDAO;
	}
}

int CDAL::Run()
{
	CPlainAutoPtr<IDatabaseConnection> connection(LoadDatabaseConfiguration());

	// NOTE: After each iteration, we apply a small delay, consistent with the configured tickrate for optimization.
	// Without this delay, main thread preemption can happen under low-end Linux machines, slowing down framerate.
	for (IDAO* pDAO; mMustIOThreadRun; Sleep(TICK_INTERVAL * 1000.0f))
	{
		{
			AUTO_LOCK(mPendingDAOs.mMutex);
			int next = mPendingDAOs.Head();

			if (!mPendingDAOs.IsValidIndex(next))
			{
				continue;
			}
			else if (next >= mOldLevelPendingDAOIndex)
			{
				// We're at last DAO from an old level shutdown. Since database configuration would
				// reload upon level change, we can't process any more DAO with current driver.
				mOldLevelPendingDAOIndex = mPendingDAOs.InvalidIndex();
				mMustIOThreadRun = false;
			}

			pDAO = mPendingDAOs[next];

			// Remove both the hash map index entry and its referenced base list entry
			mPendingDAOs.mIndicesByType.GetPtr(mPendingDAOs[next])->FindAndRemove(next);
			mPendingDAOs.Remove(next);
		}

		try
		{
			connection->mpDriver->DispatchExecuteIO(pDAO);

			// Lock this mutex again to safely determine whether DAO should be added to the completed DAO list
			AUTO_LOCK(mPendingDAOs.mMutex);

			if (mMustIOThreadRun && !mPendingDAOs.IsValidIndex(mOldLevelPendingDAOIndex))
			{
				AUTO_LOCK(mCompletedDAOs.mMutex);
				mCompletedDAOs.AddToTail(pDAO);
				continue;
			}
		}
		catch (const CDatabaseIOException&)
		{
			Warning("DAL::Run: An exception occurred for DAO of '%s', processing failed...\n",
				CDAOTypeRef(pDAO).Name());
		}

		delete pDAO;
	}

	return 0;
}

IDatabaseConnection* CDAL::LoadDatabaseConfiguration()
{
	KeyValuesAD configuration("");
	const char* pDatabaseName = DAL_DEFAULT_DATABASE_NAME;

	if (!configuration->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "database.cfg")
		&& !configuration->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "database_default.cfg"))
	{
		Warning("Failed to load database configuration from file: '%sdatabase.cfg'\n", HL2RP_CONFIG_PATH);
	}
	else
	{
		const char* pDriverName = configuration->GetString("driver");
		pDatabaseName = configuration->GetString("database");
		CSysModule* pModule;
		void* pDriver;

		if (FStrEq(pDatabaseName, ""))
		{
			pDatabaseName = DAL_DEFAULT_DATABASE_NAME;
		}

		if (Q_stricmp(pDriverName, DAL_SQLITE_DRIVER_NAME) == 0)
		{
			// Load the SQLite3 driver module
			if (LoadSQLDriver("sqlite3_driver" DLL_EXT_STRING, SQLITE3_DRIVER_IMPL_VERSION_STRING, pModule, pDriver))
			{
				char databaseName[MAX_PATH], databasePath[MAX_PATH];
				V_strcpy_safe(databaseName, pDatabaseName);
				Q_DefaultExtension(databaseName, ".sqlite3", sizeof(databaseName));
				filesystem->GetSearchPath_safe("GAME_WRITE", false, databasePath);
				Q_MakeAbsolutePath(databasePath, sizeof(databasePath), databaseName, databasePath);
				ISQLDriver* pSQLite3Driver = static_cast<ISQLDriver*>(pDriver);

				if (pSQLite3Driver->Connect(databasePath, NULL, NULL, NULL, 0))
				{
					Msg("Gameplay data will save into SQLite3 database file: '%s'\n", databaseName);
					return (new CSQLConnection(pSQLite3Driver, pModule));
				}

				Sys_UnloadModule(pModule);
				Warning("Failed to create or load SQLite3 database file: '%s'\n", databaseName);
			}
			else
			{
				Warning("Failed to load required SQLite3 driver library under 'bin' directory\n");
			}

			pDatabaseName = DAL_DEFAULT_DATABASE_NAME;
		}
		else if (Q_stricmp(pDriverName, DAL_MYSQL_DRIVER_NAME) == 0)
		{
			// Load the MySQL driver module
			if (LoadSQLDriver("mysql_driver" DLL_EXT_STRING, MYSQL_DRIVER_IMPL_VERSION_STRING, pModule, pDriver))
			{
				ISQLDriver* pMySQLDriver = static_cast<ISQLDriver*>(pDriver);
				const char* pHostName = configuration->GetString("host");
				const char* pUserName = configuration->GetString("user");
				int port = configuration->GetInt("port");

				if (FStrEq(pHostName, ""))
				{
					pHostName = DAL_DEFAULT_MYSQL_DATABASE_HOST;
				}

				if (FStrEq(pUserName, ""))
				{
					pUserName = DAL_DEFAULT_MYSQL_DATABASE_USER;
				}

				if (port < 1)
				{
					port = DAL_DEFAULT_MYSQL_PORT;
				}

				if (pMySQLDriver->Connect(pDatabaseName, pHostName, pUserName, configuration->GetString("pass"), port))
				{
					Msg("Successfully connected to MySQL database '%s' using specified parameters."
						" Gameplay data will save there.\n", pDatabaseName);
					return (new CSQLConnection(pMySQLDriver, pModule));
				}

				Sys_UnloadModule(pModule);
				Warning("Failed to connect to MySQL database '%s' using specified parameters\n", pDatabaseName);
			}
			else
			{
				Warning("Failed to load required MySQL driver library under 'bin' directory\n");
			}

			pDatabaseName = DAL_DEFAULT_DATABASE_NAME;
		}
		else if (!FStrEq(pDriverName, "") && Q_stricmp(pDriverName, DAL_KEYVALUES_DRIVER_NAME) != 0)
		{
			Warning("Failed to connect to unhandled database driver: '%s'. "
				"Check allowed drivers and try again.\n", pDriverName);
		}
	}

	Msg("Gameplay data will save in KeyValues format at directory: '%s'\n", pDatabaseName);
	return (new CKeyValuesConnection(new CKeyValuesDriver(pDatabaseName)));
}

bool CDAL::LoadSQLDriver(const char* pFileName, const char* pInterfaceName, CSysModule*& pModule, void*& pDriver)
{
	char path[MAX_PATH];
	filesystem->RelativePathToFullPath_safe(pFileName, "EXECUTABLE_PATH", path);
	return Sys_LoadInterface(path, pInterfaceName, &pModule, &pDriver);
}

void CDAL::AddDAO(IDAO* pDAO)
{
	AUTO_LOCK(mPendingDAOs.mMutex);
	UtlHashHandle_t handle = mPendingDAOs.mIndicesByType.Insert(pDAO);
	auto& indices = mPendingDAOs.mIndicesByType[handle];

	// Visit pending DAOs belonging to the same type as the new one, in order to replace
	// an eligible one that isn't locked to an old level, for security, with the new DAO
	FOR_EACH_LL(indices, i)
	{
		if ((!mPendingDAOs.IsValidIndex(mOldLevelPendingDAOIndex) || indices[i] > mOldLevelPendingDAOIndex)
			&& pDAO->MergeFrom(mPendingDAOs[indices[i]]))
		{
			delete mPendingDAOs[indices[i]];
			mPendingDAOs[indices[i]] = pDAO;
			return;
		}
	}

	indices.AddToTail(mPendingDAOs.AddToTail(pDAO));
}
