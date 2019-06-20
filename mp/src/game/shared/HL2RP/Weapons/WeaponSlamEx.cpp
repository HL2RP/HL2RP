#include <cbase.h>
#include <hl2mp/weapon_slam.h>

#ifdef CLIENT_DLL
#define CWeaponSLAMEx	C_WeaponSLAMEx
#endif // CLIENT_DLL

//---------------------------------------
// CWeaponSLAMEx (HL2RP expansion)
//---------------------------------------
class CWeaponSLAMEx : public CWeapon_SLAM
{
	DECLARE_CLASS(CWeaponSLAMEx, CWeapon_SLAM);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSLAMEx, DT_Weapon_SLAMEx);

BEGIN_NETWORK_TABLE(CWeaponSLAMEx, DT_Weapon_SLAMEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_slam, CWeaponSLAMEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSLAMEx)
END_PREDICTION_DATA();
#else
acttable_t CWeaponSLAMEx::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SLAM, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SLAM, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SLAM, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SLAM, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SLAM, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SLAM, true }
};

IMPLEMENT_ACTTABLE(CWeaponSLAMEx);
#endif // CLIENT_DLL
