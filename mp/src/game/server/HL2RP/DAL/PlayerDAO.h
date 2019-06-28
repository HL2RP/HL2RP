#ifndef PLAYERDAO_H
#define PLAYERDAO_H
#pragma once

#include "IDAO.h"
#include <utlflags.h>

class CDefaultPlayerDataSeeker;
class CHL2RP_Player;

class CPlayersInitDAO : public IDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
};

// Share DTO among player DAOs
class CPlayerDAO : public IDAO
{
protected:
	CPlayerDAO(CHL2RP_Player* pPlayer);

	bool Equals(CPlayerDAO* pDAO);
	void BuildKeyValuesPath(const char* pDirName, char* pDest, int destSize);
	void BindSQLDTO(CSQLEngine* pSQL, void* pStmt);

	uint64 m_LongSteamIdNum;
	char m_NetworkIdString[MAX_NETWORKID_LENGTH];
};

class CPlayerLoadDAO : public CPlayerDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(CKeyValuesEngine* pKVEngine, KeyValues* pResults) OVERRIDE;
	void HandleResults(CSQLEngine* pKVEngine, KeyValues* pResults) OVERRIDE;

	void HandleResults(KeyValues* pResults, CDefaultPlayerDataSeeker& seeker);
	void HandleAmmoData(int ammoIndex, int ammoCount, bool shouldInsertAmmo[MAX_AMMO_SLOTS]);
	void HandleWeaponData(KeyValues* pRow, const char* pWeaponClassname,
		bool shouldInsertWeapon[MAX_WEAPONS]);

	CHandle<CHL2RP_Player> m_hPlayer;

public:
	CPlayerLoadDAO(CHL2RP_Player* pPlayer);
};

class CPlayerSaveDAO : public CPlayerDAO
{
protected:
	CPlayerSaveDAO(CHL2RP_Player* pPlayer);

	char m_Name[MAX_PLAYER_NAME_LENGTH];
	int m_iSeconds, m_iCrime, m_iHealth, m_iArmor;
	CUtlFlags<int> m_AccessFlags;
};

class CPlayerInsertDAO : public CPlayerSaveDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerInsertDAO(CHL2RP_Player* pPlayer);
};

class CPlayerUpdateDAO : public CPlayerSaveDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

	CUtlFlags<int> m_PropSelectionFlags;

public:
	CPlayerUpdateDAO(CHL2RP_Player* pPlayer, int propSelectionFlags);
};

class CPlayersAmmoInitDAO : public IDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
};

// Shares DTO among player ammo DAOs
class CPlayerAmmoDAO : public CPlayerDAO
{
protected:
	CPlayerAmmoDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);

	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;

	bool Equals(CPlayerAmmoDAO* pDAO);

	int m_iAmmoIndex;
};

class CPlayerAmmoSaveDAO : public CPlayerAmmoDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;

protected:
	CPlayerAmmoSaveDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);

	int m_iAmmoCount;
};

class CPlayerAmmoInsertDAO : public CPlayerAmmoSaveDAO
{
	CDAOThread::EDAOCollisionResolution Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO) OVERRIDE;
	CDAOThread::EDAOCollisionResolution Collide(CSQLEngine* pSQL, IDAO* pDAO) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerAmmoInsertDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);
};

class CPlayerAmmoUpdateDAO : public CPlayerAmmoSaveDAO
{
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerAmmoUpdateDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);
};

class CPlayerAmmoDeleteDAO : public CPlayerAmmoDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerAmmoDeleteDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex);
};

class CPlayerWeaponsInitDAO : public IDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
};

// Shares DTO among player weapon DAOs
class CPlayerWeaponDAO : public CPlayerDAO
{
protected:
	CPlayerWeaponDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);

	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;

	bool Equals(CPlayerWeaponDAO* pDAO);

	char m_WeaponClassname[MAX_WEAPON_STRING];
};

class CPlayerWeaponSaveDAO : public CPlayerWeaponDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;

protected:
	CPlayerWeaponSaveDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);

	int m_iClip1, m_iClip2;
};

class CPlayerWeaponInsertDAO : public CPlayerWeaponSaveDAO
{
	CDAOThread::EDAOCollisionResolution Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO) OVERRIDE;
	CDAOThread::EDAOCollisionResolution Collide(CSQLEngine* pSQL, IDAO* pDAO) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerWeaponInsertDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);
};

class CPlayerWeaponUpdateDAO : public CPlayerWeaponSaveDAO
{
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerWeaponUpdateDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);
};

class CPlayerWeaponDeleteDAO : public CPlayerWeaponDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CPlayerWeaponDeleteDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon);
};

#endif // !PLAYERDAO_H
