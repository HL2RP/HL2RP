#ifndef PROP_RATION_DISPENSER_H
#define PROP_RATION_DISPENSER_H
#pragma once

#include "hl2rp_shareddefs.h"

#ifdef GAME_DLL
#include <idto.h>
#include <props.h>
#else
#include <c_props.h>

#define CRation              C_Ration
#define CRationDispenserProp C_RationDispenserProp
#endif // GAME_DLL

#define RATION_DISPENSER_AVAILABILITY_COOLDOWN 1800.0f

#define RATION_DISPENSER_SUPPLY_GLOW_SPRITE_PATH "sprites/plasmaember.vmt"
#define RATION_DISPENSER_DENY_GLOW_SPRITE_PATH   "sprites/redglow2.vmt"

#define RATION_DISPENSER_SUPPLY_SOUND "buttons/button5.wav"
#define RATION_DISPENSER_DENY_SOUND   "npc/attack_helicopter/aheli_damaged_alarm1.wav"

class CRation;

class CRationDispenserProp : public CDynamicProp
{
	DECLARE_CLASS(CRationDispenserProp, CDynamicProp)
	DECLARE_HL2RP_NETWORKCLASS()

	void Spawn() OVERRIDE;
	void Precache() OVERRIDE;
	int	ObjectCaps() OVERRIDE;

	CHandle<CRation> mhContainedRation;

public:
	void OnContainedRationPickup();

	CNetworkVar(float, mNextTimeAvailable); // Controls when dispenser is available to clean citizens

#ifdef GAME_DLL
	SDatabaseIdDTO mDatabaseId;

private:
	void Use(CBaseEntity*, CBaseEntity*, USE_TYPE, float) OVERRIDE;
	void HandleAnimEvent(animevent_t*) OVERRIDE;

	void DoReactionEffects(const char* pSprite, const char* pSound);

	CSimpleSimTimer mReactionEffectsAllowTimer; // Controls when certain effects are allowed, against spamming
#endif // GAME_DLL
};

#endif // !PROP_RATION_DISPENSER_H
