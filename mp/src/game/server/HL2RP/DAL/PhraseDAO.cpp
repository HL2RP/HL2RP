#include <cbase.h>
#include "PhraseDAO.h"
#include <HL2RPRules.h>
#include "IDALEngine.h"
#include <language.h>
#include <Dialogs/PlayerDialog.h>

#define PHRASE_DAO_LANGUAGE_SHORT_NAME_SIZE	64 // Should be more than enough
#define PHRASE_DAO_COLLECTION_NAME	"Phrase"
#define PHRASE_DAO_KV_FILENAME	(PHRASE_DAO_COLLECTION_NAME ".txt")

#define PHRASE_DAO_SQL_SETUP_QUERY	"CREATE TABLE IF NOT EXISTS %s "	\
"(id VARCHAR (%i) NOT NULL, language VARCHAR (%i) NOT NULL, translation VARCHAR (%i) NOT NULL, "	\
"PRIMARY KEY (id, language));"

CDAOThread::EDAOCollisionResolution CPhrasesInitDAO::Collide(IDAO * pDAO)
{
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

void CPhrasesInitDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery(PHRASE_DAO_SQL_SETUP_QUERY, PHRASE_DAO_COLLECTION_NAME,
		AUTOLOCALIZER_TOKEN_SIZE, PHRASE_DAO_LANGUAGE_SHORT_NAME_SIZE, AUTOLOCALIZER_TRANSLATION_SIZE);
}

void CPhrasesLoadDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	pKVEngine->LoadFromFile(PHRASE_DAO_KV_FILENAME, PHRASE_DAO_COLLECTION_NAME);
}

void CPhrasesLoadDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteSelectQuery("SELECT * FROM %s;",
		PHRASE_DAO_COLLECTION_NAME, PHRASE_DAO_COLLECTION_NAME);
}

void CPhrasesLoadDAO::HandleResults(KeyValues* pResults)
{
	KeyValues* pResult = pResults->FindKey(PHRASE_DAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		FOR_EACH_SUBKEY(pResult, pPhraseData)
		{
			const char* pHeader = pPhraseData->GetString("id"),
				* pLanguage = pPhraseData->GetString("language"),
				* pTranslation = pPhraseData->GetString("translation");

			FOR_EACH_LANGUAGE(eLang)
			{
				// Only add if language is recognized and header exists on KeyValues
				if (Q_strcmp(GetLanguageShortName((ELanguage)eLang), pLanguage) == 0)
				{
					GetHL2RPAutoLocalizer().SetRawTranslation(pLanguage, pHeader, pTranslation);
					break;
				}
			}
		}
	}
}

CPhraseSaveDAO::CPhraseSaveDAO(const char* pLangShortName, const char* pHeaderName,
	const char* pTranslationText)
{
	m_pLangShortName = pLangShortName;
	Q_strncpy(m_HeaderName, pHeaderName, sizeof(m_HeaderName));
	Q_strncpy(m_TranslationText, pTranslationText, sizeof(m_TranslationText));
}

CDAOThread::EDAOCollisionResolution CPhraseSaveDAO::Collide(IDAO* pDAO)
{
	CPhraseSaveDAO* pSaveDAO = pDAO->ToClass(this);
	return TryResolveReplacement(pSaveDAO != NULL && pSaveDAO->m_pLangShortName == m_pLangShortName
		&& Q_strcmp(pSaveDAO->m_HeaderName, m_HeaderName) == 0);
}

void CPhraseSaveDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	KeyValues* pResult = pKVEngine->LoadFromFile(PHRASE_DAO_KV_FILENAME, PHRASE_DAO_COLLECTION_NAME, true);
	KeyValues* pPhraseData = pResult->FindKey(m_HeaderName, true);
	pPhraseData->SetString("id", m_HeaderName);
	pPhraseData->SetString("language", m_pLangShortName);
	pPhraseData->SetString("translation", m_TranslationText);
	pKVEngine->SaveToFile(pResult, PHRASE_DAO_KV_FILENAME);
}

void CPhraseSaveDAO::Execute(CSQLiteEngine* pSQLite)
{
	void* pStmt = pSQLite->FormatAndPrepareStatement("INSERT INTO %s (id, language, translation) "
		"VALUES ('%s', '%s', ?) ON CONFLICT (id, language) "
		"DO UPDATE SET translation = excluded.translation;",
		PHRASE_DAO_COLLECTION_NAME, m_HeaderName, m_pLangShortName);
	pSQLite->BindString(pStmt, 1, m_TranslationText);
	pSQLite->ExecutePreparedStatement(pStmt);
}

void CPhraseSaveDAO::Execute(CMySQLEngine* pMySQL)
{
	void* pStmt = pMySQL->FormatAndPrepareStatement("INSERT INTO %s (id, language, translation) "
		"VALUES ('%s', '%s', ?) ON DUPLICATE KEY UPDATE translation = ?;",
		PHRASE_DAO_COLLECTION_NAME, m_HeaderName, m_pLangShortName);
	pMySQL->BindString(pStmt, 1, m_TranslationText);
	pMySQL->BindString(pStmt, 2, m_TranslationText);
	pMySQL->ExecutePreparedStatement(pStmt);
}
