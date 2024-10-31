#ifndef HL2RP_NPC_POLICE_H
#pragma once

#include <hl2rp_character.h>
#include <hl2rp_shareddefs.h>
#include <npc_metropolice.h>

class CHL2RPNPCPolice : public CHL2RPCharacter<CNPC_MetroPolice>
{
	DECLARE_CLASS(CHL2RPNPCPolice, CHL2RPCharacter)
	DECLARE_HL2RP_SERVERCLASS()
	DEFINE_CUSTOM_AI;

	enum
	{
		SCHED_HL2RP_POLICE_PATROL = BaseClass::NEXT_SCHEDULE
	};

	void Spawn() OVERRIDE;
	void UpdateOnRemove() OVERRIDE;
	void GetHUDInfo(CHL2Roleplayer*, CLocalizeFmtStr<>&) OVERRIDE HL2RP_LEGACY_FUNCTION;
	void GatherConditions() OVERRIDE;
	int SelectSchedule() OVERRIDE;
	void StartTask(const Task_t*) OVERRIDE;
	void TaskFail(AI_TaskFailureCode_t) OVERRIDE;
	void RunAI() OVERRIDE;
	void Use(CBaseEntity*, CBaseEntity*, USE_TYPE, float) OVERRIDE;
	Activity NPC_TranslateActivity(Activity) OVERRIDE;
	bool Weapon_Switch(CBaseCombatWeapon*, int = 0) OVERRIDE;
	void Event_Killed(const CTakeDamageInfo&) OVERRIDE;
	int OnTakeDamage(const CTakeDamageInfo&) OVERRIDE;
	void OnStateChange(NPC_STATE, NPC_STATE) OVERRIDE;
	void OnChangeActivity(Activity) OVERRIDE;

	void GiveWeapon(const char*);
	void SwitchToBaton();

	CAutoLessFuncAdapter<CUtlMap<EHANDLE, CSimpleSimTimer>> mForgetTimerByAttacker; // NOTE: For CBaseCombatCharacter
};

#endif // !HL2RP_NPC_POLICE_H
