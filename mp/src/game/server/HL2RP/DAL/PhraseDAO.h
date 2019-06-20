#ifndef PHRASEDAO_H
#define PHRASEDAO_H
#pragma once

#include <AutoLocalizer.h>
#include "IDAO.h"

class CPhrasesInitDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO);
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
};

class CPhrasesLoadDAO : public IDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;
};

class CPhraseSaveDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQLite(CSQLEngine* pSQL) OVERRIDE;
	void ExecuteMySQL(CSQLEngine* pSQL) OVERRIDE;

	void* PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int headerNamePos,
		int langShortNamePos, int translationTextPos);
	void ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int headerNamePos,
		int langShortNamePos, int translationTextPos);

	// NOTE: This should point to a global KeyValues object (phrase overrides), which for now is always valid.
	// Stay alerted on this variable for future changes!! (Thread safety)
	const char* m_pLangShortName;

	// Header and translation pointers are invalid when the dialog succeeds, so keep safe copies
	char m_HeaderName[AUTOLOCALIZER_TOKEN_SIZE];
	autoloc_buf m_TranslationText;

public:
	CPhraseSaveDAO(const char* pLangShortName, const char* pHeaderName,
		const char* pTranslationText);
};

#endif // !PHRASEDAO_H
