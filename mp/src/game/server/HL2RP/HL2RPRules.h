#ifndef HL2RP_RULES_H
#define HL2RP_RULES_H
#pragma once

#include <activitylist.h>
#include "AutoLocalizer.h"
#include <filesystem.h>
#include <hl2mp_gamerules.h>
#include <DAL/HL2RPDAL.h>
#include "HL2RP_Player.h"
#include <utlhashtable.h>

// Max. regional communication/HUD radius between two players
#define HL2RP_RULES_REGION_PLAYER_INTER_RADIUS_SQR	300.0f * 300.0f

class CAI_BaseNPC;
class CHL2RPPropertyDoor;
class IGameEvent;

enum EJobTeamIndex
{
	Invalid = -1,
	Citizen,
	Police,
	JobTeamIndex_Max,
};

struct SJob
{
	SJob();
	SJob(const char* pModelPath, bool isReadOnly = false);

	char m_ModelPath[MAX_PATH];
	bool m_IsReadOnly;
};

class CHL2RPRules : public CHL2MPRules
{
	DECLARE_CLASS(CHL2RPRules, CHL2MPRules);

	bool Init() OVERRIDE;
	void LevelInitPostEntity() OVERRIDE;
	void LevelShutdown() OVERRIDE;
	void Think() OVERRIDE;
	void DeathNotice(CBasePlayer* pVictim, const CTakeDamageInfo& info) OVERRIDE;
	void ClientSettingsChanged(CBasePlayer* pPlayer) OVERRIDE;
	void ClientDisconnected(edict_t* pEdict) OVERRIDE;
	const char* GetChatFormat(bool isTeamOnly, CBasePlayer* pPlayer) OVERRIDE;
	bool PlayerCanHearChat(CBasePlayer* pListener, CBasePlayer* pSpeaker) OVERRIDE;

	void SharedConstructor();
	int AddDownloadables(FileFindHandle_t handle, const char* pFileName, char relativePath[MAX_PATH],
		INetworkStringTable* pDownloadables);
	IGameEvent* BuildPlayerDeathEvent(int victimId, int killerId, const char* pKillerWeaponName);
	void HandleNPCActRemapGesture(CAI_BaseNPC* pNPC, CActivityRemap& actRemap);
	int GetActRemapSequence(CUtlVector<CActivityRemap>& actRemapList, CBaseCombatCharacter* pCharacter,
		Activity initialActivity, bool isRequired, int& actRemapIndexOut);
	void NotifyPlayerVsNPCDeath(CBaseEntity* pAttacker, CBaseEntity* pVictim, const char* pKillerWeaponName);

	CUtlVector<CActivityRemap> m_PlayerActRemap;
	CUtlVector<CActivityRemap> m_NPCsActRemap;

public:
	CHL2RPRules();

	void NetworkIdValidated(const char* pNetworkIdString);
	int GetJobTeamIndexForModel(const char* pModelPath);
	void DeathNotice(CBaseCombatCharacter* pVictim, const CTakeDamageInfo& info);
	int GetPlayerActRemapSequence(CBasePlayer* pPlayer, Activity initialActivity);
	Activity GetNPCActRemapActivity(CAI_BaseNPC* pNPC, Activity initialActivity);

	CHL2RPDAL m_DAL;
	CAutoLocalizer m_Localizer;
	CUtlDict<SJob> m_JobTeamsModels[JobTeamIndex_Max];
	int m_iBeamModelIndex;

	// Maps handle integer representations to property doors
	CUtlHashtable<unsigned long, CHandle<CHL2RPPropertyDoor>> m_PropertyDoorTable;
};

FORCEINLINE CHL2RPRules* HL2RPRules()
{
	return static_cast<CHL2RPRules*>(GameRules());
}

FORCEINLINE CHL2RPDAL& GetHL2RPDAL()
{
	return HL2RPRules()->m_DAL;
}

FORCEINLINE CAutoLocalizer& GetHL2RPAutoLocalizer()
{
	return HL2RPRules()->m_Localizer;
}

CHL2RPPropertyDoor* GetHL2RPPropertyDoor(CBaseEntity* pEntity);

#endif // !HL2RP_RULES_H
