#include <cbase.h>
#include <c_basehlcombatweapon.h>
#include <c_weapon__stubs.h>

class C_WeaponMolotov : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(C_WeaponMolotov, CBaseHLCombatWeapon);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	// Prevents base class from switching viewmodel's activity to ACT_VM_IDLE
	void WeaponIdle() OVERRIDE { };
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_molotov, C_WeaponMolotov);

IMPLEMENT_CLIENTCLASS_DT(C_WeaponMolotov, DT_WeaponMolotov, CWeaponMolotov)
END_RECV_TABLE();
