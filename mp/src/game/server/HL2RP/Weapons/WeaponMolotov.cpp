#include <cbase.h>
#include "WeaponMolotov.h"
#include <grenade_molotov.h>
#include <HL2RP_Player.h>
#include <in_buttons.h>

#define MOLOTOV_LOCAL_THROW_SPEED	500.0f

// Degrees to add up to the initial player's throw pitch. Final angles should NOT surpass the player's normal.
#define MOLOTOV_LOCAL_THROW_ANGLE_PITCH_UP	15.0f

// Seconds from prop throw before player may re-attack
#define MOLOTOV_REFILL_SECONDS	1.0f

// Time to force on ACT_RANGE_ATTACK_THROW (and ACT_VM_THROW)
#define MOLOTOV_FORCED_THROWING_SEQUENCES_DURATION	1.5f

// Delay seconds to throw prop from button release (at a moment within the throw sequences)
#define MOLOTOV_THROWING_PREPROP_WAIT_SECONDS	0.3f

// This is the pitch velocity (forward) relative to the local Molotov prop angles on throw
#define MOLOTOV_LOCAL_ANGULAR_THROW_PITCH_VELOCITY	500.0f

LINK_ENTITY_TO_CLASS(weapon_molotov, CWeaponMolotov);
PRECACHE_WEAPON_REGISTER(weapon_molotov);

acttable_t	CWeaponMolotov::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_THROW, true },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_GRENADE, true },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_GRENADE, true },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_GRENADE, true },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_GRENADE, true },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_GRENADE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_THROW, true },
	{ ACT_GESTURE_RANGE_ATTACK1_LOW, ACT_GESTURE_RANGE_ATTACK_THROW, true }
};

IMPLEMENT_ACTTABLE(CWeaponMolotov);

#ifndef HL2DM_RP
// On HL2DM, this would block players from connecting to the server.
// Excluding it cancels client-side initializations, but it's fine.
IMPLEMENT_SERVERCLASS_ST(CWeaponMolotov, DT_WeaponMolotov)
END_SEND_TABLE()
#endif // !HL2DM_RP

void CWeaponMolotov::HandleFireOnEmpty()
{
	if (m_ActiveState == EActiveState::Deployed)
	{
		CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

		if (pPlayer != NULL)
		{
			SendWeaponAnim(ACT_VM_HAULBACK);
			m_ActiveState = EActiveState::Haulback_PreThrow;
		}
	}
}

void CWeaponMolotov::ThrowMolotov(const Vector& vecSrc, const Vector& vecVelocity)
{
	Assert(GetOwner() != NULL);
	CGrenade_Molotov* pMolotov = static_cast<CGrenade_Molotov*>
		(Create("grenade_molotov", vecSrc, GetOwner()->EyeAngles()));

	if (pMolotov != NULL)
	{
		pMolotov->SetSolid(SOLID_NONE);
		pMolotov->SetMoveType(MOVETYPE_FLYGRAVITY);
		pMolotov->SetAbsVelocity(vecVelocity);

		// Forward attacker data for later damage info
		pMolotov->SetThrower(GetOwner());
		pMolotov->SetOwnerEntity(GetOwner());

		// Tumble through the air
		QAngle angVel(MOLOTOV_LOCAL_ANGULAR_THROW_PITCH_VELOCITY, 0.0f, 0.0f);
		pMolotov->SetLocalAngularVelocity(angVel);
	}
}

void CWeaponMolotov::PrimaryAttack()
{
	Assert(m_ActiveState == EActiveState::Throwing_PreProp);
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
	{
		m_ActiveState = EActiveState::Deploying;
		return;
	}

	Vector vecOrigin;
	pPlayer->GetAttachment("anim_attachment_RH", vecOrigin);

	if (UTIL_PointContents(vecOrigin) & MASK_SOLID)
	{
		m_ActiveState = EActiveState::Deploying;
		return;
	}

	// Build throw velocity.
	// TODO: Check if there is a matrix operation that simplifies the following, and override if so.
	QAngle viewAngles = pPlayer->EyeAngles();
	viewAngles[PITCH] = Max(-90.0f, viewAngles[PITCH] - MOLOTOV_LOCAL_THROW_ANGLE_PITCH_UP);
	Vector vecVelocity;
	AngleVectors(viewAngles, &vecVelocity);
	VectorMA(pPlayer->GetAbsVelocity(), MOLOTOV_LOCAL_THROW_SPEED, vecVelocity, vecVelocity);

	ThrowMolotov(vecOrigin, vecVelocity);
	pPlayer->RemoveAmmo(1, GetSecondaryAmmoType());
	SetWeaponVisible(false);

	if (!HasSecondaryAmmo())
	{
		Remove();
		pPlayer->Weapon_Detach(this);
		pPlayer->SwitchToNextBestWeapon(this);
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + MOLOTOV_REFILL_SECONDS;
	m_ActiveState = EActiveState::Throwing_PostProp;
}

void CWeaponMolotov::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
	CHL2RP_Player* pPlayer = ToHL2RP_Player(GetOwner());

	if (pPlayer == NULL)
	{
		return;
	}

	switch (m_ActiveState)
	{
	case EActiveState::Haulback_PreThrow:
	{
		if (pPlayer->m_afButtonReleased & IN_ATTACK)
		{
			SendWeaponAnim(ACT_VM_THROW);

			// Task: Synchronize throwing sequences from viewmodel and player to a fixed time
			CBaseViewModel* pViewModel = pPlayer->GetViewModel();

			if (pViewModel != NULL)
			{
				pViewModel->SetPlaybackRate(pViewModel->SequenceDuration() / MOLOTOV_FORCED_THROWING_SEQUENCES_DURATION);
			}

			pPlayer->SetAttack1Animation(ACT_RANGE_ATTACK1, MOLOTOV_FORCED_THROWING_SEQUENCES_DURATION);
			m_flNextPrimaryAttack = gpGlobals->curtime + MOLOTOV_THROWING_PREPROP_WAIT_SECONDS;
			m_ActiveState = EActiveState::Throwing_PreProp;
		}

		break;
	}
	case EActiveState::Throwing_PreProp:
	{
		if (gpGlobals->curtime >= m_flNextPrimaryAttack)
		{
			PrimaryAttack(); // It should handle next state change
		}

		break;
	}
	case EActiveState::Throwing_PostProp:
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

void CWeaponMolotov::OnActiveStateChanged(int oldState)
{
	BaseClass::OnActiveStateChanged(oldState);

	if (m_iState == WEAPON_IS_ACTIVE)
	{
		m_ActiveState = EActiveState::Deployed;
	}
}

bool CWeaponMolotov::VisibleInWeaponSelection()
{
#ifdef HL2DM_RP
	// HACK: On HL2DM, the special weapons can't be assigned a proper slot
	return false;
#endif // HL2DM

	return BaseClass::VisibleInWeaponSelection();
}
