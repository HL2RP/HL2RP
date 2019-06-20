#include <cbase.h>
#include <hl2mp/weapon_shotgun.h>

#ifdef CLIENT_DLL
#define CWeaponShotgunEx	C_WeaponShotgunEx
#else
#include <ai_basenpc.h>
#include <npcevent.h>
#endif // CLIENT_DLL

class CWeaponShotgunEx : public CWeaponShotgun
{
	DECLARE_CLASS(CWeaponShotgunEx, CWeaponShotgun);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

	int CapabilitiesGet() OVERRIDE;
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary) OVERRIDE;
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator) OVERRIDE;

	void FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool useWeaponAngles);
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponShotgunEx, DT_WeaponShotgunEx);

BEGIN_NETWORK_TABLE(CWeaponShotgunEx, DT_WeaponShotgunEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_shotgun, CWeaponShotgunEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponShotgunEx)
END_PREDICTION_DATA();
#else
acttable_t CWeaponShotgunEx::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SHOTGUN, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SHOTGUN, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SHOTGUN, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SHOTGUN, true },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },
	{ ACT_RELOAD, ACT_RELOAD_SHOTGUN, true },
	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SHOTGUN, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SHOTGUN_RELAXED, false }, // Never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SHOTGUN_STIMULATED, true },
	{ ACT_IDLE_AGITATED, ACT_IDLE_SHOTGUN_AGITATED, true }, // Always aims
	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false }, // Never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, true },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, true }, // Always aims
	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false }, // Never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, true },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, true }, // Always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false }, // Never aims
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, true },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, true }, // Always aims
	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false }, // Never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, true },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, true }, // Always aims
	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false }, // Never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, true },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, true }, // Always aims
	// End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_AIM_SHOTGUN, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_SHOTGUN, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SHOTGUN, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SHOTGUN_LOW, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SHOTGUN_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SHOTGUN, true }
};

IMPLEMENT_ACTTABLE(CWeaponShotgunEx);

int CWeaponShotgunEx::CapabilitiesGet()
{
	return bits_CAP_WEAPON_RANGE_ATTACK1;
}

void CWeaponShotgunEx::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary)
{
	FireNPCPrimaryAttack(pOperator, true);
}

void CWeaponShotgunEx::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SHOTGUN_FIRE:
	{
		FireNPCPrimaryAttack(pOperator, false);
		break;
	}
	default:
	{
		CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
	}
	}
}

void CWeaponShotgunEx::FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool useWeaponAngles)
{
	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC* pNPC = pOperator->MyNPCPointer();
	ASSERT(pNPC != NULL);
	WeaponSound(SINGLE_NPC);
	pOperator->DoMuzzleFlash();
	SubstractClip1(1);

	if (useWeaponAngles)
	{
		// BUG: We experimented this retrieves a wrong position (nearby to the weapon),
		// so the angles retrieval was discarded because bullets wouldn't hit the enemy.
		// This was verified enabling ai_debug_shoot_positions command.
		// We get around this problem by moving GetActualShootTrajectory out of the other conditional block.
		GetAttachment("muzzle", vecShootOrigin);
	}
	else
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
	}

	vecShootDir = pNPC->GetActualShootTrajectory(vecShootOrigin);
	pOperator->FireBullets(8, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH,
		m_iPrimaryAmmoType, 0, -1, -1, GetHL2MPWpnData().m_iPlayerDamage);
}
#endif // CLIENT_DLL
