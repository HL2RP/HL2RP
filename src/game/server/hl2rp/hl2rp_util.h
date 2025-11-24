#ifndef HL2RP_UTIL_H
#define HL2RP_UTIL_H
#pragma once

#include <generic.h>
#include <string_t.h>
#include <utlmap.h>
#include <utlpair.h>
#include <utlstring.h>

class KeyValues;

struct SUtlField
{
	enum class EType
	{
		Null, // For SQL NULL values saving (e.g. FK fields reset)
		Int,
		UInt64,
		Float,
		String
	};

	static SUtlField FromKeyValues(KeyValues*); // Converts the value tied to the KV

	SUtlField() : mType(EType::Null) {}
	SUtlField(int value) : mUInt64(value), mType(EType::Int) {}
	SUtlField(uint64 value) : mUInt64(value), mType(EType::UInt64) {}
	SUtlField(float value) : mFloat(value), mType(EType::Float) {}
	SUtlField(const char* pValue) : mString(pValue), mType(EType::String) {}
	SUtlField(const string_t& value) : SUtlField(STRING(value)) {}

	operator const char* () const;

	int ToInt() const;
	uint64 ToUInt64() const;
	float ToFloat() const;
	CUtlString ToString() const;

	union
	{
		int mInt;
		uint64 mUInt64;
		float mFloat;
	};

	CUtlConstString mString;
	EType mType;
};

#ifdef GAME_DLL
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

const char* UTIL_GetCommandIssuerName();
void UTIL_ReplyToCommand(int type, const char* pText, const char* pArg1 = "", const char* pArg2 = "");

void UTIL_SendDialog(CBasePlayer*, KeyValues* pData, DIALOG_TYPE);

CUtlString& UTIL_TrimQuotableString(CUtlString&&);

bool UTIL_IsPropertyDoor(CBaseEntity*);
CHL2RP_PropertyDoorData* UTIL_GetPropertyDoorData(CBaseEntity*);
void UTIL_SetDoorLockState(CBaseEntity*, CHL2Roleplayer* pActivator, bool lock, bool save);
#endif // GAME_DLL

#endif // !HL2RP_UTIL_H
