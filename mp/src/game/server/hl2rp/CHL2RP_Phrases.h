#ifndef ROLEPLAY_PHRASES_H
#define ROLEPLAY_PHRASES_H
#ifdef _WIN32
#pragma once
#endif

#include "CHL2RP_SQL.h"

const int MAX_PHRASE_HEADER_LENGTH = 128; // Better preventing than healing
const int MAX_PHRASE_TRANSLATION_LENGTH = 2048; // Not much comment, should always be enough

class CHL2RP_Player;

class CPhraseParam
{
public:
	enum PhraseParamType
	{
		PHRASE_PARAM_INT,
		PHRASE_PARAM_FLOAT,
		PHRASE_PARAM_STRING
	};

	CPhraseParam(int iValue, const char *pszValueFormat = "%i");
	CPhraseParam(float flValue, const char *pszValueFormat = "%f");
	CPhraseParam(const char *pszValue, const char *pszValueFormat = "%s");

	const char *m_pszValueFormat;

public:
	union
	{
		int m_iValue;
		float m_flValue;
		const char *m_pszValue;
	};

	PhraseParamType m_Type;
};

class CHL2RP_Phrases
{
public:
	static void Init();
	template<class... C> static void LoadForPlayer(const CHL2RP_Player *pPlayer,
		char *psDest, int iMaxlen, const char *pszHeader, const char *pPrefix, CPhraseParam param1, C&&... restParams);
	static void LoadForPlayer(const CHL2RP_Player *pPlayer, char *psDest, int iMaxlen,
		const char *pszHeader, const char *psPrefix = NULL, const CPhraseParam paramsBuff[] = NULL, int iParamCount = 0);
	static void AddPhrase(KeyValues *pLanguageKV, const char *pszHeader, const char *pszTranslation);
	static void AddFormattedPhrase(KeyValues *pLanguageKV, const char *pszHeader, const char *pszTranslation, ...);
};

// NOTE: Add phrase color codes ONLY through these functions.
// This is to apply them automatically on all languages.
// Never add them on the default translation strings!
template<class... C> void CHL2RP_Phrases::LoadForPlayer(const CHL2RP_Player *pPlayer,
	char *psDest, int iMaxlen, const char *pszHeader, const char *pPrefix, CPhraseParam param1, C&&... restParams)
{
	CPhraseParam paramsBuff[] = { param1, restParams... };
	LoadForPlayer(pPlayer, psDest, iMaxlen, pszHeader, pPrefix, paramsBuff, ARRAYSIZE(paramsBuff));
}

class CSetUpPhrasesTxn : public CAsyncSQLTxn
{
public:
	CSetUpPhrasesTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
};

class CLoadPhrasesTxn : public CAsyncSQLTxn
{
public:
	CLoadPhrasesTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
	void HandleQueryResults() const OVERRIDE;
};

class CUpsertPhraseTxn : public CAsyncSQLTxn
{
	DECLARE_CLASS(CUpsertPhraseTxn, CAsyncSQLTxn)

public:
	CUpsertPhraseTxn(const char *phraseLanguageShortName, const char *phraseHeader, const char *phraseTranslation);

private:
	void BuildSQLiteQueries() OVERRIDE;
	void BuildMySQLQueries() OVERRIDE;
	void BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const OVERRIDE;
	void BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const OVERRIDE;
	bool ShouldBeReplacedBy(const BaseClass &txn) const OVERRIDE;

	const char *m_pszPhraseLanguageShortName;

	// Header and translation pointers are dynamic on phrases structure, so keep safe copies
	char m_szPhraseHeader[MAX_PHRASE_HEADER_LENGTH], m_szPhraseTranslation[MAX_PHRASE_TRANSLATION_LENGTH];
};

#endif // ROLEPLAY_PHRASES_H
