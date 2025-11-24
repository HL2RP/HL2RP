#ifndef PROP_RATION_DISPENSER_H
#define PROP_RATION_DISPENSER_H
#pragma once

#include <ration.h>

#ifdef GAME_DLL
#include "hl2rp_util_shared.h"
#include <props.h>
#else
#include <c_props.h>
#endif // GAME_DLL

#define RATION_DISPENSER_SF_COMBINE_CONTROLLED 512

#define RATION_DISPENSER_LOCK_COOLDOWN 120.0f
#define RATION_DISPENSER_USE_COOLDOWN  1800.0f // 30 minutes

#define RATION_DISPENSER_MODEL_PATH "models/props_combine/combine_dispenser.mdl"

class CRationDispenserProp : public CDynamicProp
{
	SCOPED_ENUM(EPositionEvent,
		FullyOpen,
		FullyClosed
	);

	DECLARE_CLASS(CRationDispenserProp, CDynamicProp)
	DECLARE_HL2RP_NETWORKCLASS()

	void Spawn() OVERRIDE;
	void GetHUDInfo(CHL2Roleplayer*, CLocalizeFmtStr<>&) OVERRIDE HL2RP_CLIENT_OR_LEGACY_FUNCTION;

	CHandle<CRation> mhContainedRation;

public:
	void OnContainedRationPickup();

	CNetworkVar(bool, mIsLocked);
	CNetworkVar(float, mNextTimeAvailable); // Controls when dispenser is available to clean citizens

#ifdef GAME_DLL
	DECLARE_DATADESC()

	void Use(CBaseEntity*, CBaseEntity*, USE_TYPE, float) OVERRIDE;

	int mRationsAmmo;
	SDatabaseId mDatabaseId;
	const char* mpMapAlias = ""; // Linked map/group

private:
	void Precache() OVERRIDE;
	int	ObjectCaps() OVERRIDE;
	void HandleAnimEvent(animevent_t*) OVERRIDE;

	void UnlockThink();
	void DoReactionEffects(const char* pSound, const char* pSprite = NULL);

	CSimpleSimTimer mCrimeEffectsTimer; // Limits actions during crime effects, against spam
#else
private:
	int m_spawnflags;
#endif // GAME_DLL
};

#endif // !PROP_RATION_DISPENSER_H
