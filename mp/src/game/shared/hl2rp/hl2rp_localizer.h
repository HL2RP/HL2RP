#ifndef HL2RP_LOCALIZER_H
#define HL2RP_LOCALIZER_H
#pragma once

#ifdef GAME_DLL
#define GC
#endif // GAME_DLL

#include <ilocalize.h>

#define HL2RP_LOCALIZE_BUFFER_SIZE 256

typedef locchar_t localizebuf_t[HL2RP_LOCALIZE_BUFFER_SIZE];

#ifdef GC
#include <fmtstr.h>

typedef CFmtStrN<HL2RP_LOCALIZE_BUFFER_SIZE> CLocalizeFmtStr;
#endif // GC

#ifdef GAME_DLL
#include <generic.h>

class CPhraseDictionary : public CUtlDict<CUtlConstString>
{
public:
	void AddPhrase(const char* pToken, const char*);
};
#endif // GAME_DLL

template<>
class CLocalizedStringArg<int> : public CLocalizedStringArgPrintfImpl<int>
{
public:
	CLocalizedStringArg(int value) : CLocalizedStringArgPrintfImpl(value, LOCCHAR("%i")) { }
};

class CHL2RPLocalizer : CAutoGameSystemPerFrame
{
	void LevelInitPostEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE;

	void AddLanguageLocalizationFromFile(const char* pBasePath, const char* pLanguage = "%language%",
		const char* pTokenPrefix = "");

	template<typename... T>
	int ConstructString(locchar_t* pDest, int maxLen, bool parseVariables, const locchar_t* pFormat, T&... args)
	{
#ifdef GC // Translating variables is currently only needed when using normal chars
		char buffer[HL2RP_LOCALIZE_BUFFER_SIZE];

		if (parseVariables)
		{
			ILocalize::ConstructString(buffer, sizeof(buffer), pFormat, mVariables);
			pFormat = buffer;
		}
#endif // GC

		ILocalize::ConstructString(pDest, maxLen, pFormat, (int)sizeof...(T),
			CLocalizedStringArg<T>(args).GetLocArg()...);
		return loc_strlen(pDest);
	}

	template<typename... T>
	int Localize(CBasePlayer* pPlayer, locchar_t* pDest, int maxLen,
		bool parseVariables, const char* pToken, T&&... args)
	{
		return ConstructString(pDest, maxLen, parseVariables, Localize<locchar_t>(pPlayer, pToken), args...);
	}

	KeyValuesAD mVariables;

public:
	CHL2RPLocalizer();

	const char* LocalizeAsUTF8(CBasePlayer*, const char*);

	template<typename T = char>
	const T* Localize(CBasePlayer* pPlayer, const char* pToken)
	{
		return LocalizeAsUTF8(pPlayer, pToken);
	}

	template<typename... T>
	int Localize(CBasePlayer* pPlayer, localizebuf_t& dest, bool parseVariables, const char* pToken, T&&... args)
	{
		return Localize(pPlayer, dest, sizeof(dest), parseVariables, pToken, args...);
	}

#ifdef GC
	template<typename... T>
	void Localize(CBasePlayer* pPlayer, CLocalizeFmtStr& dest, bool parseVariables, const char* pToken, T&&... args)
	{
		int len = Localize(pPlayer, dest.Access() + dest.Length(), dest.GetMaxLength() - dest.Length(),
			parseVariables, pToken, args...);
		dest.SetLength(dest.Length() + len);
	}
#endif // GC

#ifdef CLIENT_DLL
	const wchar_t* LocalizeAsWideString(const char*);

	template<typename... T>
	int Localize(localizebuf_t& dest, const char* pToken, T&&... args)
	{
		return Localize(NULL, dest, false, pToken, args...);
	}

private:
	void PostInit() OVERRIDE;
#else
	CPhraseDictionary* CreateLocalization(const char* pLanguage);

private:
	CAutoPurgeAdapter<CUtlDict<CPhraseDictionary*>> mLocalizationByLanguage;
#endif // CLIENT_DLL
};

#ifdef CLIENT_DLL
template<>
inline const wchar_t* CHL2RPLocalizer::Localize(CBasePlayer* pPlayer, const char* pToken)
{
	return LocalizeAsWideString(pToken);
}
#endif // CLIENT_DLL

extern CHL2RPLocalizer gHL2RPLocalizer;

#endif // !HL2RP_LOCALIZER_H
