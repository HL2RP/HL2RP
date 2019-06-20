#ifndef WEAPON_MOLOTOV_H
#define WEAPON_MOLOTOV_H
#pragma once

#include <basehlcombatweapon.h>

class CGrenade_Molotov;

class CWeaponMolotov : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponMolotov, CBaseHLCombatWeapon);
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
		Haulback_PreThrow,
		Throwing_PreProp, // Throwing anim, Molotov prop not spawned yet
		Throwing_PostProp, // Throwing anim, Molotov prop spawned, refilling...
		Deploying
	};

	void HandleFireOnEmpty() OVERRIDE;
	void PrimaryAttack() OVERRIDE;
	void ItemPostFrame() OVERRIDE;
	void OnActiveStateChanged(int oldState) OVERRIDE;
	bool CanBeSelected() OVERRIDE;

	// Prevents base class from switching viewmodel's activity to ACT_VM_IDLE
	void WeaponIdle() OVERRIDE { }

	void ThrowMolotov(const Vector& vecSrc, const Vector& vecVelocity);

	EActiveState m_ActiveState;
};

#endif // !WEAPON_MOLOTOV_H
