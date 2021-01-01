// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_localize.h"

#ifdef CLIENT_DLL
#include <tier3/tier3.h>
#include <vgui/ILocalize.h>
#include <fmtstr.h>
#else
#include <tier2/fileutils.h>
#include <language.h>
#include <utlbuffer.h>
#endif // CLIENT_DLL

CHL2RPLocalize gHL2RPLocalize;

CHL2RPLocalize::CHL2RPLocalize() : mVariables("")
{

}

void CHL2RPLocalize::PostInit()
{
#ifdef CLIENT_DLL
	AddLanguageLocalizationFromFile("hl2mp"); // Base localization
	AddLanguageLocalizationFromFile("hl2rp_client");
#endif // CLIENT_DLL
}

void CHL2RPLocalize::LevelInitPostEntity()
{
	mVariables->SetString("default", "\x01");
	mVariables->SetString("indianred", "\x07" "CD5C5C");
	mVariables->SetString("static", "\x07" "00BFFF"); // Deepskyblue
	mVariables->SetString("dynamic", "\x04"); // Green
	mVariables->SetString("team", "\x03");

#ifndef CLIENT_DLL
	// Load HL2RP server localization for all languages
	FOR_EACH_LANGUAGE(language)
	{
		const char* pLangShortName = GetLanguageShortName((ELanguage)language);
		AddLanguageLocalizationFromFile("hl2rp_server", pLangShortName);

#ifdef HL2RP_LEGACY
		AddLanguageLocalizationFromFile("hl2rp", pLangShortName);
		AddLanguageLocalizationFromFile("hl2rp_legacy", pLangShortName);
#endif // HL2RP_LEGACY

		// Load the localized language names, to be displayed during in-game translating by users
		AddLanguageLocalizationFromFile("gameui", pLangShortName, "GameUI_Language_");
	}
#endif // !CLIENT_DLL
}

void CHL2RPLocalize::LevelShutdownPostEntity()
{
	mVariables->Clear();

#ifndef CLIENT_DLL
	mLocalizationByLanguage.PurgeAndDeleteElements();
#endif // !CLIENT_DLL
}

void CHL2RPLocalize::AddLanguageLocalizationFromFile(const char* pBasePath,
	const char* pLanguage, const char* pTokenPrefix)
{
	char path[MAX_PATH];
	V_sprintf_safe(path, "resource/%s_%s.txt", pBasePath, pLanguage);

#ifdef CLIENT_DLL
	g_pVGuiLocalize->AddFile(path);
#else
	CInputTextFile file(path);

	if (file.IsOk())
	{
		// Allocate buffer for null-terminated text. Then, read up to the exact size retrieved, otherwise,
		// the buffer could result larger in extreme cases where file changes, breaking null-termination.
		CUtlBuffer buffer(0, file.Size() + 2, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER);

		if (filesystem->ReadToBuffer(file.Handle(), buffer, file.Size()))
		{
			Q_memset((char*)buffer.Base() + file.Size(), '\0', 2); // Add null terminator with UTF-16 support

			if (Q_memcmp(buffer.Base(), "\xFF\xFE", 2) == 0) // Check for UTF-16 LE, like VGUI2 localization
			{
				uchar16* pSrcData = (uchar16*)buffer.Base() + 1;
				int maxLen = Q_UTF16ToUTF8(pSrcData, NULL, 0);
				char* pDestData = new char[maxLen];
				Q_UTF16ToUTF8(pSrcData, pDestData, maxLen);
				buffer.AssumeMemory(pDestData, maxLen, maxLen, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER);
			}

			KeyValuesAD languageLocalization("");
			languageLocalization->UsesEscapeSequences(true);

			if (languageLocalization->LoadFromBuffer(path, buffer))
			{
				KeyValues* pLanguageTokens = languageLocalization->FindKey("Tokens");

				if (pLanguageTokens != NULL)
				{
					int prefixLen = Q_strlen(pTokenPrefix);
					CPhraseDictionary* pPhraseByToken = CreateLocalization(pLanguage);

					FOR_EACH_VALUE(pLanguageTokens, pToken)
					{
						if (Q_strnicmp(pToken->GetName(), pTokenPrefix, prefixLen) == 0)
						{
							pPhraseByToken->AddPhrase(pToken->GetName(), pToken->GetString());
						}
					}
				}
			}
		}
	}
#endif // CLIENT_DLL
}

int CHL2RPLocalize::ConstructString(locchar_t* pDest, int maxLen, bool parseVariables,
	const locchar_t* pFormat, int argCount, ...)
{
#ifndef CLIENT_DLL
	char buffer[HL2RP_LOCALIZE_MAX_BUFFER_SIZE];

	if (parseVariables)
	{
		ILocalize::ConstructString(buffer, sizeof(buffer), pFormat, mVariables);
		pFormat = buffer;
	}
#endif // !CLIENT_DLL

	va_list args;
	va_start(args, argCount);
	ILocalize::ConstructStringVArgs(pDest, maxLen, pFormat, argCount, args);
	va_end(args);
	return loc_strlen(pDest);
}

const locchar_t* CHL2RPLocalize::Localize(CBasePlayer* pPlayer, const char* pToken)
{
#ifdef CLIENT_DLL
	return Localize(pToken);
#else
	const char* pLanguage = engine->GetClientConVarValue(pPlayer->entindex(), "cl_language");

	if (pToken[0] == '#')
	{
		++pToken;
	}

	for (int i = 0; i < 2; pLanguage = "english", ++i)
	{
		int localizationIndex = mLocalizationByLanguage.Find(pLanguage);

		if (mLocalizationByLanguage.IsValidIndex(localizationIndex))
		{
			int phraseIndex = mLocalizationByLanguage[localizationIndex]->Find(pToken);

			if (mLocalizationByLanguage[localizationIndex]->IsValidIndex(phraseIndex)
				&& !mLocalizationByLanguage[localizationIndex]->Element(phraseIndex).IsEmpty())
			{
				return mLocalizationByLanguage[localizationIndex]->Element(phraseIndex);
			}
		}
	}

	return "";
#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
const wchar_t* CHL2RPLocalize::Localize(const char* pToken)
{
	wchar_t* pFormat = g_pVGuiLocalize->Find(pToken);

	if (pFormat != NULL)
	{
		return pFormat;
	}

	return L"";
}
#else
void CPhraseDictionary::AddPhrase(const char* pToken, const char* pPhrase)
{
	int index = Find(pToken);

	if (!IsValidIndex(index))
	{
		index = Insert(pToken);
	}

	Element(index).Set(pPhrase);
}

CPhraseDictionary* CHL2RPLocalize::CreateLocalization(const char* pLanguage)
{
	int index = mLocalizationByLanguage.Find(pLanguage);

	if (!mLocalizationByLanguage.IsValidIndex(index))
	{
		index = mLocalizationByLanguage.Insert(pLanguage, new CPhraseDictionary);
	}

	return mLocalizationByLanguage[index];
}
#endif // CLIENT_DLL
