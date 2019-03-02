#ifndef HL2RP_PLAYER_H
#define HL2RP_PLAYER_H
#pragma once

#include <hl2mp_player.h>
#include <HL2RP_Player_shared.h>
#include <utlflags.h>
#include <UtlSortVector.h>

// Engine limits
#define MAX_NETMESSAGE	6
#define	MAX_TEMP_ENTITIES_PER_UPDATE	32
#define	MAX_DISPLAYABLE_USER_MSG_DATA	221 // Max. experienced length among MAX_USER_MSG_DATA (255)

#define HL2RP_PLAYER_CRIME_PER_DAMAGE_DEALEN	10

#define HL2RP_PLAYER_DEADLOAD_AMMODATA_KEYNAME	"AmmoData"
#define HL2RP_PLAYER_DEADLOAD_WEAPONDATA_KEYNAME	"WeaponData"

class CPlayerDialog;

// Informs of a delayed gesture that got covered by a complete activity instead.
// Gesture should apply when another complete activity is requested, since we can't wait for current one to complete.
struct SHiddenPlayerGesture
{
	Activity m_Activity;
	float m_flStartTime;
};

// Line that belongs to a higher group of lines, for a single HUD channel
class CHUDChannelLine
{
public:
	enum EType // HUD lines get sorted by reverse enumeration order
	{
		None, // Only multiple lines of this type can coexist in the HUD
		StickyWalkHint
	};

	CHUDChannelLine(EType type, const char* pText);

	bool operator==(const CHUDChannelLine& lineInfo) const;

	EType m_Type;
	char m_Text[MAX_DISPLAYABLE_USER_MSG_DATA];
};

class CHUDChannelLineLess
{
public:
	bool Less(CHUDChannelLine const& lineInfo1, CHUDChannelLine const& lineInfo2, void* pCtx);
};

class CHL2RP_Player : public CHL2MP_Player
{
	DECLARE_CLASS(CHL2RP_Player, CHL2MP_Player);

#ifndef HL2DM_RP
	DECLARE_SERVERCLASS();
#endif // !HL2DM_RP

	~CHL2RP_Player();

	void Precache() OVERRIDE;
	void InitialSpawn() OVERRIDE;
	void Spawn() OVERRIDE;
	void SharedSpawn() OVERRIDE;
	void ChangeTeam(int team) OVERRIDE;
	CBaseEntity* EntSelectSpawnPoint() OVERRIDE;
	void PlayerRunCommand(CUserCmd* pUcmd, IMoveHelper* pMoveHelper) OVERRIDE;
	void PostThink() OVERRIDE;
	bool ShouldCollide(int collisionGroup, int contentsMask) const OVERRIDE;
	void SetAnimation(PLAYER_ANIM playerAnim) OVERRIDE;
	void PackDeadPlayerItems() OVERRIDE;
	void Event_Killed(const CTakeDamageInfo& info) OVERRIDE;
	void SetWeapon(int i, CBaseCombatWeapon* pWeapon) OVERRIDE;
	void OnSetWeaponClip1(CBaseCombatWeapon* pWeapon) OVERRIDE;
	void OnSetWeaponClip2(CBaseCombatWeapon* pWeapon) OVERRIDE;
	void SetHealth(int health) OVERRIDE;
	void SetAmmoCount(int count, int ammoIndex) OVERRIDE;
	void EndHandleSpeedChanges(int buttonsChanged) OVERRIDE;
	void UpdateWeaponPosture() OVERRIDE;
	void OnTalkConditionsPassed(CBasePlayer* pListener, bool isTeamOnly) OVERRIDE;

	void DisplayDialog(CPlayerDialog* pDialog);
	void DisplayWelcomeHUD();
	void DisplayStickyWalkingHint();
	void OnEnterStickyWalking();
	void PlayTimeContextThink();
	void SetIdealActivity(Activity idealActivity, int sequence);
	void TrySetIdealActivityOrGesture(Activity standActivity, Activity crouchedActivity,
		Activity standGesture, Activity crouchedGesture, float duration = 0.0f);
	void TryUpsertWeapon(CBaseCombatWeapon* pWeapon);
	CBaseEntity* FindRandomNearbySpawnPoint(const char* pSpawnPointClassName);
	void CenterHUDLinesClearContextThink();

	float m_flNextOpenInventoryTime; // For interactive main player menu display
	float m_flNextTempEntityDisplayTime[MAX_TEMP_ENTITIES_PER_UPDATE];
	SHiddenPlayerGesture m_HiddenGesture;
	float m_flLockedPlaybackRate; // When it's positive, we should maintain the same playback rate
	CUtlSortVector<CHUDChannelLine, CHUDChannelLineLess> m_SharedHUDAlertList;
	CHL2RP_PlayerSharedState m_SharedState;
	int m_iRemainingStickyWalkHints;

public:
	enum EAccessFlag
	{
		AccessFlag_None,
		Admin = (1 << 0)
	};

	// Control what syncing operations are allowed at certain moments.
	// Player data consistency is critical.
	enum EDALSyncCap
	{
		LoadData = (1 << 0),
		SaveData = (1 << 1)
	};

	CHL2RP_Player();

	void SetModel(const char* pModelPath) OVERRIDE;
	void SetArmorValue(int armorValue) OVERRIDE;

	bool IsAdmin() const;
	const char* GetNetworkIDString();
	void TrySyncMainData();
	void SetCrime(int crime);
	void SetAccessFlags(int accessFlags);
	void AddAlertHUDLine(const char* pLine, float duration,
		CHUDChannelLine::EType type = CHUDChannelLine::None);
	void SetupSoundsByJobTeam();
	void TryGiveLoadedHpAndArmor(int health, int armor);
	void TryGiveLoadedWeapon(const char* pWeaponClassName, int clip1, int clip2);
	template<class T = CPlayerDialog, typename... U> void DisplayDialog(U... args);

	// Queries and caches network ID string from engine. Then, attempts to load main data.
	// Output: Whether pNetworkIdString matches this player's cached network ID string
	bool LateLoadMainData(const char* pNetworkIdString);

	// Attempts to set main attack activity, or otherwise its corresponding layered one (gesture).
	// Input: duration - Forced duration to apply on the successful sequences. 0.0f to don't alter it.
	void SetAttack1Animation(Activity standActivity = ACT_RANGE_ATTACK1, float duration = 0.0f);

	// Database fields
	int m_iSeconds, m_iCrime;
	CUtlFlags<int> m_AccessFlags;

	// Non persistent public properties
	int m_JobTeamIndex;
	float m_flNextHUDChannelTime[MAX_NETMESSAGE];
	CPlayerDialog* m_pCurrentDialog;
	int m_iLastDialogLevel;
	CUtlFlags<int> m_DALSyncFlags;

	// To be filled when the results of equipment data, loaded from DB, are handled while dead.
	// The real equipment should be given on next spawn, from this KV.
	KeyValues* m_pDeadLoadedEquipmentInfo;
};

// Downcasts shorter from CBaseEntity to this class (unsafe version)
FORCEINLINE CHL2RP_Player* ToHL2RPPlayer_Fast(CBaseEntity* pEntity)
{
	return static_cast<CHL2RP_Player*>(pEntity);
}

FORCEINLINE bool CHL2RP_Player::IsAdmin() const
{
	return (m_AccessFlags.IsFlagSet(EAccessFlag::Admin));
}

template<class T, typename... U>
FORCEINLINE void CHL2RP_Player::DisplayDialog(U... args)
{
	CPlayerDialog* pDialog = new T(this, args...);
	DisplayDialog(pDialog);
}

// Downcasts shorter from CBaseEntity to this class (safe version)
CHL2RP_Player* ToHL2RP_Player(CBaseEntity* pEntity);

#endif // !HL2RP_PLAYER_H
