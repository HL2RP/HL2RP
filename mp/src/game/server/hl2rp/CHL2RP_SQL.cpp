#include "cbase.h"
#include "CHL2RP.h"
#include "sqlite3.h"
#include "filesystem.h"
#include "CHL2RP_Player.h"
#include "CHL2RP_Phrases.h"
#include "CPropRationDispenser.h"
#define mysqlcppconn_EXPORTS // Allow dynamic linking or loading MySQL/Connector, depending on how it's instanced
#pragma warning(disable:4251) // MySQL/Connector bug
#include "tier0/valve_minmax_off.h"	// GCC 4.2.2 headers screw up our min/max defs.
#include "cppconn/driver.h"
#include "tier0/valve_minmax_on.h"	// GCC 4.2.2 headers screw up our min/max defs.
#include "cppconn/prepared_statement.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Note: MySQL ignores CHECK constraint as of date

#define DEFAULT_DATABASE_SCHEMA "roleplay"

const char SQLITE_DRIVER_NAME[] = "sqlite";
const char MYSQL_DRIVER_NAME[] = "mysql";
const char DEFAULT_DATABASE_HOST[] = "localhost";
const char DEFAULT_DATABASE_USER[] = "root";
const char *const DEFAULT_DATABASE_DRIVER = SQLITE_DRIVER_NAME;
const int DEFAULT_MYSQL_PORT = 3306;

const int MAX_SQL_QUERY_LENGTH = 2048;

CAsyncSQLTxn::CAsyncSQLTxn(const char *pszFirstQuery)
{
	AddQuery(pszFirstQuery);
}

void CAsyncSQLTxn::AddQuery(const char *pszQuery)
{
	m_QueryList.CopyAndAddToTail(pszQuery);
}

void CAsyncSQLTxn::AddFormattedQuery(const char *pszQueryFormat, ...)
{
	va_list queryFormatArgs;
	va_start(queryFormatArgs, pszQueryFormat);
	char *pszFinalQuery = new char[MAX_SQL_QUERY_LENGTH];
	Q_vsnprintf(pszFinalQuery, MAX_SQL_QUERY_LENGTH, pszQueryFormat, queryFormatArgs);
	m_QueryList.AddToTail(pszFinalQuery);
	va_end(queryFormatArgs);
}

// Purpose: Fallback to the generic parameter binder when this function is not overridden
void CAsyncSQLTxn::BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const
{
	BindParametersGeneric(iQueryId, pStmt);
}

// Purpose: Fallback to the generic parameter binder when this function is not overridden
void CAsyncSQLTxn::BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const
{
	BindParametersGeneric(iQueryId, pStmt);
}

CHL2RP_SQL::CHL2RP_SQL() : m_pHeadResultHandlingTxn(NULL)
{

}

void CHL2RP_SQL::Init()
{
	KeyValues &databasesKV = *new KeyValues("Databases");
	databasesKV.LoadFromFile(filesystem, "cfg/databases.cfg");
	KeyValues &roleplayDbKV = *databasesKV.FindKey("Roleplay", true);

	if (!Q_strcmp(roleplayDbKV.GetString("driver", DEFAULT_DATABASE_DRIVER), SQLITE_DRIVER_NAME))
	{
		sqlite3 *pConnection;
		char databasePath[MAX_PATH];
		engine->GetGameDir(databasePath, sizeof databasePath);
		int databasePathLen = Q_strlen(databasePath);
		Q_snprintf(&databasePath[databasePathLen], sizeof databasePath - databasePathLen, "/%s",
			roleplayDbKV.GetString("database", DEFAULT_DATABASE_SCHEMA ".sqlite3"));

		if (sqlite3_open(databasePath, &pConnection) == SQLITE_OK)
		{
			CHL2RP::s_pSQL = new CSQLite(pConnection);
			// SQLite foreign keys have to be enabled explicitly:
			CHL2RP::s_pSQL->ExecuteRawQuery("PRAGMA FOREIGN_KEYS = ON;");
			// Allow permanent journal files, more efficiency and higher concurrency (less prone to locking errors)
			CHL2RP::s_pSQL->ExecuteRawQuery("PRAGMA journal_mode = WAL;");
		}
		else
		{
			databasesKV.deleteThis();
			Msg("SQLite error: %s [Code: %i]\n", sqlite3_errmsg(pConnection), sqlite3_errcode(pConnection));
			return;
		}
	}
	else if (!Q_strcmp(roleplayDbKV.GetString("driver", DEFAULT_DATABASE_DRIVER), MYSQL_DRIVER_NAME))
	{
		// Start loading MySQLCPPConn dynamically...
		char *pszMySQLCPPConnFileName;

#ifdef WIN32
		pszMySQLCPPConnFileName = "mysqlcppconn";
#else
		pszMySQLCPPConnFileName = "libmysqlcppconn";
#endif

		// Search driver first on working directory binary folder, then on gamedir binary folder
		CSysModule *pMySQLCPPConn = Sys_LoadModule(pszMySQLCPPConnFileName);

		if (pMySQLCPPConn == NULL)
		{
			char szMySQLCPPConnPath[MAX_PATH];
			engine->GetGameDir(szMySQLCPPConnPath, sizeof szMySQLCPPConnPath);
			int mySQLCPPConnPathLen = Q_strlen(szMySQLCPPConnPath);
			Q_snprintf(&szMySQLCPPConnPath[mySQLCPPConnPathLen], sizeof szMySQLCPPConnPath - mySQLCPPConnPathLen, "/bin/%s", pszMySQLCPPConnFileName);
			pMySQLCPPConn = Sys_LoadModule(szMySQLCPPConnPath);
		}

		InstantiateInterfaceFn pGetDriverInstanceFn = (InstantiateInterfaceFn)Sys_GetProcAddress(pMySQLCPPConn, "get_driver_instance");

		if (pGetDriverInstanceFn != NULL)
		{
			Driver &driver = *(Driver *)pGetDriverInstanceFn();

			try
			{
				ConnectOptionsMap connectionOptions;
				connectionOptions["hostName"] = roleplayDbKV.GetString("host", DEFAULT_DATABASE_HOST);
				connectionOptions["userName"] = roleplayDbKV.GetString("user", DEFAULT_DATABASE_USER);
				connectionOptions["password"] = roleplayDbKV.GetString("pass");
				connectionOptions["schema"] = roleplayDbKV.GetString("database", DEFAULT_DATABASE_SCHEMA);
				connectionOptions["port"] = roleplayDbKV.GetInt("port", DEFAULT_MYSQL_PORT);
				connectionOptions["OPT_RECONNECT"] = true;
				Connection *pConnection = driver.connect(connectionOptions);
				CHL2RP::s_pSQL = new CMySQL(pConnection);
			}
			catch (SQLException &e)
			{
				Msg("MySQL error: %s [Code: %i] [State: %s]\n", e.what(), e.getErrorCode(), e.getSQLStateCStr());
				databasesKV.deleteThis();
				return;
			}
		}
		else
		{
			Sys_UnloadModule(pMySQLCPPConn);
			databasesKV.deleteThis();
			Msg("MySQL error: driver was configured to be used but MySQL Connector library is missing under root bin directory\n");
			return;
		}
	}
	else
	{
		databasesKV.deleteThis();
		Msg("Failed to connect to unhandled SQL driver: %s. Allowed drivers: %s, %s\n", roleplayDbKV.GetString("driver"), SQLITE_DRIVER_NAME, MYSQL_DRIVER_NAME);
		return;
	}

	databasesKV.deleteThis();

	// Database connection established at this point. Create transactions thread, only once for performance.
	ThreadId_t threadId;
	CreateSimpleThread(ProcessTxns, CHL2RP::s_pSQL, &threadId);
	ThreadSetDebugName(threadId, "ProcessTransactions");

	// Start setting up tables...
	CHL2RP::s_pSQL->AddAsyncTxn(new CSetUpPlayersTxn);
	CHL2RP::s_pSQL->AddAsyncTxn(new CSetUpPlayersWeaponsTxn);
	CHL2RP::s_pSQL->AddAsyncTxn(new CSetUpPlayersAmmoTxn);
	CHL2RP::s_pSQL->AddAsyncTxn(new CSetUpPhrasesTxn);
	CHL2RP::s_pSQL->AddAsyncTxn(new CSetUpDispensersTxn);

#ifdef HL2DM_RP
	CHL2RP::s_pSQL->AddAsyncTxn(new CSetUpCompatAnimModelsTxn());
#endif

	// Load transactions should be after the creations for not failing in case the tables didn't exist
	CHL2RP::s_pSQL->AddAsyncTxn(new CLoadPhrasesTxn());

#ifdef HL2DM_RP
	CHL2RP::s_pSQL->AddAsyncTxn(new CLoadCompatAnimModelsTxn());
#endif
}

// Purpose: Add map related load transactions
void CHL2RP_SQL::ServerActivate()
{
	if (CHL2RP::s_pSQL != NULL)
	{
		CHL2RP::s_pSQL->AddAsyncTxn(new CLoadDispensersTxn());
	}
}

void CHL2RP_SQL::Think()
{
	if ((CHL2RP::s_pSQL != NULL) && (CHL2RP::s_pSQL->m_pHeadResultHandlingTxn != NULL))
	{
		CHL2RP::s_pSQL->m_pHeadResultHandlingTxn->HandleQueryResults();
		CHL2RP::s_pSQL->m_pHeadResultHandlingTxn = NULL;
	}
}

void CHL2RP_SQL::AddAsyncTxn(CAsyncSQLTxn *pTxn)
{
	Assert(pTxn != NULL);
	CAsyncSQLTxn &txn = *pTxn;

	// Call this reused function here instead of within base transaction constructor, since polymorphic driver function wouldn't work there yet
	CHL2RP::s_pSQL->BuildTxnQueries(txn);

	m_TxnsMutex.Lock(); // Head transaction may be being selected by the processor

	// Start colliding with existing transactions to optimize out the queue...
	for (UtlPtrLinkedListIndex_t i = m_Txns.Head(); m_Txns.IsValidIndex(i); i = m_Txns.Next(i))
	{
		if (m_Txns[i]->ShouldBeReplacedBy(txn))
		{
			DevMsg("Replacing an enqueued %s with an incoming %s transaction\n", typeid(*m_Txns[i]).name(), typeid(txn).name());
			m_Txns.InsertBefore(i, pTxn);
		}
		else if (m_Txns[i]->ShouldRemoveBoth(txn))
		{
			DevMsg("Removing both an enqueued %s and an incoming %s transaction\n", typeid(*m_Txns[i]).name(), typeid(txn).name());
			delete pTxn;
		}
		else
		{
			continue;
		}

		// At this point the enqueued transaction must be removed, apply and exit
		delete m_Txns[i];
		m_Txns.Remove(i);
		m_TxnsMutex.Unlock();
		return;
	}

	// No removal collision was signaled, append it to the list normally
	m_Txns.AddToTail(pTxn);
	m_TxnsMutex.Unlock();
}

unsigned CHL2RP_SQL::ProcessTxns(void *pSQL)
{
	Assert(pSQL != NULL);
	CHL2RP_SQL &sql = *(CHL2RP_SQL *)pSQL;

	while (true)
	{
		sql.m_TxnsMutex.Lock(); // A transaction may be being inserted, replaced or removed
		UtlPtrLinkedListIndex_t head = sql.m_Txns.Head();

		if (!sql.m_Txns.IsValidIndex(head))
		{
			sql.m_TxnsMutex.Unlock();
			continue;
		}

		CAsyncSQLTxn &txn = *sql.m_Txns[head];
		sql.m_Txns.Remove(head);
		sql.m_TxnsMutex.Unlock();

		// It only makes sense to create a driver transaction when there are multiple queries
		if (txn.m_QueryList.Size() > 1)
		{
			sql.ExecuteRawQuery("BEGIN;");
		}

		// Stop when any query fails. Suitable if no deferred integrity checks would be needed
		for (int i = 0; i < txn.m_QueryList.Size() && sql.ExecuteTxnQuery(txn, i); i++)
		{
			sql.m_pHeadResultHandlingTxn = &txn;

			while (sql.m_pHeadResultHandlingTxn != NULL)
			{
				// Waiting to process results in main thread...
			}

			sql.CloseResults();
		}

		// If driver transaction was started, explicit commit is needed. Otherwise single query is auto commited
		if (txn.m_QueryList.Size() > 1)
		{
			sql.ExecuteRawQuery("COMMIT;");
		}

		delete &txn;
	}

	ReleaseThreadHandle(ThreadGetCurrentHandle());
	return 0;
}

CSQLite::CSQLite(sqlite3 *pConnection) : m_pConnection(pConnection), m_pGlobalResults(NULL), m_pGlobalCurrentResult(NULL)
{

}

void CSQLite::BuildTxnQueries(CAsyncSQLTxn &txn) const
{
	txn.BuildSQLiteQueries();
}

bool CSQLite::ExecuteTxnQuery(CAsyncSQLTxn &txn, int queryId)
{
	sqlite3_stmt *pStmt;

	if (sqlite3_prepare_v2(m_pConnection, txn.m_QueryList[queryId], -1, &pStmt, NULL) == SQLITE_OK)
	{
		txn.BindSQLiteParameters(queryId, pStmt);

		switch (sqlite3_step(pStmt))
		{
		case SQLITE_OK:
		case SQLITE_DONE:
		{
			sqlite3_finalize(pStmt);
			return true;
		}
		case SQLITE_ROW:
		{
			// Cache result from SQLite, since it accesses disk on each row step.
			// Otherwise these reads would introduce lag on main thread
			m_pGlobalResults = new KeyValues("Results");

			do
			{
				KeyValues *rowKV = m_pGlobalResults->CreateNewKey();

				for (int i = 0; i < sqlite3_column_count(pStmt); i++)
				{
					switch (sqlite3_column_type(pStmt, i))
					{
					case SQLITE_INTEGER:
					{
						rowKV->SetInt(sqlite3_column_name(pStmt, i), sqlite3_column_int(pStmt, i));
						break;
					}
					case SQLITE_FLOAT:
					{
						rowKV->SetFloat(sqlite3_column_name(pStmt, i), sqlite3_column_double(pStmt, i));
						break;
					}
					case SQLITE_TEXT:
					{
						rowKV->SetString(sqlite3_column_name(pStmt, i), (const char *)sqlite3_column_text(pStmt, i));
						break;
					}
					}
				}
			} while (sqlite3_step(pStmt) == SQLITE_ROW);

			sqlite3_finalize(pStmt);
			return true;
		}
		}

		sqlite3_finalize(pStmt);
	}

	Msg("SQLite error on query \"%s\": %s [Code: %i]\n", txn.m_QueryList[queryId], sqlite3_errmsg(m_pConnection), sqlite3_errcode(m_pConnection));
	return false;
}

void CSQLite::ExecuteRawQuery(const char *pszQuery) const
{
	sqlite3_stmt *pStmt;

	if (sqlite3_prepare_v2(m_pConnection, pszQuery, -1, &pStmt, NULL) == SQLITE_OK)
	{
		int result = sqlite3_step(pStmt);
		sqlite3_finalize(pStmt);

		switch (result)
		{
		case SQLITE_OK:
		case SQLITE_ROW:
		case SQLITE_DONE:
		{
			return; // Correct
		}
		}
	}

	Msg("SQLite error on query \"%s\": %s [Code: %i]\n", pszQuery, sqlite3_errmsg(m_pConnection), sqlite3_errcode(m_pConnection));
}

bool CSQLite::FetchNextRow()
{
	if (m_pGlobalResults != NULL)
	{
		if (m_pGlobalCurrentResult != NULL)
		{
			m_pGlobalCurrentResult = m_pGlobalCurrentResult->GetNextKey();
		}
		else
		{
			m_pGlobalCurrentResult = m_pGlobalResults->GetFirstSubKey();
		}

		return (m_pGlobalCurrentResult != NULL);
	}

	return false;
}

int CSQLite::FetchInt(const char *pszName) const
{
	return m_pGlobalCurrentResult->GetInt(pszName);
}

float CSQLite::FetchFloat(const char *pszName) const
{
	return m_pGlobalCurrentResult->GetFloat(pszName);
}

void CSQLite::FetchString(const char *pszName, char *psDest, int iMaxLen) const
{
	Q_strncpy(psDest, m_pGlobalCurrentResult->GetString(pszName), iMaxLen);
}

void CSQLite::CloseResults()
{
	if (m_pGlobalResults != NULL)
	{
		m_pGlobalResults->deleteThis();
		m_pGlobalResults = m_pGlobalCurrentResult = NULL;
	}
}

void CSQLite::BindInt(const void *pStmt, int iValue, int iPosition) const
{
	sqlite3_bind_int((sqlite3_stmt *)pStmt, iPosition, iValue);
}

void CSQLite::BindFloat(const void *pStmt, float flValue, int iPosition) const
{
	sqlite3_bind_double((sqlite3_stmt *)pStmt, iPosition, flValue);
}

void CSQLite::BindString(const void *pStmt, const char *pszValue, int iPosition) const
{
	sqlite3_bind_text((sqlite3_stmt *)pStmt, iPosition, pszValue, -1, NULL);
}

void DoMySQLColumnError(const char *pszColumnName, const SQLException &e)
{
	Error("MySQL error fetching column '%s': %s [Code: %i] [State: %s]\n", pszColumnName, e.what(), e.getErrorCode(), e.getSQLStateCStr());
}

CMySQL::CMySQL(Connection *pConnection) : m_pConnection(pConnection), m_pGlobalResults(NULL)
{

}

void CMySQL::BuildTxnQueries(CAsyncSQLTxn &txn) const
{
	txn.BuildMySQLQueries();
}

bool CMySQL::ExecuteTxnQuery(CAsyncSQLTxn &txn, int queryId)
{
	Statement *pStmt = NULL;

	try
	{
		if (txn.ShouldUsePreparedStatements())
		{
			// Automatic reconnection after timeout does not happen for prepared statements, even when OPT_RECONNECT flag is true.
			// Prevent it by attempting to reconnect first
			m_pConnection->reconnect();

			pStmt = m_pConnection->prepareStatement(txn.m_QueryList[queryId]);
			PreparedStatement *pPrepStmt = static_cast<PreparedStatement *>(pStmt);
			txn.BindMySQLParameters(queryId, pPrepStmt);
			pPrepStmt->execute();
		}
		else
		{
			pStmt = m_pConnection->createStatement();
			pStmt->execute(txn.m_QueryList[queryId]);
		}

		m_pGlobalResults = pStmt->getResultSet();
		delete pStmt;
		return true; // Delayed results closing, wait until handled on main thread
	}
	catch (SQLException &e)
	{
		Msg("MySQL error on query \"%s\": %s [Code: %i] [State: %s]\n", txn.m_QueryList[queryId], e.what(), e.getErrorCode(), e.getSQLStateCStr());
		delete pStmt;
		return false;
	}
}

void CMySQL::ExecuteRawQuery(const char *pszQuery) const
{
	Statement *stmt = m_pConnection->createStatement();

	try
	{
		stmt->execute(pszQuery);
		delete stmt->getResultSet();
		delete stmt;
	}
	catch (SQLException &e)
	{
		Msg("MySQL error on query \"%s\": %s [Code: %i] [State: %s]\n", pszQuery, e.what(), e.getErrorCode(), e.getSQLStateCStr());
		delete stmt;
	}
}

bool CMySQL::FetchNextRow()
{
	return (m_pGlobalResults != NULL && m_pGlobalResults->next());
}

int CMySQL::FetchInt(const char *pszName) const
{
	try
	{
		return (m_pGlobalResults->getInt(pszName));
	}
	catch (SQLException &e)
	{
		DoMySQLColumnError(pszName, e);
	}

	return 0;
}

float CMySQL::FetchFloat(const char *pszName) const
{
	try
	{
		return m_pGlobalResults->getDouble(pszName);
	}
	catch (SQLException &e)
	{
		DoMySQLColumnError(pszName, e);
	}

	return 0.0f;
}

void CMySQL::FetchString(const char *pszName, char *psDest, int iMaxLen) const
{
	try
	{
		m_pGlobalResults->getBlob(pszName)->getline(psDest, iMaxLen);
	}
	catch (SQLException &e)
	{
		DoMySQLColumnError(pszName, e);
	}
}

void CMySQL::CloseResults()
{
	delete m_pGlobalResults;
	m_pGlobalResults = NULL;
}

void CMySQL::BindInt(const void *pStmt, int iValue, int iPosition) const
{
	((PreparedStatement *)pStmt)->setInt(iPosition, iValue);
}

void CMySQL::BindFloat(const void *pStmt, float flValue, int iPosition) const
{
	((PreparedStatement *)pStmt)->setDouble(iPosition, flValue);
}

void CMySQL::BindString(const void *pStmt, const char *pszValue, int iPosition) const
{
	((PreparedStatement *)pStmt)->setString(iPosition, pszValue);
}
