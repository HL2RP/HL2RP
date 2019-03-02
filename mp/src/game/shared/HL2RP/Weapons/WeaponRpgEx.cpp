#include <cbase.h>
#include <hl2mp/weapon_rpg.h>

#ifdef CLIENT_DLL
#define CWeaponRPGEx	C_WeaponRPGEx
#else
#include <ai_basenpc.h>
#endif // CLIENT_DLL

//------------------------------------
// CWeaponRPGEx (HL2RP expansion)
//------------------------------------
class CWeaponRPGEx : public CWeaponRPG
{
	DECLARE_CLASS(CWeaponRPGEx, CWeaponRPG);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

	int CapabilitiesGet() OVERRIDE;
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator) OVERRIDE;
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary) OVERRIDE;
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponRPGEx, DT_WeaponRPGEx);

BEGIN_NETWORK_TABLE(CWeaponRPGEx, DT_WeaponRPGEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_rpg, CWeaponRPGEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponRPGEx)
END_PREDICTION_DATA();
#else
acttable_t CWeaponRPGEx::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_RPG, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_RPG, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_RPG, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_RPG, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_RPG, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_RPG, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_RPG, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_RPG, true },
	{ ACT_IDLE_RELAXED, ACT_IDLE_RPG_RELAXED, true },
	{ ACT_IDLE_STIMULATED, ACT_IDLE_ANGRY_RPG, true },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_RPG, true },
	{ ACT_IDLE, ACT_IDLE_RPG, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_RPG, true },
	{ ACT_WALK, ACT_WALK_RPG, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RPG, true },
	{ ACT_RUN, ACT_RUN_RPG, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RPG, true },
	{ ACT_COVER_LOW, ACT_COVER_LOW_RPG, true }
};

IMPLEMENT_ACTTABLE(CWeaponRPGEx);

int CWeaponRPGEx::CapabilitiesGet()
{
	return bits_CAP_WEAPON_RANGE_ATTACK1;
}

void CWeaponRPGEx::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_MISSILE_FIRE:
	case EVENT_WEAPON_SMG1: // Event for backwards compatibility with HL2 CWeaponRPG implementation
	{
		if (m_hMissile != NULL)
		{
			return;
		}

		Vector muzzlePoint = GetOwner()->Weapon_ShootPosition();
		CAI_BaseNPC* pNPC = pOperator->MyNPCPointer();
		ASSERT(pNPC != NULL);
		Vector shootDir = pNPC->GetActualShootTrajectory(muzzlePoint);

		// Look for a better launch location
		Vector altLaunchPoint;

		if (GetAttachment("missile", altLaunchPoint))
		{
			// Check to see if it's relativly free
			trace_t tr;
			AI_TraceHull(altLaunchPoint, altLaunchPoint + shootDir * (10.0f * 12.0f),
				Vector(-24.0f, -24.0f, -24.0f), Vector(24.0f, 24.0f, 24.0f), MASK_NPCSOLID, NULL, &tr);

			if (tr.fraction == 1.0f)
			{
				muzzlePoint = altLaunchPoint;
			}
		}

		QAngle angles;
		VectorAngles(shootDir, angles);
		CMissile* pMissile = CMissile::Create(muzzlePoint, angles, GetOwner()->edict());

		if (pMissile != NULL)
		{
			m_hMissile = pMissile;
			pMissile->m_hOwner = this;
			pMissile->SetDamage(GetHL2MPWpnData().m_iPlayerDamage);

			// NPCs always get a grace period
			pMissile->SetGracePeriod(0.5f);

			pOperator->DoMuzzleFlash();
			WeaponSound(SINGLE_NPC);

			// Make sure our laserdot is off
			m_bGuiding = false;

			if (m_hLaserDot)
			{
				EnableLaserDot((CBaseEntity*)m_hLaserDot.Get(), false);
			}
		}

		break;
	}
	default:
	{
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
	}
	}
}

void CWeaponRPGEx::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary)
{
	// NOTE: Event for backwards compatibility with HL2 CWeaponRPG implementation
	if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_SMG1))
	{
		animevent_t animEvent;
		animEvent.event = EVENT_WEAPON_MISSILE_FIRE;
		Operator_HandleAnimEvent(&animEvent, pOperator);
	}
}
#endif // CLIENT_DLL
