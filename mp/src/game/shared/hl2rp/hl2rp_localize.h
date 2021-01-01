#ifndef HL2RP_LOCALIZE_H
#define HL2RP_LOCALIZE_H
#pragma once

#ifndef CLIENT_DLL
#include <generic.h>
#include <fmtstr.h>

#define GC // Makes ilocalize.h define aliases for UTF-8 functions instead of wide string ones
#endif // !CLIENT_DLL

#include <ilocalize.h>

#define HL2RP_LOCALIZE_MAX_BUFFER_SIZE 256

#ifndef CLIENT_DLL
#undef GC

typedef CFmtStrN<HL2RP_LOCALIZE_MAX_BUFFER_SIZE> CLocalizeFmtStr;

class CPhraseDictionary : public CUtlDict<CUtlConstString>
{
public:
	void AddPhrase(const char* pToken, const char*);
};
#endif // !CLIENT_DLL

typedef locchar_t localizebuf_t[HL2RP_LOCALIZE_MAX_BUFFER_SIZE];

template<>
class CLocalizedStringArg<int> : public CLocalizedStringArgPrintfImpl<int>
{
public:
	CLocalizedStringArg(int value) : CLocalizedStringArgPrintfImpl(value, LOCCHAR("%i")) { }
};

class CHL2RPLocalize : CAutoGameSystemPerFrame
{
	void PostInit() OVERRIDE;
	void LevelInitPostEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE;

	void AddLanguageLocalizationFromFile(const char* pBasePath, const char* pLanguage = "%language%",
		const char* pTokenPrefix = "");
	int ConstructString(locchar_t* pDest, int maxLen, bool parseVariables, const locchar_t* pFormat, int argCount, ...);

	KeyValuesAD mVariables;

public:
	CHL2RPLocalize();

	const locchar_t* Localize(CBasePlayer*, const char* pToken);

	template<typename... T>
	int Localize(CBasePlayer* pPlayer, localizebuf_t& dest, bool parseVariables, const char* pToken, T... args)
	{
		return ConstructString(dest, sizeof(dest), parseVariables, Localize(pPlayer, pToken),
			sizeof...(T), CLocalizedStringArg<T>(args).GetLocArg()...);
	}

#ifdef CLIENT_DLL
	static const wchar_t* Localize(const char* pToken);

	template<typename... T>
	int Localize(wchar_t* pDest, int maxLen, const char* pToken, T... args)
	{
		return ConstructString(pDest, maxLen, false, Localize(pToken),
			sizeof...(T), CLocalizedStringArg<T>(args).GetLocArg()...);
	}

	template<typename... T>
	int Localize(localizebuf_t& dest, const char* pToken, T... args)
	{
		return Localize(dest, sizeof(dest), pToken, args...);
	}
#else
	CPhraseDictionary* CreateLocalization(const char* pLanguage);

	template<typename... T>
	void Localize(CBasePlayer* pPlayer, CLocalizeFmtStr& dest, bool parseVariables, const char* pToken, T... args)
	{
		int len = ConstructString(dest.Access() + dest.Length(), dest.GetMaxLength() - dest.Length(), parseVariables,
			Localize(pPlayer, pToken), sizeof...(T), CLocalizedStringArg<T>(args).GetLocArg()...);
		dest.SetLength(dest.Length() + len);
	}

private:
	CAutoPurgeAdapter<CUtlDict<CPhraseDictionary*>> mLocalizationByLanguage;
#endif // CLIENT_DLL
};

extern CHL2RPLocalize gHL2RPLocalize;

#endif // !HL2RP_LOCALIZE_H
