#ifndef DALENGINE_H
#define DALENGINE_H
#pragma once

#include "IDAO.h"
#include <KeyValues.h>

#define DALENGINE_KEYVALUES_SAVE_PATH_ID	"KEYVALUES_SAVE"
#define HL2RP_DAL_SQL_ENGINE_QUERY_SIZE	2048
#define DALENGINE_MYSQL_IMPL_VERSION_STRING	"MySQLStorage001"
#define DALENGINE_SQLITE_IMPL_VERSION_STRING	"SQLiteStorage001"

class CDALEngine
{
public:
	virtual ~CDALEngine() { }

	virtual CSysModule* GetModule();
	virtual void DispatchExecute(IDAO* pDAO) = 0;
	virtual void DispatchHandleResults(IDAO* pDAO) = 0;
	virtual void CloseResults();

protected:
	CDALEngine();

	// Dedicated thread code caches I/O disk results here, main thread reads them later
	KeyValuesAD m_CachedResults;
};

FORCEINLINE CDALEngine::CDALEngine() : m_CachedResults("ResultSet")
{

}

FORCEINLINE CSysModule* CDALEngine::GetModule()
{
	return NULL;
}

FORCEINLINE void CDALEngine::CloseResults()
{
	m_CachedResults->Clear();
}

class CKeyValuesEngine : public CDALEngine
{
	void DispatchExecute(IDAO* pDAO) OVERRIDE;
	void DispatchHandleResults(IDAO* pDAO) OVERRIDE;

public:
	// Opens database file contained within user-configured directory,
	// then adds it as a result key to the internal result set.
	// Throws CDAOException if the resource on the final target path is a directory.
	// Input: pResultName - Name of result key, that must be valid when a new result is created.
	// Input: forceCreateKey - Create a result key regardless of file load result (first-time writing).
	KeyValues* LoadFromFile(const char* pFileName, const char* pResultName,
		bool forceCreateResult = false);

	// Saves to database file contained within user-configured directory.
	// Throws CDAOException if the data couldn't be saved to the final target path.
	void SaveToFile(KeyValues* pResult, const char* pFileName);
};

class CSQLEngine : public CDALEngine
{
	CSysModule* GetModule() OVERRIDE;
	void DispatchHandleResults(IDAO* pDAO) OVERRIDE;

	CSysModule* m_pModule;

public:
	CSQLEngine();

	virtual bool Connect(const char* pHostName, const char* pUserName,
		const char* pPassText, const char* pSchemaName, int port) = 0;

	// Begins a transaction. Throws CDAOException on failure.
	virtual void BeginTxn() = 0;

	// Ends a transaction. Throws CDAOException on failure.
	virtual void EndTxn() = 0;

	// Prepares a statement. Throws CDAOException on failure.
	virtual void* PrepareStatement(const char* pQueryText) = 0;

	// Binds an integer on a prepared statement marker. Throws CDAOException on failure.
	virtual void BindInt(void* pStmt, int position, int value) = 0;

	// Binds a float on a prepared statement marker. Throws CDAOException on failure.
	virtual void BindFloat(void* pStmt, int position, float value) = 0;

	// Binds a string on a prepared statement marker. Throws CDAOException on failure.
	virtual void BindString(void* pStmt, int position, const char* pValue) = 0;

	// Executes a prepared statement. Throws CDAOException on failure.
	// Input: pResultName - Name of result key. MUST be valid when fetching results.
	virtual void ExecutePreparedStatement(void* pStmt, const char* pResultName = NULL) = 0;

	// Executes a query. Throws CDAOException on failure.
	// Input: pResultName - Name of result key. MUST be valid when fetching results.
	virtual void ExecuteQuery(const char* pQueryText, const char* pResultName = NULL) = 0;

	// Executes a formatted query. Throws CDAOException on failure.
	// Input: pResultName - Name of result key. MUST be valid when fetching results.
	void ExecuteFormatQuery(const char* pQueryText, const char* pResultName = NULL, ...);

	void SetModule(CSysModule* pModule);
};

FORCEINLINE CSQLEngine::CSQLEngine() : m_pModule(NULL)
{

}

FORCEINLINE CSysModule* CSQLEngine::GetModule()
{
	return m_pModule;
}

FORCEINLINE void CSQLEngine::DispatchHandleResults(IDAO* pDAO)
{
	pDAO->HandleSQLResults(m_CachedResults);
}

FORCEINLINE void CSQLEngine::SetModule(CSysModule* pModule)
{
	m_pModule = pModule;
}

// Executes a formatted query. Throws CDAOException on failure.
// Input: pResultName - Name of result key. MUST be valid when fetching results.
inline void CSQLEngine::ExecuteFormatQuery(const char* pQueryText, const char* pResultName, ...)
{
	va_list args;
	va_start(args, pResultName);
	char formattedQuery[HL2RP_DAL_SQL_ENGINE_QUERY_SIZE];
	Q_vsnprintf(formattedQuery, sizeof(formattedQuery), pQueryText, args);
	ExecuteQuery(formattedQuery, pResultName);
	va_end(args);
}

#endif // !DALENGINE_H
