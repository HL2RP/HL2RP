#include <DALEngine.h>
#include <IDAO.h>
#include <errmsg.h>
#include <mysql.h>
#include <mysqld_error.h>
#include <utlhashtable.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define MYSQL_ENGINE_FALLBACK_ERROR_CODE	-1

// Wraps a MySQL bind to automatically release allocated buffer
class CMySQLBindWrapper
{
public:
	CMySQLBindWrapper();

	~CMySQLBindWrapper();

	MYSQL_BIND m_Bind;
};

CMySQLBindWrapper::CMySQLBindWrapper()
{
	// Initialize this for crash-safety on destructor, as execution allows it to end being invalid
	m_Bind.buffer = NULL;
}

CMySQLBindWrapper::~CMySQLBindWrapper()
{
	if (m_Bind.buffer != NULL)
	{
		MemAlloc_Free(m_Bind.buffer);
	}
}

class CMySQLEngine : public CSQLEngine
{
	void DispatchExecute(IDAO* pDAO) OVERRIDE;
	void CloseResults() OVERRIDE;
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

	void AddStatementBind(void* pStmt, const MYSQL_BIND& bind, int position);

	MYSQL m_Connection;

	// A smart structure to map statements (key) to growable, aligned parameters (value),
	// to be compatible with MySQL API requirements when binding at statement execution
	CUtlHashtable<MYSQL_STMT*, CUtlVector<CMySQLBindWrapper>> m_bindWrappersMap;

public:
	~CMySQLEngine();

	void ClearBindings(MYSQL_STMT* pStmt);
};

class CMySQLDAOException : public CDAOException
{
	bool ShouldRetry() OVERRIDE;

	void PrintDetailedWarning(const char* pErrorMsg, int errorCode);

	int m_iErrorCode;

public:
	CMySQLDAOException(MYSQL& connection);
	CMySQLDAOException(MYSQL_STMT* pStmt, CMySQLEngine* pMySQLEngine);
	CMySQLDAOException(const MYSQL_BIND& bind, const char* pWarning, ...);
};

CMySQLDAOException::CMySQLDAOException(MYSQL& connection) : m_iErrorCode(mysql_errno(&connection))
{
	PrintDetailedWarning(mysql_error(&connection), m_iErrorCode);
}

CMySQLDAOException::CMySQLDAOException(MYSQL_STMT* pStmt, CMySQLEngine* pMySQLEngine)
	: m_iErrorCode(mysql_stmt_errno(pStmt))
{
	pMySQLEngine->ClearBindings(pStmt);
	PrintDetailedWarning(mysql_stmt_error(pStmt), m_iErrorCode);
	mysql_stmt_close(pStmt);
}

CMySQLDAOException::CMySQLDAOException(const MYSQL_BIND& bind, const char* pWarning, ...)
	: m_iErrorCode(MYSQL_ENGINE_FALLBACK_ERROR_CODE)
{
	// Wrap the MYSQL_BIND to automatically release allocated buffer on exit
	CMySQLBindWrapper bindWrapper;
	bindWrapper.m_Bind = bind;

	va_list args;
	va_start(args, pWarning);
	WarningV(pWarning, args);
	va_end(args);
}

bool CMySQLDAOException::ShouldRetry()
{
	// Give valid operations that failed due to network another chance
	switch (m_iErrorCode)
	{
	case CR_SERVER_LOST:
	case CR_SERVER_GONE_ERROR:
	{
		return true;
	}
	}

	return false;
}

FORCEINLINE void CMySQLDAOException::PrintDetailedWarning(const char* pErrorMsg, int errorCode)
{
	Warning("MySQL error: %s [Code: %i]\n", pErrorMsg, errorCode);
}

void CMySQLEngine::DispatchExecute(IDAO* pDAO)
{
	pDAO->ExecuteMySQL(this);
}

void CMySQLEngine::CloseResults()
{
	CSQLEngine::CloseResults();
	m_bindWrappersMap.RemoveAll();
}

void CMySQLEngine::BeginTxn()
{
	CMySQLEngine::ExecuteQuery("START TRANSACTION;");
}

void CMySQLEngine::EndTxn()
{
	CMySQLEngine::ExecuteQuery("COMMIT;");
}

CMySQLEngine::~CMySQLEngine()
{
	mysql_close(&m_Connection);
}

bool CMySQLEngine::Connect(const char* pHostName, const char* pUserName,
	const char* pPassText, const char* pSchemaName, int port)
{
	mysql_init(&m_Connection);
	bool shouldAutoReconnect = true;
	mysql_options(&m_Connection, MYSQL_OPT_RECONNECT, &shouldAutoReconnect);

	if (mysql_real_connect(&m_Connection, pHostName, pUserName, pPassText,
		pSchemaName, port, NULL, 0))
	{
		return true;
	}

	CMySQLDAOException e(m_Connection);
	return false;
}

void* CMySQLEngine::PrepareStatement(const char* pQueryText)
{
	MYSQL_STMT* pStmt = mysql_stmt_init(&m_Connection);

	if (pStmt != NULL)
	{
		if (mysql_stmt_prepare(pStmt, pQueryText, Q_strlen(pQueryText)) == 0)
		{
			return pStmt;
		}

		throw CMySQLDAOException(pStmt, this);
	}

	throw CDAOException();
}

void CMySQLEngine::BindInt(void* pStmt, int position, int value)
{
	MYSQL_BIND param{}; // Already sets some required members fine
	param.buffer_type = MYSQL_TYPE_LONG;
	param.buffer = new int(value);
	AddStatementBind(pStmt, param, position);
}

void CMySQLEngine::BindFloat(void* pStmt, int position, float value)
{
	MYSQL_BIND param{}; // Already sets some required members fine
	param.buffer_type = MYSQL_TYPE_FLOAT;
	param.buffer = new float(value);
	AddStatementBind(pStmt, param, position);
}

void CMySQLEngine::BindString(void* pStmt, int position, const char* pValue)
{
	MYSQL_BIND param{}; // Already sets some required members fine
	param.buffer_type = MYSQL_TYPE_STRING;
	param.buffer_length = Q_strlen(pValue);
	param.buffer = MemAlloc_Alloc(param.buffer_length);
	Q_memcpy(param.buffer, pValue, param.buffer_length); // The string is not terminated, but it's valid to bind
	AddStatementBind(pStmt, param, position);
}

void CMySQLEngine::ExecutePreparedStatement(void* pStmt, const char* pResultName)
{
	MYSQL_STMT* pMySQLStmt = static_cast<MYSQL_STMT*>(pStmt);
	UtlHashHandle_t handle = m_bindWrappersMap.Find(pMySQLStmt);

	if (m_bindWrappersMap.IsValidHandle(handle))
	{
		Assert(!m_bindWrappersMap[handle].IsEmpty());

		if (mysql_stmt_bind_param(pMySQLStmt, &m_bindWrappersMap[handle][0].m_Bind) != 0)
		{
			throw CMySQLDAOException(pMySQLStmt, this);
		}
	}

	if (mysql_stmt_execute(pMySQLStmt) != 0)
	{
		throw CMySQLDAOException(pMySQLStmt, this);
	}

	m_bindWrappersMap.Remove(pMySQLStmt);
	unsigned int numFields = mysql_stmt_field_count(pMySQLStmt);
	CUtlVector<MYSQL_BIND> outParamList;
	CUtlStringList bufferList;

	if (numFields < 1)
	{
		mysql_stmt_close(pMySQLStmt);
		return;
	}

	// Set up metadata for output results. With RAII for automatic char buffer destruction.
	for (unsigned int i = 0; i < numFields; i++)
	{
		int j = outParamList.AddToTail();
		Q_memset(&outParamList[j], 0, sizeof(outParamList[j]));
		outParamList[j].buffer_type = pMySQLStmt->fields[i].type;
		outParamList[j].buffer_length = pMySQLStmt->fields[i].length;
		outParamList[j].length = &outParamList[j].buffer_length;
		outParamList[j].buffer = new char[outParamList[j].buffer_length];
		bufferList.AddToTail((char*)outParamList[j].buffer);
	}

	if (mysql_stmt_bind_result(pMySQLStmt, &outParamList[0]) != 0)
	{
		throw CMySQLDAOException(pMySQLStmt, this);
	}

	if (mysql_stmt_store_result(pMySQLStmt) == 0 && mysql_stmt_num_rows(pMySQLStmt) > 0)
	{
		KeyValues* pDestResult = m_CachedResults->FindKey(pResultName, true);

		while (mysql_stmt_fetch(pMySQLStmt) == 0)
		{
			KeyValues* pDestRow = pDestResult->CreateNewKey();

			for (unsigned int i = 0; i < numFields; i++)
			{
				// NOTE: Be very explicit about the type sizes, to enforce consistency with docs
				switch (outParamList[i].buffer_type)
				{
				case MYSQL_TYPE_LONG:
				{
					int32*& pValue = (int32 * &)outParamList[i].buffer;
					pDestRow->SetInt(pMySQLStmt->fields[i].name, *pValue);
					break;
				}
				case MYSQL_TYPE_FLOAT:
				{
					float32*& pValue = (float32 * &)outParamList[i].buffer;
					pDestRow->SetFloat(pMySQLStmt->fields[i].name, *pValue);
					break;
				}
				case MYSQL_TYPE_LONGLONG:
				case MYSQL_TYPE_DOUBLE:
				{
					float64*& pValue = (float64 * &)outParamList[i].buffer;
					pDestRow->SetFloat(pMySQLStmt->fields[i].name, *pValue);
					break;
				}
				case MYSQL_TYPE_STRING:
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_VARCHAR:
				{
					const char*& pValue = (const char*&)outParamList[i].buffer;
					pDestRow->SetString(pMySQLStmt->fields[i].name, pValue);
					break;
				}
				}
			}
		}
	}

	mysql_stmt_close(pMySQLStmt);
}

void CMySQLEngine::ExecuteQuery(const char* pQueryText, const char* pResultName)
{
	if (mysql_real_query(&m_Connection, pQueryText, Q_strlen(pQueryText)) != 0)
	{
		throw CMySQLDAOException(m_Connection);
	}

	MYSQL_RES* pResult = mysql_store_result(&m_Connection);

	if (pResult != NULL)
	{
		if (mysql_num_rows(pResult) > 0)
		{
			KeyValues* pDestResult = m_CachedResults->FindKey(pResultName, true);
			unsigned int numFields = mysql_num_fields(pResult);

			for (MYSQL_ROW srcRow; (srcRow = mysql_fetch_row(pResult)) != NULL;)
			{
				KeyValues* pDestRow = pDestResult->CreateNewKey();

				for (unsigned int i = 0; i < numFields; i++)
				{
					pDestRow->SetString(pResult->fields[i].name, srcRow[i]);
				}
			}
		}

		mysql_free_result(pResult);
	}
}

void CMySQLEngine::AddStatementBind(void* pStmt, const MYSQL_BIND& bind, int position)
{
	MYSQL_STMT* pMySQLStmt = static_cast<MYSQL_STMT*>(pStmt);
	int count = mysql_stmt_param_count(pMySQLStmt);

	if (position > 0)
	{
		if (count >= position)
		{
			UtlHashHandle_t handle = m_bindWrappersMap.Find(pMySQLStmt);
			int index = position - 1;

			if (!m_bindWrappersMap.IsValidHandle(handle))
			{
				handle = m_bindWrappersMap.Insert(pMySQLStmt);
			}

			// If a bind wrapper was already at given slot, release it
			if (m_bindWrappersMap[handle].IsValidIndex(index))
			{
				m_bindWrappersMap[handle].Remove(index);
			}
			else
			{
				m_bindWrappersMap[handle].EnsureCount(position);
			}

			m_bindWrappersMap[handle][index].m_Bind = bind;
			return;
		}

		throw CMySQLDAOException(bind, "MySQL error: Got a position (%i) higher than the number of"
			" parameter markers (%i)\n", position, count);
	}

	throw CMySQLDAOException(bind, "MySQL error: Got an invalid position (%i) while binding a"
		" prepared statement. Positions should start from 1.\n", position);
}

void CMySQLEngine::ClearBindings(MYSQL_STMT* pStmt)
{
	m_bindWrappersMap.Remove(pStmt);
}

EXPOSE_SINGLE_INTERFACE(CMySQLEngine, CSQLEngine, DALENGINE_MYSQL_IMPL_VERSION_STRING);
