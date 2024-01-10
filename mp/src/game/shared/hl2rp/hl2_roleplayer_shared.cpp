// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2_roleplayer_shared.h"
#include <in_buttons.h>

#ifdef GAME_DLL
#include <hl2_roleplayer.h>
#else
#include <c_hl2_roleplayer.h>
#endif // GAME_DLL

#define HL2_ROLEPLAYER_WALK_SPEED 80.0f

extern ConVar gRegionMaxRadiusCVar;

LINK_ENTITY_TO_CLASS(player, CHL2Roleplayer)

CHL2Roleplayer* ToHL2Roleplayer(CBasePlayer* pPlayer)
{
	return static_cast<CHL2Roleplayer*>(pPlayer);
}

CHL2Roleplayer* ToHL2Roleplayer(CBaseEntity* pEntity)
{
	return ToHL2Roleplayer(ToBasePlayer(pEntity));
}

void CBaseHL2Roleplayer::Spawn()
{
	BaseClass::Spawn();
	StopWalking();
}

void CBaseHL2Roleplayer::Precache()
{
	BaseClass::Precache();
	PrecacheModel(HL2_ROLEPLAYER_BEAMS_PATH);
	PrecacheScriptSound(NETWORK_DIALOG_REWIND_SOUND);
	PrecacheScriptSound(NETWORK_MENU_ITEM_SOUND);
	PrecacheScriptSound(HL2RP_PROPERTY_DOOR_LOCK_SOUND);
	PrecacheScriptSound(HL2RP_PROPERTY_DOOR_UNLOCK_SOUND);
}

bool CBaseHL2Roleplayer::IsAdmin()
{
	return (mAccessFlags >= INDEX_TO_FLAG(EPlayerAccessFlag::Admin));
}

bool CBaseHL2Roleplayer::HasCombineGrants(bool extraCombineCheck)
{
	return ((mFaction == EFaction::Combine && extraCombineCheck) || IsAdmin());
}

bool CBaseHL2Roleplayer::IsWithinDistance(CBaseEntity* pOther, float maxDistance, bool fromEye)
{
	return (pOther->GetAbsOrigin() - (fromEye ? EyePosition() : GetAbsOrigin())).IsLengthLessThan(maxDistance);
}

void CHL2Roleplayer::HandleWalkChanges(int)
{
	if (!IsSprinting())
	{
		if (!FBitSet(m_nButtons, IN_DUCK))
		{
			if (FBitSet(m_nButtons, IN_WALK) || mIsInStickyWalkMode)
			{
				if (!IsWalking())
				{
					StartWalking();

					if (mStickyWalkChanceTimer.Expired())
					{
						mStickyWalkChanceTimer.Set(HL2_ROLEPLAYER_DOUBLE_KEYPRESS_MAX_DELAY);
						return LocalDisplayHUDHint(EPlayerHUDHintType::StickyWalking, "#HL2RP_Hint_StickyWalking");
					}

					mIsInStickyWalkMode = true;
				}
				else if (FBitSet(m_afButtonPressed, IN_WALK))
				{
					StopWalking();
				}
			}
			else if (IsWalking())
			{
				StopWalking();
			}
		}
		else if (IsWalking())
		{
			CHL2MP_Player::StopWalking(); // Stop walking, but keep the internal sticky walk state
		}
	}
}

void CBaseHL2Roleplayer::StartWalking()
{
	BaseClass::StartWalking();
	SetMaxSpeed(HL2_ROLEPLAYER_WALK_SPEED);
}

void CBaseHL2Roleplayer::StopWalking()
{
	BaseClass::StopWalking();
	mIsInStickyWalkMode = false;
}

CBaseEntity* CBaseHL2Roleplayer::GetAimingEntity(float& distance)
{
	Vector start = EyePosition(), forward;
	AngleVectors(EyeAngles(), &forward);
	distance = HasCombineGrants() ? HL2_ROLEPLAYER_COMBINE_AIM_TRACE_DIST : HL2_ROLEPLAYER_CITIZEN_AIM_TRACE_DIST;
	trace_t trace;
	UTIL_TraceLine(start, start + forward * distance, PhysicsSolidMaskForEntity(), this, GetCollisionGroup(), &trace);
	distance *= trace.fraction;
	return (trace.DidHitNonWorldEntity() ? trace.m_pEnt : NULL);
}

#ifdef HL2RP_CLIENT_OR_LEGACY
void CBaseHL2Roleplayer::GetPlayersInRegion(CUtlVector<CBasePlayer*>& players)
{
	if (mMiscFlags.IsBitSet(EPlayerMiscFlag::IsRegionListEnabled))
	{
		// Search up to max. allowed players plus one (to reflect on displayed count)
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_PlayerByIndex(i));

			if (pPlayer != NULL && pPlayer != this && pPlayer->mMiscFlags.IsBitSet(EPlayerMiscFlag::IsRegionListEnabled)
				&& pPlayer->IsAlive() && IsWithinDistance(pPlayer, gRegionMaxRadiusCVar.GetFloat(), false))
			{
				if (players.AddToTail(pPlayer) >= HL2_ROLEPLAYER_REGION_MAX_PLAYERS)
				{
					break;
				}
			}
		}
	}
}

int CBaseHL2Roleplayer::ComputeRegionHUD(const CUtlVector<CBasePlayer*>& players, CLocalizeFmtStr<>& dest)
{
	int playerLinesCount = players.Size();

	if (playerLinesCount > HL2_ROLEPLAYER_REGION_MAX_PLAYERS)
	{
		playerLinesCount = HL2_ROLEPLAYER_REGION_MAX_PLAYERS;
		dest.Format("(%s+) ", players.Size());
	}

	dest.Format("%t:  ", "#HL2RP_HUD_Region");

	for (int i = 0; i < playerLinesCount; ++i)
	{
		dest.Format("\n%s  ", players[i]->GetPlayerName());
	}

	return playerLinesCount;
}

void CHL2Roleplayer::GetHUDInfo(CHL2Roleplayer* pPlayer, LOCCHAR_T* pDest, int maxLen)
{
	CBaseLocalizeFmtStr<>(pPlayer, pDest, maxLen).Localize("#HL2RP_HUD_AimingPlayer", DATABASE_PROP_STRING(mJobName));
}

void CHL2Roleplayer::ComputeMainHUD(CLocalizeFmtStr<>& dest)
{
	SRelativeTime time(mSeconds);
	dest.Localize("#HL2RP_HUD_Main", "#HL2RP_Duration_HHMMSS", time.mHours, time.mMinutes, time.mSeconds,
		DATABASE_PROP_STRING(mJobName), mCrime->Get());
}
#endif // HL2RP_CLIENT_OR_LEGACY
