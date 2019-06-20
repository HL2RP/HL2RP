#ifndef AUTOLOCALIZER_H
#define AUTOLOCALIZER_H
#pragma once

#define AUTOLOCALIZER_TOKEN_SIZE	64
#define AUTOLOCALIZER_TRANSLATION_SIZE	512 // Max. length of a formatted localization
#define AUTOLOCALIZER_DEFAULT_LANGUAGE	"english"
#define AUTOLOCALIZER_FMTED_ARG_SIZE 64 // Max. length of each localization argument as a string

// Efficient AutoLocalizer buffer typedefs
typedef char autoloc_buf[AUTOLOCALIZER_TRANSLATION_SIZE];
typedef autoloc_buf& autoloc_buf_ref;

class CAutoUtf8Arg
{
	template<typename T>
	void FormatAndSet(T value, const char* pFormat);

	char m_utf8Arg[AUTOLOCALIZER_FMTED_ARG_SIZE];

public:
	CAutoUtf8Arg(const char* pValue, const char* pFormatSpec = "%s");
	CAutoUtf8Arg(string_t value, const char* pFormatSpec = "%s");
	CAutoUtf8Arg(int value, const char* pFormatSpec = "%i");
	CAutoUtf8Arg(float value, const char* pFormatSpec = "%.2f");

	const char* Get() { return m_utf8Arg; }
};

class CAutoLocalizer
{
	char* InternalLocalize(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		int argCount = 0, ...);

	KeyValues* m_pColorLocVars;

public:
	CAutoLocalizer();

	~CAutoLocalizer();

	void Init();
	void SetRawTranslation(const char* pLanguageShortName, const char* pToken,
		const char* pRawTranslation);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
		CAutoUtf8Arg arg4);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
		CAutoUtf8Arg arg4, CAutoUtf8Arg arg5);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
		CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
		CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6, CAutoUtf8Arg arg7);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
		CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6, CAutoUtf8Arg arg7,
		CAutoUtf8Arg arg8);
	void LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
		CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
		CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6, CAutoUtf8Arg arg7,
		CAutoUtf8Arg arg8, CAutoUtf8Arg arg9);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2, CAutoUtf8Arg arg3);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6,
		CAutoUtf8Arg arg7);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6,
		CAutoUtf8Arg arg7, CAutoUtf8Arg arg8);
	autoloc_buf_ref Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0, CAutoUtf8Arg arg1,
		CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6,
		CAutoUtf8Arg arg7, CAutoUtf8Arg arg8, CAutoUtf8Arg arg9);

	KeyValues* m_pServerPhrases;
	autoloc_buf m_Localized;
};

#endif // !AUTOLOCALIZER_H
