#ifndef HL2RP_GAMERULES_H
#define HL2RP_GAMERULES_H
#pragma once

#include "hl2rp_util.h"
#include <hl2rp_gamerules_shared.h>
#include <hl2rp_shareddefs.h>
#include "sourcehooks.h"
#include <bitflags.h>
#include <fmtstr.h>
#include <GameEventListener.h>

using FileFindHandle_t = int;

class CJobData : public CPlayerEquipment
{
	void AddModelGroup(const char*);

public:
	CJobData(KeyValues* pJobKV, int faction);

	bool mIsGod;
	CAutoLessFuncAdapter<CUtlRBTree<int>> mModelGroupIndices;
	CBitFlags<> mRequiredAccessFlag;
};

class CHL2RPRules : public CBaseHL2RPRules, CGameEventListener
{
	class CPlayerModelDict : public CUtlDict<const char*> // Model path by alias
	{
		void AddExactModel(const char* pAlias, const char* pPath, INetworkStringTable* pModelPrecache);

	public:
		void AddModel(KeyValues* pModelKV, INetworkStringTable* pModelPrecache);

		EPlayerModelType mType;
	};

	class CPlayerModelsMap : public CAutoPurgeAdapter<CUtlMap<const char*, CPlayerModelDict*>>
	{
	public:
		CUtlVector<int> mGroupIndices[EPlayerModelType::_Count];
	};

	class CActivityList : public CUtlVector<Activity>
	{
	public:
		void AddActivity(KeyValues* pActivityKV);
	};

	DECLARE_CLASS(CHL2RPRules, CBaseHL2RPRules)

	void LevelInitPostEntity() OVERRIDE;
	void InitDefaultAIRelationships() OVERRIDE;
	const char* GetChatFormat(bool, CBasePlayer*) OVERRIDE;
	bool PlayerCanHearChat(CBasePlayer*, CBasePlayer*) OVERRIDE;
	bool CanPlayerHearVoice(CBasePlayer*, CBasePlayer*, bool) OVERRIDE;
	void Think() OVERRIDE;
	void FireGameEvent(IGameEvent*) OVERRIDE;
	void ClientDisconnected(edict_t*) OVERRIDE;
	void ClientCommandKeyValues(edict_t*, KeyValues*) OVERRIDE;

	void RegisterDownloadableFiles(CFmtStrN<MAX_PATH>&, FileFindHandle_t, INetworkStringTable* pDownloadables);
	Activity TranslateActivity(CBaseCombatCharacter*, Activity,
		Activity& fallbackActivity, bool weaponActStrict, int& sequence);

	CUtlRBTree<const char*> mExcludedUploadExts;
	CAutoPurgeAdapter<CAutoLessFuncAdapter<CUtlMap<Activity, CActivityList*>>> mActivityFallbacksMap;
	CSimpleSimTimer mPoliceWaveTimer;

public:
	CHL2RPRules();

	Activity GetBestTranslatedActivity(CBaseCombatCharacter*, Activity, bool weaponActStrict, int& sequence);

	CHUDMsgInterceptor mHUDMsgInterceptor;
	IResponseSystem* mpPlayerResponseSystem;
	CAutoPurgeAdapter<CUtlMap<const char*, CJobData*>> mJobByName[EFaction::_Count];
	CPlayerModelsMap mPlayerModelsByGroup;
	CAutoLessFuncAdapter<CUtlRBTree<EHANDLE>> mWavePolices;
};

CHL2RPRules* HL2RPRules();

#endif // !HL2RP_GAMERULES_H
