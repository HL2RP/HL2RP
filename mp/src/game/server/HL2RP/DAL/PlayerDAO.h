#ifndef PLAYERDAO_H
#define PLAYERDAO_H
#pragma once

#include "IDAO.h"

class CHL2RP_Player;

class CPlayersInitDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO);
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
};

// Share DTO among player DAOs
class CPlayerDAO : public IDAO
{
protected:
	CPlayerDAO(CHL2RP_Player* pPlayer);

	bool ShouldBeReplacedBy(CPlayerDAO* pDAO);
	void BuildKeyValuesPath(const char* pDirName, char* pDest, int destSize);
	void BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int networkIdStringPos);

	uint64 m_LongSteamIdNum;
	CHandle<CHL2RP_Player> m_hPlayer;
	char m_NetworkIdString[MAX_NETWORKID_LENGTH];
};

class CPlayerLoadDAO : public CPlayerDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
	void HandleKeyValuesResults(KeyValues* pResults) OVERRIDE;
	void HandleSQLResults(KeyValues* pResults) OVERRIDE;

	bool PreHandleResults();
	void HandleMainData_NewPlayer();
	void HandleMainData_OldPlayer(KeyValues* pMainData);
	void HandleAmmoData(int ammoIndex, int ammoCount);
	void HandleWeaponData(KeyValues* pRow, const char* pWeaponClassname);
	void PostHandleResults();

public:
	CPlayerLoadDAO(CHL2RP_Player* pPlayer);
};

class CPlayerSaveDAO : public CPlayerDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQLite(CSQLEngine* pSQL) OVERRIDE;
	void ExecuteMySQL(CSQLEngine* pSQL) OVERRIDE;

	void BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int namePos, int secondsPos,
		int crimePos, int accessFlagsPos, int healthPos, int suitPos);
	void* PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int networkIdStringPos,
		int namePos, int secondsPos, int crimePos, int accessFlagsPos, int healthPos,
		int suitPos);
	void ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int networkIdStringPos,
		int namePos, int secondsPos, int crimePos, int accessFlagsPos, int healthPos,
		int suitPos);

	char m_Name[MAX_PLAYER_NAME_LENGTH];
	int m_iSeconds, m_iCrime, m_iHealth, m_iArmor, m_iAccessFlags;

public:
	CPlayerSaveDAO(CHL2RP_Player* pPlayer);
};

class CPlayersAmmoInitDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO);
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
};

// Shares DTO among player ammo DAOs
class CPlayerAmmoDAO : public CPlayerDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;

protected:
	CPlayerAmmoDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);

	void BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int networkIdStringPos, int ammoIndexPos);

	int m_iAmmoIndex;
};

class CPlayerAmmoSaveDAO : public CPlayerAmmoDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQLite(CSQLEngine* pSQL) OVERRIDE;
	void ExecuteMySQL(CSQLEngine* pSQL) OVERRIDE;

	void* PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int networkIdStringPos,
		int ammoIndexPos, int ammoCountPos);
	void ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int networkIdStringPos,
		int ammoIndexPos, int ammoCountPos);

	int m_iAmmoCount;

public:
	CPlayerAmmoSaveDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);
};

class CPlayerAmmoDeleteDAO : public CPlayerAmmoDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerAmmoDeleteDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);
};

class CPlayerWeaponsInitDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO);
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
};

// Shares DTO among player weapon DAOs
class CPlayerWeaponDAO : public CPlayerDAO
{
protected:
	CPlayerWeaponDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);

	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;

	void BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int networkIdStringPos, int weaponClassnamePos);

	char m_WeaponClassname[MAX_WEAPON_STRING];
};

class CPlayerWeaponSaveDAO : public CPlayerWeaponDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQLite(CSQLEngine* pSQL) OVERRIDE;
	void ExecuteMySQL(CSQLEngine* pSQL) OVERRIDE;

	void* PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int networkIdStringPos,
		int weaponPos, int clip1Pos, int clip2Pos);
	void ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int networkIdStringPos,
		int weaponPos, int clip1Pos, int clip2Pos);

	int m_iClip1, m_iClip2;

public:
	CPlayerWeaponSaveDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);
};

class CPlayerWeaponDeleteDAO : public CPlayerWeaponDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerWeaponDeleteDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);
};

#endif // !PLAYERDAO_H
