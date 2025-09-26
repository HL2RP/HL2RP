// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_util.h"
#include "hl2_roleplayer.h"
#include <dal.h>
#include <hl2rp_localizer.h>
#include <hl2rp_property_dao.h>
#include <inetchannel.h>
#include <utlbuffer.h>

// Engine limits
#define NET_MAX_MESSAGE  4096
#define NET_SETCONVAR    5
#define NETMSG_TYPE_BITS 6
#define SVC_MENU         29

SUtlField::operator const char* () const
{
	return mString;
}

int SUtlField::ToInt() const
{
	return ToUInt64();
}

uint64 SUtlField::ToUInt64() const
{
	switch (mType)
	{
	case EType::Float:
	{
		return Float2Int(mFloat);
	}
	case EType::String:
	{
		return Q_atoui64(mString);
	}
	}

	return mUInt64;
}

float SUtlField::ToFloat() const
{
	switch (mType)
	{
	case EType::Float:
	{
		return mFloat;
	}
	case EType::String:
	{
		return Q_atof(mString);
	}
	}

	return mInt;
}

CUtlString SUtlField::ToString() const
{
	switch (mType)
	{
	case EType::Int:
	{
		return CNumStr(mInt).String();
	}
	case EType::UInt64:
	{
		return CNumStr(mUInt64).String();
	}
	case EType::Float:
	{
		return CNumStr(mFloat).String();
	}
	}

	return mString.Get();
}

SDatabaseId::operator SUtlField()
{
	return mId;
}

bool SDatabaseId::SetForLoading()
{
	if (mId < LOADING_DATABASE_ID)
	{
		mId = LOADING_DATABASE_ID;
		return true;
	}

	return false;
}

CPlayerEquipment::CPlayerEquipment(int health, int armor, bool allowClipsFallback) : mHealth(health),
mArmor(armor), mAllowClipsFallback(allowClipsFallback), mClipsByWeaponClassName(CaselessStringLessThan)
{

}

void CPlayerEquipment::AddWeapon(const char* pClassName, int clip1, int clip2)
{
	if (!mClipsByWeaponClassName.IsValidIndex(mClipsByWeaponClassName.Find(pClassName)))
	{
		// NOTE: As classname should be level-alloc'ed by CBaseEntity::SetClassname, we alloc it that way too
		int index = mClipsByWeaponClassName.Insert(STRING(AllocPooledString(pClassName)));
		mClipsByWeaponClassName[index].first = clip1;
		mClipsByWeaponClassName[index].second = clip2;
	}
}

void CPlayerEquipment::Equip(CBasePlayer* pPlayer)
{
	pPlayer->SetHealth(mHealth > 0 ? mHealth : pPlayer->GetHealth());
	pPlayer->SetArmorValue(Max(pPlayer->ArmorValue(), pPlayer->ArmorValue() + mArmor));

	FOR_EACH_MAP_FAST(mAmmoCountByIndex, i)
	{
		pPlayer->GiveAmmo(mAmmoCountByIndex[i], mAmmoCountByIndex.Key(i));
	}

	FOR_EACH_MAP_FAST(mClipsByWeaponClassName, i)
	{
		int clip1 = 0, clip2 = 0; // Base clips, really
		CBaseCombatWeapon* pWeapon = pPlayer->Weapon_OwnsThisType(mClipsByWeaponClassName.Key(i));

		if (pWeapon == NULL)
		{
			CBaseEntity* pEntity = CBaseEntity::CreateNoSpawn(mClipsByWeaponClassName.Key(i),
				pPlayer->GetAbsOrigin(), vec3_angle);

			if (pEntity != NULL)
			{
				pWeapon = pEntity->MyCombatWeaponPointer();

				// If entity isn't a weapon, remove it
				if (pWeapon == NULL)
				{
					pEntity->Remove();
					continue;
				}

				DispatchSpawn(pWeapon);

				// Remove weapon script ammunition
				pWeapon->SetPrimaryAmmoCount(0);
				pWeapon->SetSecondaryAmmoCount(0);

				// Equip directly instead of via Touch(), to ensure picking weapon if outside world
				pPlayer->Weapon_Equip(pWeapon);

				if (mAllowClipsFallback)
				{
					if (mClipsByWeaponClassName[i].first < 1)
					{
						clip1 = pWeapon->m_iClip1;
					}

					if (mClipsByWeaponClassName[i].second < 1)
					{
						clip2 = pWeapon->m_iClip2;
					}
				}
			}
		}
		else
		{
			clip1 = pWeapon->m_iClip1;
			clip2 = pWeapon->m_iClip2;
		}

		if (pWeapon->UsesClipsForAmmo1())
		{
			pWeapon->m_iClip1 = Clamp(clip1 + mClipsByWeaponClassName[i].first, clip1, pWeapon->GetMaxClip1());
		}

		if (pWeapon->UsesClipsForAmmo2())
		{
			pWeapon->m_iClip2 = Clamp(clip2 + mClipsByWeaponClassName[i].second, clip2, pWeapon->GetMaxClip2());
		}
	}
}

void UTIL_GetServerTime(tm& destTime, int offset)
{
	time_t timeStamp = 0;
	VCRHook_Time((long*)&timeStamp);
	timeStamp += offset;
	Plat_localtime(&timeStamp, &destTime);
}

const char* UTIL_GetCommandIssuerName()
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	return (pPlayer != NULL ? pPlayer->GetPlayerName() : "CONSOLE");
}

void UTIL_ReplyToCommand(int type, const char* pText, const char* pArg1, const char* pArg2)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		return pPlayer->Print(type, pText, pArg1, pArg2);
	}

	Msg("%s\n", (*pText == '#') ? CLocalizeFmtStr<>().Localize(pText, pArg1, pArg2) : pText);
}

INetChannel* GetPlayerNetChannel(CBasePlayer* pPlayer)
{
	return static_cast<INetChannel*>(engine->GetPlayerNetInfo(pPlayer->entindex()));
}

void UTIL_SendDialog(CBasePlayer* pPlayer, KeyValues* pData, DIALOG_TYPE type)
{
	INetChannel* pNetChannel = GetPlayerNetChannel(pPlayer);

	if (pNetChannel != NULL)
	{
		old_bf_write_static<NET_MAX_MESSAGE> msg;
		msg.WriteUBitLong(SVC_MENU, NETMSG_TYPE_BITS);
		msg.WriteShort(type);
		CUtlBuffer buffer;
		pData->WriteAsBinary(buffer);
		msg.WriteWord(buffer.TellPut());
		msg.WriteBytes(buffer.Base(), buffer.TellPut());
		pNetChannel->SendData(msg);
	}
}

CUtlString& UTIL_TrimQuotableString(CUtlString&& dest)
{
	dest.Trim("'\"\t\r\n ");
	return dest;
}

bool UTIL_IsPropertyDoor(CBaseEntity* pEntity)
{
	return (UTIL_GetPropertyDoorData(pEntity) != NULL);
}

CHL2RP_PropertyDoorData* UTIL_GetPropertyDoorData(CBaseEntity* pEntity)
{
	return (pEntity != NULL ? pEntity->GetPropertyDoorData() : NULL);
}

void UTIL_SetDoorLockState(CBaseEntity* pDoor, CHL2Roleplayer* pActivator, bool lock, bool save)
{
	const char* pInput = "Unlock", * pSound = HL2RP_PROPERTY_DOOR_UNLOCK_SOUND, * pToken = "#HL2RP_Property_Door_Unlock";

	if (lock)
	{
		pInput = "Lock";
		pSound = HL2RP_PROPERTY_DOOR_LOCK_SOUND;
		pToken = "#HL2RP_Property_Door_Lock";
	}

	pDoor->AcceptInput(pInput, pActivator, pActivator);

	if (pActivator != NULL)
	{
		pDoor->EmitSound(pSound);
		pActivator->Print(HUD_PRINTTALK, pToken);
	}

	if (save)
	{
		DAL().AddDAO(new CPropertyDoorsSaveDAO(pDoor));
	}
}
