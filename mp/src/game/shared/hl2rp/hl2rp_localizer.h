#ifndef HL2RP_LOCALIZER_H
#define HL2RP_LOCALIZER_H
#pragma once

#include "hl2rp_util_shared.h"
#include <ilocalize.h>

#ifdef GAME_DLL
#define LOCCHAR_T char
#else
#define LOCCHAR_T wchar_t
#endif // GAME_DLL

#define HL2RP_LOCALIZER_TOKEN_SIZE  64
#define HL2RP_LOCALIZER_BUFFER_SIZE 512

using localizebuf_t = LOCCHAR_T[HL2RP_LOCALIZER_BUFFER_SIZE];

class CHL2RPLocalizer : CAutoGameSystemPerFrame
{
	template<typename LC> friend class CBaseLocalizeFmtStr;

	void LevelInitPostEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE;

	void AddLocalizationFromFile(const char* pBasePath, const char* pLanguage = "%language%");
	template<typename... T>
	void AddLocalizationFromFileEx(const char* pBasePath, const char* pLanguage, T... tokenPrefixes); // Filtering version
	const char* LocalizeAsUTF8(CBasePlayer*, const char* pToken, char* pAuxDest, int maxLen);
	int Format(CBasePlayer*, char* pDest, int maxLen, bool colorize, const char* pFormat,
		const char* args[], int argsCount, bool& closeArgColor);
	int Format(CBasePlayer*, wchar_t* pDest, int maxLen, bool colorize, const char* pFormat,
		const char* args[], int argsCount, bool& closeArgColor);
	template<typename LC>
	int InternalFormat(CBasePlayer*, LC* pDest, int maxLen, bool colorize,
		const char* pFormat, const char* args[], int argsCount, bool& closeArgColor);
	template<typename LC>
	int AppendButtonVariable(CBasePlayer*, LC* pDest, const char* pVarName, int maxLen);
	template<typename LC>
	int AppendArgument(LC* pDest, const char* pArg, int maxLen, bool colorize);
	template<typename LC>
	bool AdvanceCharacter(const char*&);

	template<typename LC>
	int Copy(LC* pDest, const char* pSrc, int maxLen)
	{
		StringFuncs<LC>::Copy(pDest, pSrc, maxLen);
		return StringFuncs<LC>::Length(pDest);
	}

#ifdef GAME_DLL
	CUtlPooledStringMap<>* CreateLocalization(const char* pLanguage);

	CAutoLessFuncAdapter<CUtlMap<const char*, CUtlPooledStringMap<>*>> mLocalizationByLanguage;
#else
	void PostInit() OVERRIDE;

	const wchar_t* LocalizeAsWideString(const char*);
#endif // GAME_DLL

	CUtlPooledStringMap<> mVariables;

public:
	int SetCVarColorVariable(ConVar*);

	template<typename T = char>
	const T* Localize(CBasePlayer* pPlayer, const char* pToken, char* pAuxDest = NULL, int maxLen = 0)
	{
		return LocalizeAsUTF8(pPlayer, pToken, pAuxDest, maxLen);
	}
};

#ifdef CLIENT_DLL
template<>
inline const wchar_t* CHL2RPLocalizer::Localize(CBasePlayer*, const char* pToken, char*, int)
{
	return LocalizeAsWideString(pToken);
}
#endif // CLIENT_DLL

extern CHL2RPLocalizer gHL2RPLocalizer;
extern ConVar gAutoChatColorizeCVar;

template<typename LC = LOCCHAR_T>
class CBaseLocalizeFmtStr
{
	template<typename... T>
	void InternalFormat(const char* pFormat, T... args)
	{
		const char* buffer[] = { args... };
		mLength += gHL2RPLocalizer.Format(mpPlayer, mpDest + mLength, mMaxSize - mLength,
			mColorize, pFormat, buffer, sizeof...(T), mCloseArgColor);
	}

	CBasePlayer* mpPlayer;
	LC* mpDest;
	bool mColorize = false, mCloseArgColor = false;
	int mMaxSize;

public:
	template<int N>
	CBaseLocalizeFmtStr(CBasePlayer* pPlayer, LC(&dest)[N], bool colorize = false)
		: CBaseLocalizeFmtStr(pPlayer, dest, sizeof(dest), colorize) {}

	CBaseLocalizeFmtStr(CBasePlayer* pPlayer, LC* pDest, int maxLen, bool colorize = false)
		: mpPlayer(pPlayer), mpDest(pDest), mMaxSize(maxLen / sizeof(LC))
	{
		if (colorize && gAutoChatColorizeCVar.GetBool()
#ifdef GAME_DLL
			&& pPlayer != NULL
#endif // GAME_DLL
			)
		{
			mColorize = true;
			mLength = gHL2RPLocalizer.Copy(pDest,
				gHL2RPLocalizer.mVariables.GetElementOrDefault("static", ""), mMaxSize);
		}
	}

	operator const LC* ()
	{
		return mpDest;
	}

	void operator+=(const char* pSrc)
	{
		Format(pSrc, 0);
	}

	template<typename... T>
	const LC* Localize(const char* pToken, T... args)
	{
		return Format("%t", pToken, args...);
	}

	template<typename... T>
	const LC* Format(const char* pFormat, T... args)
	{
		InternalFormat(pFormat, CLocalizedStringArg<T, char>(args).GetLocArg()...);
		return mpDest;
	}

	int mLength = 0;
};

template<typename LC>
class CLocalizeFmtStr : public CBaseLocalizeFmtStr<LC>
{
public:
	CLocalizeFmtStr(CBasePlayer* pPlayer = NULL, bool colorize = false)
		: CBaseLocalizeFmtStr<LC>(pPlayer, mDest, colorize) {}

	LC mDest[HL2RP_LOCALIZER_BUFFER_SIZE];
};

#endif // !HL2RP_LOCALIZER_H
