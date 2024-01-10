// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_property.h"
#include "hl2rp_gamerules.h"
#include "hl2_roleplayer.h"
#include <dal.h>
#include <hl2rp_property_dao.h>

ConVar gMaxMapPlayerHomesCVar("sv_max_map_player_homes", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Maximum amount of ownable homes per map and player"),
	gMaxHomeKeysCVar("sv_max_home_keys", "5", FCVAR_ARCHIVE | FCVAR_NOTIFY,
		"Maximum amount of additional players with (un)lock access to a home"),
	gMaxHomeInactivityDays("sv_max_home_inactivity_days", "60", FCVAR_ARCHIVE | FCVAR_NOTIFY,
		"Maximum days since owner's last disconnect time before disowning a home. 0 = Disable the feature.");

CHL2RP_Property::CHL2RP_Property(const char* pMapAlias, int type)
	: mpMapAlias(pMapAlias), mType((EHL2RP_PropertyType)type)
{
	*mName = '\0';
}

CHL2RP_Property::~CHL2RP_Property()
{
	UnlinkZone();
}

bool CHL2RP_Property::HasOwner()
{
	return (mOwnerSteamIdNumber > 0);
}

bool CHL2RP_Property::IsOwner(CBasePlayer* pPlayer)
{
	return (mOwnerSteamIdNumber == pPlayer->GetSteamIDAsUInt64());
}

bool CHL2RP_Property::HasAccess(CHL2Roleplayer* pPlayer, bool forLockingDoors)
{
	switch (mType)
	{
	case EHL2RP_PropertyType::Home:
	{
		return (IsOwner(pPlayer) || (forLockingDoors ?
			mGrantedSteamIdNumbers.HasElement(pPlayer->GetSteamIDAsUInt64()) : pPlayer->IsAdmin()));
	}
	case EHL2RP_PropertyType::Combine:
	{
		return pPlayer->HasCombineGrants(forLockingDoors);
	}
	case EHL2RP_PropertyType::Admin:
	{
		return pPlayer->IsAdmin();
	}
	}

	return (!forLockingDoors && pPlayer->IsAdmin()); // EHL2RP_PropertyType::Public
}

void CHL2RP_Property::LinkZone(CCityZone* pZone, const Vector& samplePoint)
{
	mhZone = pZone;
	mSampleZonePoint = samplePoint;
	pZone->mpProperty = this;
}

void CHL2RP_Property::UnlinkZone()
{
	if (mhZone != NULL)
	{
		mhZone->mpProperty = NULL;
	}

	mhZone.Term();
	mSampleZonePoint.Init();
}

void CHL2RP_Property::LinkDoor(CBaseEntity* pDoor)
{
	mDoors.Insert(pDoor);
	pDoor->mPropertyData.Attach(new CHL2RP_PropertyDoorData(this));
}

bool CHL2RP_Property::Disown(CHL2Roleplayer* pIssuer, int refundPercent /* TODO: Make a related CVar */)
{
	CHL2Roleplayer* pOwner = pIssuer;

	// NOTE: We check the issuer independently since it should only
	// be NULL when auto disowning, which ensures owner is offline
	if (pIssuer != NULL && (IsOwner(pIssuer)
		|| (pOwner = ToHL2Roleplayer(UTIL_PlayerBySteamID(mOwnerSteamIdNumber))) != NULL))
	{
		// Ensure money would be updated into the database
		if (!pOwner->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
		{
			pIssuer->Print(HUD_PRINTTALK, "#HL2RP_House_Sell_OwnerNotLoaded");
			return false;
		}

		pOwner->mHomes.Remove(this);

		// TODO: Directly update player money
	}
	else
	{
		// TODO: Add custom DAO that first retrieves owner data and then updates its money, in ExecuteIO overloads
	}

	FOR_EACH_DICT_FAST(mGrantedSteamIdNumbers, i)
	{
		DAL().AddDAO(new CPropertyGrantsSaveDAO(this, mGrantedSteamIdNumbers[i]));
	}

	FOR_EACH_DICT_FAST(mDoors, i)
	{
		if (mDoors[i] != NULL)
		{
			UTIL_SetDoorLockState(mDoors[i], NULL, false, mDoors[i]->mPropertyData->mDatabaseId.IsValid());
		}
	}

	mOwnerSteamIdNumber = 0;
	mOwnerDisconnectTime = 0;
	mGrantedSteamIdNumbers.Purge();
	DAL().AddDAO(new CPropertiesSaveDAO(this));
	return true;
}

CHL2RP_PropertyDoorData::CHL2RP_PropertyDoorData(CHL2RP_Property* pProperty) : mpProperty(pProperty)
{
	*mName = '\0';
}

void GetPropertyDoorHUDInfo(CBaseEntity* pDoor, CHL2Roleplayer* pPlayer, char* pDest, int maxLen)
{
	if (pDoor->mPropertyData.IsValid())
	{
		const char* pNameToken = "#HL2RP_Property_Name";
		CBaseLocalizeFmtStr<> message(pPlayer, pDest, maxLen);
		CHL2RP_Property* pProperty = pDoor->mPropertyData->mpProperty;

		if (pProperty->mType == EHL2RP_PropertyType::Home)
		{
			pNameToken = "#HL2RP_House_Name";

			if (pProperty->IsOwner(pPlayer))
			{
				message.Localize("#HL2RP_House_Owner_Self");
			}
			else if (pProperty->HasOwner())
			{
				message.Format("%t: %s", "#HL2RP_House_Owner_Other", HL2RPRules()->mPlayerNameBySteamIdNum
					.GetElementOrDefault(pProperty->mOwnerSteamIdNumber, ""));
			}
			else
			{
				message.Localize("#HL2RP_House_ForSale");
			}
		}

		if (*pProperty->mName != '\0')
		{
			message.Format("\n%t: '%s'", pNameToken, pProperty->mName);
		}

		if (*pDoor->mPropertyData->mName != '\0')
		{
			message.Format("\n%t: '%s'", "#HL2RP_Property_Door_Name", pDoor->mPropertyData->mName);
		}

		message.Format("\n[%t]", UTIL_IsDoorLocked(pDoor) ?
			"#HL2RP_Property_Door_Locked" : "#HL2RP_Property_Door_Unlocked");

		if (pProperty->HasAccess(pPlayer))
		{
			pPlayer->SendHUDHint(EPlayerHUDHintType::PropertyDoorAll, "#HL2RP_Hint_PropertyDoorAll",
				"#HL2RP_Hint_PropertyDoorMenu", "#HL2RP_ManagePropertyDoor");
		}
		else if (pProperty->HasAccess(pPlayer, false))
		{
			pPlayer->SendHUDHint(EPlayerHUDHintType::PropertyDoorMenu,
				"#HL2RP_Hint_PropertyDoorMenu", "#HL2RP_ManagePropertyDoor");
		}
	}
	else if (pPlayer->IsAdmin())
	{
		pPlayer->SendHUDHint(EPlayerHUDHintType::PropertyDoorMenu,
			"#HL2RP_Hint_PropertyDoorMenu", "#HL2RP_ManagePropertyDoor");
	}
}
