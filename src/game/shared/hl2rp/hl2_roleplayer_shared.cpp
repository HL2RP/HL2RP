// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2_roleplayer_shared.h"
#include <in_buttons.h>

#ifdef GAME_DLL
#include <hl2_roleplayer.h>
#include <inetwork_dialog.h>
#endif // GAME_DLL

#ifdef HL2RP_CLIENT_OR_LEGACY
#include "hl2rp_localizer.h"
#include "hl2rp_property.h"

#ifdef GAME_DLL
#include <hl2rp_gamerules.h>
#else
#include <c_hl2_roleplayer.h>
#include <c_hl2rp_gamerules.h>
#include <prediction.h>
#endif // GAME_DLL
#endif // HL2RP_CLIENT_OR_LEGACY

#define HL2_ROLEPLAYER_WALK_SPEED 80.0f

#define HL2_ROLEPLAYER_EXTRA_AIM_TRACE_DIST 32.0f // Max. distance behind hit world brush to search for an usable entity

#define HL2_ROLEPLAYER_MONEY_VARIATION_DURATION 5.0f

extern ConVar gRegionMaxRadiusCVar;

LINK_ENTITY_TO_CLASS(player, CHL2Roleplayer)

// Trace filter for player's view that needs to be permissive to detect stuff discarded by usual filters otherwise.
// For example, money props held by physcannon that may be picked up would fail by PassServerEntityFilter.
class CAimTraceFilter : public CTraceFilterSimple
{
	TraceType_t GetTraceType() const OVERRIDE
	{
		return mTraceType;
	}

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) OVERRIDE
	{
		return (StandardFilterRules(pHandleEntity, contentsMask) && pHandleEntity != GetPassEntity());
	}

public:
	CAimTraceFilter(IHandleEntity* pPassEntity, TraceType_t traceType = TRACE_EVERYTHING)
		: CTraceFilterSimple(pPassEntity, COLLISION_GROUP_NONE), mTraceType(traceType) {}

	TraceType_t mTraceType;
};

CHL2Roleplayer* ToHL2Roleplayer(CBasePlayer* pPlayer)
{
	return dynamic_cast<CHL2Roleplayer*>(pPlayer);
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

bool CBaseHL2Roleplayer::IsAdmin(int minAccessFlag)
{
	return (mAccessFlags >= INDEX_TO_FLAG(minAccessFlag));
}

bool CBaseHL2Roleplayer::HasCombineGrants(bool extraCombineCheck)
{
	return ((mFaction == EFaction::Combine && extraCombineCheck) || IsAdmin());
}

bool CBaseHL2Roleplayer::IsDamageProtected()
{
	return (!IsBot() && (!mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded) ||
		(mZonesWithin[ECityZoneType::NoKill] != NULL && mCrime < 1 && mFaction == EFaction::Citizen)));
}

bool CBaseHL2Roleplayer::IsWithinInteractRadius(CBaseEntity* pOther, float radius)
{
	return (pOther->GetAbsOrigin() - GetAbsOrigin()).IsLengthLessThan(radius);
}

void CHL2Roleplayer::HandleWalkChanges(CMoveData* pMv)
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
				else if (FBitSet(pMv->m_nButtons & ~pMv->m_nOldButtons, IN_WALK))
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

void CBaseHL2Roleplayer::GetAimInfo(SPlayerAimInfo& info)
{
	Vector start = EyePosition(), forward;
	AngleVectors(EyeAngles(), &forward);
	float maxDistance = HasCombineGrants() ?
		HL2_ROLEPLAYER_COMBINE_AIM_TRACE_DIST : HL2_ROLEPLAYER_CITIZEN_AIM_TRACE_DIST;
	trace_t trace;
	CAimTraceFilter filter(this);
	UTIL_TraceLine(start, start + forward * maxDistance, PhysicsSolidMaskForEntity(), &filter, &trace);
	info.mhMainEntity = trace.m_pEnt;

	if (trace.m_pEnt != NULL)
	{
		info.mHitPosition = trace.endpos;
		info.mHitNormal = trace.plane.normal;
		info.mEndDistance = maxDistance * trace.fraction;

		// Search for an entity behind hit world brush, which we may accept in some cases (e.g. sliding doors)
		if (trace.DidHitWorld())
		{
			maxDistance = Min(HL2_ROLEPLAYER_EXTRA_AIM_TRACE_DIST, maxDistance - info.mEndDistance);
			trace_t extraTrace;
			filter.mTraceType = TRACE_ENTITIES_ONLY;
			UTIL_TraceLine(trace.endpos, trace.endpos + forward * maxDistance,
				PhysicsSolidMaskForEntity(), &filter, &extraTrace);

			if (extraTrace.m_pEnt != NULL)
			{
				if (FBitSet(trace.contents, MASK_OPAQUE))
				{
					if (FBitSet(extraTrace.m_pEnt->ObjectCaps(), FCAP_HL2RP_USE_BEHIND_ANY))
					{
						info.mhBackEntity = extraTrace.m_pEnt;
						info.mEndDistance += maxDistance * extraTrace.fraction;
					}
				}
				else if (FBitSet(extraTrace.m_pEnt->ObjectCaps(),
					FCAP_HL2RP_USE_BEHIND_ANY | FCAP_HL2RP_USE_BEHIND_TRANS))
				{
					info.mhMainEntity = extraTrace.m_pEnt;
					info.mEndDistance += maxDistance * extraTrace.fraction;
				}
			}
		}
	}
}

#ifdef HL2RP_CLIENT_OR_LEGACY
void CPlayerMoney::SVariationData::Update(int amount)
{
	if (mEndTimer.Expired())
	{
		mOldAmount = amount;
	}

	mEndTimer.Set(HL2_ROLEPLAYER_MONEY_VARIATION_DURATION);
}

const char* CPlayerMoney::Format(CLocalizeFmtCStr&& dest) // Formats current amount, including recent variations
{
	UTIL_FormatMoney(dest, mAmount);

	if (!mVariationData.mEndTimer.Expired() && mAmount != mVariationData.mOldAmount)
	{
		int amountDiff = mAmount - mVariationData.mOldAmount;
		dest += (amountDiff > 0) ? " +" : " ";
		UTIL_FormatMoney(dest, amountDiff);
	}

	return dest;
}

bool CBaseHL2Roleplayer::GetZoneHUD(CLocalizeFmtStr<>& dest)
{
	for (int i = mZonesWithin.Count(); --i >= 0;)
	{
		if (mZonesWithin[i] != NULL)
		{
			if (i == ECityZoneType::NoKill)
			{
				dest.Format("%t: %t", CCityZone::sTypeTokens[i], IsDamageProtected() ?
					"#HL2RP_Zone_NoKill_Protected" : "#HL2RP_Zone_NoKill_Unprotected");
				return true;
			}

			dest.Localize("#HL2RP_Zone_Notice");
			const char* pToken = CCityZone::sTypeTokens[i], * pArg = "";
			CHL2RP_Property* pProperty = mZonesWithin[i]->mpProperty;

			if (pProperty != NULL)
			{
				if (*pProperty->mName != '\0')
				{
					dest.Format(" '%s'", pProperty->mName);
				}

				if (pProperty->IsOwner(this))
				{
					pToken = "#HL2RP_Zone_Home_Owned_Self";
				}
				else if (pProperty->HasOwner())
				{
					pToken = "#HL2RP_Zone_Home_Owned_Other";
					pArg = HL2RPRules()->mPlayerNameBySteamIdNum.GetElementOrDefault(pProperty->mOwnerSteamIdNumber, "");
				}
			}

			dest.Format(" [%t]", pToken, pArg);
			return true;
		}
	}

	return false;
}

void CBaseHL2Roleplayer::GetPlayersInRegion(CUtlVector<CBasePlayer*>& players)
{
	if (mMiscFlags.IsBitSet(EPlayerMiscFlag::IsRegionListEnabled))
	{
		// Search up to max. allowed players plus one (to reflect on displayed count)
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_PlayerByIndex(i));

			if (pPlayer != NULL && pPlayer != this && pPlayer->mMiscFlags.IsBitSet(EPlayerMiscFlag::IsRegionListEnabled)
				&& pPlayer->IsAlive() && IsWithinInteractRadius(pPlayer, gRegionMaxRadiusCVar.GetFloat()))
			{
				if (players.AddToTail(pPlayer) >= HL2_ROLEPLAYER_REGION_MAX_PLAYERS)
				{
					break;
				}
			}
		}
	}
}

int CBaseHL2Roleplayer::GetRegionHUD(const CUtlVector<CBasePlayer*>& players, CLocalizeFmtStr<>& dest)
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

void CHL2Roleplayer::GetHUDInfo(CHL2Roleplayer* pPlayer, CLocalizeFmtStr<>& text)
{
	text.Localize("#HL2RP_HUD_AimingPlayer", DATABASE_PROP_STRING(mJobName));
}

void CHL2Roleplayer::GetMainHUD(CLocalizeFmtStr<>& dest)
{
	dest.Localize("#HL2RP_HUD_Main", UTIL_FormatDuration(this, mSeconds), mPocket.Format(this),
		DATABASE_PROP_STRING(mJobName), UTIL_FormatInteger(this, mCrime));
}
#endif // HL2RP_CLIENT_OR_LEGACY

void CBaseHL2Roleplayer::EmitLocalSound(const char* pName, bool overwrite)
{
#ifdef CLIENT_DLL
	if (prediction->IsFirstTimePredicted())
#endif // CLIENT_DLL
	{
		if (overwrite)
		{
			StopSound(pName);
		}

		CSingleUserRecipientFilter filter(this);
		EmitSound(filter, GetSoundSourceIndex(), pName);
	}
}
