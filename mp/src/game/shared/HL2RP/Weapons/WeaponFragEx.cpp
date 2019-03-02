#include <cbase.h>
#include <hl2mp/weapon_frag.h>

#ifdef CLIENT_DLL
#define CWeaponFragEx	C_WeaponFragEx
#else
#include <ai_basenpc.h>
#include <basegrenade_shared.h>
#include <grenade_frag.h>
#endif // CLIENT_DLL

#define GRENADE_LOCAL_THROW_SPEED	1200.0f

//--------------------------------------
// CWeaponFragEx (HL2RP expansion)
//--------------------------------------
class CWeaponFragEx : public CWeaponFrag
{
	DECLARE_CLASS(CWeaponFragEx, CWeaponFrag);
	DECLARE_PREDICTABLE();

#ifndef HL2DM_RP
	DECLARE_NETWORKCLASS();
#endif // !HL2DM_RP

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

	int CapabilitiesGet() OVERRIDE;
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary) OVERRIDE;

	bool CheckNPCCanThrowGrenade(CBaseCombatCharacter* pNPC, const Vector& start, Vector& targetOut);
#endif // !CLIENT_DLL
};

#ifndef HL2DM_RP
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponFragEx, DT_WeaponFragEx);

BEGIN_NETWORK_TABLE(CWeaponFragEx, DT_WeaponFragEx)
END_NETWORK_TABLE();
#endif // !HL2DM_RP

LINK_ENTITY_TO_CLASS(weapon_frag, CWeaponFragEx);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponFragEx)
END_PREDICTION_DATA();
#else
acttable_t CWeaponFragEx::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_GRENADE, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_GRENADE, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_GRENADE, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_GRENADE, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, true },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_GRENADE, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_GRENADE, true }
};

IMPLEMENT_ACTTABLE(CWeaponFragEx);

int CWeaponFragEx::CapabilitiesGet()
{
	return bits_CAP_WEAPON_RANGE_ATTACK1;
}

// Purpose: Allow generic grenade throwing by NPCs. The implementation is experimental.
void CWeaponFragEx::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool isSecondary)
{
	Vector start, velocity = pOperator->GetEnemy()->GetAbsOrigin();
	pOperator->GetAttachment("anim_attachment_RH", start);

	if (CheckNPCCanThrowGrenade(pOperator, start, velocity))
	{
		AngularImpulse angImpulse(RandomFloat(-1000.0f, 1000.0f), RandomFloat(-1000.0f, 1000.0f),
			RandomFloat(-1000.0f, 1000.0f));

		// NOTE: Setting combineSpawned to true ignores 'buddha' cheat command, so set it to false
		CBaseGrenade* pGrenade = Fraggrenade_Create(start, vec3_angle, velocity, angImpulse,
			pOperator, GRENADE_TIMER, false);

		if (pGrenade != NULL)
		{
			pGrenade->SetDamage(GetHL2MPWpnData().m_iPlayerDamage);
			pGrenade->SetDamageRadius(GRENADE_DAMAGE_RADIUS);
		}
	}
}

// Purpose: Try building a proper grenade throw velocity vector. Adapted implementation from CNPC_CombineS's.
bool CWeaponFragEx::CheckNPCCanThrowGrenade(CBaseCombatCharacter* pNPC, const Vector& start,
	Vector& targetOut)
{
	Vector vecToss, vecMins = -Vector(4.0f, 4.0f, 4.0f), vecMaxs = Vector(4.0f, 4.0f, 4.0f);

	if (pNPC->FInViewCone(targetOut) && pNPC->FVisible(targetOut))
	{
		vecToss = VecCheckThrow(pNPC, start, targetOut, GRENADE_LOCAL_THROW_SPEED,
			1.0f, &vecMins, &vecMaxs);
	}
	else
	{
		// Have to try a high toss. Do I have enough room?
		trace_t tr;
		AI_TraceLine(start, start + Vector(0.0f, 0.0f, 64.0f), MASK_SHOT, pNPC,
			COLLISION_GROUP_NONE, &tr);

		if (tr.fraction != 1.0f)
		{
			return false;
		}

		vecToss = VecCheckToss(pNPC, start, targetOut, -1.0f, 1.0f, true, &vecMins, &vecMaxs);
	}

	if (vecToss != vec3_origin)
	{
		targetOut = vecToss;
		return true;
	}

	return false;
}
#endif // CLIENT_DLL
