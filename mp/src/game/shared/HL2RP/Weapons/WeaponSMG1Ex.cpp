#include <cbase.h>
#include <hl2mp/weapon_smg1.h>

#ifdef CLIENT_DLL
#define CWeaponSMG1Ex	C_WeaponSMG1Ex
#else
#include <ai_basenpc.h>
#include <npcevent.h>
#endif // CLIENT_DLL

//--------------------------------------
// CWeaponSMG1Ex (HL2RP expansion)
//--------------------------------------
class CWeaponSMG1Ex : public CWeaponSMG1
{
	DECLARE_CLASS(CWeaponSMG1Ex, CWeaponSMG1);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

	int CapabilitiesGet() OVERRIDE;
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator) OVERRIDE;
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary) OVERRIDE;

	void FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, Vector& shootOrigin,
		Vector& shootDir);
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSMG1Ex, DT_WeaponSMG1Ex);

BEGIN_NETWORK_TABLE(CWeaponSMG1Ex, DT_WeaponSMG1Ex)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_smg1, CWeaponSMG1Ex);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSMG1Ex)
END_PREDICTION_DATA();
#else
acttable_t CWeaponSMG1Ex::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SMG1, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SMG1, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SMG1, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SMG1, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SMG1, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SMG1, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SMG1, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },
	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false }, // Never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, true },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, false }, // Always aims
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

	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SMG1, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, true },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_SMG1_LOW, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, true },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true }
};

IMPLEMENT_ACTTABLE(CWeaponSMG1Ex);

int CWeaponSMG1Ex::CapabilitiesGet()
{
	return bits_CAP_WEAPON_RANGE_ATTACK1;
}

void CWeaponSMG1Ex::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	{
		Vector shootOrigin;

		// Support old style attachment point firing
		if (pEvent->options == NULL || pEvent->options[0] == '\0'
			|| !pOperator->GetAttachment(pEvent->options, shootOrigin))
		{
			shootOrigin = pOperator->Weapon_ShootPosition();
		}

		CAI_BaseNPC* pNPC = pOperator->MyNPCPointer();
		ASSERT(pNPC != NULL);
		Vector shootDir = pNPC->GetActualShootTrajectory(shootOrigin);
		FireNPCPrimaryAttack(pOperator, shootOrigin, shootDir);
		break;
	}
	default:
	{
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
	}
	}
}

void CWeaponSMG1Ex::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary)
{
	if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_SMG1))
	{
		animevent_t animEvent;
		animEvent.event = EVENT_WEAPON_SMG1;
		animEvent.options = "muzzle";
		Operator_HandleAnimEvent(&animEvent, pOperator);
	}
}

void CWeaponSMG1Ex::FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, Vector& shootOrigin,
	Vector& shootDir)
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(),
		SOUNDENT_VOLUME_MACHINEGUN, 0.2f, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	pOperator->FireBullets(1, shootOrigin, shootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH,
		m_iPrimaryAmmoType, 2, entindex(), 0, GetHL2MPWpnData().m_iPlayerDamage);
	pOperator->DoMuzzleFlash();
	SubstractClip1(1);
}
#endif // CLIENT_DLL
