#include <cbase.h>
#include <hl2mp/weapon_physcannon.h>

#ifdef CLIENT_DLL
#define CWeaponPhysCannonEx	C_WeaponPhysCannonEx
#endif // CLIENT_DLL

//--------------------------------------------------
// CWeaponPhysCannonEx (HL2RP expansion)
//--------------------------------------------------
class CWeaponPhysCannonEx : public CWeaponPhysCannon
{
	DECLARE_CLASS(CWeaponPhysCannonEx, CWeaponPhysCannon);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponPhysCannonEx, DT_WeaponPhysCannonEx);

BEGIN_NETWORK_TABLE(CWeaponPhysCannonEx, DT_WeaponPhysCannonEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_physcannon, CWeaponPhysCannonEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponPhysCannonEx)
END_PREDICTION_DATA();
#else
acttable_t CWeaponPhysCannonEx::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PHYSGUN, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PHYSGUN, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PHYSGUN, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PHYSGUN, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_PHYSGUN, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_PHYSGUN, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PHYSGUN, true }
};

IMPLEMENT_ACTTABLE(CWeaponPhysCannonEx);
#endif // CLIENT_DLL
