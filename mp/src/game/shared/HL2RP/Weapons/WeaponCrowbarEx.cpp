#include <cbase.h>
#include <hl2mp/weapon_crowbar.h>
#include <npcevent.h>

#ifdef CLIENT_DLL
#define CWeaponCrowbarEx	C_WeaponCrowbarEx
#endif // CLIENT_DLL

//--------------------------------------------
// CWeaponCrowbarEx (HL2RP expansion)
//--------------------------------------------
class CWeaponCrowbarEx : public CWeaponCrowbar
{
	DECLARE_CLASS(CWeaponCrowbarEx, CWeaponCrowbar);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

	int CapabilitiesGet() OVERRIDE;
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary) OVERRIDE;
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponCrowbarEx, DT_WeaponCrowbarEx);

BEGIN_NETWORK_TABLE(CWeaponCrowbarEx, DT_WeaponCrowbarEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_crowbar, CWeaponCrowbarEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponCrowbarEx)
END_PREDICTION_DATA();
#else
acttable_t	CWeaponCrowbarEx::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_MELEE, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_MELEE, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_MELEE, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_MELEE, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_MELEE, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_MELEE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_MELEE_ATTACK1, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, true },
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true }
};

IMPLEMENT_ACTTABLE(CWeaponCrowbarEx);

int CWeaponCrowbarEx::CapabilitiesGet()
{
	return bits_CAP_WEAPON_MELEE_ATTACK1;
}

void CWeaponCrowbarEx::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary)
{
	if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_MELEE_HIT))
	{
		animevent_t animEvent;
		animEvent.event = EVENT_WEAPON_MELEE_HIT;
		Operator_HandleAnimEvent(&animEvent, pOperator);
	}
}
#endif // CLIENT_DLL
