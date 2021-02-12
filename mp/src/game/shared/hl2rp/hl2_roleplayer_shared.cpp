// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2_roleplayer_shared.h"
#include "prop_ration_dispenser.h"
#include <in_buttons.h>

#ifdef GAME_DLL
#include <hl2_roleplayer.h>
#else
#include <c_hl2_roleplayer.h>
#include <hl2mp_gamerules.h>
#endif // GAME_DLL

// Maximum timeout between two walk key presses within which player is able to enter sticky walk mode
#define HL2_ROLEPLAYER_STICKY_WALKING_CHANCE_TIMEOUT 0.3f

#define HL2_ROLEPLAYER_WALK_SPEED 80.0f

#define HL2_ROLEPLAYER_CITIZEN_AIM_TRACE_DIST 150.0f
#define HL2_ROLEPLAYER_POLICE_AIM_TRACE_DIST  (HL2_ROLEPLAYER_CITIZEN_AIM_TRACE_DIST * 4.0f)

LINK_ENTITY_TO_CLASS(player, CHL2Roleplayer)

CHL2Roleplayer* ToHL2Roleplayer(CBasePlayer* pPlayer)
{
	return static_cast<CHL2Roleplayer*>(pPlayer);
}

CHL2Roleplayer* ToHL2Roleplayer(CBaseEntity* pEntity)
{
	return ToHL2Roleplayer(ToBasePlayer(pEntity));
}

bool ShouldPlayerMoveTraceHit(IHandleEntity* pHandleEntity, int)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(EntityFromEntityHandle(pHandleEntity));
	return (pPlayer == NULL || !FBitSet(pPlayer->mMovementFlags, EPlayerMovementFlag::Interpenetrating));
}

struct SRelativeTime
{
	SRelativeTime(int seconds) : mHours(seconds / 3600), mMinutes(seconds % 3600 / 60), mSeconds(seconds % 60) {}

	int mHours, mMinutes, mSeconds;
};

// Unified advanced filtering for player's aiming entities within HL2RP context
class CHL2RoleplayerAimTraceFilter : public CTraceFilterSimple
{
	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) OVERRIDE
	{
		return CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask);
	}

public:
	CHL2RoleplayerAimTraceFilter(CHL2Roleplayer* pPlayer) : CTraceFilterSimple(pPlayer, pPlayer->GetCollisionGroup()) {}
};

void CBaseHL2Roleplayer::Spawn()
{
	BaseClass::Spawn();
	StopWalking();
}

void CBaseHL2Roleplayer::PreThink()
{
	BaseClass::PreThink();
	CLEARBITS(mMovementFlags, EPlayerMovementFlag::Interpenetrating);

	if (IsAlive())
	{
		// Handle stuck prevention with other players
		trace_t trace;
		UTIL_TraceEntity(this, GetAbsOrigin(), GetAbsOrigin(), PhysicsSolidMaskForEntity(), &trace);
		CHL2Roleplayer* pPlayer = ToHL2Roleplayer(trace.m_pEnt);

		if (pPlayer != NULL)
		{
			SETBITS(mMovementFlags, EPlayerMovementFlag::Interpenetrating);
		}
	}
}

void CHL2Roleplayer::HandleWalkChanges()
{
	// Check if player is allowed to walk
	if (!IsSprinting() && !FBitSet(m_nButtons, IN_DUCK))
	{
		if (FBitSet(m_afButtonPressed, IN_WALK))
		{
			if (IsWalking())
			{
				return StopWalking();
			}

			StartWalking();

			if (mStickyWalkChanceExpireTimer.Expired())
			{
				mStickyWalkChanceExpireTimer.Set(HL2_ROLEPLAYER_STICKY_WALKING_CHANCE_TIMEOUT);
				return SendHUDHint(EPlayerHUDHintType::StickyWalking, "#HL2RP_StickyWalkHint", false);
			}

			SETBITS(mMovementFlags, EPlayerMovementFlag::InStickyWalkMode);
		}
		else if (FBitSet(m_afButtonReleased, IN_WALK) && !FBitSet(mMovementFlags, EPlayerMovementFlag::InStickyWalkMode))
		{
			StopWalking();
		}
	}
	else if (IsWalking())
	{
		StopWalking();
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
	CLEARBITS(mMovementFlags, EPlayerMovementFlag::InStickyWalkMode);
}

#ifdef HL2RP_CLIENT_OR_LEGACY
void CBaseHL2Roleplayer::ComputeMainHUD(localizebuf_t& dest)
{
	SRelativeTime time(mSeconds);
	gHL2RPLocalizer.Localize(this, dest, false, "#HL2RP_MainHUD", time.mHours, time.mMinutes, time.mSeconds,
		mCrime->Get());
}
#endif // HL2RP_CLIENT_OR_LEGACY

bool CHL2Roleplayer::ComputeAimingEntityAndHUD(localizebuf_t& dest)
{
	dest[0] = '\0';
	bool success = false;
	Vector forward;
	AngleVectors(EyeAngles(), &forward);
	float maxDistance = (GetTeamNumber() == TEAM_COMBINE) ?
		HL2_ROLEPLAYER_POLICE_AIM_TRACE_DIST : HL2_ROLEPLAYER_CITIZEN_AIM_TRACE_DIST;
	trace_t trace;
	CHL2RoleplayerAimTraceFilter filter(this);
	UTIL_TraceLine(EyePosition(), EyePosition() + forward * maxDistance, PhysicsSolidMaskForEntity(), &filter, &trace);

	if (trace.DidHitNonWorldEntity())
	{
#ifdef HL2RP_CLIENT_OR_LEGACY
		// Aiming entities must only be displayed within citizen's range, regardless of team
		if (maxDistance * trace.fraction < HL2_ROLEPLAYER_CITIZEN_AIM_TRACE_DIST
#ifdef HL2RP_LEGACY
			&& AcquireHUDTime(EPlayerHUDType::AimingEntity, trace.m_pEnt != mhAimingEntity)
#endif // HL2RP_LEGACY
			)
		{
			CRationDispenserProp* pDispenser = dynamic_cast<CRationDispenserProp*>(trace.m_pEnt);

			if (pDispenser != NULL && pDispenser->SequenceDuration() <= 0.0f)
			{
				if (gpGlobals->curtime >= pDispenser->mNextTimeAvailable)
				{
					gHL2RPLocalizer.Localize(this, dest, false, "#HL2RP_Dispenser_Available");
				}
				else
				{
					SRelativeTime time(pDispenser->mNextTimeAvailable - gpGlobals->curtime);
					gHL2RPLocalizer.Localize(this, dest, false, "#HL2RP_Dispenser_Restoring",
						time.mHours, time.mMinutes, time.mSeconds);
				}
			}

			success = (dest[0] != '\0');
		}
#endif // HL2RP_CLIENT_OR_LEGACY

		mhAimingEntity = trace.m_pEnt;
		return success;
	}

	mhAimingEntity = NULL;
	return success;
}
