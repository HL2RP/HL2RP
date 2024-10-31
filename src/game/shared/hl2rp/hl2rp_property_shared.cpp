// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_property.h"

#ifdef GAME_DLL
#include <hl2_roleplayer.h>
#endif // GAME_DLL

#ifdef HL2RP_CLIENT_OR_LEGACY
#include "hl2rp_localizer.h"

#ifdef GAME_DLL
#include <hl2rp_gamerules.h>
#else
#include <c_hl2_roleplayer.h>
#include <c_hl2rp_gamerules.h>
#endif // GAME_DLL
#endif // HL2RP_CLIENT_OR_LEGACY

#ifdef CLIENT_DLL
static void RecvProxy_DoorProperty(const CRecvProxyData* pData, void*, void* pOut)
{
	SDatabaseId id(pData->m_Value.m_Int);
	CHL2RP_Property*& pProperty = *(CHL2RP_Property**)pOut;
	pProperty = HL2RPRules()->mPropertyById.GetElementOrDefault<int, CHL2RP_Property*>(id);

	// Create property in case it wasn't received yet via user message
	if (pProperty == NULL && id.IsValid())
	{
		pProperty = new CHL2RP_Property;
		HL2RPRules()->mPropertyById.Insert(id, pProperty);
	}
}
#elif (defined HL2RP_FULL)
static void SendProxy_DoorProperty(const SendProp*, const void*, const void* pData, DVariant* pOut, int, int)
{
	CHL2RP_Property* pProperty = *(CHL2RP_Property**)pData;
	pOut->m_Int = (pProperty != NULL) ? pProperty->mDatabaseId : SDatabaseId();
}
#endif // CLIENT_DLL

#ifdef HL2RP_FULL
#ifdef GAME_DLL
BEGIN_SEND_TABLE_NOBASE(CHL2RP_PropertyDoorData, DT_HL2RP_PropertyDoorData)
// NOTE: We set the size of sent data explicitly to prevent sending arbitrary values
// on the exceed bits of the 64-bit pointer, which also triggers ENTITY_CHANGE_NONE warnings
SendPropInt(SENDINFO(mProperty), sizeof(int) * 8, 0, SendProxy_DoorProperty),

SendPropString(SENDINFO(mName))
#else
BEGIN_RECV_TABLE_NOBASE(CHL2RP_PropertyDoorData, DT_HL2RP_PropertyDoorData)
RecvPropInt(RECVINFO(mProperty), 0, RecvProxy_DoorProperty),
RecvPropString(RECVINFO(mName))
#endif // GAME_DLL
END_NETWORK_TABLE()
#endif // HL2RP_FULL

CHL2RP_Property::CHL2RP_Property(const char* pMapAlias, int type) : mType((EHL2RP_PropertyType)type)
#ifdef GAME_DLL
	, mpMapAlias(pMapAlias)
#endif // GAME_DLL
{
	*mName = '\0';
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

#ifdef HL2RP_CLIENT_OR_LEGACY
void CHL2RP_PropertyDoorData::GetHUDInfo(CBaseEntity* pDoor, CHL2Roleplayer* pPlayer, CLocalizeFmtStr<>& text)
{
	if (mProperty != NULL)
	{
		const char* pNameToken = "#HL2RP_Property_Name";
		CHL2RP_Property* pProperty = mProperty;

		if (pProperty->mType == EHL2RP_PropertyType::Home)
		{
			pNameToken = "#HL2RP_House_Name";

			if (pProperty->IsOwner(pPlayer))
			{
				text.Format("%t\n", "#HL2RP_House_Owner_Self");
			}
			else if (pProperty->HasOwner())
			{
				text.Format("%t: %s\n", "#HL2RP_House_Owner_Other", HL2RPRules()->mPlayerNameBySteamIdNum
					.GetElementOrDefault(pProperty->mOwnerSteamIdNumber, ""));
			}
			else
			{
				text.Format("%t\n", "#HL2RP_House_ForSale");
			}
		}

		if (*pProperty->mName != '\0')
		{
			text.Format("%t: '%s'\n", pNameToken, pProperty->mName);
		}

		if (*mName != '\0')
		{
			text.Format("%t: '%s'\n", "#HL2RP_Property_Door_Name", mName.Get());
		}

		text.Format("[%t]", pDoor->IsLocked() ? "#HL2RP_Property_Door_Locked" : "#HL2RP_Property_Door_Unlocked");

		if (pProperty->HasAccess(pPlayer))
		{
			pPlayer->LocalDisplayHUDHint(EPlayerHUDHintType::PropertyDoorAll,
				"#HL2RP_Hint_PropertyDoorAll", "#HL2RP_Hint_PropertyDoorMenu", "#HL2RP_ManagePropertyDoor");
		}
		else if (pProperty->HasAccess(pPlayer, false))
		{
			pPlayer->LocalDisplayHUDHint(EPlayerHUDHintType::PropertyDoorMenu,
				"#HL2RP_Hint_PropertyDoorMenu", "#HL2RP_ManagePropertyDoor");
		}
	}
	else if (pPlayer->IsAdmin())
	{
		pPlayer->LocalDisplayHUDHint(EPlayerHUDHintType::PropertyDoorMenu,
			"#HL2RP_Hint_PropertyDoorMenu", "#HL2RP_ManagePropertyDoor");
	}
}
#endif // HL2RP_CLIENT_OR_LEGACY
