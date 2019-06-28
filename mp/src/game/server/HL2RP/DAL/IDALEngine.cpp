#include <cbase.h>
#include <filesystem.h>
#include "IDALEngine.h"

void CSQLEngine::FormatAndExecuteQuery(const char* pQueryText, ...)
{
	va_list args;
	va_start(args, pQueryText);
	char formattedQuery[DAL_ENGINE_SQL_QUERY_SIZE];
	Q_vsnprintf(formattedQuery, sizeof(formattedQuery), pQueryText, args);
	ExecuteQuery(formattedQuery, NULL);
	va_end(args);
}

void CSQLEngine::FormatAndExecuteSelectQuery(const char* pQueryText, const char* pResultName, ...)
{
	va_list args;
	va_start(args, pResultName);
	char formattedQuery[DAL_ENGINE_SQL_QUERY_SIZE];
	Q_vsnprintf(formattedQuery, sizeof(formattedQuery), pQueryText, args);
	ExecuteQuery(formattedQuery, pResultName);
	va_end(args);
}

void* CSQLEngine::FormatAndPrepareStatement(const char* pQueryText, ...)
{
	va_list args;
	va_start(args, pQueryText);
	char formattedQuery[DAL_ENGINE_SQL_QUERY_SIZE];
	Q_vsnprintf(formattedQuery, sizeof(formattedQuery), pQueryText, args);
	void* pStmt = PrepareStatement(formattedQuery);
	va_end(args);
	return pStmt;
}

CDAOThread::EDAOCollisionResolution
CKeyValuesEngine::DispatchCollide(IDAO* pThisDAO, IDAO* pOtherDAO)
{
	return pThisDAO->Collide(this, pOtherDAO);
}

void CKeyValuesEngine::DispatchExecute(IDAO* pDAO)
{
	pDAO->Execute(this);
}

void CKeyValuesEngine::DispatchHandleResults(IDAO* pDAO)
{
	pDAO->HandleResults(this, m_CachedResults);
}

KeyValues* CKeyValuesEngine::LoadFromFile(const char* pFileName, const char* pResultName,
	bool forceCreateResult)
{
	if (filesystem->IsDirectory(pFileName, DAL_ENGINE_KEYVALUES_SAVE_PATH_ID))
	{
		// The resource is a directory and it will likely stay as is in the current frame. Throw.
		throw CDAOException();
	}

	if (forceCreateResult || filesystem->FileExists(pFileName, DAL_ENGINE_KEYVALUES_SAVE_PATH_ID))
	{
		KeyValues* pCurResultKey = m_CachedResults->FindKey(pResultName, true);
		pCurResultKey->UsesEscapeSequences(true);

		if (pCurResultKey->LoadFromFile(filesystem, pFileName, DAL_ENGINE_KEYVALUES_SAVE_PATH_ID)
			|| forceCreateResult)
		{
			return pCurResultKey;
		}

		// We may not need to remove it, but we stay consistent with our design
		m_CachedResults->RemoveSubKey(pCurResultKey);
	}

	return NULL;
}

void CKeyValuesEngine::SaveToFile(KeyValues* pResult, const char* pFileName)
{
	CUtlString dirTreeString(pFileName);
	dirTreeString = dirTreeString.StripFilename();
	filesystem->CreateDirHierarchy(dirTreeString, DAL_ENGINE_KEYVALUES_SAVE_PATH_ID);

	if (!pResult->SaveToFile(filesystem, pFileName, DAL_ENGINE_KEYVALUES_SAVE_PATH_ID))
	{
		throw CDAOException();
	}
}
