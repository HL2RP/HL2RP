#include <DALEngine.h>
#include <filesystem.h>
#include <IDAO.h>
#include <sqlite3.h>

class CSQLiteDAOException : public CDAOException
{
public:
	CSQLiteDAOException(sqlite3* pConnection, const char* pQueryText);
};

CSQLiteDAOException::CSQLiteDAOException(sqlite3* pConnection, const char* pQueryText)
{
	Warning("SQLite error on query \"%s\": %s [Code: %i]\n", pQueryText,
		sqlite3_errmsg(pConnection), sqlite3_errcode(pConnection));
}

// NOTE: Many SQLite API operations are HARMLESS when passing NULL pointers
class CSQLiteEngine : public CSQLEngine
{
	void DispatchExecute(IDAO* pDAO) OVERRIDE;
	void BeginTxn() OVERRIDE;
	void EndTxn() OVERRIDE;
	bool Connect(const char* pHostName, const char* pUserName,
		const char* pPassText, const char* pSchemaName, int port) OVERRIDE;
	void* PrepareStatement(const char* pQueryText) OVERRIDE;
	void BindInt(void* pStmt, int position, int value) OVERRIDE;
	void BindFloat(void* pStmt, int position, float value) OVERRIDE;
	void BindString(void* pStmt, int position, const char* pValue) OVERRIDE;
	void ExecutePreparedStatement(void* pStmt, const char* pResultName) OVERRIDE;
	void ExecuteQuery(const char* pQueryText, const char* pResultName = NULL) OVERRIDE;

	sqlite3* m_pConnection;

public:
	CSQLiteEngine();

	~CSQLiteEngine();
};

CSQLiteEngine::CSQLiteEngine() : m_pConnection(NULL)
{

}

CSQLiteEngine::~CSQLiteEngine()
{
	sqlite3_close_v2(m_pConnection);
}

void CSQLiteEngine::DispatchExecute(IDAO* pDAO)
{
	pDAO->ExecuteSQLite(this);
}

void CSQLiteEngine::BeginTxn()
{
	CSQLiteEngine::ExecuteQuery("BEGIN TRANSACTION");
}

void CSQLiteEngine::EndTxn()
{
	CSQLiteEngine::ExecuteQuery("COMMIT TRANSACTION");
}

bool CSQLiteEngine::Connect(const char* pHostName, const char* pUserName,
	const char* pPassText, const char* pSchemaName, int port)
{
	if (sqlite3_open(pSchemaName, &m_pConnection) == SQLITE_OK)
	{
		// SQLite foreign keys have to be enabled explicitly
		sqlite3_config(SQLITE_DBCONFIG_ENABLE_FKEY, 1, NULL);

		if (IsRelease())
		{
			// Enable Write Ahead Logging. Makes execution faster and reduces concurrent locking errors.
			// I enabled this in release only since this causes writes to happen on an intermediate file,
			// delaying writes on the main SQLite3 file until N bytes and making it harder to debug the data.
			CSQLiteEngine::ExecuteQuery("PRAGMA journal_mode = WAL;");
		}

		return true;
	}

	Warning("SQLite error: %s [Code: %i]\n", sqlite3_errmsg(m_pConnection),
		sqlite3_errcode(m_pConnection));
	return false;
}

void* CSQLiteEngine::PrepareStatement(const char* pQueryText)
{
	sqlite3_stmt* pStmt;

	if (sqlite3_prepare_v2(m_pConnection, pQueryText, -1, &pStmt, NULL) != SQLITE_OK)
	{
		throw CSQLiteDAOException(m_pConnection, pQueryText);
	}

	return pStmt;
}

void CSQLiteEngine::BindInt(void* pStmt, int position, int value)
{
	sqlite3_stmt* pSQLite3Stmt = reinterpret_cast<sqlite3_stmt*>(pStmt);

	if (sqlite3_bind_int(pSQLite3Stmt, position, value) != SQLITE_OK)
	{
		throw CSQLiteDAOException(m_pConnection, sqlite3_sql(pSQLite3Stmt));
	}
}

void CSQLiteEngine::BindFloat(void* pStmt, int position, float value)
{
	sqlite3_stmt* pSQLite3Stmt = reinterpret_cast<sqlite3_stmt*>(pStmt);

	if (sqlite3_bind_double(pSQLite3Stmt, position, value) != SQLITE_OK)
	{
		throw CSQLiteDAOException(m_pConnection, sqlite3_sql(pSQLite3Stmt));
	}
}

void CSQLiteEngine::BindString(void* pStmt, int position, const char* pValue)
{
	sqlite3_stmt* pSQLite3Stmt = reinterpret_cast<sqlite3_stmt*>(pStmt);

	if (sqlite3_bind_text(pSQLite3Stmt, position, pValue, -1, NULL) != SQLITE_OK)
	{
		throw CSQLiteDAOException(m_pConnection, sqlite3_sql(pSQLite3Stmt));
	}
}

void CSQLiteEngine::ExecutePreparedStatement(void* pStmt, const char* pResultName)
{
	sqlite3_stmt* pSQLite3Stmt = reinterpret_cast<sqlite3_stmt*>(pStmt);

	switch (sqlite3_step(pSQLite3Stmt))
	{
	case SQLITE_OK:
	case SQLITE_DONE:
	{
		break;
	}
	case SQLITE_ROW:
	{
		// Cache result from SQLite, since it accesses disk on each row step.
		// Otherwise these reads would introduce lag on main thread.
		KeyValues* pDestResult = m_CachedResults->FindKey(pResultName, true);

		do
		{
			KeyValues* pRow = pDestResult->CreateNewKey();

			for (int i = 0; i < sqlite3_column_count(pSQLite3Stmt); i++)
			{
				switch (sqlite3_column_type(pSQLite3Stmt, i))
				{
				case SQLITE_INTEGER:
				{
					pRow->SetInt(sqlite3_column_name(pSQLite3Stmt, i),
						sqlite3_column_int(pSQLite3Stmt, i));
					break;
				}
				case SQLITE_FLOAT:
				{
					pRow->SetFloat(sqlite3_column_name(pSQLite3Stmt, i),
						sqlite3_column_double(pSQLite3Stmt, i));
					break;
				}
				case SQLITE3_TEXT:
				{
					pRow->SetString(sqlite3_column_name(pSQLite3Stmt, i),
						(const char*)sqlite3_column_text(pSQLite3Stmt, i));
					break;
				}
				}
			}
		} while (sqlite3_step(pSQLite3Stmt) == SQLITE_ROW);

		break;
	}
	default:
	{
		throw CSQLiteDAOException(m_pConnection, sqlite3_sql(pSQLite3Stmt));
	}
	}

	sqlite3_finalize(pSQLite3Stmt);
}

void CSQLiteEngine::ExecuteQuery(const char* pQueryText, const char* pResultName)
{
	void* pStmt = CSQLiteEngine::PrepareStatement(pQueryText);
	CSQLiteEngine::ExecutePreparedStatement(pStmt, pResultName);
}

EXPOSE_SINGLE_INTERFACE(CSQLiteEngine, CSQLEngine, DALENGINE_SQLITE_IMPL_VERSION_STRING);
