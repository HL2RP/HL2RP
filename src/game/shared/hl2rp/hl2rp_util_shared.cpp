// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_util_shared.h"
#include "hl2rp_localizer.h"
#include "hl2rp_shareddefs.h"
#include <filesystem.h>
#include <fmtstr.h>
#include <gamestringpool.h>

SCOPED_ENUM(ECurrencyFormatMode,
	AppendSymbolLeft,
	AppendSymbolRight,
	UseLocalizationFormat
)

static ConVar sCurrencySymbolCVar("sv_currency_symbol", "T", FCVAR_ARCHIVE | FCVAR_REPLICATED_HL2RP,
	"Character used to represent currency/money. Default 'T' for 'Token'."),
	sCurrencyFormatModeCVar("sv_currency_format_mode", "1", FCVAR_ARCHIVE | FCVAR_REPLICATED_HL2RP,
		"Determines the position of the currency symbol within amount representations.\n"
		" - 0: Add the currency symbol to the left of the amount.\n"
		" - 1: Add the currency symbol to the right of the amount.\n"
		" - 2: Use format from localization files.",
		true, 0, true, ECurrencyFormatMode::_Count - 1);

CUtlPooledString::CUtlPooledString(const char* pString) : mpString(STRING(AllocPooledString(pString)))
{

}

CUtlPooledString::operator const char* ()
{
	return mpString;
}

SDatabaseId::SDatabaseId(int id) : mId(id)
{

}

SDatabaseId::operator int()
{
	return mId;
}

bool SDatabaseId::IsValid()
{
	return (mId > LOADING_DATABASE_ID);
}

bool HL2RP_LoadConfigFile(KeyValues* pConfig, const char* pName)
{
	pConfig->Clear();
	return pConfig->LoadFromFile(filesystem, CFmtStrN<MAX_PATH>("%s/%s", HL2RP_CONFIG_PATH, pName));
}

#ifdef HL2RP_CLIENT_OR_LEGACY
SRelativeTime::SRelativeTime(int seconds) : mHours(seconds / 3600), mMinutes(seconds / 60 % 60), mSeconds(seconds % 60)
{

}

const char* UTIL_FormatDuration(CLocalizeFmtCStr&& dest, int seconds)
{
	SRelativeTime duration(seconds);
	dest.Localize("#HL2RP_Duration_HHMMSS",
		UTIL_FormatInteger(dest.mpPlayer, duration.mHours), duration.mMinutes, duration.mSeconds);
	return dest;
}
#endif // HL2RP_CLIENT_OR_LEGACY

const char* UTIL_FormatInteger(CBasePlayer* pPlayer, int value)
{
	return Q_pretifynum(value, *gHL2RPLocalizer.Localize(pPlayer, "#HL2RP_Thousand_Separator"));
}

const char* UTIL_FormatMoney(CLocalizeFmtCStr& dest, int amount)
{
	if (sCurrencyFormatModeCVar.GetInt() < ECurrencyFormatMode::UseLocalizationFormat)
	{
		return dest.Format(sCurrencyFormatModeCVar.GetInt() == ECurrencyFormatMode::AppendSymbolLeft ?
			"%1s%2" : "%2s%1", sCurrencySymbolCVar.GetString(), UTIL_FormatInteger(dest.mpPlayer, amount));
	}

	return dest.Localize("#HL2RP_Money_Display",
		sCurrencySymbolCVar.GetString(), UTIL_FormatInteger(dest.mpPlayer, amount));
}

const char* UTIL_FormatMoney(CLocalizeFmtCStr&& dest, int amount)
{
	return UTIL_FormatMoney(dest, amount);
}
