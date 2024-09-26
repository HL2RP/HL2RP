// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_npc_police.h"
#include "hl2_roleplayer.h"
#include "hl2rp_gamerules.h"
#include <hl2rp_localizer.h>
#include <ai_memory.h>
#include <npcevent.h>

#define HL2RP_NPC_POLICE_HEALTH 150

#define HL2RP_NPC_POLICE_FREE_KNOWLEDGE_DURATION 2.0f
#define HL2RP_NPC_POLICE_ENEMY_DISCARD_DELAY     5.0f
#define HL2RP_NPC_POLICE_ATTACKER_FORGET_DELAY   8.0f

extern int ACT_ACTIVATE_BATON;

LINK_ENTITY_TO_CLASS(npc_hl2rp_police, CHL2RPNPCPolice)

#ifdef HL2RP_FULL
IMPLEMENT_HL2RP_NETWORKCLASS(HL2RPNPCPolice)
END_SEND_TABLE()
#endif // HL2RP_FULL

AI_BEGIN_CUSTOM_NPC(npc_hl2rp_police, CHL2RPNPCPolice)
DEFINE_SCHEDULE(SCHED_HL2RP_POLICE_PATROL,
	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_HL2RP_POLICE_PATROL"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_REASONABLE			0"
	"		TASK_WAIT						12"
	"		TASK_GET_PATH_TO_RANDOM_NODE	2048" // NOTE: Length should be high enough to help leaving unneeded zones
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_FACE_REASONABLE			0"
	"		TASK_WAIT						6"
	"		TASK_WANDER						1281024"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_FACE_REASONABLE			0"
	"		TASK_WAIT						6"
	"		TASK_WANDER						1281024"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_HL2RP_POLICE_PATROL" // Keep doing it
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
)
AI_END_CUSTOM_NPC()

void CHL2RPNPCPolice::Spawn()
{
	BaseClass::Spawn();
	SetHealth(HL2RP_NPC_POLICE_HEALTH);
	ChangeTeam(TEAM_COMBINE);
	AddIntersectingEnts();
	GiveWeapon("weapon_pistol");
	GiveWeapon("weapon_smg1");
	GiveWeapon("weapon_stunstick"); // Give stunstick last so it results the active weapon
	m_fWeaponDrawn = true;
	GetEnemies()->SetFreeKnowledgeDuration(HL2RP_NPC_POLICE_FREE_KNOWLEDGE_DURATION);
	GetEnemies()->SetEnemyDiscardTime(HL2RP_NPC_POLICE_ENEMY_DISCARD_DELAY);
}

void CHL2RPNPCPolice::UpdateOnRemove()
{
	HL2RPRules()->mWavePolices.Remove(this);
	BaseClass::UpdateOnRemove();
}

#ifdef HL2RP_LEGACY
void CHL2RPNPCPolice::GetHUDInfo(CHL2Roleplayer* pPlayer, CLocalizeFmtStr<>& text)
{
	text.Localize("#HL2RP_Police");
}
#endif // HL2RP_LEGACY

void CHL2RPNPCPolice::GatherConditions()
{
	BaseClass::GatherConditions();

	if (GetEnemy() == NULL)
	{
		ClearCondition(COND_METROPOLICE_PLAYER_TOO_CLOSE); // Fix NPC_STATE_ALERT infinite selection case
	}
	else if (HasCondition(COND_NEW_ENEMY)
		&& !mForgetTimerByAttacker.IsValidIndex(mForgetTimerByAttacker.Find(GetEnemy())))
	{
		SwitchToBaton();
	}
}

int CHL2RPNPCPolice::SelectSchedule()
{
	if (GetState() == NPC_STATE_IDLE)
	{
		return SCHED_HL2RP_POLICE_PATROL;
	}

	int schedule = BaseClass::SelectSchedule();

	// Prevent selecting stunstick activation schedule forever when the sequence is missing
	if (schedule == SCHED_METROPOLICE_ACTIVATE_BATON && !HaveSequenceForActivity((Activity)ACT_ACTIVATE_BATON))
	{
		SetBatonState(false);
	}

	return schedule;
}

void CHL2RPNPCPolice::StartTask(const Task_t* pTask)
{
	BaseClass::StartTask(pTask);

	// If NPC is idle, discard paths to unneeded zones, unless we're already in one (to help leaving it)
	if ((pTask->iTask == TASK_GET_PATH_TO_RANDOM_NODE || pTask->iTask == TASK_WANDER) && TaskIsComplete())
	{
		CBitFlags<> allowedZoneTypes;
		allowedZoneTypes.SetBits(ECityZoneType::AutoCrime, ECityZoneType::NoCrime, ECityZoneType::NoKill);

		for (CCityZone* pZone = NULL; (pZone = gEntList.NextEntByClass(pZone)) != NULL;)
		{
			if (!allowedZoneTypes.IsBitSet(pZone->mType)
				&& pZone->IsPointWithin(GetNavigator()->GetGoalPos()) && !pZone->IsEntityWithin(this))
			{
				return ChainStartTask(TASK_STOP_MOVING);
			}
		}
	}
}

void CHL2RPNPCPolice::TaskFail(AI_TaskFailureCode_t code)
{
	// Ignore few visibility/reachability failures, to advance schedules anyway
	switch (code)
	{
	case FAIL_NO_SHOOT:
	case FAIL_NO_ROUTE:
	case FAIL_NO_REACHABLE_NODE:
	{
		return TaskComplete();
	}
	}

	BaseClass::TaskFail(code);
}

void CHL2RPNPCPolice::RunAI()
{
	BaseClass::RunAI();
	int oldAttackersCount = mForgetTimerByAttacker.Count();

	FOR_EACH_MAP_FAST(mForgetTimerByAttacker, i)
	{
		if (mForgetTimerByAttacker.Key(i) == NULL || mForgetTimerByAttacker[i].Expired())
		{
			RemoveEntityRelationship(mForgetTimerByAttacker.Key(i));
			mForgetTimerByAttacker.RemoveAt(i);
		}
	}

	if (GetEnemy() != NULL)
	{
		// Switch to next best weapon than enemy's
		CBaseCombatCharacter* pCharacter = GetEnemy()->MyCombatCharacterPointer();

		if (mForgetTimerByAttacker.IsValidIndex(mForgetTimerByAttacker.Find(pCharacter)))
		{
			CBaseCombatWeapon* pWeapon = GameRules()->GetNextBestWeapon(this, pCharacter->GetActiveWeapon());

			if (pWeapon != GetActiveWeapon())
			{
				Weapon_Switch(pWeapon);
			}
		}
		else if (mForgetTimerByAttacker.Count() < 1 && oldAttackersCount > 0)
		{
			SwitchToBaton();
		}
	}

	ForEachRoleplayer([&](CHL2Roleplayer* pPlayer)
	{
		if (pPlayer->mCrime > 0 && pPlayer->IsAlive())
		{
			UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
			return AddEntityRelationship(pPlayer, D_HT, DEF_RELATIONSHIP_PRIORITY);
		}

		RemoveEntityRelationship(pPlayer);
	});
}

void CHL2RPNPCPolice::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE type, float value)
{
	if (type >= USE_SPECIAL1)
	{
		CHL2Roleplayer* pPlayer = ToHL2Roleplayer(pActivator);

		if (pPlayer != NULL && pPlayer->mFaction == EFaction::Combine)
		{
			Remove(); // Remove NPC in cases where it blocks the way or for any other reason
		}

		return;
	}

	BaseClass::Use(pActivator, pCaller, type, value);
}

Activity CHL2RPNPCPolice::NPC_TranslateActivity(Activity activity)
{
	int sequence = 0;
	Activity translatedActivity = HL2RPRules()->GetBestTranslatedActivity(this,
		BaseClass::NPC_TranslateActivity(activity), false, sequence);
	return (sequence > 0 ? translatedActivity
		: HL2RPRules()->GetBestTranslatedActivity(this, ACT_IDLE, GetState() != NPC_STATE_IDLE, sequence));
}

bool CHL2RPNPCPolice::Weapon_Switch(CBaseCombatWeapon* pWeapon, int viewModelIndex)
{
	CBaseCombatWeapon* pOldWeapon = GetActiveWeapon();
	bool success = BaseClass::Weapon_Switch(pWeapon, viewModelIndex);

	if (GetActiveWeapon() != pOldWeapon)
	{
		SetBatonState(false); // Allow changing activity to ACT_IDLE faster before deploying stunstick
		OnChangeActiveWeapon(pOldWeapon, pWeapon); // Notify the change, which base logic misses
	}

	return success;
}

void CHL2RPNPCPolice::Event_Killed(const CTakeDamageInfo& info)
{
	RemoveAllWeapons(); // Remove active weapon instantly, unlike via ClearActiveWeapon
	BaseClass::Event_Killed(info);
}

int CHL2RPNPCPolice::OnTakeDamage(const CTakeDamageInfo& info)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(info.GetAttacker());
	CBaseCombatCharacter* pCharacter = pPlayer;

	if (pPlayer == NULL)
	{
		pCharacter = ToBaseCombatCharacter(info.GetAttacker());

		if (pCharacter != NULL && pCharacter->GetEnemy() != this)
		{
			return 0; // Prevent accidental damage from other NPCs
		}
	}
	else if (pPlayer->mFaction == EFaction::Combine)
	{
		return 0;
	}

	int result = BaseClass::OnTakeDamage(info);

	if (result > 0 && pCharacter != NULL)
	{
		CSimpleSimTimer timer;
		timer.Set(HL2RP_NPC_POLICE_ATTACKER_FORGET_DELAY);
		mForgetTimerByAttacker.InsertOrReplace(pCharacter, timer);
		AddEntityRelationship(pCharacter, D_HT, timer.GetNext());
	}

	return result;
}

void CHL2RPNPCPolice::OnStateChange(NPC_STATE oldState, NPC_STATE newState)
{
	BaseClass::OnStateChange(oldState, newState);

	if (newState == NPC_STATE_IDLE)
	{
		SwitchToBaton();
	}
}

void CHL2RPNPCPolice::OnChangeActivity(Activity activity)
{
	BaseClass::OnChangeActivity(activity);

	// Trigger weapon actions that may lack a dependent animation event
	switch (activity)
	{
	case ACT_MELEE_ATTACK1:
	{
		if (GetTask()->iTask == TASK_MELEE_ATTACK1 && !HasAnimEvent(GetSequence(), EVENT_WEAPON_MELEE_HIT))
		{
			animevent_t event;
			event.event = EVENT_WEAPON_MELEE_HIT;
			GetActiveWeapon()->Operator_HandleAnimEvent(&event, this);
			int sequence;
			RestartGesture(HL2RPRules()->GetBestTranslatedActivity(this, ACT_GESTURE_RANGE_ATTACK1, false, sequence));
		}

		break;
	}
	case ACT_RANGE_ATTACK1:
	{
		if (GetTask()->iTask == TASK_RANGE_ATTACK1)
		{
			// Cap weapon activity duration to min. burst interval, otherwise we may loose
			// anim events since the shot regulator logic resets the activity every shot
			float minBurstInterval, maxBurstInterval;
			GetShotRegulator()->GetBurstInterval(&minBurstInterval, &maxBurstInterval);
			GetActiveWeapon()->SetPlaybackRate(Max(GetActiveWeapon()->GetPlaybackRate(),
				GetActiveWeapon()->SequenceDuration() / minBurstInterval));
		}

		break;
	}
	case ACT_RELOAD:
	{
		if (GetTask()->iTask == TASK_RELOAD && !HasAnimEvent(GetSequence(), EVENT_WEAPON_RELOAD_FILL_CLIP))
		{
			animevent_t event;
			event.event = EVENT_WEAPON_RELOAD;
			HandleAnimEvent(&event);
			int sequence;
			RestartGesture(HL2RPRules()->GetBestTranslatedActivity(this, ACT_GESTURE_RELOAD, false, sequence));
		}

		break;
	}
	}
}

void CHL2RPNPCPolice::GiveWeapon(const char* pClassName)
{
	if (!Weapon_OwnsThisType(pClassName))
	{
		CBaseCombatWeapon* pWeapon = Weapon_Create(pClassName);

		if (pWeapon != NULL)
		{
			Weapon_Equip(pWeapon);
			OnGivenWeapon(pWeapon);
		}
	}
}

void CHL2RPNPCPolice::SwitchToBaton()
{
	Weapon_Switch(Weapon_OwnsThisType("weapon_stunstick"));
}
