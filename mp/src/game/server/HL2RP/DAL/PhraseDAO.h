#ifndef PHRASEDAO_H
#define PHRASEDAO_H
#pragma once

#include <AutoLocalizer.h>
#include "IDAO.h"

class CPhrasesInitDAO : public IDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO);
	void Execute(CSQLEngine* pSQL) OVERRIDE;
};

class CPhrasesLoadDAO : public IDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;
};

class CPhraseSaveDAO : public IDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLiteEngine* pSQLite) OVERRIDE;
	void Execute(CMySQLEngine* pMySQL) OVERRIDE;

	CDAOThread::EDAOCollisionResolution Collide(CPhraseSaveDAO* pDAO);

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
