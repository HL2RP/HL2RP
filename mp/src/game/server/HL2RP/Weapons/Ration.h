#ifndef RATION_H
#define RATION_H
#pragma once

#include <basehlcombatweapon.h>

#define RATION_ENSURE_VISIBLE_CONTEXT_NAME	"RationEnsureVisibleContext"

class CPropRationDispenser;

class CRation : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CRation, CBaseHLCombatWeapon);
	DECLARE_DATADESC();
	DECLARE_ACTTABLE();

#ifndef HL2DM_RP
	// On HL2DM, this would block players from connecting to the server.
	// Excluding it cancels client-side initializations, but it's fine.
	DECLARE_SERVERCLASS();
#endif // !HL2DM_RP

	enum EActiveState
	{
		Inactive,
		Deployed,
		Holstering_PreThrow,
		Holstering_PostHeal,
		Holstering_PostThrow,
		Holstered_RefillingPostHeal,
		Holstered_RefillingPostThrow,
		Deploying
	};

	void Precache() OVERRIDE;
	void Spawn() OVERRIDE;
	void HandleFireOnEmpty() OVERRIDE;
	void RemoveAmmoAndHandleEmpty();
	void SecondaryAttack() OVERRIDE;
	void ItemPostFrame() OVERRIDE;
	void OnActiveStateChanged(int oldState) OVERRIDE;
	void OnPickedUp(CBaseCombatCharacter* pNewOwner);
	bool FVisible(CBaseEntity* pEntity, int traceMask, CBaseEntity** ppBlocker) OVERRIDE;
	bool VisibleInWeaponSelection() OVERRIDE;

	// Prevents base class from switching viewmodel's activity to ACT_VM_IDLE
	void WeaponIdle() OVERRIDE { }

	void AllowPickup();

	EActiveState m_ActiveState; // Sub-state for player active weapon

public:
	CRation();

	// Workarounds dropped Ration from showing invisible randomly for non listen clients
	void EnsureVisible();

	CHandle<CPropRationDispenser> m_hParentDispenser;
};

#endif // RATION_H
