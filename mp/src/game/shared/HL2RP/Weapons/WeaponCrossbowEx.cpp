#include <cbase.h>
#include <hl2mp/weapon_crossbow.h>

#ifdef CLIENT_DLL
#define CWeaponCrossbowEx	C_WeaponCrossbowEx
#endif // CLIENT_DLL

//----------------------------------------------
// CWeaponCrossbowEx (HL2RP expansion)
//----------------------------------------------
class CWeaponCrossbowEx : public CWeaponCrossbow
{
	DECLARE_CLASS(CWeaponCrossbowEx, CWeaponCrossbow);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponCrossbowEx, DT_WeaponCrossbowEx);

BEGIN_NETWORK_TABLE(CWeaponCrossbowEx, DT_WeaponCrossbowEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_crossbow, CWeaponCrossbowEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponCrossbowEx)
END_PREDICTION_DATA();
#else
acttable_t CWeaponCrossbowEx::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_CROSSBOW, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_CROSSBOW, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_CROSSBOW, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_CROSSBOW, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_CROSSBOW, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_CROSSBOW, true }
};

IMPLEMENT_ACTTABLE(CWeaponCrossbowEx);
#endif // CLIENT_DLL
