//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_citizenpackage.h"
#ifdef ROLEPLAY
#include "CHL2RP.h"
#include "CPropRationDispenser.h"
#include "CHL2RP_Colors.h"
#include "in_buttons.h"
#include "CHL2RP_Player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef ROLEPLAY
#define PACKAGE_PLAYER_THROW_PICKUP_WAIT_TIME 3.0f
#define MIN_PLAYER_PACKAGE_USAGE_WAIT_TIME 2.0f
#define CITIZEN_PACKAGE_ALLOW_PICKUP_CONTEXT "CitizenPackageAllowPickupContext"
#endif

#ifdef HL2RP
IMPLEMENT_SERVERCLASS_ST(CWeaponCitizenPackage, DT_WeaponCitizenPackage)
END_SEND_TABLE()
#endif

BEGIN_DATADESC(CWeaponCitizenPackage)
END_DATADESC()

LINK_ENTITY_TO_CLASS(weapon_citizenpackage, CWeaponCitizenPackage);
PRECACHE_WEAPON_REGISTER(weapon_citizenpackage);

acttable_t	CWeaponCitizenPackage::m_acttable[] =
{
	{ ACT_IDLE, ACT_IDLE_PACKAGE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_PACKAGE, true },
	{ ACT_WALK, ACT_WALK_PACKAGE, false },
	{ ACT_WALK_AIM, ACT_WALK_PACKAGE, false },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH, false },
	{ ACT_RUN_AIM, ACT_RUN, false },
	{ ACT_JUMP, ACT_JUMP, false },
	{ ACT_RANGE_AIM_LOW, ACT_COVER_LOW, false },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PHYSGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PHYSGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PHYSGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PHYSGUN, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PHYSGUN, false }
};
IMPLEMENT_ACTTABLE(CWeaponCitizenPackage);

#ifdef ROLEPLAY
CWeaponCitizenPackage::CWeaponCitizenPackage() : m_hParentDispenser(NULL)
{

}

void CWeaponCitizenPackage::Spawn()
{
	BaseClass::Spawn();

	// Force 0 ammo to prevent ammo from script:
	SetSecondaryAmmoCount(0);
}

void CWeaponCitizenPackage::PrimaryAttack()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	const Vector &origin = pOwner->Weapon_ShootPosition();

	if (!(UTIL_PointContents(origin) & MASK_SOLID))
	{
		Vector facing = pOwner->BodyDirection3D(), velocity = pOwner->GetAbsVelocity();
		velocity += facing * 200.0f;
		velocity.z += 100.0f;

		pOwner->SetAnimation(PLAYER_ATTACK1);
		Holster(this);
		SetWeaponVisible(false);
		pOwner->RemoveAmmo(1, GetSecondaryAmmoType());
		
		QAngle angles;
		angles.Random(0.0f, 360.0f);

		CWeaponCitizenPackage *package = static_cast<CWeaponCitizenPackage *>(CreateNoSpawn(GetClassname(), origin, angles));
		package->AddSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP | SF_NORESPAWN);
		DispatchSpawn(package);
		package->SetSecondaryAmmoCount(1);
		package->SetContextThink(&CWeaponCitizenPackage::AllowPickup, gpGlobals->curtime + PACKAGE_PLAYER_THROW_PICKUP_WAIT_TIME, CITIZEN_PACKAGE_ALLOW_PICKUP_CONTEXT);
		package->Teleport(NULL, NULL, &velocity);
	}
}

// Purpose: prevent a CPlayerAmmoDeleter from being created with invalid ammo index
bool CWeaponCitizenPackage::UsesClipsForAmmo1() const
{
	return false;
}

// Purpose: force to retrieve secondary ammo
// (this is called when player owns the weapon already)
// instead of default clip2 without changing default code:
bool CWeaponCitizenPackage::UsesClipsForAmmo2() const
{
	return false;
}

// Purpose: force to retrieve secondary ammo
// (this is called when player doesn't own the weapon yet)
// instead of default clip2 without changing default code:
int CWeaponCitizenPackage::GetDefaultClip2() const
{
	return (const_cast<CWeaponCitizenPackage *>(this))->GetSecondaryAmmoCount();
}

void CWeaponCitizenPackage::SecondaryAttack()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	char msg[MAX_VALVE_USER_MSG_TEXT_DATA];

	if (pOwner->GetHealth() < pOwner->GetMaxHealth())
	{
		pOwner->SetHealth(pOwner->GetMaxHealth());
		pOwner->RemoveAmmo(1, GetSecondaryAmmoType());
		
		CPASAttenuationFilter user;
		user.AddAllPlayers();

		enginesound->EmitSound(user, pOwner->entindex(), CHAN_AUTO, "items/smallmedkit1.wav", 1.0, SNDLVL_60dB);

		Q_snprintf(msg, sizeof msg, STATIC_TALK_COLOR "You have recovered health by using a Combine-supplied Citizen Ration Unit");
	}
	else
		Q_snprintf(msg, sizeof msg, STATIC_TALK_COLOR "[Note] Disordered Citizen! Make a reasonable use of the Citizen Ration, you are healthy. (Signed: The Combine)");

	ClientPrint(pOwner, HUD_PRINTTALK, msg);
}

void CWeaponCitizenPackage::AllowPickup()
{
	RemoveSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
}

// Purpose: Force ration to be picked up even when attached dispenser is solid
bool CWeaponCitizenPackage::FVisible(CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker)
{
	CBaseEntity *pLocalBlocker = NULL;
	bool bVisible = (BaseClass::FVisible(pEntity, traceMask, &pLocalBlocker) || pLocalBlocker == m_hParentDispenser);

	if (ppBlocker != NULL)
	{
		// Caller passed a blocker, copy the returned local blocker to it
		*ppBlocker = pLocalBlocker;
	}

	return bVisible;
}

void CWeaponCitizenPackage::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner != NULL)
	{
		if (pOwner->m_nButtons & IN_ATTACK)
		{
			if (m_flNextPrimaryAttack <= gpGlobals->curtime
			&& pOwner->GetViewModel()->GetSequenceActivity(pOwner->GetViewModel()->GetSequence()) == ACT_VM_DRAW)
			{
				PrimaryAttack();
				m_flNextPrimaryAttack = gpGlobals->curtime + MIN_PLAYER_PACKAGE_USAGE_WAIT_TIME;
			}
		}
		else if (pOwner->m_nButtons & IN_ATTACK2)
		{
			if (m_flNextSecondaryAttack <= gpGlobals->curtime)
			{
				SecondaryAttack();
				m_flNextSecondaryAttack = gpGlobals->curtime + MIN_PLAYER_PACKAGE_USAGE_WAIT_TIME;
			}
		}

		// Both buttons may have wasted ammo:
		if (!HasSecondaryAmmo())
		{
			if (!pOwner->SwitchToNextBestWeapon(this))
			{
				CBaseCombatWeapon *physCannon = pOwner->Weapon_OwnsThisType("weapon_physcannon");

				if (physCannon != NULL)
				{
					pOwner->Weapon_Switch(physCannon);
					physCannon->m_flNextPrimaryAttack = gpGlobals->curtime + 3.0f;
				}
			}

			Remove();
		}
		else if (HasWeaponIdleTimeElapsed() && IsEffectActive(EF_NODRAW))
		{
			// Deploying a new ration unit effect:
			Deploy();
		}
	}
}

void CWeaponCitizenPackage::UpdateOnRemove()
{
	if (m_hParentDispenser != NULL)
	{
		m_hParentDispenser->OnContainedRationPickup();
	}

	BaseClass::UpdateOnRemove();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Remove the citizen package if it's ever dropped
//-----------------------------------------------------------------------------
void CWeaponCitizenPackage::Drop(const Vector &vecVelocity)
{
	BaseClass::Drop( vecVelocity );
}

#ifdef HL2RP
IMPLEMENT_SERVERCLASS_ST(CWeaponCitizenSuitcase, DT_WeaponCitizenSuitcase)
END_SEND_TABLE()
#endif

BEGIN_DATADESC( CWeaponCitizenSuitcase )
END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_citizensuitcase, CWeaponCitizenSuitcase );
PRECACHE_WEAPON_REGISTER(weapon_citizensuitcase);

acttable_t	CWeaponCitizenSuitcase::m_acttable[] = 
{
	{ ACT_IDLE, ACT_IDLE_SUITCASE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_SUITCASE, true },
	{ ACT_WALK, ACT_WALK_SUITCASE, false },
	{ ACT_WALK_AIM, ACT_WALK_SUITCASE, false },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH, false },
	{ ACT_RUN_AIM, ACT_RUN, false },
	{ ACT_JUMP, ACT_JUMP, false },
	{ ACT_RANGE_AIM_LOW, ACT_COVER_LOW, false },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PHYSGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PHYSGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PHYSGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PHYSGUN, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PHYSGUN, false }
};
IMPLEMENT_ACTTABLE(CWeaponCitizenSuitcase);

#ifdef ROLEPLAY
// Purpose: prevent a CPlayerAmmoDeleter from being created with invalid ammo index
bool CWeaponCitizenSuitcase::UsesClipsForAmmo1() const
{
	return false;
}

// Purpose: force GetDefaultClip2 to get called
// when player picks it up and owns it already:
bool CWeaponCitizenSuitcase::UsesClipsForAmmo2() const
{
	return false;
}

// Purpose: block increasing ammo when
// player gets the suitcase:
int CWeaponCitizenSuitcase::GetDefaultClip2() const
{
	return 0;
}
#endif
