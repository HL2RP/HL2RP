#include <cbase.h>
#include "DALEngine.h"
#include <filesystem.h>

void CKeyValuesEngine::DispatchExecute(IDAO* pDAO)
{
	pDAO->ExecuteKeyValues(this);
}

void CKeyValuesEngine::DispatchHandleResults(IDAO* pDAO)
{
	pDAO->HandleKeyValuesResults(m_CachedResults);
}

KeyValues* CKeyValuesEngine::LoadFromFile(const char* pFileName, const char* pResultName,
	bool forceCreateResult)
{
	if (filesystem->IsDirectory(pFileName, DALENGINE_KEYVALUES_SAVE_PATH_ID))
	{
		// The resource is a directory and it will likely stay as is in the current frame. Throw.
		throw CDAOException();
	}

	// Check if file exists to call LoadFromFile crash-safely
	if (forceCreateResult || filesystem->FileExists(pFileName, DALENGINE_KEYVALUES_SAVE_PATH_ID))
	{
		KeyValues* pCurResultKey = m_CachedResults->FindKey(pResultName, true);
		pCurResultKey->UsesEscapeSequences(true);

		if (pCurResultKey->LoadFromFile(filesystem, pFileName, DALENGINE_KEYVALUES_SAVE_PATH_ID)
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
	filesystem->CreateDirHierarchy(dirTreeString, DALENGINE_KEYVALUES_SAVE_PATH_ID);

	if (!pResult->SaveToFile(filesystem, pFileName, DALENGINE_KEYVALUES_SAVE_PATH_ID))
	{
		throw CDAOException();
	}
}
