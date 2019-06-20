#include <cbase.h>
#include <hl2mp/weapon_ar2.h>

#ifdef CLIENT_DLL
#define CWeaponAR2Ex	C_WeaponAR2Ex
#else
#include <npcevent.h>
#include <npc_combine.h>
#include <prop_combine_ball.h>

extern ConVar sk_weapon_ar2_alt_fire_radius;
extern ConVar sk_weapon_ar2_alt_fire_duration;
extern ConVar sk_weapon_ar2_alt_fire_mass;
#endif // CLIENT_DLL

//------------------------------------
// CWeaponAr2Ex (HL2RP expansion)
//------------------------------------
class CWeaponAR2Ex : public CWeaponAR2
{
	DECLARE_CLASS(CWeaponAR2Ex, CWeaponAR2);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

	int CapabilitiesGet() OVERRIDE;
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator) OVERRIDE;
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary) OVERRIDE;

	void FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool useWeaponAngles);
	void FireNPCSecondaryAttack(CBaseCombatCharacter* pOperator, bool useWeaponAngles);
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponAR2Ex, DT_WeaponAR2Ex);

BEGIN_NETWORK_TABLE(CWeaponAR2Ex, DT_WeaponAR2Ex)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_ar2, CWeaponAR2Ex);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponAR2Ex)
END_PREDICTION_DATA();
#else
acttable_t CWeaponAR2Ex::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR2, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_AR2, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_AR2, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_AR2, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_AR2, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_AR2, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_AR2, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },
	{ ACT_WALK, ACT_WALK_RIFLE, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false }, // Never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, true },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, true }, // Always aims
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

	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_AR2, true },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, true },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_AR2_LOW, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, true },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true }
};

IMPLEMENT_ACTTABLE(CWeaponAR2Ex);

int CWeaponAR2Ex::CapabilitiesGet()
{
	return bits_CAP_RANGE_ATTACK_GROUP;
}

void CWeaponAR2Ex::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_AR2:
	{
		FireNPCPrimaryAttack(pOperator, false);
		break;
	}
	case EVENT_WEAPON_AR2_ALTFIRE:
	{
		FireNPCSecondaryAttack(pOperator, false);
		break;
	}
	default:
	{
		CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
	}
	}
}

void CWeaponAR2Ex::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary)
{
	if (isSecondary)
	{
		if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_AR2_ALTFIRE))
		{
			animevent_t animEvent;
			animEvent.event = EVENT_WEAPON_AR2_ALTFIRE;
			Operator_HandleAnimEvent(&animEvent, pOperator);
		}
	}
	else if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_AR2))
	{
		animevent_t animEvent;
		animEvent.event = EVENT_WEAPON_AR2;
		Operator_HandleAnimEvent(&animEvent, pOperator);
	}
}

void CWeaponAR2Ex::FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool useWeaponAngles)
{
	Vector shootOrigin, vecShootDir;
	CAI_BaseNPC* pNPC = pOperator->MyNPCPointer();
	ASSERT(pNPC != NULL);

	if (useWeaponAngles)
	{
		QAngle angShootDir;
		GetAttachment(LookupAttachment("muzzle"), shootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);
	}
	else
	{
		shootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = pNPC->GetActualShootTrajectory(shootOrigin);
	}

	WeaponSoundRealtime(SINGLE_NPC);
	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(),
		SOUNDENT_VOLUME_MACHINEGUN, 0.2f, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	pOperator->FireBullets(1, shootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH,
		m_iPrimaryAmmoType, 2, -1, -1, GetHL2MPWpnData().m_iPlayerDamage);
	SubstractClip1(1);
}

void CWeaponAR2Ex::FireNPCSecondaryAttack(CBaseCombatCharacter* pOperator, bool useWeaponAngles)
{
	WeaponSound(WPN_DOUBLE);

	if (GetOwner() == NULL)
	{
		return;
	}

	CAI_BaseNPC* pNPC = GetOwner()->MyNPCPointer();

	if (pNPC != NULL)
	{
		// Fire!
		Vector src, aiming;

		if (useWeaponAngles)
		{
			QAngle shootDir;
			GetAttachment(LookupAttachment("muzzle"), src, shootDir);
			AngleVectors(shootDir, &aiming);
		}
		else
		{
			src = pNPC->Weapon_ShootPosition();
			Vector target;
			CNPC_Combine* pSoldier = dynamic_cast<CNPC_Combine*>(pNPC);

			if (pSoldier != NULL)
			{
				// In the distant misty past, elite soldiers tried to use bank shots.
				// Therefore, we must ask them specifically what direction they are shooting.
				target = pSoldier->GetAltFireTarget();
			}
			// All other users of the AR2 alt-fire shoot directly at their enemy
			else if (pNPC->GetEnemy() != NULL)
			{
				target = pNPC->GetEnemy()->BodyTarget(src);
			}
			else
			{
				return;
			}

			aiming = target - src;
			VectorNormalize(aiming);
		}

		Vector impactPoint = src + (aiming * MAX_TRACE_LENGTH);
		float flAmmoRatio = 1.0f;
		float flDuration = RemapValClamped(flAmmoRatio, 0.0f, 1.0f, 0.5f,
			sk_weapon_ar2_alt_fire_duration.GetFloat());
		float flRadius = RemapValClamped(flAmmoRatio, 0.0f, 1.0f, 4.0f,
			sk_weapon_ar2_alt_fire_radius.GetFloat());

		// Fire the bullets
		Vector velocity = aiming * 1000.0f;

		// Fire the combine ball
		CreateCombineBall(src, velocity, flRadius, sk_weapon_ar2_alt_fire_mass.GetFloat(),
			flDuration, pNPC);
	}
}
#endif // CLIENT_DLL
