#ifndef HL2RP_PROPERTY_H
#define HL2RP_PROPERTY_H
#pragma once

#include <idto.h>

#define HL2RP_PROPERTY_NAME_SIZE 32

SCOPED_ENUM(EHL2RP_PropertyType,
	Public,
	Home,
	Combine,
	Admin
);

class CCityZone;
class CHL2Roleplayer;

void GetPropertyDoorHUDInfo(CBaseEntity*, CHL2Roleplayer*, char* pDest, int maxLen);

class CHL2RP_Property
{
public:
	CHL2RP_Property(const char* pMapAlias, int type);

	~CHL2RP_Property();

	bool HasOwner();
	bool IsOwner(CBasePlayer*);
	bool HasAccess(CHL2Roleplayer*, bool forLockingDoors = true);
	void LinkZone(CCityZone*, const Vector& samplePoint);
	void UnlinkZone();
	void LinkDoor(CBaseEntity*);
	bool Disown(CHL2Roleplayer* pIssuer, int refundPercent);

	SDatabaseIdDTO mDatabaseId;
	const char* mpMapAlias; // Linked map/group
	EHL2RP_PropertyType mType;
	char mName[HL2RP_PROPERTY_NAME_SIZE];
	uint64 mOwnerSteamIdNumber = 0;
	int mOwnerDisconnectTime = 0;
	CHandle<CCityZone> mhZone;
	Vector mSampleZonePoint = vec3_origin;
	CAutoLessFuncAdapter<CUtlRBTree<EHANDLE>> mDoors;
	CAutoLessFuncAdapter<CUtlRBTree<uint64>> mGrantedSteamIdNumbers; // Additional Steam IDs with "keys" (for homes)
};

class CHL2RP_PropertyDoorData
{
public:
	CHL2RP_PropertyDoorData(CHL2RP_Property*);

	SDatabaseIdDTO mDatabaseId;
	CHL2RP_Property* mpProperty;
	char mName[HL2RP_PROPERTY_NAME_SIZE];
};

template<class T = CBaseEntity>
class CHL2RP_PropertyDoor : public T
{
	DECLARE_CLASS(CHL2RP_PropertyDoor, T)

	void GetHUDInfo(CHL2Roleplayer* pPlayer, char* pDest, int maxLen) OVERRIDE
	{
		GetPropertyDoorHUDInfo(this, pPlayer, pDest, maxLen);
	}

protected:
	int	ObjectCaps() OVERRIDE { return (T::ObjectCaps() | FCAP_HL2RP_PROPERTY_DOOR); }
};

#endif // !HL2RP_PROPERTY_H
