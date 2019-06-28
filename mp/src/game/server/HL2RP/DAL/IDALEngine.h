#ifndef DALENGINE_H
#define DALENGINE_H
#pragma once

#include <KeyValues.h>
#include "IDAO.h"

#define DAL_ENGINE_KEYVALUES_SAVE_PATH_ID	"KEYVALUES_SAVE"
#define DAL_ENGINE_SQL_QUERY_SIZE	2048
#define DAL_ENGINE_MYSQL_IMPL_VERSION_STRING	"MySQLStorage001"
#define DAL_ENGINE_SQLITE_IMPL_VERSION_STRING	"SQLiteStorage001"

abstract_class IDALEngine
{
public:
	virtual ~IDALEngine() { }

	virtual CSysModule * GetModule()
	{
		return NULL;
	}

	virtual CDAOThread::EDAOCollisionResolution DispatchCollide(IDAO * pThisDAO, IDAO * pOtherDAO) = 0;
	virtual void DispatchExecute(IDAO* pDAO) = 0;
	virtual void DispatchHandleResults(IDAO* pDAO) = 0;

	virtual void CloseResults()
	{
		m_CachedResults->Clear();
	}

	void Release() { }

protected:
	IDALEngine() : m_CachedResults("ResultSet")
	{

	}

	// Dedicated thread code caches I/O disk results here, main thread reads them later
	KeyValuesAD m_CachedResults;
};

class CKeyValuesEngine : public IDALEngine
{
	CDAOThread::EDAOCollisionResolution DispatchCollide(IDAO* pThisDAO, IDAO* pOtherDAO) OVERRIDE;
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

class CSQLEngine : public IDALEngine
{
	CSysModule* CSQLEngine::GetModule() OVERRIDE
	{
		return m_pModule;
	}

	CDAOThread::EDAOCollisionResolution DispatchCollide(IDAO* pThisDAO, IDAO* pOtherDAO) OVERRIDE
	{
		return pThisDAO->Collide(this, pOtherDAO);
	}

	void DispatchHandleResults(IDAO* pDAO) OVERRIDE
	{
		pDAO->HandleResults(this, m_CachedResults);
	}

	CSysModule* m_pModule;

public:
	CSQLEngine::CSQLEngine() : m_pModule(NULL)
	{

	}

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
	// Input: pResultName - Name of result key. Must be valid when fetching results.
	virtual void ExecuteQuery(const char* pQueryText, const char* pResultName = NULL) = 0;

	// Formats a select query and executes it. Throws CDAOException on failure.
	void FormatAndExecuteQuery(const char* pQueryText, ...);

	void FormatAndExecuteSelectQuery(const char* pQueryText, const char* pResultName, ...);

	// Formats a query and prepares a stament with it. Throws CDAOException on failure.
	void* FormatAndPrepareStatement(const char* pResultName = NULL, ...);

	void SetModule(CSysModule* pModule)
	{
		m_pModule = pModule;
	}
};

// Stubs to enable automatic conversions to base class.
// Real implementation is located at decoupled driver.
class CSQLiteEngine : public CSQLEngine
{

};

class CMySQLEngine : public CSQLEngine
{

};

#endif // !DALENGINE_H
