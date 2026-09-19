// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include <hl2rp_property.h>
#include "hl2_roleplayer.h"
#include <dal.h>
#include <hl2rp_gamerules.h>
#include <hl2rp_localizer.h>
#include <hl2rp_property_dao.h>
#include <player_dao.h>
#include <player_dialogs.h>

ConVar gMaxMapPlayerHomesCVar("sv_max_map_player_homes", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Maximum amount of ownable Homes per map and player"),
	gMaxHomeKeysCVar("sv_max_home_keys", "5", FCVAR_ARCHIVE | FCVAR_NOTIFY,
		"Maximum amount of additional players with (un)lock access to a Home"),
	gMaxHomeInactivityDaysCVar("sv_max_home_inactivity_days", "60", FCVAR_ARCHIVE | FCVAR_NOTIFY,
		"Maximum days since owner's last seen time before disowning a Home. 0 = Disable the feature."),
	gHouseRefundPercentCVar("sv_house_refund_percent", "50", FCVAR_ARCHIVE | FCVAR_NOTIFY,
		"Percentage off purchase price from Houses to refund the owner on self sale", true, 0.0f, true, 100.0f);

const char* CHL2RP_Property::GetTypeToken(EHL2RP_PropertyType type)
{
	const char* tokens[] = { "#HL2RP_Menu_Property_Type_Public",
		"#HL2RP_Menu_Property_Type_Home", "#HL2RP_Faction_Combine", "#HL2RP_Admin" };
	return tokens[type];
}

CHL2RP_Property::~CHL2RP_Property()
{
	UnlinkZone();
}

void CHL2RP_Property::LinkZone(CCityZone* pZone, const Vector& samplePoint)
{
	mhZone = pZone;
	mSampleZonePoint = samplePoint;
	pZone->mpProperty = this;
	pZone->SendToPlayers();
}

void CHL2RP_Property::UnlinkZone()
{
	if (mhZone != NULL)
	{
		mhZone->mpProperty = NULL;
		mhZone->SendToPlayers();
	}

	mhZone.Term();
	mSampleZonePoint.Init();
}

void CHL2RP_Property::LinkDoor(CBaseEntity* pDoor)
{
	mDoors.Insert(pDoor);
	pDoor->GetPropertyDoorData()->mProperty = this;
}

bool CHL2RP_Property::Disown(CHL2Roleplayer* pIssuer)
{
	CHL2Roleplayer* pOwner = pIssuer;
	int refund = mLastBuyPrice;
	float percent;
	gHouseRefundPercentCVar.GetMax(percent);

	// NOTE: When issuer is NULL, the request should come from auto expiration, which ensures owner is offline
	if (pIssuer != NULL && !IsOwner(pIssuer))
	{
		pOwner = ToHL2Roleplayer(UTIL_PlayerBySteamID(mOwnerSteamIdNumber));
	}

	if (pOwner != NULL) // NOTE: Success also implies pIssuer is valid, so be calm
	{
		// Ensure money would be safely updated into the database
		if (!pOwner->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
		{
			pIssuer->Print(HUD_PRINTTALK, "#HL2RP_House_Sell_OwnerNotLoaded");
			return false;
		}
		else if (pOwner == pIssuer)
		{
			percent = gHouseRefundPercentCVar.GetFloat();
			refund = mLastBuyPrice * percent / 100.0f;
		}
		else
		{
			pOwner->Print(HUD_PRINTTALK, "#HL2RP_House_Sold_Other", pIssuer->GetPlayerName(),
				mName, UTIL_FormatMoney(pOwner, refund), "#HL2RP_House_Refund_Percent", CNumStr(percent));
			UTIL_LogAdminAction(pIssuer, "sold House '%s' from '%s' (%s)",
				mName, pOwner->GetPlayerName(), pOwner->GetNetworkIDString());
		}

		pOwner->mHomes.Remove(this);
		pOwner->AddPocket(refund); // TODO: Use bank instead, for safety and convenience
	}
	else
	{
		DAL().AddDAO(new COfflinePlayerRefundDAO(mOwnerSteamIdNumber, refund));
		DevLog("Automatically sold House '%s' from offline player '%s' (%s) due to inactivity (%i day(s))\n", mName,
			HL2RPRules()->mPlayerNameBySteamIdNum.GetElementOrDefault(mOwnerSteamIdNumber, ""),
			CSteamID::Render(mOwnerSteamIdNumber), gMaxHomeInactivityDaysCVar.GetInt());
	}

	if (pIssuer != NULL)
	{
		pIssuer->Print(HUD_PRINTTALK, "#HL2RP_House_Sold_Issuer", mName,
			UTIL_FormatMoney(pIssuer, refund), "#HL2RP_House_Refund_Percent", CNumStr(percent));
	}

	FOR_EACH_DICT_FAST(mGrantedSteamIdNumbers, i)
	{
		DAL().AddDAO(new CPropertyGrantsSaveDAO(this, mGrantedSteamIdNumbers[i]));

		if (pIssuer != NULL)
		{
			CHL2Roleplayer* pGrantee = ToHL2Roleplayer(UTIL_PlayerBySteamID(mGrantedSteamIdNumbers[i]));

			if (pGrantee != NULL)
			{
				pGrantee->Print(HUD_PRINTTALK, "#HL2RP_Property_Key_Taken_Target", pIssuer->GetPlayerName(), mName);
			}
		}
	}

	FOR_EACH_DICT_FAST(mDoors, i)
	{
		if (mDoors[i] != NULL)
		{
			UTIL_SetDoorLockState(mDoors[i], NULL, false, mDoors[i]->GetPropertyDoorData()->mDatabaseId.IsValid());
		}
	}

	mOwnerSteamIdNumber = mOwnerLastSeenTime = 0;
	mGrantedSteamIdNumbers.Purge();
	Synchronize();
	return true;
}

void CHL2RP_Property::Synchronize(bool create, bool save, CRecipientFilter&& filter)
{
#ifdef HL2RP_FULL
	filter.MakeReliable();
	UserMessageBegin(filter, HL2RP_PROPERTY_UPDATE_USER_MESSAGE);
	WRITE_LONG(mDatabaseId);

	if (create)
	{
		WRITE_BYTE(mType);
		WRITE_STRING(mName);
		WRITE_LONG(mPrice);
		WRITE_VARINT64(mOwnerSteamIdNumber);
	}

	MessageEnd();
#endif // HL2RP_FULL

	if (save)
	{
		DAL().AddDAO(new CPropertiesSaveDAO(this, create));
	}
}

#ifdef HL2RP_FULL
void CHL2RP_Property::SendSteamIdGrantToPlayers(uint64 steamIdNumber, bool grant, CRecipientFilter&& filter)
{
	filter.MakeReliable();
	UserMessageBegin(filter, HL2RP_PROPERTY_GRANT_UPDATE_USER_MESSAGE);
	WRITE_LONG(mDatabaseId);
	WRITE_BOOL(grant);
	WRITE_VARINT64(steamIdNumber);
	MessageEnd();
}
#endif // HL2RP_FULL

void CHL2RP_PropertyDoorData::SpecialUse(CBaseEntity* pDoor, CBaseEntity* pActivator, USE_TYPE type, bool isLocked)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(pActivator);

	if (pPlayer != NULL)
	{
		if (type == USE_SPECIAL1)
		{
			if (mProperty != NULL && mProperty.Get()->HasAccess(pPlayer))
			{
				UTIL_SetDoorLockState(pDoor, pPlayer, !isLocked, mDatabaseId.IsValid());
			}
		}
		else if (type == USE_SPECIAL2)
		{
			pPlayer->SendRootDialog(new CPropertyDoorMenu(pPlayer, pDoor));
		}
	}
}
