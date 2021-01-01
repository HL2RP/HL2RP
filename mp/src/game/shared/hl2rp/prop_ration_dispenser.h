#ifndef PROP_RATION_DISPENSER_H
#define PROP_RATION_DISPENSER_H
#pragma once

#include "hl2rp_shareddefs.h"

#ifdef CLIENT_DLL
#include <c_props.h>

#define CRationDispenserProp C_RationDispenserProp
#else
#include <idto.h>
#include <props.h>

#define RATION_DISPENSER_SF_COMBINE_CONTROLLED 512

class CRation;
#endif // CLIENT_DLL

class CRationDispenserProp : public CDynamicProp
{
	DECLARE_CLASS(CRationDispenserProp, CDynamicProp)
	DECLARE_HL2RP_NETWORKCLASS()

public:
	CNetworkVar(float, mNextTimeAvailable); // Controls when dispenser is available to clean citizens

#ifndef CLIENT_DLL
	void OnContainedRationPickup();

	SDatabaseIdDTO mDatabaseId;
	CHandle<CRation> mhContainedRation;

private:
	~CRationDispenserProp();

	void Spawn() OVERRIDE;
	void Precache() OVERRIDE;
	int	ObjectCaps() OVERRIDE;
	void Use(CBaseEntity*, CBaseEntity*, USE_TYPE, float) OVERRIDE;
	void HandleAnimEvent(animevent_t*) OVERRIDE;

	void DoReactionEffects(const char* pSprite, const char* pSound);

	CSimpleSimTimer mReactionEffectsAllowTimer; // Controls when certain effects are allowed, against spamming
#endif // !CLIENT_DLL
};

#endif // !PROP_RATION_DISPENSER_H
