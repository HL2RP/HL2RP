#ifndef SUITCASE_H
#define SUITCASE_H
#pragma once

#include <basehlcombatweapon.h>

class CSuitcase : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CSuitcase, CBaseHLCombatWeapon);
	DECLARE_ACTTABLE();

#ifndef HL2DM_RP
	// On HL2DM, this would block players from connecting to the server.
	// Excluding it cancels client-side initializations, but it's fine.
	DECLARE_SERVERCLASS();
#endif

	enum EActiveState
	{
		Inactive,
		Deployed,
		Holstering
	};

	void SecondaryAttack() OVERRIDE;
	void ItemPostFrame() OVERRIDE;
	void OnActiveStateChanged(int oldState) OVERRIDE;
	bool VisibleInWeaponSelection() OVERRIDE;

	// Prevents base class from switching viewmodel's activity to ACT_VM_IDLE
	void WeaponIdle() OVERRIDE { }

	EActiveState m_ActiveState;
};

#endif // !SUITCASE_H
