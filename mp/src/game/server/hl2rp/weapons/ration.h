#ifndef RATION_H
#define RATION_H
#pragma once

#include <hl2rp_shareddefs.h>

class CRationDispenserProp;

class CRation : public CBaseCombatWeapon
{
	DECLARE_CLASS(CRation, CBaseCombatWeapon)
	DECLARE_DATADESC()
	DECLARE_ACTTABLE()
	DECLARE_HL2RP_SERVERCLASS()

	enum class EPOVState
	{
		Deployed,
		HaulingBackForThrow,
		ActioningBeforeDeploy
	};

	~CRation();

	void Spawn() OVERRIDE;
	void Precache() OVERRIDE;
	void Materialize() OVERRIDE;
	bool VisibleInWeaponSelection() OVERRIDE;
	bool Deploy() OVERRIDE;
	void ItemPostFrame() OVERRIDE;
	void HandleAnimEvent(animevent_t*) OVERRIDE;
	void OnPickedUp(CBaseCombatCharacter*) OVERRIDE;

	void RevertExcessClip1(CBasePlayer*);

	EPOVState mPOVState;

public:
	void AllowPickup();
	void DisallowPickup();

	CHandle<CRationDispenserProp> mhParentDispenser;
};

#endif // RATION_H
