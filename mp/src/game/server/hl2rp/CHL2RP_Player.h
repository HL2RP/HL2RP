#ifndef HL2RP_PLAYER_H
#define HL2RP_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "hl2mp_player.h"
#include "CHL2RP_SQL.h"

const int MAX_VALVE_NETMESSAGE = 6; // Valve engine (HUD's etc.)
const int MAX_VALVE_TEMP_ENTITIES_PER_UPDATE = 32;

const int MAX_VALVE_USER_MSG_TEXT_DATA = 221; // Max remaining text length observed for the total MAX_USER_MSG_DATA (255)

class CHL2RP_PlayerDialog;

class CHL2RP_Player : public CHL2MP_Player
{
	DECLARE_CLASS(CHL2RP_Player, CHL2MP_Player)

public:
	enum AccessFlag
	{
		ACCESS_FLAG_NONE = 0,
		ACCESS_FLAG_ADMIN = 1
	};

	// Control what syncing operations are allowed at certain moments.
	// Player data consistency is important
	enum SyncCapability
	{
		SYNC_CAPABILITY_NONE = 0,
		SYNC_CAPABILITY_UPSERT_MAIN_DATA = (1 << 0),
		SYNC_CAPABILITY_ALTER_AMMO = (1 << 1),
		SYNC_CAPABILITY_LOAD_WEAPONS = (1 << 2),
		SYNC_CAPABILITY_ALTER_WEAPONS = (1 << 3)
	};

	CHL2RP_Player();

	static ThisClass *ToThisClass(const CBaseEntity *pEntity);
	static ThisClass *ToThisClassFast(const CBaseEntity *pEntity);
	static void TrySyncWeaponFromBaseCombatCharacter(CBaseCombatWeapon &weapon, const CBaseCombatCharacter *pCharacter);
	bool IsAdmin() const;
	void TrySyncMainData();

	// Database fields:
	int m_iSeconds, m_iCrime;
	AccessFlag m_AccessFlag;

	// Non persistent public properties:
	float m_flNextHUDChannelTime[MAX_VALVE_NETMESSAGE];
	CHL2RP_PlayerDialog *m_pCurrentDialog;
	int m_iLastDialogLevel, m_iSyncCapabilities;

private:
	~CHL2RP_Player();

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED_EX(int, m_iHealth);
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED_EX(int, m_ArmorValue);
	IMPLEMENT_NETWORK_ARRAY_FOR_DERIVED_EX(CBaseCombatWeaponHandle, m_hMyWeapons);
	IMPLEMENT_NETWORK_ARRAY_FOR_DERIVED_EX(int, m_iAmmo);

	void Spawn() OVERRIDE;
	void Precache() OVERRIDE;
	void PreThink() OVERRIDE;
	void Event_Killed(const CTakeDamageInfo &info) OVERRIDE;
	void PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper) OVERRIDE;
	void InitialSpawn() OVERRIDE;
	void DisplayWelcomeHUD();
	template<class Handler>
	void CheckNetworkVarExChanged(Handler &networkVarBaseEx) const;
	template<class Handler>
	void CheckNetworkArrayExChanged(Handler &networkArrayBaseEx) const;

	float m_flNextOpenInventoryTime; // For interactive main player menu display
	float m_flNextTempEntityDisplayTime[MAX_VALVE_TEMP_ENTITIES_PER_UPDATE];
};

// Purpose: Downcast shorter from CBaseEntity to this class (safe version)
FORCEINLINE CHL2RP_Player *CHL2RP_Player::ToThisClass(const CBaseEntity *pEntity)
{
	return (((pEntity != NULL) && pEntity->IsPlayer()) ? ToThisClassFast(pEntity) : NULL);
}

// Purpose: Downcast shorter from CBaseEntity to this class (unsafe version)
FORCEINLINE CHL2RP_Player *CHL2RP_Player::ToThisClassFast(const CBaseEntity *pEntity)
{
	return static_cast<ThisClass *>((CBaseEntity *)pEntity);
}

FORCEINLINE bool CHL2RP_Player::IsAdmin() const
{
	return (m_AccessFlag == ACCESS_FLAG_ADMIN);
}

// Purpose: Share SteamID identification among player transactions
abstract_class CPlayerTxn : public CAsyncSQLTxn
{
	DECLARE_CLASS(CPlayerTxn, CAsyncSQLTxn)

protected:
	CPlayerTxn() { ; }
	CPlayerTxn(CHL2RP_Player &player);

	void ConstructPlayerTxn(CHL2RP_Player &player);
	bool IsValidPlayerTxnAndSteamIDEquals(const ThisClass *pTxn) const;

private:
	void BindParametersGeneric(int iQueryId, const void *pStmt) const OVERRIDE;

protected:
	char m_szPlayerSteamID[MAX_NETWORKID_LENGTH];
};

class CSetUpPlayersTxn : public CAsyncSQLTxn
{
public:
	CSetUpPlayersTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
};

class CLoadPlayerTxn : public CPlayerTxn
{
public:
	CLoadPlayerTxn(CHL2RP_Player &player);

private:
	void HandleQueryResults() const OVERRIDE;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;
};

class CUpsertPlayerTxn : public CPlayerTxn
{
public:
	CUpsertPlayerTxn(CHL2RP_Player &player);

private:
	void BuildSQLiteQueries() OVERRIDE;
	void BuildMySQLQueries() OVERRIDE;
	void BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const OVERRIDE;
	void BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const OVERRIDE;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;

	char m_szPlayerName[MAX_PLAYER_NAME_LENGTH];
	int m_iPlayerSeconds, m_iPlayerCrime, m_iPlayerAccessFlag, m_iPlayerHealth, m_iPlayerArmor;
};

// Purpose: Share classname identification among player weapon transactions
abstract_class CPlayerWeaponTxn : public CPlayerTxn
{
	DECLARE_CLASS(CPlayerWeaponTxn, CPlayerTxn)

protected:
	CPlayerWeaponTxn(CBaseCombatWeapon &weapon);

	void BindIdentificators(const void *pStmt, int iStartParamIndex) const;

private:
	void BindParametersGeneric(int iQueryId, const void *pStmt) const OVERRIDE;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;

	CUtlString m_sPlayerWeaponClassname;
};

class CSetUpPlayersWeaponsTxn : public CAsyncSQLTxn
{
public:
	CSetUpPlayersWeaponsTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
};

class CLoadPlayerWeaponsTxn : public CPlayerTxn
{
public:
	CLoadPlayerWeaponsTxn(CHL2RP_Player &player);

private:
	void HandleQueryResults() const OVERRIDE;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;
};

class CUpsertPlayerWeaponTxn : public CPlayerWeaponTxn
{
public:
	CUpsertPlayerWeaponTxn(CHL2RP_Player &player, CBaseCombatWeapon &weapon);

private:
	void BuildSQLiteQueries() OVERRIDE;
	void BuildMySQLQueries() OVERRIDE;
	void BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const OVERRIDE;
	void BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const OVERRIDE;

	int m_iPlayerWeaponClip1, m_iPlayerWeaponClip2;
};

class CDeletePlayerWeaponTxn : public CPlayerWeaponTxn
{
public:
	CDeletePlayerWeaponTxn(CHL2RP_Player &player, CBaseCombatWeapon &weapon);
};

// Purpose: Share slot identification among player ammo transactions
abstract_class CPlayerAmmoTxn : public CPlayerTxn
{
	DECLARE_CLASS(CPlayerAmmoTxn, CPlayerTxn)

protected:
	CPlayerAmmoTxn(int iPlayerAmmoIndex);

	void BindIdentificators(const void *pStmt, int iStartParamIndex) const;

private:
	void BindParametersGeneric(int iQueryId, const void *pStmt) const OVERRIDE;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;

	int m_iPlayerAmmoIndex;
};

class CSetUpPlayersAmmoTxn : public CAsyncSQLTxn
{
public:
	CSetUpPlayersAmmoTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
};

class CLoadPlayerAmmoTxn : public CPlayerTxn
{
	DECLARE_CLASS(CLoadPlayerAmmoTxn, CPlayerTxn)

public:
	CLoadPlayerAmmoTxn(CHL2RP_Player &player);

private:
	void HandleQueryResults() const OVERRIDE;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;
};

class CUpsertPlayerAmmoTxn : public CPlayerAmmoTxn
{
public:
	CUpsertPlayerAmmoTxn(CHL2RP_Player &player, int iPlayerAmmoIndex);

private:
	void BuildSQLiteQueries() OVERRIDE;
	void BuildMySQLQueries() OVERRIDE;
	void BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const OVERRIDE;
	void BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const OVERRIDE;

	int m_iPlayerAmmoCount;
};

class CDeletePlayerAmmoTxn : public CPlayerAmmoTxn
{
public:
	CDeletePlayerAmmoTxn(CHL2RP_Player &player, int iPlayerAmmoIndex);
};

#endif // HL2RP_PLAYER_H
