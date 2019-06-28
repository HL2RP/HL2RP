#include <cbase.h>
#include "HL2RPDAL.h"
#include "DispenserDAO.h"
#include <filesystem.h>
#include <HL2RPRules.h>
#include "IDALEngine.h"
#include "DAL/JobDAO.h"
#include "PhraseDAO.h"
#include "PlayerDAO.h"
#include <sqlite3.h>
#include <typeinfo>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define HL2RP_DAL_KEYVALUES_DRIVER_NAME	"keyvalues"
#define HL2RP_DAL_SQLITE_DRIVER_NAME	"sqlite"
#define HL2RP_DAL_MYSQL_DRIVER_NAME	"mysql"
#define HL2RP_DAL_KEYVALUES_DEFAULT_SAVEDIR_NAME	"save"
#define HL2RP_DAL_DATABASE_DEFAULT_SCHEMA	"roleplay"
#define HL2RP_DAL_DATABASE_DEFAULT_DRIVER	HL2RP_DAL_KEYVALUES_DRIVER_NAME
#define HL2RP_DAL_DATABASE_DEFAULT_HOST	"localhost"
#define HL2RP_DAL_DATABASE_DEFAULT_USER	"root"
#define HL2RP_DAL_MYSQL_DEFAULT_PORT	3306

CDAOThread::CDAOThread() : m_WorkerWaitingState(WorkerWaitingState_None), m_pHandlingDAO(NULL)
{
	SetName("CDAOThread");
}

bool CDAOThread::Init()
{
	m_WorkerWaitingState = WorkerWaitingState_ProcessDAOList;
	return true;
}

int CDAOThread::Run()
{
	for (unsigned int request;;)
	{
		// We nest this call here to ensure it doesn't stop the loop regardless of the result
		if (WaitForCall(&request))
		{
			if (request == CallRequest_ProcessDAOList)
			{
				// Reply before processing DAO list, to prevent blocking main thread due to I/O
				m_WorkerWaitingState = WorkerWaitingState_None;
				Reply(CallResponse_None);

				ProcessDAOList();
				Assert(m_WorkerWaitingState == WorkerWaitingState_ProcessDAOList);
			}
			else if (request == ECallRequest::Stop)
			{
				m_WorkerWaitingState = WorkerWaitingState_None;
				Reply(ECallResponse::Stopped);
				break;
			}
			else
			{
				// Unknown request - Fallback just in case, to ensure main thread continuation
				m_WorkerWaitingState = WorkerWaitingState_ProcessDAOList;
				Reply(CallResponse_None);
			}
		}
	}

	return 0;
}

void CDAOThread::InsertDAO(IDAO* pDAO)
{
	Assert(GetHL2RPDALEngine() != NULL);
	AUTO_LOCK(m_DAOListMutex);

	// Start colliding with existing DAOs to optimize out the list
	for (int i = m_DAOList.Tail(), incomingDAOIndex = m_DAOList.AddToTail(pDAO);
		m_DAOList.IsValidIndex(i); i = m_DAOList.Previous(i))
	{
		EDAOCollisionResolution resolution = GetHL2RPDALEngine()->DispatchCollide(m_DAOList[i], pDAO);

		switch (resolution)
		{
		case EDAOCollisionResolution::ReplaceQueued:
		{
			ConDColorMsg(COLOR_GREEN, "DAL Optimization: Replacing %s with %s!\n",
				typeid(*m_DAOList[i]).name(), typeid(*pDAO).name());
			delete m_DAOList[i];
			m_DAOList.Remove(incomingDAOIndex);
			m_DAOList[i] = pDAO;
			incomingDAOIndex = i;
			break;
		}
		case EDAOCollisionResolution::RemoveBothAndStop:
		{
			ConDColorMsg(COLOR_GREEN, "DAL Optimization: Removing %s and %s!\n",
				typeid(*m_DAOList[i]).name(), typeid(*pDAO).name());
			delete m_DAOList[i], pDAO;
			m_DAOList.Remove(incomingDAOIndex);
			return m_DAOList.Remove(i);
		}
		}
	}
}

void CDAOThread::ProcessDAOList()
{
	Assert(GetHL2RPDALEngine() != NULL);

	for (;;)
	{
		{
			// Always protect the shared DAO list (via lock)
			AUTO_LOCK(m_DAOListMutex);

			if (m_DAOList.Count() < 1)
			{
				m_WorkerWaitingState = EWorkerWaitingState::WorkerWaitingState_ProcessDAOList;
				break;
			}

			// Extract next DAO (head) and release the lock, as it's safe already
			int head = m_DAOList.Head();
			m_pHandlingDAO = m_DAOList[head];
			m_DAOList.Remove(head);
		}

		try
		{
			GetHL2RPDALEngine()->DispatchExecute(m_pHandlingDAO);

			// Wait for main thread to process results for concurrency safety
			m_WorkerWaitingState = WorkerWaitingState_AckResultsHandled;

			for (unsigned int request;;)
			{
				// We nest this call here to ensure it doesn't stop the loop regardless of the result
				if (!WaitForCall(&request))
				{
					continue;
				}
				else if (request == CallRequest_AckResultsHandled)
				{
					GetHL2RPDALEngine()->CloseResults();
					m_pHandlingDAO = NULL;
					m_WorkerWaitingState = WorkerWaitingState_None;
					Reply(ECallResponse::ResultsClosed);
					break;
				}

				Reply(CallResponse_None);
			}
		}
		catch (CDAOException& exception)
		{
			Warning("CDAOThread: Blaming '%s' for most recent DAL Engine access error\n",
				typeid(*m_pHandlingDAO).name());
			GetHL2RPDALEngine()->CloseResults();

			if (exception.ShouldRetry())
			{
				// We'll re-attempt this DAO without blocking the rest
				InsertDAO(m_pHandlingDAO);
			}

			m_pHandlingDAO = NULL;
		}
	}
}

void CDAOThread::AddAsyncDAO(IDAO* pDAO)
{
	InsertDAO(pDAO);

	// Check if worker thread awaits refreshing DAO list process
	if (m_WorkerWaitingState == WorkerWaitingState_ProcessDAOList)
	{
		CallWorker(CallRequest_ProcessDAOList);
	}
}

bool CDAOThread::TryHandleDAOResults()
{
	// Check if worker thread awaits handling results in the main thread
	if (m_WorkerWaitingState == WorkerWaitingState_AckResultsHandled)
	{
		Assert(m_pHandlingDAO != NULL);
		GetHL2RPDALEngine()->DispatchHandleResults(m_pHandlingDAO);
		delete m_pHandlingDAO;
		CallWorker(CallRequest_AckResultsHandled);
		return true;
	}

	return false;
}

// Stops the thread, not before fully processing the DAO list
void CDAOThread::ProcessDAOListAndStop()
{
	int daoCount;

	{
		AUTO_LOCK(m_DAOListMutex);
		daoCount = m_DAOList.Count();

		if (daoCount > 0)
		{
			if (m_pHandlingDAO != NULL)
			{
				daoCount++;
			}

			Warning("CDAOThread: Server paused to complete %i pending database operations. "
				"If you shutdown the server, game data will be lost. Please wait...\n", daoCount);
		}
	}

	// Was the thread even started?
	if (IsThreadRunning())
	{
		while (m_WorkerWaitingState != WorkerWaitingState_ProcessDAOList)
		{
			if (TryHandleDAOResults())
			{
				if (--daoCount > 0)
				{
					Msg("CDAOThread: There are now %i pending database operations before resuming the server\n",
						daoCount);
					continue;
				}

				Msg("CDAOThread: All pending database operations have been completed. Resuming server...\n");
			}
		}

		CallWorker(ECallRequest::Stop);
	}
}

CHL2RPDAL::~CHL2RPDAL()
{
	// In presence of a containing module, we must unload it outside the interface,
	// and the interface will be destroyed along it. Closing the module inside the interface is unsafe.
	if (m_EngineRef != NULL && m_EngineRef->GetModule() != NULL)
	{
		Sys_UnloadModule(m_EngineRef->GetModule());
		m_EngineRef = NULL;
	}
}

void CHL2RPDAL::Init()
{
	KeyValuesAD databaseInfo("Roleplay");

	if (!databaseInfo->LoadFromFile(filesystem, "cfg/database.cfg")
		&& !databaseInfo->LoadFromFile(filesystem, "cfg/database_default.cfg"))
	{
		Warning("CHL2RPDAL: Failed to connect to database, no appropiate config file was found\n");
		return;
	}

	const char* pDriverName = databaseInfo->GetString("driver", HL2RP_DAL_DATABASE_DEFAULT_DRIVER);

	if (Q_strcmp(pDriverName, HL2RP_DAL_KEYVALUES_DRIVER_NAME) == 0)
	{
		char savePath[MAX_PATH];
		engine->GetGameDir(savePath, sizeof(savePath));
		Q_MakeAbsolutePath(savePath, sizeof(savePath),
			databaseInfo->GetString("database", HL2RP_DAL_KEYVALUES_DEFAULT_SAVEDIR_NAME), savePath);
		filesystem->AddSearchPath(savePath, DAL_ENGINE_KEYVALUES_SAVE_PATH_ID);
		m_EngineRef = new CKeyValuesEngine;
	}
	else if (Q_strcmp(pDriverName, HL2RP_DAL_SQLITE_DRIVER_NAME) == 0)
	{
		char schemaPath[MAX_PATH];
		engine->GetGameDir(schemaPath, sizeof(schemaPath));
		Q_MakeAbsolutePath(schemaPath, sizeof(schemaPath),
			databaseInfo->GetString("database", HL2RP_DAL_DATABASE_DEFAULT_SCHEMA ".sqlite3"), schemaPath);

		// Start loading SQLite3 Engine dynamically...
		// We load the binary directly from mod's bin dir, to compatibilize path when launching as an SSDK2013 mod.
		char sqliteEnginePath[MAX_PATH];
		filesystem->RelativePathToFullPath_safe("sqlite_engine" DLL_EXT_STRING, "GAMEBIN",
			sqliteEnginePath, FILTER_CULLPACK);
		CSysModule* pSQLiteEngineModule;
		CSQLEngine* pSQLiteEngineIface;

		if (!Sys_LoadInterface(sqliteEnginePath, DAL_ENGINE_SQLITE_IMPL_VERSION_STRING,
			&pSQLiteEngineModule, (void**)& pSQLiteEngineIface))
		{
			return Msg("SQLite3 error: driver was configured to be used but SQLite3 library is"
				" missing under bin directories\n");
		}

		char sqlite3DatabasePath[MAX_PATH];
		engine->GetGameDir(sqlite3DatabasePath, sizeof(sqlite3DatabasePath));
		Q_MakeAbsolutePath(sqlite3DatabasePath, sizeof(sqlite3DatabasePath), schemaPath, sqlite3DatabasePath);

		if (!pSQLiteEngineIface->Connect(NULL, NULL, NULL, sqlite3DatabasePath, 0))
		{
			return Sys_UnloadModule(pSQLiteEngineModule); // This will delete the interface, too
		}

		pSQLiteEngineIface->SetModule(pSQLiteEngineModule);
		m_EngineRef = pSQLiteEngineIface;
	}
	else if (Q_strcmp(pDriverName, HL2RP_DAL_MYSQL_DRIVER_NAME) == 0)
	{
		// Start loading MySQL Engine dynamically...
		// We load the binary directly from mod's bin dir, to compatibilize path when launching as an SSDK2013 mod.
		char mySQLEnginePath[MAX_PATH];
		filesystem->RelativePathToFullPath_safe("mysql_engine" DLL_EXT_STRING, "GAMEBIN",
			mySQLEnginePath, FILTER_CULLPACK);
		CSysModule* pMySQLEngineModule;
		CSQLEngine* pMySQLEngineIface;

		if (!Sys_LoadInterface(mySQLEnginePath, DAL_ENGINE_MYSQL_IMPL_VERSION_STRING,
			&pMySQLEngineModule, (void**)& pMySQLEngineIface))
		{
			return Msg("MySQL error: driver was configured to be used but MySQL Connector library"
				" is missing under bin directories\n");
		}
		else if (!pMySQLEngineIface->Connect(databaseInfo->GetString("host", HL2RP_DAL_DATABASE_DEFAULT_HOST),
			databaseInfo->GetString("user", HL2RP_DAL_DATABASE_DEFAULT_USER), databaseInfo->GetString("pass"),
			databaseInfo->GetString("database", HL2RP_DAL_DATABASE_DEFAULT_SCHEMA),
			databaseInfo->GetInt("port", HL2RP_DAL_MYSQL_DEFAULT_PORT)))
		{
			return Sys_UnloadModule(pMySQLEngineModule); // This will delete the interface, too
		}

		pMySQLEngineIface->SetModule(pMySQLEngineModule);
		m_EngineRef = pMySQLEngineIface;
	}
	else
	{
		return Warning("CHL2RPDAL: Failed to connect to unhandled DAL Engine driver: '%s'."
			" Please, check allowed drivers.\n", pDriverName);
	}

	// Database connection established at this point. Create DAO thread.
	m_DAOThread.Start();

	// Set up tables
	AddAsyncDAO(new CPlayersInitDAO);
	AddAsyncDAO(new CPlayersAmmoInitDAO);
	AddAsyncDAO(new CPlayerWeaponsInitDAO);
	AddAsyncDAO(new CPhrasesInitDAO);
	AddAsyncDAO(new CDispensersInitDAO);
	AddAsyncDAO(new CJobsInitDAO);

	// Load main data. Must after the creations for not throwing SQL exception when tables aren't set.
	AddAsyncDAO(new CPhrasesLoadDAO);
	AddAsyncDAO(new CJobsLoadDAO);
}

// Adds map-related load DAOs
void CHL2RPDAL::LevelInitPostEntity()
{
	TryCreateAsyncDAO<CDispensersLoadDAO>();
}

void CHL2RPDAL::LevelShutdown()
{
	// We must do this before the destructor is called for correct CWorkerThread ending
	m_DAOThread.ProcessDAOListAndStop();
}

void CHL2RPDAL::Think()
{
	m_DAOThread.TryHandleDAOResults();
}
