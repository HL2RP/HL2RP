// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_localizer.h"

#ifdef GAME_DLL
#include <language.h>
#include <tier2/fileutils.h>
#include <utlbuffer.h>
#else
#include <tier3/tier3.h>
#include <vgui/ILocalize.h>
#endif // GAME_DLL

#define HL2RP_LOCALIZER_DEFAULT_LANGUAGE "english"

void OnChatColorCVarChanged(IConVar* pCVar, const char*, float)
{
	gHL2RPLocalizer.SetCVarColorVariable(static_cast<ConVar*>(pCVar));
}

ConVar gAutoChatColorizeCVar(
#ifdef GAME_DLL
	"sv_auto_colorize_chat",
#else
	"cl_auto_colorize_chat",
#endif // GAME_DLL
	"1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Automatically colorize localized text for chat display"
),
gStaticChatColorCVar(
#ifdef GAME_DLL
	"sv_static_chat_color",
#else
	"cl_static_chat_color",
#endif // GAME_DLL
	"deepskyblue", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Chat color alias/string for fixed parts of localized text",
	OnChatColorCVarChanged
),
gDynamicChatColorCVar(
#ifdef GAME_DLL
	"sv_dynamic_chat_color",
#else
	"cl_dynamic_chat_color",
#endif // GAME_DLL
	"green", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Chat color alias/string for localized text arguments",
	OnChatColorCVarChanged
);

CHL2RPLocalizer gHL2RPLocalizer;

void CHL2RPLocalizer::LevelInitPostEntity()
{
	mVariables.Insert("default", "\x1");
	mVariables.Insert("team", "\x3");
	mVariables.Insert("green", "\x4");
	mVariables.Insert("deepskyblue", "\x7" "00BFFF");
	mVariables.Insert("indianred", "\x7" "CD5C5C");
	SetCVarColorVariable(&gStaticChatColorCVar);
	SetCVarColorVariable(&gDynamicChatColorCVar);

#ifdef GAME_DLL
	// Load HL2RP server localization for all languages
	FOR_EACH_LANGUAGE(language)
	{
		const char* pLangShortName = GetLanguageShortName((ELanguage)language);
		AddLocalizationFromFile("hl2rp_server", pLangShortName);
		AddLocalizationFromFile("hl2rp_shared", pLangShortName);

#ifdef HL2RP_LEGACY
		AddLocalizationFromFile("hl2rp_legacy", pLangShortName);
		AddLocalizationFromFile("hl2rp_client_legacy", pLangShortName);
		AddLocalizationFromFileEx("gameui", pLangShortName, "GameUI_Confirm", "GameUI_Accept", "GameUI_Cancel");
#endif // HL2RP_LEGACY
	}
#endif // GAME_DLL
}

void CHL2RPLocalizer::LevelShutdownPostEntity()
{
	mVariables.Purge();

#ifdef GAME_DLL
	mLocalizationByLanguage.PurgeAndDeleteElements();
#endif // GAME_DLL
}

void CHL2RPLocalizer::AddLocalizationFromFile(const char* pBasePath, const char* pLanguage)
{
	AddLocalizationFromFileEx(pBasePath, pLanguage, "");
}

// NOTE: Requires 1+ filtering token prefixes
template<typename... T>
void CHL2RPLocalizer::AddLocalizationFromFileEx(const char* pBasePath, const char* pLanguage, T... tokenPrefixes)
{
	char path[MAX_PATH];
	V_sprintf_safe(path, "resource/%s_%s.txt", pBasePath, pLanguage);

#ifdef GAME_DLL
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
					CUtlPooledStringMap<>* pPhraseByToken = CreateLocalization(pLanguage);
					const char* prefixesBuffer[] = { tokenPrefixes... };
					int prefixLengths[] = { Q_strlen(tokenPrefixes)... };

					FOR_EACH_VALUE(pLanguageTokens, pToken)
					{
						for (int i = 0; i < ARRAYSIZE(prefixesBuffer); ++i)
						{
							if (Q_strnicmp(pToken->GetName(), prefixesBuffer[i], prefixLengths[i]) == 0)
							{
								pPhraseByToken->Insert(pToken->GetName(), pToken->GetString());
								break;
							}
						}
					}
				}
			}
		}
	}
#else
	g_pVGuiLocalize->AddFile(path);
#endif // GAME_DLL
}

#ifdef GAME_DLL
CUtlPooledStringMap<>* CHL2RPLocalizer::CreateLocalization(const char* pLanguage)
{
	int index = mLocalizationByLanguage.Find(pLanguage);

	if (!mLocalizationByLanguage.IsValidIndex(index))
	{
		index = mLocalizationByLanguage.Insert(pLanguage, new CUtlPooledStringMap<>);
	}

	return mLocalizationByLanguage[index];
}

const char* CHL2RPLocalizer::LocalizeAsUTF8(CBasePlayer* pPlayer, const char* pToken, char*, int)
{
	if (*pToken == '#') // Check if token is localizable, for reusability
	{
		++pToken;
		const char* pLanguage = (pPlayer != NULL && !pPlayer->IsBot()) ?
			engine->GetClientConVarValue(pPlayer->entindex(), "cl_language") : HL2RP_LOCALIZER_DEFAULT_LANGUAGE;

		for (int i = 0; i < 2; pLanguage = HL2RP_LOCALIZER_DEFAULT_LANGUAGE, ++i)
		{
			int localizationIndex = mLocalizationByLanguage.Find(pLanguage);

			if (mLocalizationByLanguage.IsValidIndex(localizationIndex))
			{
				const char* pText = mLocalizationByLanguage[localizationIndex]->GetElementOrDefault(pToken, "");

				if (*pText != '\0')
				{
					return pText;
				}
			}
		}
	}

	return pToken;
}
#else
void CHL2RPLocalizer::PostInit()
{
	AddLocalizationFromFile("hl2mp"); // Base localization
	AddLocalizationFromFile("hl2rp_shared");
	AddLocalizationFromFile("hl2rp_client_legacy");
}

const char* CHL2RPLocalizer::LocalizeAsUTF8(CBasePlayer*, const char* pToken, char* pAuxDest, int maxLen)
{
	const char* pText = g_pVGuiLocalize->FindAsUTF8(pToken);

	if (pAuxDest != NULL)
	{
		Copy(pAuxDest, pText, maxLen);
		return pAuxDest;
	}

	return pText;
}

const wchar_t* CHL2RPLocalizer::LocalizeAsWideString(const char* pToken)
{
	wchar_t* pText = g_pVGuiLocalize->Find(pToken);
	return (pText != NULL ? pText : L"");
}
#endif // GAME_DLL

int CHL2RPLocalizer::SetCVarColorVariable(ConVar* pCVar)
{
	const char* pVarName = (pCVar == &gStaticChatColorCVar) ? "static" : "dynamic";
	mVariables.Remove(pVarName);
	int index = mVariables.Find(pCVar->GetString());

	if (mVariables.IsValidIndex(index))
	{
		return mVariables.Insert(pVarName, mVariables[index]);
	}

	// Parse escaped color hexcode (hud_basechat.h TextColor)
	const char* pPrefix = Q_stristr(pCVar->GetString(), "\\x");

	if (pPrefix != NULL && V_isdigit(pPrefix[2]))
	{
		char hexColor[16];
		V_sprintf_safe(hexColor, "%c%s", pPrefix[2] - '0', pPrefix + 3);
		return mVariables.Insert(pVarName, hexColor);
	}

	return mVariables.Insert(pVarName, pCVar->GetString());
}

int CHL2RPLocalizer::Format(CBasePlayer* pPlayer, char* pDest, int maxLen, bool colorize,
	const char* pFormat, const char* args[], int argsCount, bool& closeArgColor)
{
	return InternalFormat(pPlayer, pDest, maxLen, colorize, pFormat, args, argsCount, closeArgColor);
}

int CHL2RPLocalizer::Format(CBasePlayer* pPlayer, wchar_t* pDest, int maxLen, bool colorize,
	const char* pFormat, const char* args[], int argsCount, bool& closeArgColor)
{
	return InternalFormat(pPlayer, pDest, maxLen, colorize, pFormat, args, argsCount, closeArgColor);
}

template<typename LC>
int CHL2RPLocalizer::InternalFormat(CBasePlayer* pPlayer, LC* pDest, int maxLen, bool colorize,
	const char* pFormat, const char* args[], int argsCount, bool& closeArgColor)
{
	// NOTE: nextArgIndex counts 'consumed arguments', which can't be re-accessed unless explicitly indexed
	int len = 0, nextArgIndex = 0;

	if (maxLen > 0)
	{
		const char* pCurToken = NULL;

		do
		{
			if (pCurToken == NULL)
			{
				if (*pFormat == '%' || *pFormat == '{')
				{
					pCurToken = pFormat;
					continue;
				}
			}
			else if (V_isdigit(*pFormat))
			{
				continue; // Keep reading detail
			}
			else if (*pCurToken == '{')
			{
				if (*pFormat == '}')
				{
					// Resolve and copy variable
					char varName[32]{};
					V_strcat_safe(varName, pCurToken + 1, pFormat - pCurToken - 1);
					len += (*varName == '+') ? AppendButtonVariable(pPlayer, pDest + len, varName, maxLen - len)
						: Copy(pDest + len, mVariables.GetElementOrDefault(varName, ""), maxLen - len);
					pCurToken = NULL;
				}

				continue;
			}
			else
			{
				const char* pDetail = pCurToken + 1;
				int num = Q_atoi(pDetail);
				pCurToken = NULL;

				if (*pFormat == 't') // (T)ranslation
				{
					if (pDetail >= pFormat && nextArgIndex < argsCount)
					{
						num = ++nextArgIndex; // Consume translation name
					}

					if (num > 0 && num <= argsCount)
					{
						// NOTE: We use a buffer for the new format to prevent conflicts with FindAsUTF8's static buffer
						char auxFormat[HL2RP_LOCALIZER_BUFFER_SIZE];
						len += InternalFormat(pPlayer, pDest + len, maxLen - len, colorize,
							Localize(pPlayer, args[num - 1], auxFormat, sizeof(auxFormat)),
							args + num, argsCount - num, closeArgColor);
					}

					continue;
				}
				else if (num > 0 && num <= argsCount)
				{
					// Copy explicit argument, without consuming it
					len += AppendArgument(pDest + len, args[num - 1], maxLen - len, colorize);
					closeArgColor = colorize;

					if (*pFormat == 's')
					{
						continue; // Skip the specifier to parse any special following character correctly
					}
				}
				else if (*pFormat == 's')
				{
					if (pDetail >= pFormat && nextArgIndex < argsCount)
					{
						// Copy and consume next argument
						len += AppendArgument(pDest + len, args[nextArgIndex++], maxLen - len, colorize);
						closeArgColor = colorize;
					}

					continue;
				}
			}

			if (closeArgColor && *pFormat != '\0')
			{
				len += Copy(pDest + len, mVariables.GetElementOrDefault("static", ""), maxLen - len);
				closeArgColor = false;
			}

			len += Copy(pDest + len, pFormat, Min(2, maxLen - len));
		} while (len + 1 < maxLen && AdvanceCharacter<LC>(pFormat));
	}

	Assert(pDest[len] == '\0');
	return len;
}

template<typename LC>
int CHL2RPLocalizer::AppendButtonVariable(CBasePlayer* pPlayer, LC* pDest, const char* pVarName, int maxLen)
{
#ifdef CLIENT_DLL
	const char* pKey = engine->Key_LookupBindingExact(pVarName);

	if (pKey != NULL)
	{
		int len = Copy(pDest, pKey, maxLen);
		StringFuncs<LC>::ToUpper(pDest);
		return len;
	}
#endif // CLIENT_DLL

	const char* commandTokens[][2] = { {"+use", "#HL2RP_Key_Use"}, {"+reload", "#HL2RP_Key_Reload"} };

	for (auto tokens : commandTokens)
	{
		if (Q_stricmp(tokens[0], pVarName) == 0)
		{
			return Copy(pDest, Localize(pPlayer, tokens[1]), maxLen);
		}
	}

	return 0;
}

template<typename LC>
int CHL2RPLocalizer::AppendArgument(LC* pDest, const char* pArg, int maxLen, bool colorize)
{
	int len = colorize ? Copy(pDest, mVariables.GetElementOrDefault("dynamic", ""), maxLen) : 0;
	return (len + Copy(pDest + len, pArg, maxLen - len));
}

template<>
bool CHL2RPLocalizer::AdvanceCharacter<char>(const char*& pSrc)
{
	return (*pSrc++ != '\0');
}

template<>
bool CHL2RPLocalizer::AdvanceCharacter<wchar_t>(const char*& pSrc)
{
	const char* pOldSrc = pSrc;
	return ((pSrc = Q_UnicodeAdvance(pSrc, 1)) > pOldSrc);
}
