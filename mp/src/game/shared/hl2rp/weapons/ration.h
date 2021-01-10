#ifndef RATION_H
#define RATION_H
#pragma once

#include <hl2rp_shareddefs.h>
#include <generic.h>

#ifdef HL2RP_FULL
#include <weapon_hl2mpbasehlmpcombatweapon.h>

#define RATION_BASECLASS CBaseHL2MPCombatWeapon
#else
#define RATION_BASECLASS CBaseCombatWeapon
#endif // HL2RP_FULL

#ifdef CLIENT_DLL
#define CRation              C_Ration
#define CRationDispenserProp C_RationDispenserProp
#endif // CLIENT_DLL

ENUM(ERationPOVState,
	Deployed,
	HaulingBackForThrow,
	ActioningBeforeDeploy
)

class CRationDispenserProp;

class CRation : public RATION_BASECLASS
{
	DECLARE_CLASS(CRation, RATION_BASECLASS)
	DECLARE_PREDICTABLE();
	DECLARE_HL2RP_NETWORKCLASS()
	DECLARE_ACTTABLE()

#ifdef GAME_DLL
	DECLARE_DATADESC()

	void Spawn() OVERRIDE;
	void FallInit() OVERRIDE;
	void Materialize() OVERRIDE;
	void HandleAnimEvent(animevent_t*) OVERRIDE;
#endif // GAME_DLL

	void Precache() OVERRIDE;
	bool VisibleInWeaponSelection() OVERRIDE;
	bool Deploy() OVERRIDE;
	void ItemPostFrame() OVERRIDE;
	void OnPickedUp(CBaseCombatCharacter*) OVERRIDE;

	void RevertExcessClip1();
	void EmitSoundToOwner(int type, CBasePlayer*);

	CNetworkVar(ERationPOVState::_Value, mPOVState)

public:
	void AllowPickup();
	void DisallowPickup();

	CHandle<CRationDispenserProp> mhParentDispenser;
};

#endif // RATION_H
