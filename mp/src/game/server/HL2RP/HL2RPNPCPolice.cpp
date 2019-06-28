#include <cbase.h>
#include <npc_metropolice.h>
#include "HL2RPAINavigator.h"
#include "HL2RPRules.h"
#include "HL2RP_Player.h"
#include "HL2RP_Util.h"
#include <npcevent.h>

#define HL2RP_NPC_POLICE_INITIAL_HEALTH	300
#define HL2RP_NPC_POLICE_NAME	"City Police Officer"

extern int ACT_METROPOLICE_DRAW_PISTOL, AE_METROPOLICE_DRAW_PISTOL;

class CHL2RPNPCPolice : public CNPC_MetroPolice
{
	DECLARE_CLASS(CHL2RPNPCPolice, CNPC_MetroPolice);
	DEFINE_CUSTOM_AI;

	enum
	{
		// Allow for proper decisions, only when NPC should really fight the enemy
		COND_HL2RP_POLICE_SHOULD_FIGHT_ENEMY = BaseClass::NEXT_CONDITION
	};

	enum
	{
		SCHED_HL2RP_POLICE_WANDER = BaseClass::NEXT_SCHEDULE
	};

	void Precache() OVERRIDE;
	void Spawn() OVERRIDE;
	CAI_Navigator* CreateNavigator() OVERRIDE;
	Activity NPC_TranslateActivity(Activity baseAct) OVERRIDE;
	void Think() OVERRIDE;
	void OnChangeActivity(Activity newActivity) OVERRIDE;
	int OnTakeDamage(const CTakeDamageInfo& info) OVERRIDE;
	void Event_Killed(const CTakeDamageInfo& info) OVERRIDE;
	void GatherConditions() OVERRIDE;
	int SelectSchedule() OVERRIDE;
};

class CNPC_HL2RPPoliceCriminalsFunctor : private IPlayerFunctor
{
public:
	CNPC_HL2RPPoliceCriminalsFunctor(CHL2RPNPCPolice* pPolice);

	bool operator() (CBasePlayer* pPlayer) OVERRIDE;

private:
	CHL2RPNPCPolice* m_pPolice;
};

CNPC_HL2RPPoliceCriminalsFunctor::CNPC_HL2RPPoliceCriminalsFunctor(CHL2RPNPCPolice* pPolice)
	: m_pPolice(pPolice)
{
	Assert(m_pPolice != NULL);
}

bool CNPC_HL2RPPoliceCriminalsFunctor::operator() (CBasePlayer* pPlayer)
{
	CHL2RP_Player* pHL2RP_Player = ToHL2RPPlayer_Fast(pPlayer);

	if (pHL2RP_Player->m_iCrime > 0)
	{
		// Prioritize enemy by crime points
		m_pPolice->AddEntityRelationship(pPlayer, D_HT, pHL2RP_Player->m_iCrime);
	}
	else
	{
		m_pPolice->RemoveEntityRelationship(pPlayer);
	}

	return true;
}

LINK_ENTITY_TO_CLASS(hl2rp_npc_police, CHL2RPNPCPolice);
PRECACHE_REGISTER(hl2rp_npc_police);

void CHL2RPNPCPolice::Precache()
{
	BaseClass::Precache();

	// Register remappable activities before we load our custom activity remap file at HL2RPRules
	RegisterPrivateActivity("ACT_ACTIVATE_BATON");
}

void CHL2RPNPCPolice::Spawn()
{
	BaseClass::Spawn();
	SetName(MAKE_STRING(HL2RP_NPC_POLICE_NAME));
	SetMaxHealth(HL2RP_NPC_POLICE_INITIAL_HEALTH);
	SetHealth(HL2RP_NPC_POLICE_INITIAL_HEALTH);
}

CAI_Navigator* CHL2RPNPCPolice::CreateNavigator()
{
	return new CHL2RPAINavigator(this);
}

Activity CHL2RPNPCPolice::NPC_TranslateActivity(Activity baseAct)
{
	return HL2RPRules()->GetNPCActRemapActivity(this, baseAct);
}

void CHL2RPNPCPolice::OnChangeActivity(Activity newActivity)
{
	BaseClass::OnChangeActivity(newActivity);

	// Handle weapon actions that miss an animation event, and thus can't get triggered by it
	switch (newActivity)
	{
	case ACT_RANGE_ATTACK1:
	{
		Assert(m_hActiveWeapon != NULL);
		m_hActiveWeapon->Operator_ForceNPCFire(this, false);
		break;
	}
	case ACT_MELEE_ATTACK1:
	{
		if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_MELEE_HIT))
		{
			Assert(m_hActiveWeapon != NULL);
			m_hActiveWeapon->Operator_ForceNPCFire(this, false);
		}

		break;
	}
	case ACT_RELOAD:
	{
		if (!HasAnimEvent(GetSequence(), EVENT_WEAPON_RELOAD))
		{
			animevent_t animEvent;
			animEvent.event = EVENT_WEAPON_RELOAD;
			HandleAnimEvent(&animEvent);
		}

		break;
	}
	default:
	{
		if (newActivity == ACT_METROPOLICE_DRAW_PISTOL
			&& !HasAnimEvent(GetSequence(), AE_METROPOLICE_DRAW_PISTOL))
		{
			animevent_t animEvent;
			animEvent.event = AE_METROPOLICE_DRAW_PISTOL;
			HandleAnimEvent(&animEvent);
		}
	}
	}
}

int CHL2RPNPCPolice::OnTakeDamage(const CTakeDamageInfo& info)
{
	CHL2RP_Player* pPlayer = ToHL2RP_Player(info.GetAttacker());

	if (pPlayer != NULL)
	{
		pPlayer->SetCrime(UTIL_EnsureAddition(pPlayer->m_iCrime,
			info.GetDamage() * HL2RP_PLAYER_CRIME_PER_DAMAGE_DEALEN));
		pPlayer->TrySyncMainData(CHL2RP_Player::EDALMainPropSelection::Crime);
	}

	return BaseClass::OnTakeDamage(info);
}

void CHL2RPNPCPolice::Think()
{
	BaseClass::Think();
	CNPC_HL2RPPoliceCriminalsFunctor functor(this);
	ForEachPlayer(functor);
}

void CHL2RPNPCPolice::Event_Killed(const CTakeDamageInfo& info)
{
	BaseClass::Event_Killed(info);
	HL2RPRules()->DeathNotice(this, info);
}

int CHL2RPNPCPolice::SelectSchedule()
{
	switch (GetState())
	{
	case NPC_STATE_IDLE:
	{
		return SCHED_HL2RP_POLICE_WANDER;
	}
	}

	return BaseClass::SelectSchedule();
}

void CHL2RPNPCPolice::GatherConditions()
{
	BaseClass::GatherConditions();

	if (HasCondition(COND_SEE_ENEMY) && !HasCondition(COND_ENEMY_DEAD))
	{
		SetCondition(COND_HL2RP_POLICE_SHOULD_FIGHT_ENEMY);
	}
}

AI_BEGIN_CUSTOM_NPC(hl2rp_npc_police, CHL2RPNPCPolice)
DECLARE_CONDITION(COND_HL2RP_POLICE_SHOULD_FIGHT_ENEMY);
DEFINE_SCHEDULE
(
	SCHED_HL2RP_POLICE_WANDER,
	"	Tasks"
	"	TASK_WANDER 5001000" // MIN_DIST * 10000 + MAX_DIST
	"	TASK_WALK_PATH 0"
	"	TASK_WAIT 5"
	"	Interrupts"
	"	COND_HL2RP_SHOULD_FIGHT_ENEMY"
);
AI_END_CUSTOM_NPC();
