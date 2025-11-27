#ifndef HL2_ROLEPLAYER_H
#define HL2_ROLEPLAYER_H
#pragma once

#include <hl2_roleplayer_shared.h>
#include <smartptr.h>

#define HL2_ROLEPLAYER_HUD_THINK_PERIOD    1.0f
#define HL2_ROLEPLAYER_HUD_EXTRA_HOLD_TIME 0.1f // Simple way to fix flickering due to networking inaccuracy

#define HL2_ROLEPLAYER_ALERT_HUD_HOLD_TIME 6.0f

#define HL2_ROLEPLAYER_CRIME_PER_HP 10

#define HL2_ROLEPLAYER_NORMAL_BEAMS_WIDTH 2.0f
#define HL2_ROLEPLAYER_SMALL_BEAMS_WIDTH  1.0f

#define CPlayerDatabaseProp(Type, name, enumValue) \
	class CPlayerDatabasePropListener_##name : public CDefaultNetworkVarListener \
	{ \
	public: \
		static void NetworkVarChanged(void *pVar) \
		{ \
			MyBasePointerOf(ThisClass, pVar, name)->ThisClass:: \
				CALL_PLAYER_DATABASE_PROP_CHANGE_FUNC(Type, *(CPlayerDatabasePropBase<Type>*)pVar, enumValue) \
		} \
	}; \
\
	CPlayerDatabasePropBase<Type, CPlayerDatabasePropListener_##name> name;

SCOPED_ENUM(EPlayerHUDType,
	None = -1,
	Main,
	Zone,
	Alert,
	AimingEntity,
	Crime,
	Communication
);

abstract_class INetworkDialog;
class CJobData;

class CRestorablePlayerEquipment : public CPlayerEquipment
{
public:
	CRestorablePlayerEquipment(CHL2Roleplayer*, bool copyWeapons = true);

	void Restore(CHL2Roleplayer*, int altHealth);
};

class CHL2Roleplayer : public CBaseHL2Roleplayer
{
	class CHUDExpireTimer : public CSimpleSimTimer
	{
	public:
		void Set(EPlayerHUDType originalType, float delay);

		EPlayerHUDType mOriginalType; // To allow reusing channel for a new message only among HL2RP HUDs
	};

	DECLARE_CLASS(CHL2Roleplayer, CBaseHL2Roleplayer)
	DECLARE_HL2RP_SERVERCLASS()

	void InitialSpawn() OVERRIDE;
	void Spawn() OVERRIDE;
	CBaseEntity* EntSelectSpawnPoint() OVERRIDE;
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED(m_iMaxHealth);
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED_EX(m_iHealth);
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED_EX(m_ArmorValue);
	IMPLEMENT_NETWORK_ARRAY_FOR_DERIVED_EX(m_hMyWeapons);
	IMPLEMENT_NETWORK_ARRAY_FOR_DERIVED_EX(m_iAmmo);
	void OnWeaponChanged(CBaseCombatWeapon*, bool) OVERRIDE;
	bool IsFakeClient() const OVERRIDE;
	bool IsNetClient() const OVERRIDE;
	IResponseSystem* GetResponseSystem() OVERRIDE;
	void SetPlayerName(const char*) OVERRIDE;
	void SetModel(const char*) OVERRIDE;
	void HandleWalkChanges(CMoveData*) OVERRIDE;
	void UpdateWeaponPosture() OVERRIDE;
	void PlayerUse() OVERRIDE;
	CBaseEntity* FindUseEntity() OVERRIDE;
	void PostThink() OVERRIDE;
	void ModifyOrAppendCriteria(AI_CriteriaSet&) OVERRIDE;
	int OnTakeDamage(const CTakeDamageInfo&) OVERRIDE;
	void Event_Killed(const CTakeDamageInfo&) OVERRIDE;
	void OnChatMessagePassed(CBasePlayer*, bool) OVERRIDE;
	void GetHUDInfo(CHL2Roleplayer*, CLocalizeFmtStr<>&) OVERRIDE HL2RP_LEGACY_FUNCTION;

	void HUDThink();
	void GetMainHUD(CLocalizeFmtStr<>&);
	void SendMainHUD() HL2RP_LEGACY_FUNCTION;
	void SendAimingEntityHUD() HL2RP_LEGACY_FUNCTION;
	bool FixHUDChannel(int&);

	float mSpecialUseLastTime;
	CHUDExpireTimer mHUDExpireTimers[EPlayerHUDType::_Count];

public:
	CHL2Roleplayer();

	bool IsBot() const OVERRIDE;
	void ChangeTeam(int = TEAM_INVALID) OVERRIDE;
	void SetAnimation(PLAYER_ANIM) OVERRIDE;
	void OnDatabasePropChanged(const SUtlField&, EPlayerDatabasePropType) OVERRIDE;

	void PrintWelcomeInfo();
	void OnDisconnect();
	void LoadFromDatabase();
	bool HasFactionAccess(int);
	bool HasJobAccess(CJobData*);
	int CheckCurrentJobAccess(); // Returns current/re-assigned job index
	bool HasModelGroupAccess(int groupIndex, int fallbackType = -1); // Checks access to a specific group or type
	void ChangeFaction(int, string_t newJobName = NULL_STRING);
	void SetModel(int groupIndex, int aliasIndex);
	int DropMoney(int, const Vector& origin); // Returns the actual amount of 'drop-queued' money
	void LocalPrint(int type, const char* pText, const char* pArg = "") HL2RP_LEGACY_FUNCTION;
	void LocalDisplayHUDHint(EPlayerHUDHintType, const char* pToken,
		const char* pArg1 = "", const char* pArg2 = "") HL2RP_LEGACY_FUNCTION;
	void Print(int type, const char* pText, const char* pArg1 = "", const char* pArg2 = "");
	void SendHUDHint(EPlayerHUDHintType, const char* pToken, const char* pArg1 = "", const char* pArg2 = "");
	void OnPreSendHUDMessage(bf_write*);
	void SendHUDMessage(EPlayerHUDType, const char* pMessage, float xPos, float yPos,
		const Color&, float holdTime = HL2_ROLEPLAYER_HUD_THINK_PERIOD + HL2_ROLEPLAYER_HUD_EXTRA_HOLD_TIME);
	void SendBeam(const Vector& end, const Color&, float width = HL2_ROLEPLAYER_NORMAL_BEAMS_WIDTH);
	void SendRootDialog(INetworkDialog*);
	void PushAndSendDialog(INetworkDialog*);
	void RewindCurrentDialog();

	float mFirstSpawnTime;
	CPlayerDatabaseNetworkProp(string_t, mJobName, EPlayerDatabasePropType::Job);
	CPlayerDatabaseProp(string_t, mModelGroup, EPlayerDatabasePropType::ModelGroup);
	CPlayerDatabaseProp(string_t, mModelAlias, EPlayerDatabasePropType::ModelAlias);
	CPlayerDatabaseProp(CBitFlags<>, mLearnedHUDHints, EPlayerDatabasePropType::LearnedHUDHints);
	CPlainAutoPtr<CRestorablePlayerEquipment> mRestorableEquipment;
	SPlayerAimInfo mAimInfo;
	CAutoLessFuncAdapter<CUtlRBTree<CHL2RP_Property*>> mHomes;
	int mLastDialogLevel = INT_MAX;
	CUtlVectorAutoPurge<INetworkDialog*> mDialogStack;
	bool mIsWeaponManuallyLowered;
};

extern const char* gPlayerDatabasePropNames[];

#endif // !HL2_ROLEPLAYER_H
