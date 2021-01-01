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

const char* UTIL_GetCommandIssuerName();
void UTIL_ReplyToCommand(int type, const char* pText, const char* pArg1 = "", const char* pArg2 = "");
void UTIL_SendDialog(CBasePlayer*, KeyValues* pData, DIALOG_TYPE);

#endif // !HL2RP_UTIL_H
