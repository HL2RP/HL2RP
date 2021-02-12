// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>

#define GC // To use UTF-8 localization on client

#include "ration.h"
#include <prop_ration_dispenser.h>
#include <activitylist.h>
#include <ammodef.h>
#include <in_buttons.h>

#ifdef CLIENT_DLL
#include <c_hl2_roleplayer.h>
#include <prediction.h>
#else
#include <hl2_roleplayer.h>
#endif // CLIENT_DLL

#define RATION_USAGE_HEALTH_GAIN 5

LINK_ENTITY_TO_CLASS(ration, CRation)
PRECACHE_WEAPON_REGISTER(ration);

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CRation)
DEFINE_PRED_FIELD(mPOVState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE)
END_PREDICTION_DATA()
#endif // CLIENT_DLL

#ifdef HL2RP_FULL
IMPLEMENT_HL2RP_NETWORKCLASS(Ration)
#ifdef GAME_DLL
SendPropInt(SENDINFO(mPOVState))
#else
RecvPropInt(RECVINFO(mPOVState))
#endif // GAME_DLL
END_NETWORK_TABLE()
#endif // HL2RP_FULL

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
	BaseClass::Deploy();
	RevertExcessClip1();
	GetOwner()->SetNextAttack(gpGlobals->curtime + GetViewModelSequenceDuration());
	mPOVState = ERationPOVState::Deployed;
	return true;
}

void CRation::ItemPostFrame()
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(GetOwner());

	if (pPlayer != NULL)
	{
		pPlayer->SetNextAttack(gpGlobals->curtime + TICK_INTERVAL);

		if (m_iClip1 < 1)
		{
			if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) < 1)
			{
#ifdef GAME_DLL // Limit to server to prevent prediction from throwing a Host_Error() on next tick
				Remove(); // NOTE: This will already try to switch to another weapon from modified base code
#endif // GAME_DLL

				return SetWeaponVisible(false); // Prevent viewmodel from showing indefinitely while player lacks weapons
			}

			++m_iClip1;
			pPlayer->RemoveAmmo(1, m_iPrimaryAmmoType);
		}

		switch (mPOVState)
		{
		case ERationPOVState::Deployed:
		{
			if (FBitSet(pPlayer->m_afButtonPressed, IN_ATTACK))
			{
				SendWeaponAnim(ACT_VM_PULLBACK);
				pPlayer->SendHUDHint(EPlayerHUDHintType::RationThrowing, "#HL2RP_RationThrowingHint", false);
				m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration(); // Available throw time
				mPOVState = ERationPOVState::HaulingBackForThrow;
			}
			else if (FBitSet(pPlayer->m_afButtonPressed, IN_ATTACK2))
			{
				// Try healing
				int healthDiff = pPlayer->GetMaxHealth() - pPlayer->GetHealth();
				localizebuf_t message;

				if (healthDiff > 0)
				{
					healthDiff = Min(healthDiff, RATION_USAGE_HEALTH_GAIN);
					pPlayer->SetHealth(pPlayer->m_iHealth + healthDiff);
					--m_iClip1;
					AddEffects(EF_NODRAW);
					SendWeaponAnim(ACT_VM_RELEASE);
					EmitSoundToOwner(SPECIAL1, pPlayer);
					gHL2RPLocalizer.Localize(pPlayer, message, true, "#HL2RP_Ration_Heal_Success", healthDiff);
					pPlayer->SetNextAttack(gpGlobals->curtime + GetViewModelSequenceDuration());
					mPOVState = ERationPOVState::ActioningBeforeDeploy;
				}
				else
				{
					gHL2RPLocalizer.Localize(pPlayer, message, true, "#HL2RP_Ration_Heal_Deny");
				}

				pPlayer->Print(HUD_PRINTTALK, message, false);
			}
			else
			{
				pPlayer->SendHUDHint(EPlayerHUDHintType::RationDeployed, "#HL2RP_RationDeployedHint", false);
			}

			RevertExcessClip1();
			break;
		}
		case ERationPOVState::HaulingBackForThrow:
		{
			if (FBitSet(pPlayer->m_nButtons, IN_ATTACK))
			{
				if (FBitSet(pPlayer->m_afButtonPressed, IN_ATTACK2))
				{
					Deploy();
					EmitSoundToOwner(SPECIAL3, pPlayer);
				}
				else if (FBitSet(pPlayer->m_afButtonPressed, IN_RELOAD) && m_iClip1 < INT_MAX
					&& pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0)
				{
					++m_iClip1;
					pPlayer->RemoveAmmo(1, m_iPrimaryAmmoType);
					EmitSoundToOwner(SPECIAL2, pPlayer);
				}
			}
			else if (gpGlobals->curtime >= m_flNextPrimaryAttack)
			{
				SendWeaponAnim(ACT_VM_THROW);
				pPlayer->SetAnimation(PLAYER_ATTACK1);
				pPlayer->SetNextAttack(gpGlobals->curtime + GetViewModelSequenceDuration());
				mPOVState = ERationPOVState::ActioningBeforeDeploy;
			}

			break;
		}
		default: // ERationPOVState::ActioningBeforeDeploy
		{
			Deploy();
		}
		}
	}
}

// Transfers exceeding clip units back into reserve ammo, which maybe were added during throw
void CRation::RevertExcessClip1()
{
	if (m_iClip1 > 1)
	{
		int ammoCount = GetOwner()->GetAmmoCount(m_iPrimaryAmmoType),
			maxClips = Min(m_iClip1 - 1, GetAmmoDef()->MaxCarry(m_iPrimaryAmmoType) - ammoCount);
		m_iClip1 -= maxClips;

#ifdef GAME_DLL // Let only the server add up the ammo to prevent duplicated HUD history icons at client
		GetOwner()->SetAmmoCount(ammoCount + maxClips, m_iPrimaryAmmoType);
#endif // GAME_DLL
	}
}

void CRation::EmitSoundToOwner(int type, CBasePlayer* pPlayer)
{
#ifdef HL2RP_CLIENT_OR_LEGACY
#ifdef CLIENT_DLL
	if (prediction->IsFirstTimePredicted()) // Critical to prevent duplicated sounds especially with higher pings
#endif // CLIENT_DLL
	{
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, pPlayer->entindex(), GetShootSound(type));
	}
#endif // HL2RP_CLIENT_OR_LEGACY
}

void CRation::AllowPickup()
{
	AddEffects(EF_ITEM_BLINK);

#ifdef GAME_DLL
	RemoveSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
#endif // GAME_DLL
}

void CRation::DisallowPickup()
{
	RemoveEffects(EF_ITEM_BLINK);

#ifdef GAME_DLL
	AddSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
#endif // GAME_DLL
}

void CRation::OnPickedUp(CBaseCombatCharacter* pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);

	if (mhParentDispenser != NULL)
	{
		mhParentDispenser->OnContainedRationPickup();
		mhParentDispenser = NULL;
	}
}
