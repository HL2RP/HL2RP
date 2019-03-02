#include <cbase.h>
#include "AutoLocalizer.h"
#include <filesystem.h>
#include "HL2RP_Defs.h"
#include "HL2RP_Player.h"
#include <ilocalize.h>
#include <language.h>
#include "Dialogs/PlayerDialog.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

CAutoUtf8Arg::CAutoUtf8Arg(const char* pValue, const char* pFormatSpec)
{
	FormatAndSet(pValue, pFormatSpec);
}

CAutoUtf8Arg::CAutoUtf8Arg(string_t value, const char* pFormatSpec)
{
	FormatAndSet(value.ToCStr(), pFormatSpec);
}

CAutoUtf8Arg::CAutoUtf8Arg(int value, const char* pFormatSpec)
{
	FormatAndSet(value, pFormatSpec);
}

CAutoUtf8Arg::CAutoUtf8Arg(float value, const char* pFormatSpec)
{
	FormatAndSet(value, pFormatSpec);
}

template<typename T>
void CAutoUtf8Arg::FormatAndSet(T value, const char* pFormat)
{
	Q_snprintf(m_utf8Arg, sizeof(m_utf8Arg), pFormat, value);
}

CAutoLocalizer::CAutoLocalizer() : m_pServerPhrases(NULL), m_pColorLocVars(NULL)
{
	m_Localized[0] = '\0';
}

CAutoLocalizer::~CAutoLocalizer()
{
	if (m_pServerPhrases != NULL)
	{
		Assert(m_pColorLocVars != NULL);
		m_pServerPhrases->deleteThis();
		m_pColorLocVars->deleteThis();
	}
}

void CAutoLocalizer::Init()
{
	m_pServerPhrases = new KeyValues("Phrases");
	m_pServerPhrases->UsesEscapeSequences(true);
	m_pColorLocVars = new KeyValues("Colors");
	m_pColorLocVars->SetString("default", HL2RP_COLOR_TALK_DEFAULT);
	m_pColorLocVars->SetString("indianred", HL2RP_COLOR_TALK_REGION);
	m_pColorLocVars->SetString("static", HL2RP_COLOR_TALK_STATIC);
	m_pColorLocVars->SetString("dynamic", HL2RP_COLOR_TALK_DYNAMIC);
	m_pColorLocVars->SetString("team", HL2RP_COLOR_TALK_TEAM);

	// Begin caching all custom translations for server-side usage, even when files may contain client-side VGUI ones
	FOR_EACH_LANGUAGE(eLang)
	{
		const char* pShortLangName = GetLanguageShortName(static_cast<ELanguage>(eLang));
		char modDir[MAX_PATH], path[MAX_PATH];

		// Add the server-localizable phrases for all languages
		if (UTIL_GetModDir(modDir, ARRAYSIZE(modDir)))
		{
			Q_snprintf(path, sizeof(path), "resource/%s_%s.txt", modDir, pShortLangName);
			KeyValues* pLangLocalization = new KeyValues("lang");
			pLangLocalization->UsesEscapeSequences(true);

			if (pLangLocalization->LoadFromFile(filesystem, path, "MOD"))
			{
				KeyValues* pLangTokens = pLangLocalization->FindKey("Tokens", true);

				// Remove the tokens not exclusive to HL2RP, as these are not localized by server
				for (KeyValues* pLangToken = pLangTokens->GetFirstValue(), *pNextFileLangToken;
					pLangToken != NULL; pLangToken = pNextFileLangToken)
				{
					char prefix[] = "HL2RP_";
					pNextFileLangToken = pLangToken->GetNextValue();

					if (Q_strncmp(pLangToken->GetName(), prefix, sizeof(prefix) - 1) != 0)
					{
						pLangTokens->RemoveSubKey(pLangToken);
					}
				}

				KeyValues* pServerLangPhrases = m_pServerPhrases->FindKey(pShortLangName, true);
				pServerLangPhrases->RecursiveMergeKeyValues(pLangTokens);
			}

			pLangLocalization->deleteThis();
		}

		// Add the files where translated language names are,
		// to be displayed during in-game translating by users. Don't alter VGUI Localization!!
		Q_snprintf(path, sizeof(path), "resource/gameui_%s.txt", pShortLangName);
		KeyValues* pLangLocalization = new KeyValues("lang");
		pLangLocalization->UsesEscapeSequences(true);

		if (pLangLocalization->LoadFromFile(filesystem, path, "GAME"))
		{
			KeyValues* pServerLangPhrases = m_pServerPhrases->FindKey(pShortLangName, true);
			pServerLangPhrases->RecursiveMergeKeyValues(pLangLocalization->FindKey("Tokens", true));
		}

		pLangLocalization->deleteThis();
	}
}

void CAutoLocalizer::SetRawTranslation(const char* pLanguageShortName, const char* pToken,
	const char* pRawTranslation)
{
	Assert(m_pServerPhrases);
	m_pServerPhrases->FindKey(pLanguageShortName, true)->SetString(pToken, pRawTranslation);
}

// Always returns a safe string, to save callers from safe-checking it
char* CAutoLocalizer::InternalLocalize(CBasePlayer* pPlayer, char* pDest, int destSize,
	const char* pToken, int argCount, ...)
{
	// Ignore initial hash (it may be present or not)
	if (pToken[0] == '#')
	{
		pToken++;
	}

	const char* pLanguage = engine->GetClientConVarValue(pPlayer->entindex(), "cl_language");
	KeyValues* pLangPhrases = m_pServerPhrases->FindKey(pLanguage);
	const char* pRawLocalization;
	autoloc_buf colorLocalized;

	if (pLangPhrases != NULL)
	{
		pRawLocalization = pLangPhrases->GetString(pToken, NULL);
	}
	else
	{
		pRawLocalization = NULL;
	}

	// Fallback to the default language
	if (pRawLocalization == NULL)
	{
		pLangPhrases = m_pServerPhrases->FindKey(AUTOLOCALIZER_DEFAULT_LANGUAGE);

		if (pLangPhrases == NULL)
		{
			pDest[0] = '\0';
			return pDest;
		}

		pRawLocalization = pLangPhrases->GetString(pToken, NULL);

		if (pRawLocalization == NULL)
		{
			pDest[0] = '\0';
			return pDest;
		}
	}

	if (argCount > 0)
	{
		// We resolve the colors first, so the argument parser doesn't print warnings
		ILocalize::ConstructString(colorLocalized, sizeof(colorLocalized), pRawLocalization,
			m_pColorLocVars);
		va_list argList;
		va_start(argList, argCount);
		ILocalize::ConstructStringVArgs(pDest, destSize, colorLocalized, argCount, argList);
		va_end(argList);
	}
	else
	{
		ILocalize::ConstructString(pDest, destSize, pRawLocalization, m_pColorLocVars);
	}

	return pDest;
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken)
{
	InternalLocalize(pPlayer, pDest, destSize, pToken);
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0)
{
	InternalLocalize(pPlayer, pDest, destSize, pToken, 1, arg0.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1)
{
	InternalLocalize(pPlayer, pDest, destSize, pToken, 2, arg0.Get(), arg1.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 3, arg0.Get(), arg1.Get(),
		arg2.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 4, arg0.Get(), arg1.Get(),
		arg2.Get(), arg3.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
	CAutoUtf8Arg arg4)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 5, arg0.Get(), arg1.Get(),
		arg2.Get(), arg3.Get(), arg4.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
	CAutoUtf8Arg arg4, CAutoUtf8Arg arg5)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 6, arg0.Get(), arg1.Get(),
		arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
	CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 7, arg0.Get(), arg1.Get(),
		arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
	CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6, CAutoUtf8Arg arg7)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 8, arg0.Get(), arg1.Get(),
		arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get(), arg7.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
	CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6, CAutoUtf8Arg arg7,
	CAutoUtf8Arg arg8)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 9, arg0.Get(), arg1.Get(),
		arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get(), arg7.Get(), arg8.Get());
}

void CAutoLocalizer::LocalizeAt(CBasePlayer* pPlayer, char* pDest, int destSize, const char* pToken,
	CAutoUtf8Arg arg0, CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3,
	CAutoUtf8Arg arg4, CAutoUtf8Arg arg5, CAutoUtf8Arg arg6, CAutoUtf8Arg arg7,
	CAutoUtf8Arg arg8, CAutoUtf8Arg arg9)
{
	InternalLocalize(pPlayer, pDest, sizeof(destSize), pToken, 10, arg0.Get(), arg1.Get(),
		arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get(), arg7.Get(), arg8.Get(),
		arg9.Get());
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken);
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 1, arg0.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 2, arg0.Get(), arg1.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 3, arg0.Get(),
		arg1.Get(), arg2.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 4, arg0.Get(),
		arg1.Get(), arg2.Get(), arg3.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 5, arg0.Get(),
		arg1.Get(), arg2.Get(), arg3.Get(), arg4.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 6, arg0.Get(),
		arg1.Get(), arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5,
	CAutoUtf8Arg arg6)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 7, arg0.Get(),
		arg1.Get(), arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5,
	CAutoUtf8Arg arg6, CAutoUtf8Arg arg7)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 8, arg0.Get(),
		arg1.Get(), arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get(), arg7.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5,
	CAutoUtf8Arg arg6, CAutoUtf8Arg arg7, CAutoUtf8Arg arg8)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 9, arg0.Get(),
		arg1.Get(), arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get(), arg7.Get(),
		arg8.Get());
	return m_Localized;
}

autoloc_buf_ref CAutoLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken, CAutoUtf8Arg arg0,
	CAutoUtf8Arg arg1, CAutoUtf8Arg arg2, CAutoUtf8Arg arg3, CAutoUtf8Arg arg4, CAutoUtf8Arg arg5,
	CAutoUtf8Arg arg6, CAutoUtf8Arg arg7, CAutoUtf8Arg arg8, CAutoUtf8Arg arg9)
{
	InternalLocalize(pPlayer, m_Localized, sizeof(m_Localized), pToken, 10, arg0.Get(),
		arg1.Get(), arg2.Get(), arg3.Get(), arg4.Get(), arg5.Get(), arg6.Get(), arg7.Get(),
		arg8.Get(), arg9.Get());
	return m_Localized;
}
