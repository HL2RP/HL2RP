// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_money.h"
#include "hl2_roleplayer.h"
#include "hl2rp_gamerules.h"
#include <hl2rp_localizer.h>

#define MONEY_PROP_EFFECTS_START_CONTEXT "StartEffects" // Rings, sound
#define MONEY_PROP_EFFECTS_START_DELAY   0.2f

#define MONEY_PROP_RING_EFFECT_RADIUS   15.0f
#define MONEY_PROP_RING_EFFECT_DURATION 0.2f

#define MONEY_PROP_RING_MODE_CUSTOM_COLOR 1 // Use custom field for ring effect color, instead of rendercolor

#define MONEY_DROP_MAX_DURATION        0.5f // NOTE: This is also the max. remaining time from pending spawns to allow a new drop
#define MONEY_DROP_MAX_SPAWNS_PER_TICK 10 // Tied to queued spawn frequency
#define MONEY_DROP_MAX_AXIS_VELOCITY   45.0f // Max. spawn velocity on each axis, with final one varying from drop mode

#define MONEY_DROP_SPIRAL_STEP_ANGLE    42.0f
#define MONEY_DROP_SPIRAL_STEP_HEIGHT   3.0f
#define MONEY_DROP_SPIRAL_STEP_VELOCITY 8.0f
#define MONEY_DROP_SPIRAL_MAX_HEIGHT    100.0f // Max. relative height before resetting spiral origin

#define MONEY_DROP_SOUND "HL2RP.MoneyDrop"
#define MONEY_GAIN_SOUND "HL2RP.MoneyGain"

static ConVar sMoneyDropSpiralUseCVar("sv_money_drop_use_spiral", "1", FCVAR_ARCHIVE,
	"Use spiral money dropping style (1) or classic one (0 - circular; random direction/velocity per prop)"),
	sMoneyDropRingModeCVar("sv_money_drop_ring_mode", "1", FCVAR_ARCHIVE,
		"0: Disable special ring effect on dropped money.\n"
		" - 1: Create rings with specified 'ringcolor' from the currency config.\n"
		" - 2: Like 1, but use 'rendercolor' instead (base field).", true, 0.0f, true, 2.0f);

LINK_ENTITY_TO_CLASS(prop_money, CMoneyProp)

BEGIN_DATADESC(CMoneyProp)
DEFINE_KEYFIELD_NOT_SAVED(mRingEffectColor, FIELD_COLOR32, "ringcolor")
END_DATADESC()

// Determines if given amount of *created* props is maxed out due to certain limits
static bool IsMoneyDropFull(int propsCount)
{
	return (engine->GetEntityCount() >= MAX_EDICTS
		|| TICKS_TO_TIME(propsCount / MONEY_DROP_MAX_SPAWNS_PER_TICK) >= MONEY_DROP_MAX_DURATION);
}

bool CHL2RPRules::DropMoney(int amount, CHL2Roleplayer* pPlayer, bool front)
{
	if (!mMoneyPropsData.IsEmpty())
	{
		int newPropsCount = 0;
		trace_t trace;
		Vector& start = trace.startpos = pPlayer->GetAbsOrigin(), &end = trace.endpos, origin;
		start.z += HL2_ROLEPLAYER_PROP_SPAWN_FLOOR_OFFSET;
		origin = end = start;
		float circleAngle = 0.0f, stepAngle = MONEY_DROP_SPIRAL_STEP_ANGLE;

		if (IsMoneyDropFull(mMoneyPropsToSpawn.Count()) || enginetrace->PointOutsideWorld(start))
		{
			pPlayer->Print(HUD_PRINTTALK, "#HL2RP_Money_Drop_Unavailable");
			return false;
		}
		// If we have to spawn in front, update ideal origin
		else if (front)
		{
			end += pPlayer->EyeDirection2D() * HL2_ROLEPLAYER_PROP_SPAWN_FRONT_OFFSET;
		}

		// Validate origin
		UTIL_TraceLine(start, end, pPlayer->PlayerSolidMask(true), pPlayer, pPlayer->GetCollisionGroup(), &trace);
		origin = start = end;

		if (trace.startsolid)
		{
			pPlayer->Print(HUD_PRINTTALK, "#HL2RP_Money_Drop_Unavailable");
			return false;
		}

		if (sMoneyDropSpiralUseCVar.GetBool())
		{
			end.z += MONEY_DROP_SPIRAL_MAX_HEIGHT;
			circleAngle = RandomFloat(0.0f, 360.0f);
			stepAngle *= Sign(RandomInt(-1, 0));
			UTIL_TraceLine(start, end, pPlayer->PlayerSolidMask(true), pPlayer, pPlayer->GetCollisionGroup(), &trace); // Limit max. height
		}

		for (int oldTail = mMoneyPropsToSpawn.Tail(); amount > 0;)
		{
			SMoneyPropData searchData(amount);
			int dataIndex = mMoneyPropsData.FindLessOrEqual(&searchData);

			if (dataIndex < 0)
			{
				// At this point, the remaining amount doesn't reach the lowest configured one. Try to reuse last prop.
				if (newPropsCount > 0)
				{
					mMoneyPropsToSpawn[mMoneyPropsToSpawn.Tail()]->mAmount += amount;
					return true;
				}

				dataIndex = 0; // Fallback
			}

			// If max. height were overpassed (from spiral), reset spawn origin to the initial (already valid)
			if (origin.z > end.z)
			{
				origin.z = start.z;
			}

			CMoneyProp* pProp = static_cast<CMoneyProp*>(CBaseEntity::CreateNoSpawn("prop_money", origin, vec3_angle));

			if (pProp != NULL)
			{
				mMoneyPropsToSpawn.AddToTail(pProp);
				pProp->mAmount = Min(amount, mMoneyPropsData[dataIndex]->mAmount);
				amount -= pProp->mAmount;
				auto& fields = mMoneyPropsData[dataIndex]->mFieldByName;

				FOR_EACH_MAP_FAST(fields, i)
				{
					pProp->KeyValue(fields.Key(i), fields[i].ToString());
				}

				// Apply remaining properties after loaded ones (KeyValues) to ensure these don't override ours
				QAngle angles(0.0f, circleAngle, 0.0f);
				Vector velocity(MONEY_DROP_MAX_AXIS_VELOCITY), forward;

				if (sMoneyDropSpiralUseCVar.GetBool())
				{
					origin.z += MONEY_DROP_SPIRAL_STEP_HEIGHT;
					circleAngle += stepAngle;
					velocity.x = velocity.y = Min(MONEY_DROP_SPIRAL_STEP_VELOCITY * newPropsCount, velocity.z);
					velocity.z = 0.0f;
				}
				else
				{
					angles.y = RandomFloat(0.0f, 360.0f);
					velocity.x = velocity.y = velocity.z * RandomFloat();
				}

				AngleVectors(angles, &forward);
				velocity.x *= forward.x;
				velocity.y *= forward.y;
				angles.y = RandomFloat(0.0f, 360.0f); // Now prop angles, independent to the direction
				pProp->Teleport(NULL, &angles, &velocity);
				pProp->AddEffects(EF_ITEM_BLINK);
				pProp->SetCollisionGroup(COLLISION_GROUP_DEBRIS_TRIGGER);
				auto pThinkFunc = (newPropsCount > 0) ? &CMoneyProp::CreateRings : &CMoneyProp::StartEffects; // Prepare minimal allowed effects
				pProp->SetContextThink(pThinkFunc, TICK_NEVER_THINK, MONEY_PROP_EFFECTS_START_CONTEXT);
				++newPropsCount;

				if (!IsMoneyDropFull(newPropsCount)) // Pre-check before the fallback action at loop end
				{
					continue;
				}
			}
			else if (newPropsCount < 1)
			{
				return false; // First prop failed to be created - we don't have anything else to do
			}

			// At this point, we can't create more props. Add the remaining amount to our first created prop.
			oldTail = mMoneyPropsToSpawn.IsValidIndex(oldTail) ?
				mMoneyPropsToSpawn.Next(oldTail) : mMoneyPropsToSpawn.Head();
			Assert(mMoneyPropsToSpawn.IsValidIndex(oldTail));
			mMoneyPropsToSpawn[oldTail]->mAmount += amount;
			return true;
		}

		return (newPropsCount > 0);
	}

	return false;
}

void CHL2RPRules::SpawnMoneyProps()
{
	for (int i = 0, head; i < MONEY_DROP_MAX_SPAWNS_PER_TICK
		&& !mMoneyPropsToSpawn.IsEmpty(); mMoneyPropsToSpawn.Remove(head))
	{
		head = mMoneyPropsToSpawn.Head();

		if (mMoneyPropsToSpawn[head] != NULL)
		{
			DispatchSpawn(mMoneyPropsToSpawn[head]);
			mMoneyPropsToSpawn[head]->SetPhysVelocity(mMoneyPropsToSpawn[head]->GetAbsVelocity()); // Apply configured velocity
			++i;
		}
	}
}

void CMoneyProp::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound(MONEY_DROP_SOUND);
	PrecacheScriptSound(MONEY_GAIN_SOUND);
}

void CMoneyProp::Spawn()
{
	BaseClass::Spawn();
	SetNextThink(gpGlobals->curtime + MONEY_PROP_EFFECTS_START_DELAY, MONEY_PROP_EFFECTS_START_CONTEXT);
}

// Ensures the prop can always be picked up
int CMoneyProp::ObjectCaps()
{
	return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE);
}

void CMoneyProp::StartEffects()
{
	CreateRings();

	// Play sound at current origin (so the source stays in same location)
	CPASAttenuationFilter filter(this, MONEY_DROP_SOUND);

	ForEachRoleplayer([&](CHL2Roleplayer* pPlayer)
	{
		if (pPlayer->mMiscFlags.IsBitSet(EPlayerMiscFlag::IsMoneyDropSoundDisabled))
		{
			filter.RemoveRecipient(pPlayer);
		}
	});

	EmitSound(filter, SOUND_FROM_WORLD, MONEY_DROP_SOUND, &GetAbsOrigin());
}

void CMoneyProp::CreateRings()
{
	if (sMoneyDropRingModeCVar.GetBool())
	{
		CPVSFilter filter(GetAbsOrigin());
		const color32& color =
			(sMoneyDropRingModeCVar.GetInt() == MONEY_PROP_RING_MODE_CUSTOM_COLOR) ? mRingEffectColor : GetRenderColor();
		te->BeamRingPoint(filter, 0.0f, GetAbsOrigin(), 0.0f, MONEY_PROP_RING_EFFECT_RADIUS,
			PrecacheModel(HL2RP_MAIN_BEAM_PATH), 0, 0, 0, MONEY_PROP_RING_EFFECT_DURATION,
			HL2RP_SMALL_BEAM_WIDTH, 0, 0.0f, color.r, color.g, color.b, color.a, 0);
	}
}

void CMoneyProp::Use(CBaseEntity* pActivator, CBaseEntity*, USE_TYPE type, float)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(pActivator);

	// NOTE: In case of overflow prevention, require at least not coming with a full pocket (INT_MAX) to pick the prop
	if (pPlayer != NULL && type == USE_TOGGLE
		&& pPlayer->mAimInfo.mhMainEntity == this && pPlayer->AddPocket(mAmount) > 0)
	{
		Remove();
		pPlayer->Print(HUD_PRINTTALK, "#HL2RP_Money_Picked", UTIL_FormatMoney(pPlayer, mAmount));
	}
}

int CBaseHL2Roleplayer::AddMoney(int amount, CPlayerMoney& money, EPlayerDatabasePropType type)
{
	if (amount != 0)
	{
#ifdef HL2RP_LEGACY
		money.mVariationData.Update(money);
#endif // HL2RP_LEGACY

		if (!mMiscFlags.IsBitSet(EPlayerMiscFlag::IsMoneyVariationSoundDisabled))
		{
			EmitLocalSound(MONEY_GAIN_SOUND, true);
		}

		int oldAmount = money;
		money.mAmount += amount;
		OnDatabasePropChanged((int)money, type);
		return (money - oldAmount);
	}

	return 0;
}

static void HandleMoneyDropCmd(const CCommand& args, bool substract)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_GetCommandClient());

	if (pPlayer != NULL && UTIL_CheckCmdArgCount(args))
	{
		int amount = Q_atoi(args.Arg(1));

		if (substract)
		{
			if (amount > pPlayer->mPocket)
			{
				return pPlayer->Print(HUD_PRINTTALK, "#HL2RP_Not_Enough_Money", UTIL_FormatMoney(pPlayer, amount));
			}
		}
		else if (!UTIL_CheckCommandAccess(EPlayerAccessFlag::Root))
		{
			return;
		}

		if (HL2RPRules()->DropMoney(amount, pPlayer, Q_stricmp(args.Arg(2), "front") == 0))
		{
			if (substract)
			{
				pPlayer->AddPocket(-amount);
			}
			else
			{
				UTIL_LogAdminAction(pPlayer, "dropped %s outside their balance", UTIL_FormatMoney(NULL, amount));
			}

			pPlayer->Print(HUD_PRINTTALK, "#HL2RP_Money_Dropped", UTIL_FormatMoney(pPlayer, amount));
		}
	}
}

CON_COMMAND(drop_money, "<amount> [front] - Drop money by specified amount, substracting from pocket."
	" Pass 'front' as second argument to spawn in front of you, as opposed to your origin (default).")
{
	HandleMoneyDropCmd(args, true);
}

CON_COMMAND(drop_unowned_money, "<amount> [front] - Root only: Drop money without substracting from your pocket")
{
	HandleMoneyDropCmd(args, false);
}

CON_COMMAND(add_money, "[userid] <amount>"
	" - Adds money into a player's pocket by specified amount. If userid isn't set, command will target aiming player.")
{
	if (UTIL_CheckCmdArgCount(args) && UTIL_CheckCommandAccess(EPlayerAccessFlag::Root))
	{
		CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_GetCommandClient()), * pTarget;

		if (UTIL_FindCmdTarget(args, pTarget))
		{
			int index = Min(args.ArgC() - 1, 2), amount = Q_atoi(args.Arg(index));

			if (amount > 0)
			{
				amount = pTarget->AddPocket(amount);
				CLocalizeFmtCStr defAmountStr; // Default-localized
				UTIL_FormatMoney(defAmountStr, amount);
				UTIL_ReplyToCommand(HUD_PRINTTALK, "#HL2RP_Money_Added_Issuer",
					(pPlayer != NULL) ? UTIL_FormatMoney(pPlayer, amount) : defAmountStr, pTarget->GetPlayerName());
				UTIL_LogAdminAction(pPlayer, "added %s to the Pocket of '%s' (%s)",
					defAmountStr.mDest, pTarget->GetPlayerName(), pTarget->GetNetworkIDString());

				if (pPlayer != pTarget)
				{
					pTarget->Print(HUD_PRINTTALK, "#HL2RP_Money_Added_Target",
						UTIL_GetCommandIssuerName(), UTIL_FormatMoney(pTarget, amount));
				}
			}
		}
	}
}
