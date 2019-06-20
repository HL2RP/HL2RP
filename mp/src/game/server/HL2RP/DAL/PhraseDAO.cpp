#include <cbase.h>
#include "PhraseDAO.h"
#include "DALEngine.h"
#include <HL2RPRules.h>
#include <language.h>
#include <Dialogs/PlayerDialog.h>

#define PHRASEDAO_LANGUAGE_SHORT_NAME_SIZE	64 // Should be more than enough
#define PHRASEDAO_COLLECTION_NAME	"Phrase"
#define PHRASEDAO_KV_FILENAME	(PHRASEDAO_COLLECTION_NAME ".txt")

#define PHRASEDAO_SQL_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (id VARCHAR (%i) NOT NULL, language VARCHAR (%i) NOT NULL, translation VARCHAR (%i) NOT NULL,"	\
" PRIMARY KEY (id, language));"

#define PHRASEDAO_SQL_LOAD_QUERY	"SELECT * FROM " PHRASEDAO_COLLECTION_NAME ";"

#define PHRASEDAO_SQLITE_UPDATE_QUERY	\
"UPDATE " PHRASEDAO_COLLECTION_NAME " SET translation = ? WHERE id = ? AND language = ?;"

#define PHRASEDAO_SQLITE_INSERT_QUERY	\
"INSERT OR IGNORE INTO " PHRASEDAO_COLLECTION_NAME " (id, language, translation) VALUES (?, ?, ?);"

#define PHRASEDAO_MYSQL_SAVE_QUERY	\
"INSERT INTO " PHRASEDAO_COLLECTION_NAME " (id, language, translation) VALUES (?, ?, ?)"	\
" ON DUPLICATE KEY UPDATE translation = ?;"

bool CPhrasesInitDAO::ShouldBeReplacedBy(IDAO * pDAO)
{
	return (pDAO->ToClass(this) != NULL);
}

void CPhrasesInitDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteFormatQuery(PHRASEDAO_SQL_SETUP_QUERY, NULL, PHRASEDAO_COLLECTION_NAME,
		AUTOLOCALIZER_TOKEN_SIZE, PHRASEDAO_LANGUAGE_SHORT_NAME_SIZE, AUTOLOCALIZER_TRANSLATION_SIZE);
}

void CPhrasesLoadDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	pKeyValues->LoadFromFile(PHRASEDAO_KV_FILENAME, PHRASEDAO_COLLECTION_NAME);
}

void CPhrasesLoadDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteQuery(PHRASEDAO_SQL_LOAD_QUERY, PHRASEDAO_COLLECTION_NAME);
}

void CPhrasesLoadDAO::HandleResults(KeyValues* pResults)
{
	KeyValues* pResult = pResults->FindKey(PHRASEDAO_COLLECTION_NAME);

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

bool CPhraseSaveDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	CPhraseSaveDAO* pPhraseSaveDAO = pDAO->ToClass(this);
	return (pPhraseSaveDAO != NULL && pPhraseSaveDAO->m_pLangShortName == m_pLangShortName
		&& !Q_strcmp(pPhraseSaveDAO->m_HeaderName, m_HeaderName));
}

void CPhraseSaveDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	KeyValues* pResult = pKeyValues->LoadFromFile(PHRASEDAO_KV_FILENAME, PHRASEDAO_COLLECTION_NAME,
		true);
	KeyValues* pPhraseData = pResult->FindKey(m_HeaderName, true);
	pPhraseData->SetString("id", m_HeaderName);
	pPhraseData->SetString("language", m_pLangShortName);
	pPhraseData->SetString("translation", m_TranslationText);
	pKeyValues->SaveToFile(pResult, PHRASEDAO_KV_FILENAME);
}

void CPhraseSaveDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	pSQL->BeginTxn();
	ExecutePreparedSQLStatement(pSQL, PHRASEDAO_SQLITE_UPDATE_QUERY, 2, 3, 1);
	ExecutePreparedSQLStatement(pSQL, PHRASEDAO_SQLITE_INSERT_QUERY, 1, 2, 3);
	pSQL->EndTxn();
}

void CPhraseSaveDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	void* pStmt = PrepareSQLStatement(pSQL, PHRASEDAO_MYSQL_SAVE_QUERY, 1, 2, 3);
	pSQL->BindString(pStmt, 4, m_TranslationText);
	pSQL->ExecutePreparedStatement(pStmt);
}

void* CPhraseSaveDAO::PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int headerNamePos,
	int langShortNamePos, int translationTextPos)
{
	void* pStmt = pSQL->PrepareStatement(pQueryText);
	pSQL->BindString(pStmt, headerNamePos, m_HeaderName);
	pSQL->BindString(pStmt, langShortNamePos, m_pLangShortName);
	pSQL->BindString(pStmt, translationTextPos, m_TranslationText);
	return pStmt;
}

void CPhraseSaveDAO::ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int headerNamePos,
	int langShortNamePos, int translationTextPos)
{
	void* pStmt = PrepareSQLStatement(pSQL, pQueryText, headerNamePos, langShortNamePos, translationTextPos);
	pSQL->ExecutePreparedStatement(pStmt);
}
