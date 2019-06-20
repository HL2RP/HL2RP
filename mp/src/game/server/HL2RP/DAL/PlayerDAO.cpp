#include <cbase.h>
#include "PlayerDAO.h"
#include <ammodef.h>
#include <cinttypes>
#include "DALEngine.h"
#include <HL2RPRules.h>
#include <HL2RP_Player.h>
#include <HL2RP_Util.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define DEFAULT_RATIONS_AMMO	5
#define PLAYER_MAINDAO_COLLECTION_NAME	"Player"
#define PLAYER_WEAPONDAO_COLLECTION_NAME	"PlayerWeapon"
#define PLAYER_AMMODAO_COLLECTION_NAME	"PlayerAmmo"

#define PLAYER_MAINDAO_SQL_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (steamid VARCHAR (%i) NOT NULL, name VARCHAR (%i),"	\
" seconds INTEGER, crime INTEGER, accessflags INTEGER, health INTEGER, suit INTEGER, PRIMARY KEY (steamid),"	\
" CONSTRAINT seconds CHECK (seconds >= 0), CONSTRAINT crime CHECK (crime >= 0),"	\
" CONSTRAINT accessflags CHECK (accessflags >= %i), CONSTRAINT health CHECK (health > 0), CONSTRAINT suit CHECK (suit >= 0));"

#define PLAYER_MAINDAO_SQL_LOAD_QUERY	"SELECT * FROM " PLAYER_MAINDAO_COLLECTION_NAME " WHERE steamid = ?;"

#define PLAYER_MAINDAO_SQLITE_UPDATE_QUERY	\
"UPDATE " PLAYER_MAINDAO_COLLECTION_NAME " SET name = ?, seconds = ?, crime = ?, accessflags = ?, health = ?, suit = ?"	\
" WHERE steamid = ?;"

#define PLAYER_MAINDAO_SQLITE_INSERT_QUERY	\
"INSERT OR IGNORE INTO " PLAYER_MAINDAO_COLLECTION_NAME " (steamid, name, seconds, crime, accessflags, health, suit)"	\
" VALUES (?, ?, ?, ?, ?, ?, ?);"

#define PLAYER_MAINDAO_MYSQL_SAVE_QUERY	\
"INSERT INTO " PLAYER_MAINDAO_COLLECTION_NAME " (steamid, name, seconds, crime, accessflags, health, suit)"	\
" VALUES (?, ?, ?, ?, ?, ?, ?)"	\
" ON DUPLICATE KEY	UPDATE name = ?, seconds = ?, crime = ?, accessflags = ?, health = ?, suit = ?;"

#define PLAYER_AMMODAO_SQL_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (steamid VARCHAR (%i) NOT NULL, `index` INTEGER NOT NULL,"	\
" count INTEGER NOT NULL, PRIMARY KEY (steamid, `index`), FOREIGN KEY (steamid) REFERENCES %s (steamid)"	\
" ON UPDATE CASCADE ON DELETE CASCADE, CONSTRAINT `index` CHECK (`index` > 0), CONSTRAINT count CHECK (count >= 0));"

#define PLAYER_AMMODAO_SQL_LOAD_QUERY	"SELECT * FROM " PLAYER_AMMODAO_COLLECTION_NAME " WHERE steamid = ?;"

#define PLAYER_AMMODAO_SQLITE_INSERT_QUERY	\
"UPDATE " PLAYER_AMMODAO_COLLECTION_NAME " SET count = ? WHERE steamid = ? AND `index` = ?;"

#define PLAYER_AMMODAO_SQLITE_UPDATE_QUERY	\
"INSERT OR IGNORE INTO " PLAYER_AMMODAO_COLLECTION_NAME " (steamid, `index`, count) VALUES (?, ?, ?);"

#define PLAYER_AMMODAO_MYSQL_SAVE_QUERY	\
"INSERT INTO " PLAYER_AMMODAO_COLLECTION_NAME " (steamid, `index`, count) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE count = ?;"

#define PLAYER_AMMODAO_MYSQL_DELETE_QUERY	"DELETE FROM " PLAYER_AMMODAO_COLLECTION_NAME " WHERE steamid = ? AND `index` = ?;"

#define PLAYER_WEAPONDAO_SQL_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (steamid VARCHAR (%i) NOT NULL, weapon VARCHAR (%i) NOT NULL,"	\
" clip1 INTEGER, clip2 INTEGER, PRIMARY KEY (steamid, weapon), FOREIGN KEY (steamid) REFERENCES %s (steamid)"	\
" ON UPDATE CASCADE ON DELETE CASCADE, CONSTRAINT clip1 CHECK (clip1 >= %i), CONSTRAINT clip2 CHECK (clip2 >= %i));"

#define PLAYER_WEAPONDAO_SQL_LOAD_QUERY	"SELECT * FROM " PLAYER_WEAPONDAO_COLLECTION_NAME " WHERE steamid = ?;"

#define PLAYER_WEAPONDAO_SQLITE_UPDATE_QUERY	\
"UPDATE " PLAYER_WEAPONDAO_COLLECTION_NAME " SET clip1 = ?, clip2 = ? WHERE steamid = ? AND weapon = ?;"

#define PLAYER_WEAPONDAO_SQLITE_INSERT_QUERY	\
"INSERT OR IGNORE INTO " PLAYER_WEAPONDAO_COLLECTION_NAME " (steamid, weapon, clip1, clip2) VALUES (?, ?, ?, ?);"

#define PLAYER_WEAPONDAO_MYSQL_SAVE_QUERY	\
"INSERT INTO " PLAYER_WEAPONDAO_COLLECTION_NAME " (steamid, weapon, clip1, clip2) VALUES (?, ?, ?, ?)"	\
" ON DUPLICATE KEY UPDATE clip1 = ?, clip2 = ?;"

#define PLAYER_WEAPONDAO_SQL_DELETE_QUERY	"DELETE FROM " PLAYER_WEAPONDAO_COLLECTION_NAME " WHERE steamid = ? AND weapon = ?;"

bool CPlayersInitDAO::ShouldBeReplacedBy(IDAO * pDAO)
{
	return (pDAO->ToClass(this) != NULL);
}

void CPlayersInitDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteFormatQuery(PLAYER_MAINDAO_SQL_SETUP_QUERY, NULL, PLAYER_MAINDAO_COLLECTION_NAME,
		MAX_NETWORKID_LENGTH, MAX_PLAYER_NAME_LENGTH, CHL2RP_Player::AccessFlag_None);
}

CPlayerDAO::CPlayerDAO(CHL2RP_Player* pPlayer) : m_hPlayer(pPlayer),
m_LongSteamIdNum(pPlayer->GetSteamIDAsUInt64())
{
	Q_strcpy(m_NetworkIdString, pPlayer->GetNetworkIDString());
}

// Validates passed DAO and check if it SteamIDs equal
bool CPlayerDAO::ShouldBeReplacedBy(CPlayerDAO* pDAO)
{
	return (pDAO != NULL && pDAO->m_LongSteamIdNum == m_LongSteamIdNum);
}

void CPlayerDAO::BuildKeyValuesPath(const char* pDirName, char* pDest, int destSize)
{
	Q_snprintf(pDest, destSize, "%s/%" PRIu64 ".txt", pDirName, m_LongSteamIdNum);
}

void CPlayerDAO::BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int networkIdStringPos)
{
	pSQL->BindString(pStmt, networkIdStringPos, m_NetworkIdString);
}

CPlayerLoadDAO::CPlayerLoadDAO(CHL2RP_Player* pPlayer) : CPlayerDAO(pPlayer)
{

}

bool CPlayerLoadDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	CPlayerLoadDAO* pPlayerLoadDAO = pDAO->ToClass(this);
	return (pPlayerLoadDAO != NULL && CPlayerDAO::ShouldBeReplacedBy(pPlayerLoadDAO));
}

void CPlayerLoadDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_MAINDAO_COLLECTION_NAME, path, sizeof(path));
	pKeyValues->LoadFromFile(path, PLAYER_MAINDAO_COLLECTION_NAME);
	BuildKeyValuesPath(PLAYER_AMMODAO_COLLECTION_NAME, path, sizeof(path));
	pKeyValues->LoadFromFile(path, PLAYER_AMMODAO_COLLECTION_NAME);
	BuildKeyValuesPath(PLAYER_WEAPONDAO_COLLECTION_NAME, path, sizeof(path));
	pKeyValues->LoadFromFile(path, PLAYER_WEAPONDAO_COLLECTION_NAME);
}

void CPlayerLoadDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->PrepareStatement(PLAYER_MAINDAO_SQL_LOAD_QUERY);
	BindSQLDTO(pSQL, pStmt, 1);
	pSQL->ExecutePreparedStatement(pStmt, PLAYER_MAINDAO_COLLECTION_NAME);
	pStmt = pSQL->PrepareStatement(PLAYER_AMMODAO_SQL_LOAD_QUERY);
	BindSQLDTO(pSQL, pStmt, 1);
	pSQL->ExecutePreparedStatement(pStmt, PLAYER_AMMODAO_COLLECTION_NAME);
	pStmt = pSQL->PrepareStatement(PLAYER_WEAPONDAO_SQL_LOAD_QUERY);
	BindSQLDTO(pSQL, pStmt, 1);
	pSQL->ExecutePreparedStatement(pStmt, PLAYER_WEAPONDAO_COLLECTION_NAME);
}

void CPlayerLoadDAO::HandleKeyValuesResults(KeyValues* pResults)
{
	if (PreHandleResults())
	{
		Assert(m_hPlayer != NULL);
		KeyValues* pResult = pResults->FindKey(PLAYER_MAINDAO_COLLECTION_NAME);

		if (pResult == NULL)
		{
			HandleMainData_NewPlayer();
		}
		else
		{
			HandleMainData_OldPlayer(pResult);
			pResult = pResults->FindKey(PLAYER_AMMODAO_COLLECTION_NAME);

			if (pResult != NULL)
			{
				FOR_EACH_SUBKEY(pResult, pAmmoData)
				{
					HandleAmmoData(Q_atoi(pAmmoData->GetName()), pAmmoData->GetInt());
				}
			}

			pResult = pResults->FindKey(PLAYER_WEAPONDAO_COLLECTION_NAME);

			if (pResult != NULL)
			{
				FOR_EACH_SUBKEY(pResult, pWeaponData)
				{
					HandleWeaponData(pWeaponData, pWeaponData->GetName());
				}
			}
		}

		PostHandleResults();
	}
}

void CPlayerLoadDAO::HandleSQLResults(KeyValues* pResults)
{
	if (PreHandleResults())
	{
		Assert(m_hPlayer != NULL);
		KeyValues* pResult = pResults->FindKey(PLAYER_MAINDAO_COLLECTION_NAME);

		if (pResult == NULL)
		{
			HandleMainData_NewPlayer();
		}
		else
		{
			HandleMainData_OldPlayer(pResult->GetFirstSubKey());
			pResult = pResults->FindKey(PLAYER_AMMODAO_COLLECTION_NAME);

			if (pResult != NULL)
			{
				FOR_EACH_SUBKEY(pResult, pAmmoData)
				{
					HandleAmmoData(pAmmoData->GetInt("index"), pAmmoData->GetInt("count"));
				}
			}

			pResult = pResults->FindKey(PLAYER_WEAPONDAO_COLLECTION_NAME);

			if (pResult != NULL)
			{
				FOR_EACH_SUBKEY(pResult, pWeaponData)
				{
					HandleWeaponData(pWeaponData, pWeaponData->GetString("weapon"));
				}
			}
		}

		PostHandleResults();
	}
}

bool CPlayerLoadDAO::PreHandleResults()
{
	// We include a definitive identity check via SteamID, even when CHandle is extremely reliable
	if (m_hPlayer != NULL && !m_hPlayer->IsFakeClient()
		&& m_hPlayer->GetSteamIDAsUInt64() == m_LongSteamIdNum)
	{
		if (!m_hPlayer->IsAlive())
		{
			m_hPlayer->m_pDeadLoadedEquipmentInfo = new KeyValues("Equipment");
		}

		return true;
	}

	return false;
}

void CPlayerLoadDAO::HandleMainData_NewPlayer()
{
	Assert(m_hPlayer != NULL);
	m_hPlayer->GiveNamedItem("ration");
	m_hPlayer->GiveAmmo(DEFAULT_RATIONS_AMMO, GetAmmoDef()->Index("ration"), false);
}

void CPlayerLoadDAO::HandleMainData_OldPlayer(KeyValues* pPlayerData)
{
	Assert(m_hPlayer != NULL);

	// Add up the resources instead of setting them either if player exists or is new,
	// since he may already got some of them in a legit way since his connection
	int seconds = pPlayerData->GetInt("seconds"), crime = pPlayerData->GetInt("crime"),
		accessFlags = pPlayerData->GetInt("accessflags"),
		health = pPlayerData->GetInt("health"), suit = pPlayerData->GetInt("suit");
	m_hPlayer->m_iSeconds = UTIL_EnsureAddition(m_hPlayer->m_iSeconds, seconds);
	m_hPlayer->m_iCrime = UTIL_EnsureAddition(m_hPlayer->m_iCrime, crime);
	m_hPlayer->m_AccessFlags.SetFlag(accessFlags);

	if (m_hPlayer->IsAlive())
	{
		return m_hPlayer->TryGiveLoadedHpAndArmor(health, suit);
	}

	m_hPlayer->m_pDeadLoadedEquipmentInfo->SetInt("health", health);
	m_hPlayer->m_pDeadLoadedEquipmentInfo->SetInt("suit", suit);
}

void CPlayerLoadDAO::HandleAmmoData(int ammoIndex, int ammoCount)
{
	Assert(m_hPlayer != NULL);

	if (ammoIndex < MAX_AMMO_TYPES)
	{
		if (m_hPlayer->IsAlive())
		{
			m_hPlayer->GiveAmmo(ammoCount, ammoIndex, false);
		}
		else if (ammoIndex >= 0)
		{
			KeyValues* pAmmoData = m_hPlayer->m_pDeadLoadedEquipmentInfo->
				FindKey(HL2RP_PLAYER_DEADLOAD_AMMODATA_KEYNAME, true);
			m_hPlayer->m_pDeadLoadedEquipmentInfo->SetInt(NULL, ammoIndex);
			pAmmoData->SetInt(m_hPlayer->m_pDeadLoadedEquipmentInfo->GetString(), ammoCount);
		}
	}
}

void CPlayerLoadDAO::HandleWeaponData(KeyValues* pWeaponData, const char* pWeaponClassname)
{
	int clip1 = pWeaponData->GetInt("clip1"), clip2 = pWeaponData->GetInt("clip2");

	if (m_hPlayer->IsAlive())
	{
		return m_hPlayer->TryGiveLoadedWeapon(pWeaponClassname, clip1, clip2);
	}

	pWeaponData = m_hPlayer->m_pDeadLoadedEquipmentInfo->
		FindKey(HL2RP_PLAYER_DEADLOAD_WEAPONDATA_KEYNAME, true);
	pWeaponData = pWeaponData->FindKey(pWeaponClassname, true);
	pWeaponData->SetInt("clip1", clip1);
	pWeaponData->SetInt("clip2", clip2);
}

void CPlayerLoadDAO::PostHandleResults()
{
	Assert(m_hPlayer != NULL);

	// Allow saves only from now on (once loaded), for security
	m_hPlayer->m_DALSyncFlags.SetFlag(CHL2RP_Player::EDALSyncCap::SaveData);

	// Synchronize final main data to DB, since player may had some before loading
	TryCreateAsyncDAO<CPlayerSaveDAO>(m_hPlayer);

	if (m_hPlayer->IsAlive())
	{
		// Begin synchronizing final ammunition to DB, since player may had some before loading
		for (int i = 0; i < MAX_AMMO_SLOTS; i++)
		{
			if (m_hPlayer->GetAmmoCount(i) > 0)
			{
				TryCreateAsyncDAO<CPlayerAmmoSaveDAO>(m_hPlayer, i);
			}
		}

		// Begin synchronizing final weapons to DB, since player may had some before loading
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			if (m_hPlayer->GetWeapon(i) != NULL)
			{
				TryCreateAsyncDAO<CPlayerWeaponSaveDAO>(m_hPlayer, m_hPlayer->GetWeapon(i));
			}
		}
	}

	ClientPrint(m_hPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
		Localize(m_hPlayer, "#HL2RP_Chat_Player_Loaded"));
}

CPlayerSaveDAO::CPlayerSaveDAO(CHL2RP_Player* pPlayer) : CPlayerDAO(pPlayer)
{
	Q_strcpy(m_Name, pPlayer->GetPlayerName());
	m_iSeconds = pPlayer->m_iSeconds;
	m_iCrime = pPlayer->m_iCrime;
	m_iAccessFlags = *(int*)& pPlayer->m_AccessFlags;

	// Dead flag isn't set yet by callers. Check for death health instead.
	if (pPlayer->GetHealth() < 1)
	{
		m_iHealth = pPlayer->GetMaxHealth();
		m_iArmor = 0;
	}
	else
	{
		// We copy these values, excessive ones should be corrected in the load anyways
		m_iHealth = pPlayer->GetHealth();
		m_iArmor = pPlayer->ArmorValue();
	}
}

bool CPlayerSaveDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return CPlayerDAO::ShouldBeReplacedBy(pDAO->ToClass(this));
}

void CPlayerSaveDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_MAINDAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pPlayerData = pKeyValues->LoadFromFile(path, PLAYER_MAINDAO_COLLECTION_NAME, true);
	pPlayerData->SetString("steamid", m_NetworkIdString);
	pPlayerData->SetString("name", m_Name);
	pPlayerData->SetInt("seconds", m_iSeconds);
	pPlayerData->SetInt("crime", m_iCrime);
	pPlayerData->SetInt("accessflags", m_iAccessFlags);
	pPlayerData->SetInt("health", m_iHealth);
	pPlayerData->SetInt("suit", m_iArmor);
	pKeyValues->SaveToFile(pPlayerData, path);
}

void CPlayerSaveDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	pSQL->BeginTxn();
	ExecutePreparedSQLStatement(pSQL, PLAYER_MAINDAO_SQLITE_UPDATE_QUERY, 7, 1, 2, 3, 4, 5, 6);
	ExecutePreparedSQLStatement(pSQL, PLAYER_MAINDAO_SQLITE_INSERT_QUERY, 1, 2, 3, 4, 5, 6, 7);
	pSQL->EndTxn();
}

void CPlayerSaveDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	pSQL->BeginTxn();
	void* pStmt = PrepareSQLStatement(pSQL, PLAYER_MAINDAO_MYSQL_SAVE_QUERY, 1, 2, 3, 4, 5, 6, 7);
	BindSQLDTO(pSQL, pStmt, 8, 9, 10, 11, 12, 13);
	pSQL->ExecutePreparedStatement(pStmt);
	pSQL->EndTxn();
}

void CPlayerSaveDAO::BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int namePos, int secondsPos,
	int crimePos, int accessFlagsPos, int healthPos, int suitPos)
{
	pSQL->BindString(pStmt, namePos, m_Name);
	pSQL->BindInt(pStmt, secondsPos, m_iSeconds);
	pSQL->BindInt(pStmt, crimePos, m_iCrime);
	pSQL->BindInt(pStmt, accessFlagsPos, m_iAccessFlags);
	pSQL->BindInt(pStmt, healthPos, m_iHealth);
	pSQL->BindInt(pStmt, suitPos, m_iArmor);
}

void* CPlayerSaveDAO::PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText,
	int networkIdStringPos, int namePos, int secondsPos, int crimePos, int accessFlagsPos,
	int healthPos, int suitPos)
{
	void* pStmt = pSQL->PrepareStatement(pQueryText);
	CPlayerDAO::BindSQLDTO(pSQL, pStmt, networkIdStringPos);
	BindSQLDTO(pSQL, pStmt, namePos, secondsPos, crimePos, accessFlagsPos, healthPos, suitPos);
	return pStmt;
}

void CPlayerSaveDAO::ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText,
	int networkIdStringPos, int namePos, int secondsPos, int crimePos, int accessFlagsPos,
	int healthPos, int suitPos)
{
	void* pStmt = PrepareSQLStatement(pSQL, pQueryText, networkIdStringPos, namePos, secondsPos,
		crimePos, accessFlagsPos, healthPos, suitPos);
	pSQL->ExecutePreparedStatement(pStmt);
}

bool CPlayersAmmoInitDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return (pDAO->ToClass(this) != NULL);
}

void CPlayersAmmoInitDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteFormatQuery(PLAYER_AMMODAO_SQL_SETUP_QUERY, NULL, PLAYER_AMMODAO_COLLECTION_NAME,
		MAX_NETWORKID_LENGTH, PLAYER_MAINDAO_COLLECTION_NAME);
}

CPlayerAmmoDAO::CPlayerAmmoDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerDAO(pPlayer), m_iAmmoIndex(playerAmmoIndex)
{

}

bool CPlayerAmmoDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	CPlayerAmmoDAO* pPlayerAmmoDAO = pDAO->ToClass(this);
	return (CPlayerDAO::ShouldBeReplacedBy(pPlayerAmmoDAO)
		&& pPlayerAmmoDAO->m_iAmmoIndex == m_iAmmoIndex);
}

void CPlayerAmmoDAO::BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int networkIdStringPos, int ammoIndexPos)
{
	CPlayerDAO::BindSQLDTO(pSQL, pStmt, networkIdStringPos);
	pSQL->BindInt(pStmt, ammoIndexPos, m_iAmmoIndex);
}

CPlayerAmmoSaveDAO::CPlayerAmmoSaveDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerAmmoDAO(pPlayer, playerAmmoIndex), m_iAmmoCount(pPlayer->GetAmmoCount(playerAmmoIndex))
{

}

void CPlayerAmmoSaveDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_AMMODAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKeyValues->LoadFromFile(path, PLAYER_AMMODAO_COLLECTION_NAME, true);
	pResult->SetInt(NULL, m_iAmmoIndex);
	pResult->SetInt(pResult->GetString(), m_iAmmoCount);
	pKeyValues->SaveToFile(pResult, path);
}

void CPlayerAmmoSaveDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	// Secure CHECK constraints!
	if (m_iAmmoIndex > 0 && m_iAmmoCount >= 0)
	{
		pSQL->BeginTxn();
		ExecutePreparedSQLStatement(pSQL, PLAYER_AMMODAO_SQLITE_INSERT_QUERY, 2, 3, 1);
		ExecutePreparedSQLStatement(pSQL, PLAYER_AMMODAO_SQLITE_UPDATE_QUERY, 1, 2, 3);
		pSQL->EndTxn();
	}
}

void CPlayerAmmoSaveDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	void* pStmt = PrepareSQLStatement(pSQL, PLAYER_AMMODAO_MYSQL_SAVE_QUERY, 1, 2, 3);
	pSQL->BindInt(pStmt, 4, m_iAmmoCount);
	pSQL->ExecutePreparedStatement(pStmt);
}

void* CPlayerAmmoSaveDAO::PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText,
	int networkIdStringPos, int ammoIndexPos, int ammoCountPos)
{
	void* pStmt = pSQL->PrepareStatement(pQueryText);
	BindSQLDTO(pSQL, pStmt, networkIdStringPos, ammoIndexPos);
	pSQL->BindInt(pStmt, ammoCountPos, m_iAmmoCount);
	return pStmt;
}

void CPlayerAmmoSaveDAO::ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText,
	int networkIdStringPos, int ammoIndexPos, int ammoCountPos)
{
	void* pStmt = PrepareSQLStatement(pSQL, pQueryText, networkIdStringPos, ammoIndexPos, ammoCountPos);
	pSQL->ExecutePreparedStatement(pStmt);
}

CPlayerAmmoDeleteDAO::CPlayerAmmoDeleteDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerAmmoDAO(pPlayer, playerAmmoIndex)
{

}

void CPlayerAmmoDeleteDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_AMMODAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKeyValues->LoadFromFile(path, PLAYER_AMMODAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		pResult->SetInt(NULL, m_iAmmoIndex);
		KeyValues* pAmmoData = pResult->FindKey(pResult->GetString());

		if (pAmmoData != NULL)
		{
			pResult->RemoveSubKey(pAmmoData);
			pKeyValues->SaveToFile(pResult, path);
		}
	}
}

void CPlayerAmmoDeleteDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->PrepareStatement(PLAYER_AMMODAO_MYSQL_DELETE_QUERY);
	BindSQLDTO(pSQL, pStmt, 1, 2);
	pSQL->ExecutePreparedStatement(pStmt);
}

bool CPlayerWeaponsInitDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return (pDAO->ToClass(this) != NULL);
}

void CPlayerWeaponsInitDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteFormatQuery(PLAYER_WEAPONDAO_SQL_SETUP_QUERY, NULL,
		PLAYER_WEAPONDAO_COLLECTION_NAME, MAX_NETWORKID_LENGTH, MAX_WEAPON_STRING,
		PLAYER_MAINDAO_COLLECTION_NAME, WEAPON_NOCLIP, WEAPON_NOCLIP);
}

CPlayerWeaponDAO::CPlayerWeaponDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerDAO(pPlayer)
{
	Q_strcpy(m_WeaponClassname, pWeapon->GetClassname());
}

bool CPlayerWeaponDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	CPlayerWeaponDAO* pPlayerWeaponDAO = pDAO->ToClass(this);
	return (CPlayerDAO::ShouldBeReplacedBy(pPlayerWeaponDAO)
		&& !Q_strcmp(pPlayerWeaponDAO->m_WeaponClassname, m_WeaponClassname));
}

void CPlayerWeaponDAO::BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int networkIdStringPos, int weaponClassnamePos)
{
	CPlayerDAO::BindSQLDTO(pSQL, pStmt, networkIdStringPos);
	pSQL->BindString(pStmt, weaponClassnamePos, m_WeaponClassname);
}

CPlayerWeaponSaveDAO::CPlayerWeaponSaveDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerWeaponDAO(pPlayer, pWeapon), m_iClip1(pWeapon->Clip1()), m_iClip2(pWeapon->Clip2())
{

}

void CPlayerWeaponSaveDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_WEAPONDAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKeyValues->LoadFromFile(path, PLAYER_WEAPONDAO_COLLECTION_NAME, true);
	KeyValues* pWeaponData = pResult->FindKey(m_WeaponClassname, true);
	pWeaponData->SetInt("clip1", m_iClip1);
	pWeaponData->SetInt("clip2", m_iClip2);
	pKeyValues->SaveToFile(pResult, path);
}

void CPlayerWeaponSaveDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	// Secure CHECK constraints!
	if (m_iClip1 >= WEAPON_NOCLIP && m_iClip2 >= WEAPON_NOCLIP)
	{
		pSQL->BeginTxn();
		ExecutePreparedSQLStatement(pSQL, PLAYER_WEAPONDAO_SQLITE_UPDATE_QUERY, 3, 4, 1, 2);
		ExecutePreparedSQLStatement(pSQL, PLAYER_WEAPONDAO_SQLITE_INSERT_QUERY, 1, 2, 3, 4);
		pSQL->EndTxn();
	}
}

void CPlayerWeaponSaveDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	pSQL->BeginTxn();
	void* pStmt = PrepareSQLStatement(pSQL, PLAYER_WEAPONDAO_MYSQL_SAVE_QUERY, 1, 2, 3, 4);
	pSQL->BindInt(pStmt, 5, m_iClip1);
	pSQL->BindInt(pStmt, 6, m_iClip2);
	pSQL->ExecutePreparedStatement(pStmt);
	pSQL->EndTxn();
}

void* CPlayerWeaponSaveDAO::PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText,
	int networkIdStringPos, int weaponPos, int clip1Pos, int clip2Pos)
{
	void* pStmt = pSQL->PrepareStatement(pQueryText);
	BindSQLDTO(pSQL, pStmt, networkIdStringPos, weaponPos);
	pSQL->BindInt(pStmt, clip1Pos, m_iClip1);
	pSQL->BindInt(pStmt, clip2Pos, m_iClip2);
	return pStmt;
}

void CPlayerWeaponSaveDAO::ExecutePreparedSQLStatement(CSQLEngine* pSQL, const char* pQueryText,
	int networkIdStringPos, int weaponPos, int clip1Pos, int clip2Pos)
{
	void* pStmt = PrepareSQLStatement(pSQL, pQueryText, networkIdStringPos, weaponPos, clip1Pos, clip2Pos);
	pSQL->ExecutePreparedStatement(pStmt);
}

CPlayerWeaponDeleteDAO::CPlayerWeaponDeleteDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerWeaponDAO(pPlayer, pWeapon)
{

}

void CPlayerWeaponDeleteDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_WEAPONDAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKeyValues->LoadFromFile(path, PLAYER_WEAPONDAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		KeyValues* pWeaponData = pResult->FindKey(m_WeaponClassname);

		if (pWeaponData != NULL)
		{
			pResult->RemoveSubKey(pWeaponData);
			pKeyValues->SaveToFile(pResult, path);
		}
	}
}

void CPlayerWeaponDeleteDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->PrepareStatement(PLAYER_WEAPONDAO_SQL_DELETE_QUERY);
	BindSQLDTO(pSQL, pStmt, 1, 2);
	pSQL->ExecutePreparedStatement(pStmt);
}
