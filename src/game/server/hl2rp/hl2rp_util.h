#ifndef HL2RP_UTIL_H
#define HL2RP_UTIL_H
#pragma once

#include <utlpair.h>

class CPlayerEquipment
{
	bool mAllowClipsFallback; // Whether to keep default clip/s from a new weapon when we don't have
	CUtlMap<const char*, CUtlPair<int, int>> mClipsByWeaponClassName;

public:
	void AddWeapon(const char*, int clip1, int clip2);
	void Equip(CBasePlayer*);

	int mHealth, mArmor;
	CAutoLessFuncAdapter<CUtlMap<int, int>> mAmmoCountByIndex;

protected:
	CPlayerEquipment(int health, int armor, bool allowClipsFallback);
};

void UTIL_GetServerTime(tm&, int offset = 0);

// Command helpers
const char* UTIL_GetCommandIssuerName();
bool UTIL_CheckCmdArgCount(const CCommand& args, int minCount = 1); // Checks for min. arg count (*NOT* counting command name), printing usage at failure
bool UTIL_CheckCommandAccess(int minAccessFlag); // If issuer is a player, checks for minimum access flag. Otherwise, checks for host.
void UTIL_ReplyToCommand(int type, const char* pText, const char* pArg1 = "", const char* pArg2 = "");

// Locates a player (based on userid or aim target), replying with a message in case of failure.
// The combination of userIdMinArgs and userIdPos determines if userid-based search is required, in the following way:
// 1. Function checks if arg count is at least userIdMinArgs (min. arg count to check userid), *NOT* counting command name.
// 2. If userid is numeric, then it'll be the only possible search method. Player is then searched by it.
//    Else: if there are more args required behind userid to employ related search, the process ends with failure.
//    Otherwise, the function assumes that userid arg may be valid for other text args, so it allows aim target search.
// If first check fails, the function directly searches by aim target (when issuer is a player).
bool UTIL_FindCmdTarget(const CCommand& args, CHL2Roleplayer*& pTarget, int userIdMinArgs = 2, int userIdPos = 1);

void UTIL_LogAdminAction(CHL2Roleplayer*, const char*, ...); // Logs an admin action (if player is valid), prefixed by their identity for auditing

void UTIL_SendDialog(CBasePlayer*, DIALOG_TYPE, KeyValues* pData);

CUtlString& UTIL_TrimQuotableString(CUtlString&&);

bool UTIL_IsPropertyDoor(CBaseEntity*);
CHL2RP_PropertyDoorData* UTIL_GetPropertyDoorData(CBaseEntity*);
void UTIL_SetDoorLockState(CBaseEntity*, CHL2Roleplayer* pActivator, bool lock, bool save);

#endif // !HL2RP_UTIL_H
