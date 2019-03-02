#include <cbase.h>
#include <c_basehlcombatweapon.h>
#include <c_weapon__stubs.h>

class C_Ration : public CBaseHLCombatWeapon
{
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	// Prevents base class from switching viewmodel's activity to ACT_VM_IDLE
	void WeaponIdle() OVERRIDE { }
};

STUB_WEAPON_CLASS_IMPLEMENT(ration, C_Ration);

IMPLEMENT_CLIENTCLASS_DT(C_Ration, DT_Ration, CRation)
END_RECV_TABLE();
