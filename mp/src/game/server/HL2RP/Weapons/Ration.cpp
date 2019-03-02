#include <cbase.h>
#include "Ration.h"
#include <activitylist.h>
#include <engine/IEngineSound.h>
#include "HL2RPRules.h"
#include <HL2RP_Player.h>
#include <in_buttons.h>
#include <PropRationDispenser.h>

#define RATION_PICKUP_THROWN_DELAY	3.0f
#define RATION_ALLOW_PICKUP_CONTEXT_NAME	"RationAllowPickupContext"
#define RATION_PRETHROW_ACCELERATION	2000.0f
#define RATION_MAX_ARM_THROW_SPEED	300.0f
#define RATION_USAGE_HEALTH_GAIN	5

// Degrees to add up to the initial player's throw pitch. Final angles should NOT surpass the player's normal.
#define RATION_LOCAL_THROW_ANGLE_PITCH_UP	30.0f

// Time that represents new ration unit deploy from player, namely after using one.
// It's the cooldown since weapon gets fully holstered before player may reuse it.
#define RATION_HOLSTERED_REFILL_SECONDS	0.3f

LINK_ENTITY_TO_CLASS(ration, CRation);
PRECACHE_WEAPON_REGISTER(ration);

BEGIN_DATADESC(CRation)
DEFINE_KEYFIELD_NOT_SAVED(m_iSecondaryAmmoCount, FIELD_INTEGER, "secondaryammocount")
END_DATADESC();

acttable_t CRation::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_INVALID, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SLAM, false },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SLAM, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SLAM, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SLAM, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SLAM, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SLAM, false },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_THROW, false },
	{ ACT_GESTURE_RANGE_ATTACK1_LOW, ACT_GESTURE_RANGE_ATTACK_THROW, false },
	{ ACT_CROUCH, ACT_CROUCH, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_PACKAGE, false },
	{ ACT_JUMP, ACT_JUMP, false },
	{ ACT_RUN_AIM, ACT_RUN, false },
	{ ACT_WALK_AIM, ACT_WALK_PACKAGE, false },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH, false }
};

IMPLEMENT_ACTTABLE(CRation);

#ifndef HL2DM_RP
// On HL2DM, this would block players from connecting to the server.
// Excluding it cancels client-side initializations, but it's fine.
IMPLEMENT_SERVERCLASS_ST(CRation, DT_Ration)
END_SEND_TABLE();
#endif

CRation::CRation() : m_hParentDispenser(NULL)
{

}

void CRation::Precache()
{
	BaseClass::Precache();

	// Register healing activity to use on Ration throw
	for (acttable_t& actTable : m_acttable)
	{
		if (actTable.baseAct == ACT_RANGE_ATTACK1)
		{
			// This Activity is used on Ration throw, originally from CNPC_Citizen
			actTable.weaponAct = ActivityList_RegisterPrivateActivity("ACT_CIT_HEAL");
			break;
		}
	}
}

void CRation::Spawn()
{
	// Spawn, preserving the ammo that maybe was defined from map rather than default from script
	int mapSecondaryAmmoCount = GetSecondaryAmmoCount();
	BaseClass::Spawn();

	if (mapSecondaryAmmoCount > 0)
	{
		SetSecondaryAmmoCount(mapSecondaryAmmoCount);
	}
}

void CRation::HandleFireOnEmpty()
{
	if (m_ActiveState == EActiveState::Deployed)
	{
		SendWeaponAnim(ACT_VM_HOLSTER);
		m_flNextPrimaryAttack = gpGlobals->curtime; // For dynamic arm speed
		m_ActiveState = EActiveState::Holstering_PreThrow;
	}
}

// Reusable function for ammo removal on Ration usage
void CRation::RemoveAmmoAndHandleEmpty()
{
	Assert(GetOwner() != NULL);
	GetOwner()->RemoveAmmo(1, GetSecondaryAmmoType());

	if (!HasSecondaryAmmo())
	{
		Remove();
		GetOwner()->Weapon_Detach(this);
		GetOwner()->SwitchToNextBestWeapon(this);
	}
}

void CRation::SecondaryAttack()
{
	if (m_ActiveState != EActiveState::Deployed)
	{
		return;
	}

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer != NULL && (pPlayer->m_afButtonPressed & IN_ATTACK2))
	{
		int health = pPlayer->GetHealth();
		const char* pMsg;

		if (health < pPlayer->GetMaxHealth())
		{
			// BUG: When ACT_VM_DRAW is later sent by the FSM, it does not run smoothly, but inmediately finishes.
			SendWeaponAnim(ACT_VM_HOLSTER);

			health = Clamp(health + RATION_USAGE_HEALTH_GAIN, health, pPlayer->GetMaxHealth());
			pPlayer->SetHealth(health);
			RemoveAmmoAndHandleEmpty();
			WeaponSound(SPECIAL1);
			SetWeaponVisible(false);
			pMsg = GetHL2RPAutoLocalizer().Localize(pPlayer, "HL2RP_Chat_Ration_Healed");
			m_ActiveState = EActiveState::Holstering_PostHeal;
		}
		else
		{
			pMsg = GetHL2RPAutoLocalizer().Localize(pPlayer, "HL2RP_Chat_Ration_Heal_Denied");
		}

		ClientPrint(pPlayer, HUD_PRINTTALK, pMsg);
	}
}

void CRation::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
	{
		return;
	}

	switch (m_ActiveState)
	{
	case EActiveState::Holstering_PreThrow:
	{
		if (pPlayer->m_afButtonReleased & IN_ATTACK)
		{
			Vector vecOrigin;
			pPlayer->GetAttachment("anim_attachment_RH", vecOrigin);

			if (UTIL_PointContents(vecOrigin) & MASK_SOLID)
			{
				m_ActiveState = EActiveState::Deploying;
				break;
			}

			CRation* pRation = static_cast<CRation*>(CreateNoSpawn("ration", vecOrigin, vec3_angle));

			if (pRation == NULL)
			{
				m_ActiveState = EActiveState::Deploying;
				break;
			}

			pPlayer->SetAnimation(PLAYER_ATTACK1);
			SetWeaponVisible(false);
			pRation->AddSpawnFlags(SF_NORESPAWN | SF_WEAPON_NO_PLAYER_PICKUP);
			DispatchSpawn(pRation);
			pRation->SetContextThink(&CRation::EnsureVisible, gpGlobals->curtime + TICK_INTERVAL,
				RATION_ENSURE_VISIBLE_CONTEXT_NAME);
			float armSpeed = Clamp((gpGlobals->curtime - m_flNextPrimaryAttack)
				* RATION_PRETHROW_ACCELERATION, 0.0f, RATION_MAX_ARM_THROW_SPEED);

			// Build throw velocity.
			// TODO: Check if there is a matrix operation that simplifies the following, and override if so.
			QAngle viewAngles = pPlayer->EyeAngles();
			viewAngles[PITCH] = Max(-90.0f, viewAngles[PITCH] - RATION_LOCAL_THROW_ANGLE_PITCH_UP);
			Vector vecVelocity;
			AngleVectors(viewAngles, &vecVelocity);
			VectorMA(pPlayer->GetAbsVelocity(), armSpeed, vecVelocity, vecVelocity);

			pRation->Teleport(NULL, NULL, &vecVelocity);
			RemoveAmmoAndHandleEmpty();
			pRation->SetContextThink(&CRation::AllowPickup,
				gpGlobals->curtime + RATION_PICKUP_THROWN_DELAY, RATION_ALLOW_PICKUP_CONTEXT_NAME);
			m_ActiveState = EActiveState::Holstering_PostThrow;
		}

		break;
	}
	case EActiveState::Holstering_PostHeal:
	{
		if (IsViewModelSequenceFinished())
		{
			m_flNextSecondaryAttack = gpGlobals->curtime + RATION_HOLSTERED_REFILL_SECONDS;
			m_ActiveState = EActiveState::Holstered_RefillingPostHeal;
		}

		break;
	}
	case EActiveState::Holstering_PostThrow:
	{
		if (IsViewModelSequenceFinished())
		{
			m_flNextPrimaryAttack = gpGlobals->curtime + RATION_HOLSTERED_REFILL_SECONDS;
			m_ActiveState = EActiveState::Holstered_RefillingPostThrow;
		}

		break;
	}
	case EActiveState::Holstered_RefillingPostHeal:
	{
		if (gpGlobals->curtime >= m_flNextSecondaryAttack)
		{
			SetWeaponVisible(true);
			m_ActiveState = EActiveState::Deploying;
		}

		break;
	}
	case EActiveState::Holstered_RefillingPostThrow:
	{
		if (gpGlobals->curtime >= m_flNextPrimaryAttack)
		{
			SetWeaponVisible(true);
			m_ActiveState = EActiveState::Deploying;
		}

		break;
	}
	case EActiveState::Deploying:
	{
		SendWeaponAnim(ACT_VM_DRAW);
		m_ActiveState = EActiveState::Deployed;
		break;
	}
	}
}

void CRation::OnActiveStateChanged(int oldState)
{
	BaseClass::OnActiveStateChanged(oldState);

	if (m_iState == WEAPON_IS_ACTIVE)
	{
		m_ActiveState = EActiveState::Deployed;
	}
}

void CRation::OnPickedUp(CBaseCombatCharacter* pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);

	if (m_hParentDispenser != NULL)
	{
		m_hParentDispenser->OnContainedRationPickup();
	}
}

// HACK: Force ration to be picked up from the dispenser
bool CRation::FVisible(CBaseEntity* pEntity, int traceMask, CBaseEntity** ppCallerBlocker)
{
	CBaseEntity* pAuxBlocker;
	bool isVisible = BaseClass::FVisible(pEntity, traceMask, &pAuxBlocker);

	if (pAuxBlocker == m_hParentDispenser)
	{
		isVisible = true;
	}

	if (ppCallerBlocker != NULL)
	{
		// Caller wants to fill his blocker pointer
		*ppCallerBlocker = pAuxBlocker;
	}

	return isVisible;
}

bool CRation::CanBeSelected()
{
#ifdef HL2DM_RP
	// HACK: On HL2DM, the special weapons can't be assigned a proper slot
	return false;
#endif // HL2DM

	return BaseClass::CanBeSelected();
}

// Workarounds dropped Ration from showing invisible randomly for non listen clients.
// HACK: Provokes an entity update on the next frame for it.
void CRation::EnsureVisible()
{
	// The owner could see the viewmodel even when holstered, so, check there is no owner
	if (GetOwner() == NULL)
	{
		ClearEffects();
	}
}

void CRation::AllowPickup()
{
	RemoveSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
}
