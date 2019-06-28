#include <cbase.h>
#include "PlayerDAO.h"
#include <ammodef.h>
#include <cinttypes>
#include <HL2RPRules.h>
#include <HL2RP_Player.h>
#include <HL2RP_Util.h>
#include "IDALEngine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define PLAYER_MAIN_DAO_DEFAULT_RATIONS_AMMO	5
#define PLAYER_MAIN_DAO_COLLECTION_NAME	"Player"
#define PLAYER_WEAPON_DAO_COLLECTION_NAME	"PlayerWeapon"
#define PLAYER_AMMO_DAO_COLLECTION_NAME	"PlayerAmmo"

#define PLAYER_MAIN_DAO_SQL_SETUP_QUERY	"CREATE TABLE IF NOT EXISTS %s "	\
"(steamId VARCHAR (%i) NOT NULL, name VARCHAR (%i), seconds INTEGER, crime INTEGER, accessFlags INTEGER, "	\
"health INTEGER, suit INTEGER, PRIMARY KEY (steamId), "	\
"CONSTRAINT seconds CHECK (seconds >= 0), CONSTRAINT crime CHECK (crime >= 0), "	\
"CONSTRAINT accessFlags CHECK (accessFlags >= %i), CONSTRAINT health CHECK (health > 0), "	\
"CONSTRAINT suit CHECK (suit >= 0));"

#define PLAYER_AMMO_DAO_SQL_SETUP_QUERY	"CREATE TABLE IF NOT EXISTS %s "	\
"(steamId VARCHAR (%i) NOT NULL, `index` INTEGER NOT NULL, count INTEGER NOT NULL, "	\
"PRIMARY KEY (steamId, `index`), FOREIGN KEY (steamId) REFERENCES %s (steamId) "	\
"ON UPDATE CASCADE ON DELETE CASCADE, CONSTRAINT `index` CHECK (`index` > 0), CONSTRAINT count CHECK (count >= 0));"

#define PLAYER_WEAPON_DAO_SQL_SETUP_QUERY	"CREATE TABLE IF NOT EXISTS %s "	\
"(steamId VARCHAR (%i) NOT NULL, weapon VARCHAR (%i) NOT NULL, "	\
"clip1 INTEGER, clip2 INTEGER, PRIMARY KEY (steamId, weapon), FOREIGN KEY (steamId) REFERENCES %s (steamId) "	\
"ON UPDATE CASCADE ON DELETE CASCADE, CONSTRAINT clip1 CHECK (clip1 >= %i), CONSTRAINT clip2 CHECK (clip2 >= %i));"

// A inheritable resolver for subtle differences in cached results among DAL Engines.
// This base class is also used for KeyValues Engine.
class CDefaultPlayerDataSeeker
{
public:
	virtual KeyValues* SeekMainData(KeyValues* pMainData)
	{
		return pMainData;
	}

	virtual const char* SeekWeaponClassname(KeyValues* pWeaponData)
	{
		return pWeaponData->GetName();
	}

	virtual int SeekAmmoIndex(KeyValues* pAmmoData)
	{
		return Q_atoi(pAmmoData->GetName());
	}

	virtual int SeekAmmoCount(KeyValues* pAmmoData)
	{
		return pAmmoData->GetInt();
	}
};

class CSQLEnginePlayerDataSeeker : public CDefaultPlayerDataSeeker
{
	KeyValues* SeekMainData(KeyValues* pMainData) OVERRIDE
	{
		return pMainData->GetFirstSubKey();
	}

	const char* SeekWeaponClassname(KeyValues* pWeaponData) OVERRIDE
	{
		return pWeaponData->GetString("weapon");
	}

	int SeekAmmoIndex(KeyValues* pAmmoData) OVERRIDE
	{
		return pAmmoData->GetInt("index");
	}

	int SeekAmmoCount(KeyValues* pAmmoData) OVERRIDE
	{
		return pAmmoData->GetInt("count");
	}
};

CDAOThread::EDAOCollisionResolution CPlayersInitDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

void CPlayersInitDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery(PLAYER_MAIN_DAO_SQL_SETUP_QUERY, PLAYER_MAIN_DAO_COLLECTION_NAME,
		MAX_NETWORKID_LENGTH, MAX_PLAYER_NAME_LENGTH, CHL2RP_Player::AccessFlag_None);
}

CPlayerDAO::CPlayerDAO(CHL2RP_Player* pPlayer) : m_LongSteamIdNum(pPlayer->GetSteamIDAsUInt64())
{
	Q_strcpy(m_NetworkIdString, pPlayer->GetNetworkIDString());
}

// Validates passed DAO and checks if SteamIDs are equal
bool CPlayerDAO::Equals(CPlayerDAO* pDAO)
{
	return (pDAO != NULL && pDAO->m_LongSteamIdNum == m_LongSteamIdNum);
}

void CPlayerDAO::BuildKeyValuesPath(const char* pDirName, char* pDest, int destSize)
{
	Q_snprintf(pDest, destSize, "%s/%" PRIu64 ".txt", pDirName, m_LongSteamIdNum);
}

void CPlayerDAO::BindSQLDTO(CSQLEngine* pSQL, void* pStmt)
{
	pSQL->BindString(pStmt, 1, m_NetworkIdString);
}

CPlayerLoadDAO::CPlayerLoadDAO(CHL2RP_Player* pPlayer) : CPlayerDAO(pPlayer), m_hPlayer(pPlayer)
{

}

CDAOThread::EDAOCollisionResolution CPlayerLoadDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(Equals(pDAO->ToClass(this)));
}

void CPlayerLoadDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_MAIN_DAO_COLLECTION_NAME, path, sizeof(path));
	pKVEngine->LoadFromFile(path, PLAYER_MAIN_DAO_COLLECTION_NAME);
	BuildKeyValuesPath(PLAYER_AMMO_DAO_COLLECTION_NAME, path, sizeof(path));
	pKVEngine->LoadFromFile(path, PLAYER_AMMO_DAO_COLLECTION_NAME);
	BuildKeyValuesPath(PLAYER_WEAPON_DAO_COLLECTION_NAME, path, sizeof(path));
	pKVEngine->LoadFromFile(path, PLAYER_WEAPON_DAO_COLLECTION_NAME);
}

void CPlayerLoadDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("SELECT * FROM %s WHERE steamId = ?;",
		PLAYER_MAIN_DAO_COLLECTION_NAME);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt, PLAYER_MAIN_DAO_COLLECTION_NAME);
	pStmt = pSQL->FormatAndPrepareStatement("SELECT * FROM %s WHERE steamId = ?;",
		PLAYER_AMMO_DAO_COLLECTION_NAME);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt, PLAYER_AMMO_DAO_COLLECTION_NAME);
	pStmt = pSQL->FormatAndPrepareStatement("SELECT * FROM %s WHERE steamId = ?;",
		PLAYER_WEAPON_DAO_COLLECTION_NAME);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt, PLAYER_WEAPON_DAO_COLLECTION_NAME);
}

void CPlayerLoadDAO::HandleResults(CKeyValuesEngine* pKVEngine, KeyValues* pResults)
{
	CDefaultPlayerDataSeeker seeker;
	HandleResults(pResults, seeker);
}

void CPlayerLoadDAO::HandleResults(CSQLEngine* pKVEngine, KeyValues* pResults)
{
	CSQLEnginePlayerDataSeeker seeker;
	HandleResults(pResults, seeker);
}

void CPlayerLoadDAO::HandleResults(KeyValues* pResults, CDefaultPlayerDataSeeker& seeker)
{
	// We include a definitive identity check via SteamID, even when CHandle is extremely reliable
	if (m_hPlayer != NULL && !m_hPlayer->IsFakeClient()
		&& m_hPlayer->GetSteamIDAsUInt64() == m_LongSteamIdNum)
	{
		bool shouldInsertAmmo[MAX_AMMO_SLOTS]{};
		bool shouldInsertWeapon[MAX_WEAPONS]{};
		KeyValues* pResult = pResults->FindKey(PLAYER_MAIN_DAO_COLLECTION_NAME);

		if (m_hPlayer->IsAlive())
		{
			Q_memset(shouldInsertAmmo, true, ARRAYSIZE(shouldInsertAmmo));
			Q_memset(shouldInsertWeapon, true, ARRAYSIZE(shouldInsertWeapon));
		}
		else
		{
			m_hPlayer->m_pDeadLoadedEquipmentInfo = new KeyValues("Equipment");
		}

		if (pResult == NULL)
		{
			CreateAsyncDAO<CPlayerInsertDAO>(m_hPlayer);
			m_hPlayer->m_DALSyncCaps.SetFlag(CHL2RP_Player::EDALSyncCap::SaveData);
			m_hPlayer->GiveNamedItem("ration");
			m_hPlayer->GiveAmmo(PLAYER_MAIN_DAO_DEFAULT_RATIONS_AMMO, GetAmmoDef()->Index("ration"), false);
		}
		else
		{
			// Add up the resources instead of setting them either if player exists or is new,
			// since he may already got some of them in a legit way since his connection
			pResult = seeker.SeekMainData(pResult);
			int seconds = pResult->GetInt("seconds"), crime = pResult->GetInt("crime"),
				accessFlags = pResult->GetInt("accessFlags"),
				health = pResult->GetInt("health"), suit = pResult->GetInt("suit");
			m_hPlayer->m_iSeconds = UTIL_EnsureAddition(m_hPlayer->m_iSeconds, seconds);
			m_hPlayer->m_iCrime = UTIL_EnsureAddition(m_hPlayer->m_iCrime, crime);
			m_hPlayer->m_AccessFlags.SetFlag(accessFlags);

			if (m_hPlayer->IsAlive())
			{
				m_hPlayer->TryGiveLoadedHpAndArmor(health, suit);
			}
			else
			{
				m_hPlayer->m_pDeadLoadedEquipmentInfo->SetInt("health", health);
				m_hPlayer->m_pDeadLoadedEquipmentInfo->SetInt("suit", suit);
			}

			CreateAsyncDAO<CPlayerUpdateDAO>(m_hPlayer, INT_MAX);
			pResult = pResults->FindKey(PLAYER_AMMO_DAO_COLLECTION_NAME);

			if (pResult != NULL)
			{
				FOR_EACH_SUBKEY(pResult, pAmmoData)
				{
					HandleAmmoData(seeker.SeekAmmoIndex(pAmmoData), seeker.SeekAmmoCount(pAmmoData),
						shouldInsertAmmo);
				}
			}

			pResult = pResults->FindKey(PLAYER_WEAPON_DAO_COLLECTION_NAME);

			if (pResult != NULL)
			{
				FOR_EACH_SUBKEY(pResult, pWeaponData)
				{
					HandleWeaponData(pWeaponData, seeker.SeekWeaponClassname(pWeaponData),
						shouldInsertWeapon);
				}
			}

			m_hPlayer->m_DALSyncCaps.SetFlag(CHL2RP_Player::EDALSyncCap::SaveData);
		}

		if (m_hPlayer->IsAlive())
		{
			// Begin synchronizing final ammunition to DB, since player may had some before loading
			for (int i = 0; i < ARRAYSIZE(shouldInsertAmmo); i++)
			{
				if (m_hPlayer->GetAmmoCount(i) > 0)
				{
					if (shouldInsertAmmo[i])
					{
						CreateAsyncDAO<CPlayerAmmoInsertDAO>(m_hPlayer, i);
						continue;
					}

					CreateAsyncDAO<CPlayerAmmoUpdateDAO>(m_hPlayer, i);
				}
			}

			// Begin synchronizing final weapons to DB, since player may had some before loading
			for (int i = 0; i < ARRAYSIZE(shouldInsertWeapon); i++)
			{
				if (m_hPlayer->GetWeapon(i) != NULL)
				{
					if (shouldInsertWeapon[i])
					{
						CreateAsyncDAO<CPlayerWeaponInsertDAO>(m_hPlayer, m_hPlayer->GetWeapon(i));
						continue;
					}

					CreateAsyncDAO<CPlayerWeaponUpdateDAO>(m_hPlayer, m_hPlayer->GetWeapon(i));
				}
			}
		}

		ClientPrint(m_hPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
			Localize(m_hPlayer, "#HL2RP_Chat_Player_Loaded"));
	}
}

void CPlayerLoadDAO::HandleAmmoData(int ammoIndex, int ammoCount, bool shouldInsertAmmo[MAX_AMMO_SLOTS])
{
	Assert(m_hPlayer != NULL);

	if (ammoIndex >= 0 && ammoIndex < MAX_AMMO_SLOTS)
	{
		if (m_hPlayer->IsAlive())
		{
			m_hPlayer->GiveAmmo(ammoCount, ammoIndex, false);
			shouldInsertAmmo[ammoIndex] = false;
			return;
		}

		KeyValues* pAmmoData = m_hPlayer->m_pDeadLoadedEquipmentInfo->
			FindKey(HL2RP_PLAYER_DEADLOAD_AMMODATA_KEYNAME, true);
		m_hPlayer->m_pDeadLoadedEquipmentInfo->SetInt(NULL, ammoIndex);
		pAmmoData->SetInt(m_hPlayer->m_pDeadLoadedEquipmentInfo->GetString(), ammoCount);
	}
}

void CPlayerLoadDAO::HandleWeaponData(KeyValues* pWeaponData, const char* pWeaponClassname,
	bool shouldInsertWeapon[MAX_WEAPONS])
{
	int clip1 = pWeaponData->GetInt("clip1"), clip2 = pWeaponData->GetInt("clip2");

	if (m_hPlayer->IsAlive())
	{
		m_hPlayer->TryGiveLoadedWeapon(pWeaponClassname, clip1, clip2);

		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			CBaseCombatWeapon* pAuxWeapon = m_hPlayer->GetWeapon(i);

			if (pAuxWeapon != NULL && FClassnameIs(pAuxWeapon, pWeaponClassname))
			{
				shouldInsertWeapon[i] = false;
				break;
			}
		}
	}
	else
	{
		pWeaponData = m_hPlayer->m_pDeadLoadedEquipmentInfo->
			FindKey(HL2RP_PLAYER_DEADLOAD_WEAPONDATA_KEYNAME, true);
		pWeaponData = pWeaponData->FindKey(pWeaponClassname, true);
		pWeaponData->SetInt("clip1", clip1);
		pWeaponData->SetInt("clip2", clip2);
	}
}

CPlayerSaveDAO::CPlayerSaveDAO(CHL2RP_Player* pPlayer) : CPlayerDAO(pPlayer)
{
	Q_strcpy(m_Name, pPlayer->GetPlayerName());
	m_iSeconds = pPlayer->m_iSeconds;
	m_iCrime = pPlayer->m_iCrime;
	m_AccessFlags = pPlayer->m_AccessFlags;

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

CPlayerInsertDAO::CPlayerInsertDAO(CHL2RP_Player* pPlayer) : CPlayerSaveDAO(pPlayer)
{

}

CDAOThread::EDAOCollisionResolution CPlayerInsertDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(Equals(pDAO->ToClass(this)));
}

void CPlayerInsertDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_MAIN_DAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pPlayerData = pKVEngine->LoadFromFile(path, PLAYER_MAIN_DAO_COLLECTION_NAME, true);
	pPlayerData->SetString("steamId", m_NetworkIdString);
	pPlayerData->SetString("name", m_Name);
	pPlayerData->SetInt("seconds", m_iSeconds);
	pPlayerData->SetInt("crime", m_iCrime);
	pPlayerData->SetInt("accessFlags", m_AccessFlags);
	pPlayerData->SetInt("health", m_iHealth);
	pPlayerData->SetInt("suit", m_iArmor);
	pKVEngine->SaveToFile(pPlayerData, path);
}

void CPlayerInsertDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("INSERT INTO Player "
		"(steamId, name, seconds, crime, accessFlags, health, suit) "
		"VALUES (?, ?, %i, %i, %i, %i, %i);", m_iSeconds, m_iCrime, m_AccessFlags, m_iHealth, m_iArmor);
	pSQL->BindString(pStmt, 1, m_NetworkIdString);
	pSQL->BindString(pStmt, 2, m_Name);
	pSQL->ExecutePreparedStatement(pStmt);
}

CPlayerUpdateDAO::CPlayerUpdateDAO(CHL2RP_Player* pPlayer, int propSelectionMask)
	: CPlayerSaveDAO(pPlayer),
	m_PropSelectionFlags(propSelectionMask | CHL2RP_Player::EDALMainPropSelection::Name)
{

}

CDAOThread::EDAOCollisionResolution CPlayerUpdateDAO::Collide(IDAO* pDAO)
{
	CPlayerUpdateDAO* pPlayerUpdateDAO = pDAO->ToClass(this);

	if (Equals(pPlayerUpdateDAO))
	{
		pPlayerUpdateDAO->m_PropSelectionFlags.SetFlag(m_PropSelectionFlags);
		return CDAOThread::EDAOCollisionResolution::ReplaceQueued;
	}

	return CDAOThread::EDAOCollisionResolution::None;
}

void CPlayerUpdateDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_MAIN_DAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pPlayerData = pKVEngine->LoadFromFile(path, PLAYER_MAIN_DAO_COLLECTION_NAME, true);
	pPlayerData->SetString("steamId", m_NetworkIdString);
	pPlayerData->SetString("name", m_Name);

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Seconds))
	{
		pPlayerData->SetInt("seconds", m_iSeconds);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Crime))
	{
		pPlayerData->SetInt("crime", m_iCrime);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::AccessFlags))
	{
		pPlayerData->SetInt("accessFlags", m_AccessFlags);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Health))
	{
		pPlayerData->SetInt("health", m_iHealth);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Armor))
	{
		pPlayerData->SetInt("suit", m_iArmor);
	}

	pKVEngine->SaveToFile(pPlayerData, path);
}

void CPlayerUpdateDAO::Execute(CSQLEngine* pSQL)
{
	char query[DAL_ENGINE_SQL_QUERY_SIZE];
	int queryLen = Q_snprintf(query, sizeof(query), "UPDATE %s SET name = ?",
		PLAYER_MAIN_DAO_COLLECTION_NAME);

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Seconds))
	{
		queryLen += Q_snprintf(query + queryLen, sizeof(query) - queryLen,
			", seconds = %i", m_iSeconds);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Crime))
	{
		queryLen += Q_snprintf(query + queryLen, sizeof(query) - queryLen,
			", crime = %i", m_iCrime);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::AccessFlags))
	{
		queryLen += Q_snprintf(query + queryLen, sizeof(query) - queryLen,
			", accessFlags = %i", m_AccessFlags);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Health))
	{
		queryLen += Q_snprintf(query + queryLen, sizeof(query) - queryLen,
			", health = %i", m_iHealth);
	}

	if (m_PropSelectionFlags.IsFlagSet(CHL2RP_Player::EDALMainPropSelection::Armor))
	{
		queryLen += Q_snprintf(query + queryLen, sizeof(query) - queryLen,
			", suit = %i", m_iArmor);
	}

	Q_strncpy(query + queryLen, " WHERE steamId = ?;", sizeof(query) - queryLen);
	void* pStmt = pSQL->PrepareStatement(query);
	pSQL->BindString(pStmt, 1, m_Name);
	pSQL->BindString(pStmt, 2, m_NetworkIdString);
	pSQL->ExecutePreparedStatement(pStmt);
}

CDAOThread::EDAOCollisionResolution CPlayersAmmoInitDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

void CPlayersAmmoInitDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery(PLAYER_AMMO_DAO_SQL_SETUP_QUERY, PLAYER_AMMO_DAO_COLLECTION_NAME,
		MAX_NETWORKID_LENGTH, PLAYER_MAIN_DAO_COLLECTION_NAME);
}

CPlayerAmmoDAO::CPlayerAmmoDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerDAO(pPlayer), m_iAmmoIndex(playerAmmoIndex)
{
	Assert(m_iAmmoIndex > 0);
}

// Assumes order: CPlayerAmmoInsertDAO < CPlayerAmmoUpdateDAO < CPlayerAmmoDeleteDAO
CDAOThread::EDAOCollisionResolution CPlayerAmmoDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(Equals(pDAO->ToClass(this)));
}

bool CPlayerAmmoDAO::Equals(CPlayerAmmoDAO* pDAO)
{
	return (CPlayerDAO::Equals(pDAO) && pDAO->m_iAmmoIndex == m_iAmmoIndex);
}

CPlayerAmmoSaveDAO::CPlayerAmmoSaveDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerAmmoDAO(pPlayer, playerAmmoIndex), m_iAmmoCount(Max(pPlayer->GetAmmoCount(playerAmmoIndex), 0))
{

}

void CPlayerAmmoSaveDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_AMMO_DAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKVEngine->LoadFromFile(path, PLAYER_AMMO_DAO_COLLECTION_NAME, true);
	pResult->SetInt(NULL, m_iAmmoIndex);
	pResult->SetInt(pResult->GetString(), m_iAmmoCount);
	pKVEngine->SaveToFile(pResult, path);
}

CPlayerAmmoInsertDAO::CPlayerAmmoInsertDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerAmmoSaveDAO(pPlayer, playerAmmoIndex)
{

}

CDAOThread::EDAOCollisionResolution CPlayerAmmoInsertDAO::Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO)
{
	if (Equals(pDAO->ToClass<CPlayerAmmoDeleteDAO>()))
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return CPlayerAmmoDAO::Collide(pDAO);
}

CDAOThread::EDAOCollisionResolution CPlayerAmmoInsertDAO::Collide(CSQLEngine* pSQL, IDAO* pDAO)
{
	if (Equals(pDAO->ToClass<CPlayerAmmoDeleteDAO>()))
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return TryResolveReplacement(Equals(pDAO->ToClass(this)));
}

void CPlayerAmmoInsertDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("INSERT INTO %s (steamId, `index`, count) "
		"VALUES (?, %i, %i);", PLAYER_AMMO_DAO_COLLECTION_NAME, m_iAmmoIndex, m_iAmmoCount);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt);
}

CPlayerAmmoUpdateDAO::CPlayerAmmoUpdateDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerAmmoSaveDAO(pPlayer, playerAmmoIndex)
{

}

void CPlayerAmmoUpdateDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("UPDATE %s SET count = %i "
		"WHERE steamId = ? AND `index` = %i;",
		PLAYER_AMMO_DAO_COLLECTION_NAME, m_iAmmoCount, m_iAmmoIndex);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt);
}

CPlayerAmmoDeleteDAO::CPlayerAmmoDeleteDAO(CHL2RP_Player* pPlayer, int playerAmmoIndex)
	: CPlayerAmmoDAO(pPlayer, playerAmmoIndex)
{

}

void CPlayerAmmoDeleteDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_AMMO_DAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKVEngine->LoadFromFile(path, PLAYER_AMMO_DAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		pResult->SetInt(NULL, m_iAmmoIndex);
		KeyValues* pAmmoData = pResult->FindKey(pResult->GetString());

		if (pAmmoData != NULL)
		{
			pResult->RemoveSubKey(pAmmoData);
			pKVEngine->SaveToFile(pResult, path);
		}
	}
}

void CPlayerAmmoDeleteDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("DELETE FROM %s WHERE steamId = ? AND `index` = %i;",
		PLAYER_AMMO_DAO_COLLECTION_NAME, m_iAmmoIndex);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt);
}

CDAOThread::EDAOCollisionResolution CPlayerWeaponsInitDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

void CPlayerWeaponsInitDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery(PLAYER_WEAPON_DAO_SQL_SETUP_QUERY, PLAYER_WEAPON_DAO_COLLECTION_NAME,
		MAX_NETWORKID_LENGTH, MAX_WEAPON_STRING, PLAYER_MAIN_DAO_COLLECTION_NAME, WEAPON_NOCLIP,
		WEAPON_NOCLIP);
}

CPlayerWeaponDAO::CPlayerWeaponDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerDAO(pPlayer)
{
	Q_strcpy(m_WeaponClassname, pWeapon->GetClassname());
}

// Assumes order: CPlayerWeaponInsertDAO < CPlayerWeaponUpdateDAO < CPlayerWeaponDeleteDAO
CDAOThread::EDAOCollisionResolution CPlayerWeaponDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(Equals(pDAO->ToClass(this)));
}

bool CPlayerWeaponDAO::Equals(CPlayerWeaponDAO* pDAO)
{
	return (CPlayerDAO::Equals(pDAO) && Q_strcmp(pDAO->m_WeaponClassname, m_WeaponClassname) == 0);
}

CPlayerWeaponSaveDAO::CPlayerWeaponSaveDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerWeaponDAO(pPlayer, pWeapon), m_iClip1(Max(pWeapon->Clip1(), WEAPON_NOCLIP)),
	m_iClip2(Max(pWeapon->Clip2(), WEAPON_NOCLIP))
{

}

void CPlayerWeaponSaveDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_WEAPON_DAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKVEngine->LoadFromFile(path, PLAYER_WEAPON_DAO_COLLECTION_NAME, true);
	KeyValues* pWeaponData = pResult->FindKey(m_WeaponClassname, true);
	pWeaponData->SetInt("clip1", m_iClip1);
	pWeaponData->SetInt("clip2", m_iClip2);
	pKVEngine->SaveToFile(pResult, path);
}

CPlayerWeaponInsertDAO::CPlayerWeaponInsertDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerWeaponSaveDAO(pPlayer, pWeapon)
{

}

CDAOThread::EDAOCollisionResolution CPlayerWeaponInsertDAO::Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO)
{
	if (Equals(pDAO->ToClass<CPlayerWeaponDeleteDAO>()))
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return CPlayerWeaponDAO::Collide(pDAO);
}

CDAOThread::EDAOCollisionResolution CPlayerWeaponInsertDAO::Collide(CSQLEngine* pSQL, IDAO* pDAO)
{
	if (Equals(pDAO->ToClass<CPlayerWeaponDeleteDAO>()))
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return TryResolveReplacement(Equals(pDAO->ToClass(this)));
}

void CPlayerWeaponInsertDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("INSERT INTO %s (steamId, weapon, clip1, clip2) "
		"VALUES (?, '%s', %i, %i);", PLAYER_WEAPON_DAO_COLLECTION_NAME, m_WeaponClassname,
		m_iClip1, m_iClip2);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt);
}

CPlayerWeaponUpdateDAO::CPlayerWeaponUpdateDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerWeaponSaveDAO(pPlayer, pWeapon)
{

}

void CPlayerWeaponUpdateDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("UPDATE %s SET clip1 = %i, clip2 = %i "
		"WHERE steamId = ? AND weapon = '%s';", PLAYER_WEAPON_DAO_COLLECTION_NAME,
		m_iClip1, m_iClip2, m_WeaponClassname);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt);
}

CPlayerWeaponDeleteDAO::CPlayerWeaponDeleteDAO(CHL2RP_Player* pPlayer, CBaseCombatWeapon* pWeapon)
	: CPlayerWeaponDAO(pPlayer, pWeapon)
{

}

void CPlayerWeaponDeleteDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(PLAYER_WEAPON_DAO_COLLECTION_NAME, path, sizeof(path));
	KeyValues* pResult = pKVEngine->LoadFromFile(path, PLAYER_WEAPON_DAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		KeyValues* pWeaponData = pResult->FindKey(m_WeaponClassname);

		if (pWeaponData != NULL)
		{
			pResult->RemoveSubKey(pWeaponData);
			pKVEngine->SaveToFile(pResult, path);
		}
	}
}

void CPlayerWeaponDeleteDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("DELETE FROM %s WHERE steamId = ? AND weapon = '%s';",
		PLAYER_WEAPON_DAO_COLLECTION_NAME, m_WeaponClassname);
	BindSQLDTO(pSQL, pStmt);
	pSQL->ExecutePreparedStatement(pStmt);
}
