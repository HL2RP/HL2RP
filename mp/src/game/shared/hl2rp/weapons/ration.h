#ifndef RATION_H
#define RATION_H
#pragma once

#include <hl2rp_shareddefs.h>

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

class CRationDispenserProp;

class CRation : public RATION_BASECLASS
{
	SCOPED_ENUM(EPOVState,
		Deployed,
		HaulingBackForThrow,
		ActioningBeforeDeploy
	);

	DECLARE_CLASS(CRation, RATION_BASECLASS)
	DECLARE_HL2RP_NETWORKCLASS()
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE()

#ifdef GAME_DLL
	DECLARE_DATADESC()

	void Spawn() OVERRIDE;
	void FallInit() OVERRIDE;
	void Materialize() OVERRIDE;
	bool VisibleInWeaponSelection() OVERRIDE;
	void HandleAnimEvent(animevent_t*) OVERRIDE;
#endif // GAME_DLL

	bool Deploy() OVERRIDE;
	void FinishReload() OVERRIDE {}
	void ItemPostFrame() OVERRIDE;
	void DefaultTouch(CBaseEntity*) OVERRIDE;
	void OnPickedUp(CBaseCombatCharacter*) OVERRIDE;

	void MergeNearbyRations();
	void RevertExcessClip1();
	void EmitSoundToOwner(int type, CBasePlayer*);

	CNetworkVar(EPOVState, mPOVState)

public:
	void SetTotalUnits(int);
	void AllowPickup();
	void DisallowPickup();

	CHandle<CRationDispenserProp> mhParentDispenser;
};

#endif // RATION_H
