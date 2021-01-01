// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "ration.h"
#include <hl2_roleplayer.h>
#include <prop_ration_dispenser.h>
#include <activitylist.h>
#include <ammodef.h>
#include <in_buttons.h>
#include <npcevent.h>

#define RATION_THROW_AIR_SPEED       200.0f
#define RATION_THROW_ANGULAR_VEL_YAW -100.0f

#define RATION_THROW_EYE_OFFSET_RIGHT   16.0f
#define RATION_THROW_EYE_OFFSET_FORWARD 32.0f
#define RATION_THROW_EYE_OFFSET_UP      -8.0f

#define RATION_THROW_MAX_PICKUP_BLOCK_TIME 3.0f

#define RATION_USAGE_HEALTH_GAIN 5

#define RATION_PICKUP_ALLOW_CONTEXT "RationAllowPickupContext"

LINK_ENTITY_TO_CLASS(ration, CRation)
PRECACHE_WEAPON_REGISTER(ration);

BEGIN_DATADESC(CRation)
DEFINE_KEYFIELD_NOT_SAVED(m_iPrimaryAmmoCount, FIELD_INTEGER, "ammocount")
END_DATADESC()

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

IMPLEMENT_ACTTABLE(CRation)

#ifdef HL2RP_FULL
IMPLEMENT_SERVERCLASS_ST(CRation, DT_Ration)
END_SEND_TABLE()
#endif // HL2RP_FULL

CRation::~CRation()
{
	if (mhParentDispenser != NULL)
	{
		mhParentDispenser->mhContainedRation = NULL;
	}
}

void CRation::Spawn()
{
	int mapAmmoCount = GetPrimaryAmmoCount();
	BaseClass::Spawn();
	SetPrimaryAmmoCount(Max(0, mapAmmoCount)); // Preserve the ammo that may have been defined from map
}

void CRation::Precache()
{
	BaseClass::Precache();

	// Register healing activity to use on ration throw
	for (auto& actTable : m_acttable)
	{
		if (actTable.baseAct == ACT_RANGE_ATTACK1)
		{
			// Register throw activity, original from CNPC_Citizen
			actTable.weaponAct = ActivityList_RegisterPrivateActivity("ACT_CIT_HEAL");
			break;
		}
	}
}

void CRation::Materialize()
{
	BaseClass::Materialize();

	// Ration hit the floor, so, allow picking it up immediately for fluency
	SetContextThink(&ThisClass::AllowPickup, TICK_INTERVAL, RATION_PICKUP_ALLOW_CONTEXT);
}

bool CRation::VisibleInWeaponSelection()
{
#ifdef HL2RP_LEGACY
	// On HL2DM, the special weapons can't be assigned a proper slot
	return false;
#endif // HL2RP_LEGACY

	return BaseClass::VisibleInWeaponSelection();
}

bool CRation::Deploy()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer != NULL)
	{
		RevertExcessClip1(pPlayer);
	}

	BaseClass::Deploy();
	mPOVState = EPOVState::Deployed;
	return true;
}

void CRation::ItemPostFrame()
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(GetOwner());

	if (pPlayer != NULL)
	{
		if (m_iClip1 < 1 && mPOVState != EPOVState::ActioningBeforeDeploy)
		{
			if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) < 1)
			{
				// Hide viewmodel since RecvProxy_Weapon() may replay pre-deploy animation, causing undesired effect
				SetWeaponVisible(false);

				return Remove(); // NOTE: This will already switch to another weapon from modified base code
			}

			++m_iClip1;
			pPlayer->RemoveAmmo(1, m_iPrimaryAmmoType);
		}

		switch (mPOVState)
		{
		case EPOVState::Deployed:
		{
			if (FBitSet(pPlayer->m_afButtonPressed, IN_ATTACK))
			{
				SendWeaponAnim(ACT_VM_PULLBACK);
				pPlayer->SendHUDHint(EPlayerHUDHintType::RationThrowing, "HL2RP_RationThrowingHint");
				mPOVState = EPOVState::HaulingBackForThrow;
			}
			else if (FBitSet(pPlayer->m_afButtonPressed, IN_ATTACK2))
			{
				// Try healing
				int health = pPlayer->GetHealth();
				localizebuf_t message;

				if (health < pPlayer->GetMaxHealth())
				{
					health = Min(health + RATION_USAGE_HEALTH_GAIN, pPlayer->GetMaxHealth());
					pPlayer->SetHealth(health);
					--m_iClip1;
					AddEffects(EF_NODRAW);
					SendWeaponAnim(ACT_VM_RELEASE);
					WeaponSound(SPECIAL1);
					gHL2RPLocalize.Localize(pPlayer, message, true, "HL2RP_Ration_Heal_Success");
					pPlayer->SetNextAttack(gpGlobals->curtime + SequenceDuration()); // Delay ItemPostFrame()
					mPOVState = EPOVState::ActioningBeforeDeploy;
				}
				else
				{
					gHL2RPLocalize.Localize(pPlayer, message, true, "HL2RP_Ration_Heal_Deny");
				}

				ClientPrint(pPlayer, HUD_PRINTTALK, message);
			}
			else
			{
				pPlayer->SendHUDHint(EPlayerHUDHintType::RationDeployed, "HL2RP_RationDeployedHint");
			}

			RevertExcessClip1(pPlayer);
			break;
		}
		case EPOVState::HaulingBackForThrow:
		{
			if (FBitSet(pPlayer->m_nButtons, IN_ATTACK))
			{
				if (FBitSet(pPlayer->m_afButtonPressed, IN_ATTACK2))
				{
					Deploy();
					CSingleUserRecipientFilter filter(pPlayer);
					EmitSound(filter, pPlayer->entindex(), GetShootSound(SPECIAL3));
					RevertExcessClip1(pPlayer);
				}
				else if (FBitSet(pPlayer->m_afButtonPressed, IN_RELOAD) && m_iClip1 < INT_MAX
					&& pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0)
				{
					++m_iClip1;
					pPlayer->RemoveAmmo(1, m_iPrimaryAmmoType);
					CSingleUserRecipientFilter filter(pPlayer);
					EmitSound(filter, pPlayer->entindex(), GetShootSound(SPECIAL2));
				}
			}
			else if (IsViewModelSequenceFinished())
			{
				SendWeaponAnim(ACT_VM_THROW);
				pPlayer->SetAnimation(PLAYER_ATTACK1);
				mPOVState = EPOVState::ActioningBeforeDeploy;
			}

			break;
		}
		default: // EPOVState::ActioningBeforeDeploy
		{
			if (IsViewModelSequenceFinished())
			{
				Deploy();
			}
		}
		}
	}
}

// Transfers exceeding clip units back into reserve ammo, which maybe were added during throw
void CRation::RevertExcessClip1(CBasePlayer* pPlayer)
{
	if (m_iClip1 > 1)
	{
		int maxClips = Min(m_iClip1 - 1,
			GetAmmoDef()->MaxCarry(m_iPrimaryAmmoType) - pPlayer->GetAmmoCount(m_iPrimaryAmmoType));
		m_iClip1 -= pPlayer->GiveAmmo(maxClips, m_iPrimaryAmmoType, true);
	}
}

void CRation::HandleAnimEvent(animevent_t* pEvent)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer != NULL && pEvent->event == EVENT_WEAPON_THROW && m_iClip1 > 0)
	{
		// Prepare new ration to throw
		QAngle angles(pPlayer->EyeAngles().x, pPlayer->EyeAngles().y, 0.0f);
		CRation* pRation = static_cast<CRation*>(Create("ration", pPlayer->WorldSpaceCenter(), angles));
		pRation->AddSpawnFlags(SF_NORESPAWN | SF_WEAPON_NO_PLAYER_PICKUP);
		pRation->SetPrimaryAmmoCount(m_iClip1);
		pRation->m_iClip1 = m_iClip1 = 0;
		Vector origin, forward, right, up, mins, maxs;
		pPlayer->EyePositionAndVectors(&origin, &forward, &right, &up);
		origin += right * RATION_THROW_EYE_OFFSET_RIGHT + forward * RATION_THROW_EYE_OFFSET_FORWARD
			+ up * RATION_THROW_EYE_OFFSET_UP;

		// Fix position if ration would end in solid
		matrix3x4_t rotation;
		AngleMatrix(angles, rotation);
		RotateAABB(rotation, pRation->WorldAlignMins(), pRation->WorldAlignMaxs(), mins, maxs);
		trace_t trace;
		UTIL_TraceHull(pPlayer->WorldSpaceCenter(), origin, mins, maxs, pPlayer->PhysicsSolidMaskForEntity(),
			pPlayer, pPlayer->GetCollisionGroup(), &trace);

		if (trace.DidHit())
		{
			origin = trace.endpos;
		}

		// Throw it
		Vector velocity = pPlayer->GetAbsVelocity() + forward * RATION_THROW_AIR_SPEED;
		pRation->Teleport(&origin, NULL, &velocity);
		AngularImpulse angularVelocity(0.0f, 0.0f, RATION_THROW_ANGULAR_VEL_YAW);
		pRation->ApplyLocalAngularVelocityImpulse(angularVelocity);
		AddEffects(EF_NODRAW);
		pRation->SetContextThink(&ThisClass::AllowPickup, gpGlobals->curtime + RATION_THROW_MAX_PICKUP_BLOCK_TIME,
			RATION_PICKUP_ALLOW_CONTEXT);
	}
}

void CRation::AllowPickup()
{
	RemoveSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
	AddEffects(EF_ITEM_BLINK);
}

void CRation::DisallowPickup()
{
	AddSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
	RemoveEffects(EF_ITEM_BLINK);
}

void CRation::OnPickedUp(CBaseCombatCharacter* pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);

	if (mhParentDispenser != NULL)
	{
		mhParentDispenser->OnContainedRationPickup();
		mhParentDispenser = mhParentDispenser->mhContainedRation = NULL;
	}
}
