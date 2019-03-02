#include <cbase.h>
#include <hl2mp/weapon_pistol.h>
#include <npcevent.h>

#ifdef CLIENT_DLL
#define CWeaponPistolEx	C_WeaponPistolEx
#else
#include <ai_basenpc.h>
#include <soundent.h>
#endif // CLIENT_DLL

//------------------------------------------
// CWeaponPistolEx (HL2RP expansion)
//------------------------------------------
class CWeaponPistolEx : public CWeaponPistol
{
	DECLARE_CLASS(CWeaponPistol, CBaseHL2MPCombatWeapon);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // HL2RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

	int CapabilitiesGet() OVERRIDE;
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator) OVERRIDE;
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary) OVERRIDE;
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponPistolEx, DT_WeaponPistolEx);

BEGIN_NETWORK_TABLE(CWeaponPistolEx, DT_WeaponPistolEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_pistol, CWeaponPistolEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponPistolEx)
END_PREDICTION_DATA();
#else
acttable_t CWeaponPistolEx::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PISTOL, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PISTOL, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PISTOL, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PISTOL, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_PISTOL, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PISTOL, true },
	{ ACT_IDLE, ACT_IDLE_PISTOL, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_PISTOL, true },
	{ ACT_RELOAD, ACT_RELOAD_PISTOL, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_PISTOL, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_PISTOL, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_PISTOL, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD_PISTOL_LOW, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_PISTOL_LOW, true },
	{ ACT_COVER_LOW, ACT_COVER_PISTOL_LOW, true },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_PISTOL_LOW, true },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_PISTOL, true },
	{ ACT_WALK, ACT_WALK_PISTOL, true },
	{ ACT_RUN, ACT_RUN_PISTOL, true }
};

IMPLEMENT_ACTTABLE(CWeaponPistolEx);

int CWeaponPistolEx::CapabilitiesGet()
{
	return bits_CAP_WEAPON_RANGE_ATTACK1;
}

void CWeaponPistolEx::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_PISTOL_FIRE:
	{
		Vector shootOrigin;
		shootOrigin = pOperator->Weapon_ShootPosition();
		CAI_BaseNPC* pNPC = pOperator->MyNPCPointer();
		ASSERT(pNPC != NULL);
		Vector shootDir = pNPC->GetActualShootTrajectory(shootOrigin);
		CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(),
			SOUNDENT_VOLUME_PISTOL, 0.2f, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
		WeaponSound(SINGLE_NPC);
		pOperator->FireBullets(1, shootOrigin, shootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH,
			m_iPrimaryAmmoType, 2, -1, -1, GetHL2MPWpnData().m_iPlayerDamage);
		pOperator->DoMuzzleFlash();
		SubstractClip1(1);
		break;
	}
	default:
	{
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
	}
	}
}

void CWeaponPistolEx::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary)
{
	if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_PISTOL_FIRE))
	{
		animevent_t animEvent;
		animEvent.event = EVENT_WEAPON_PISTOL_FIRE;
		Operator_HandleAnimEvent(&animEvent, pOperator);
	}
}
#endif // CLIENT_DLL
