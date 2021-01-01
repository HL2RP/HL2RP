#ifndef HL2RP_GAMERULES_H
#define HL2RP_GAMERULES_H
#pragma once

#include "sourcehooks.h"
#include <hl2rp_gamerules_shared.h>
#include <hl2rp_shareddefs.h>
#include <bitflags.h>
#include <fmtstr.h>
#include <GameEventListener.h>

SCOPED_ENUM(EHL2RPDatabaseIOFlag,
	ArePropertiesLoaded
)

class CHL2RP_Property;

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

	class CPlayerModelsMap : public CAutoDeleteAdapter<CUtlMap<const char*, CPlayerModelDict*>>
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

	~CHL2RPRules();

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
	CAutoLessFuncAdapter<CAutoDeleteAdapter<CUtlMap<Activity, CActivityList*>>> mActivityFallbacksMap;
	CSimpleSimTimer mPoliceWaveTimer;

public:
	CHL2RPRules();

	const char* GetIdealMapAlias(); // Returns the unique map group, if possible, current map otherwise
	Activity GetBestTranslatedActivity(CBaseCombatCharacter*, Activity, bool weaponActStrict, int& sequence);
	void AddPlayerName(uint64, const char*);
	void SendPlayerName(int index, CRecipientFilter && = CBroadcastRecipientFilter()) HL2RP_FULL_FUNCTION;

	CHUDMsgInterceptor mHUDMsgInterceptor;
	IResponseSystem* mpPlayerResponseSystem;
	CBitFlags<> mDatabaseIOFlags;
	CDefaultGetAdapter<CUtlRBTree<const char*>> mMapGroups;
	CAutoLessFuncAdapter<CUtlRBTree<CHL2RP_Property*>> mProperties;
	CAutoDeleteAdapter<CUtlMap<const char*, CJobData*>> mJobByName[EFaction::_Count];
	CPlayerModelsMap mPlayerModelsByGroup;
	CAutoLessFuncAdapter<CUtlRBTree<EHANDLE>> mWavePolices;
};

#endif // !HL2RP_GAMERULES_H
