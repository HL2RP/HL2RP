#ifndef HL2RP_PROPERTY_H
#define HL2RP_PROPERTY_H
#pragma once

#include "hl2rp_shareddefs.h"
#include "hl2rp_util_shared.h"

#ifdef GAME_DLL
EXTERN_SEND_TABLE(DT_HL2RP_PropertyDoorData)
#else
EXTERN_RECV_TABLE(DT_HL2RP_PropertyDoorData)
#endif // GAME_DLL

#define HL2RP_PROPERTY_NAME_SIZE 32

SCOPED_ENUM(EHL2RP_PropertyType, // NOTE: Don't change order
	Public,
	Home,
	Combine,
	Admin
);

class CCityZone;

class CHL2RP_Property
{
public:
	CHL2RP_Property(const char* pMapAlias = "", int type = EHL2RP_PropertyType::Public);

	bool HasOwner();
	bool IsOwner(CBasePlayer*);
	bool HasAccess(CHL2Roleplayer*, bool forLockingDoors = true);

#ifdef GAME_DLL
	static const char* GetTypeToken(EHL2RP_PropertyType);

	~CHL2RP_Property();

	void LinkZone(CCityZone*, const Vector& samplePoint);
	void UnlinkZone();
	void LinkDoor(CBaseEntity*);
	bool Disown(CHL2Roleplayer* pIssuer);
	void Synchronize(bool create = true, bool save = true, CRecipientFilter && = CBroadcastRecipientFilter());
	void SendSteamIdGrantToPlayers(uint64, bool grant = true, CRecipientFilter && = CBroadcastRecipientFilter()) HL2RP_FULL_FUNCTION;

	SDatabaseId mDatabaseId;
	const char* mpMapAlias; // Linked map/group
	long mOwnerLastSeenTime = 0;
	CHandle<CCityZone> mhZone;
	Vector mSampleZonePoint = vec3_origin;
	CAutoLessFuncAdapter<CUtlRBTree<EHANDLE>> mDoors;
#endif // GAME_DLL

	EHL2RP_PropertyType mType;
	char mName[HL2RP_PROPERTY_NAME_SIZE];
	CPositiveVar<> mPrice, mLastBuyPrice; // mLastBuyPrice ensures safe refunding, in case current sale price has changed
	uint64 mOwnerSteamIdNumber = 0;
	CAutoLessFuncAdapter<CUtlRBTree<uint64>> mGrantedSteamIdNumbers; // Additional Steam IDs with "keys" (for homes)
};

class CHL2RP_PropertyDoorData
{
	DECLARE_CLASS_NOBASE(CHL2RP_PropertyDoorData)
	DECLARE_EMBEDDED_NETWORKVAR()

public:
	void GetHUDInfo(CBaseEntity*, CHL2Roleplayer*, CLocalizeFmtStr<>&) HL2RP_CLIENT_OR_LEGACY_FUNCTION;
	void SpecialUse(CBaseEntity*, CBaseEntity* pActivator, USE_TYPE, bool isLocked);

	// Checks if a USE request should fail while opening for a different activator than the opener (based on the attempt)
	bool BlockDoorCloseSpam(CBaseEntity* pOldActivator, CBaseEntity* pNewActivator);

#ifdef GAME_DLL
	SDatabaseId mDatabaseId;
	bool mCloseSpamBegun:1; // True when a different activator started closing while opening, to block next attempt for current move
#endif // GAME_DLL

	CNetworkVar(CHL2RP_Property*, mProperty);
	CNetworkString(mName, HL2RP_PROPERTY_NAME_SIZE);
};

template<class S = CBaseEntity>
class CHL2RP_PropertyDoor : public S
{
	DECLARE_CLASS(CHL2RP_PropertyDoor, S)

	CHL2RP_PropertyDoorData* GetPropertyDoorData() OVERRIDE
	{
		return &mPropertyDoorData;
	}

public:
	bool IsLocked() OVERRIDE
	{
		return m_bLocked;
	}

	CNetworkVar(bool, m_bLocked);

protected:
#ifdef GAME_DLL
	void MoveDone() OVERRIDE
	{
		BaseClass::MoveDone();
		mPropertyDoorData.mCloseSpamBegun = false;
	}

	void StartBlocked(CBaseEntity*) OVERRIDE
	{
		mPropertyDoorData.mCloseSpamBegun = false; // Prevent an unrelated blocker from causing further close requests to fail indefinitely
	}
#endif // GAME_DLL

	void GetHUDInfo(CHL2Roleplayer* pPlayer, CLocalizeFmtStr<>& text) OVERRIDE
	{
		mPropertyDoorData.GetHUDInfo(this, pPlayer, text);
	}

	CNetworkVarEmbedded(CHL2RP_PropertyDoorData, mPropertyDoorData);
};

#endif // !HL2RP_PROPERTY_H
