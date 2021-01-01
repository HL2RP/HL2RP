// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "player_dao.h"
#include "dal.h"
#include <hl2_roleplayer.h>

#define PLAYER_DAO_MAIN_COLLECTION_NAME   "Player"
#define PLAYER_DAO_AMMO_COLLECTION_NAME   "PlayerAmmo"
#define PLAYER_DAO_WEAPON_COLLECTION_NAME "PlayerWeapon"

#define PLAYER_DAO_PLAYER_ID_FOREIGN_COLUMN "playerId"

class CSQLPlayerMainTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPlayerMainTableSetupDTO() : CSQLTableSetupDTO(PLAYER_DAO_MAIN_COLLECTION_NAME)
	{
		mPrimaryKeyColumnIndices.AddToTail(CreateUInt64Column(IDTO_PRIMARY_COLUMN_NAME));
		CreateVarCharColumn("name", MAX_PLAYER_NAME_LENGTH);
		CreateIntColumn("seconds");
		CreateIntColumn("crime");
		CreateIntColumn("accessFlags");
		CreateIntColumn("health");
		CreateIntColumn("armor");
		CreateIntColumn("miscFlags");

#ifdef HL2RP_LEGACY
		CreateIntColumn("sentHUDHints");
#endif // HL2RP_LEGACY
	}
};

class CSQLPlayerAmmoTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPlayerAmmoTableSetupDTO() : CSQLTableSetupDTO(PLAYER_DAO_AMMO_COLLECTION_NAME)
	{
		int index = mPrimaryKeyColumnIndices.AddToTail(CreateUInt64Column(PLAYER_DAO_PLAYER_ID_FOREIGN_COLUMN));
		AddForeignKey(index, PLAYER_DAO_MAIN_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME);
		mPrimaryKeyColumnIndices.AddToTail(CreateIntColumn("type"));
		CreateIntColumn("count");
	}
};

class CSQLPlayerWeaponTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPlayerWeaponTableSetupDTO() : CSQLTableSetupDTO(PLAYER_DAO_WEAPON_COLLECTION_NAME)
	{
		int index = mPrimaryKeyColumnIndices.AddToTail(CreateUInt64Column(PLAYER_DAO_PLAYER_ID_FOREIGN_COLUMN));
		AddForeignKey(index, PLAYER_DAO_MAIN_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME);
		mPrimaryKeyColumnIndices.AddToTail(CreateVarCharColumn("weapon", MAX_WEAPON_STRING));
		CreateIntColumn("clip1");
		CreateIntColumn("clip2");
	}
};

REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPlayerMainTableSetupDTO)
REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPlayerAmmoTableSetupDTO)
REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPlayerWeaponTableSetupDTO)

CPlayerLoadDAO::CPlayerLoadDAO(CHL2Roleplayer* pPlayer) : mSteamIdNumber(pPlayer->GetSteamIDAsUInt64())
{
	mQueryDatabase.AddCollection(PLAYER_DAO_MAIN_COLLECTION_NAME)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, mSteamIdNumber);
	mQueryDatabase.AddCollection(PLAYER_DAO_AMMO_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_PLAYER_ID_FOREIGN_COLUMN, mSteamIdNumber);
	mQueryDatabase.AddCollection(PLAYER_DAO_WEAPON_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_PLAYER_ID_FOREIGN_COLUMN, mSteamIdNumber);
}

bool CPlayerLoadDAO::MergeFrom(IDAO* pDAO)
{
	return (pDAO->As(this)->mSteamIdNumber == mSteamIdNumber);
}

void CPlayerLoadDAO::HandleCompletion()
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_PlayerBySteamID(mSteamIdNumber));

	if (pPlayer == NULL || pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
	{
		return;
	}

	pPlayer->mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::IsLoaded);
	pPlayer->m_takedamage = DAMAGE_AIM;
	CRecordListDTO* pMainData = mResultDatabase.GetPtr(PLAYER_DAO_MAIN_COLLECTION_NAME);
	CRecordListDTO* pAmmunition = mResultDatabase.GetPtr(PLAYER_DAO_AMMO_COLLECTION_NAME);
	CRecordListDTO* pWeapons = mResultDatabase.GetPtr(PLAYER_DAO_WEAPON_COLLECTION_NAME);
	CRestorablePlayerEquipment equipment(pPlayer);

	if (!pMainData->IsEmpty())
	{
		CFieldDictionaryDTO& mainData = pMainData->Head();
		pPlayer->mSeconds += mainData.GetInt("seconds");
		pPlayer->mCrime += mainData.GetInt("crime");
		pPlayer->mAccessFlags.SetFlag(mainData.GetInt("accessFlags"));
		equipment.mHealth = mainData.GetInt("health");
		equipment.mArmor += mainData.GetInt("armor");
		pPlayer->mMiscFlags.SetFlag(mainData.GetInt("miscFlags"));

#ifdef HL2RP_LEGACY
		pPlayer->mSentHUDHints.SetFlag(mainData.GetInt("sentHUDHints"));
#endif // HL2RP_LEGACY
	}

	for (auto& ammo : *pAmmunition)
	{
		equipment.mAmmoCountByIndex.InsertOrReplace(ammo.GetInt("type"), ammo.GetInt("count"));
	}

	for (auto& weapon : *pWeapons)
	{
		const char* pWeaponName = weapon.GetString("weapon");
		int index = equipment.mClipsByWeaponClassname.Find(pWeaponName);

		if (!equipment.mClipsByWeaponClassname.IsValidIndex(index))
		{
			index = equipment.mClipsByWeaponClassname.Insert(pWeaponName);
		}

		equipment.mClipsByWeaponClassname[index].first = Max(0, weapon.GetInt("clip1"));
		equipment.mClipsByWeaponClassname[index].second = Max(0, weapon.GetInt("clip2"));
	}

	equipment.Restore(pPlayer);

	if (pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::UpdateMainDataOnLoaded))
	{
		DAL().AddDAO(new CPlayersMainDataSaveDAO(pPlayer));
		pPlayer->mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::UpdateMainDataOnLoaded);
	}

	if (pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::UpdateAmmunitionOnLoaded))
	{
		for (int i = 0; i < MAX_AMMO_SLOTS; ++i)
		{
			int count = pPlayer->GetAmmoCount(i);

			if (count > 0)
			{
				DAL().AddDAO(new CPlayersAmmunitionSaveDAO(pPlayer, i, count));
			}
		}

		pPlayer->mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::UpdateAmmunitionOnLoaded);
	}

	if (pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::UpdateWeaponsOnLoaded))
	{
		for (int i = 0; i < pPlayer->WeaponCount(); ++i)
		{
			if (pPlayer->GetWeapon(i) != NULL)
			{
				DAL().AddDAO(new CPlayersWeaponsSaveDAO(pPlayer, pPlayer->GetWeapon(i), true));
			}
		}

		pPlayer->mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::UpdateWeaponsOnLoaded);
	}
}

CPlayersMainDataSaveDAO::CPlayersMainDataSaveDAO(CHL2Roleplayer* pPlayer, const CBitFlags<>& selectedProps)
{
	CRecordNodeDTO* pMainData = mSaveDatabase.AddCollection(PLAYER_DAO_MAIN_COLLECTION_NAME)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pPlayer->GetSteamIDAsUInt64());

	if (selectedProps.IsBitSet(EPlayerDatabasePropType::Name))
	{
		pMainData->AddNormalField("name", pPlayer->GetPlayerName());
	}

	if (selectedProps.IsBitSet(EPlayerDatabasePropType::Seconds))
	{
		pMainData->AddNormalField("seconds", pPlayer->mSeconds);
	}

	if (selectedProps.IsBitSet(EPlayerDatabasePropType::Crime))
	{
		pMainData->AddNormalField("crime", pPlayer->mCrime);
	}

	if (selectedProps.IsBitSet(EPlayerDatabasePropType::AccessFlags))
	{
		pMainData->AddNormalField("accessFlags", pPlayer->mAccessFlags);
	}

	if (selectedProps.IsBitSet(EPlayerDatabasePropType::Health))
	{
		pMainData->AddNormalField("health", pPlayer->GetHealth());
	}

	if (selectedProps.IsBitSet(EPlayerDatabasePropType::Armor))
	{
		pMainData->AddNormalField("armor", pPlayer->ArmorValue());
	}

	if (selectedProps.IsBitSet(EPlayerDatabasePropType::MiscFlags))
	{
		pMainData->AddNormalField("miscFlags", pPlayer->mMiscFlags);
	}

#ifdef HL2RP_LEGACY
	if (selectedProps.IsBitSet(EPlayerDatabasePropType::SentHUDHints))
	{
		pMainData->AddNormalField("sentHUDHints", pPlayer->mSentHUDHints);
	}
#endif // HL2RP_LEGACY
}

CPlayersAmmunitionSaveDAO::CPlayersAmmunitionSaveDAO(CHL2Roleplayer* pPlayer, int index, int count)
{
	bool save = count > 0;
	CRecordNodeDTO* pAmmoData = GetCorrectDatabase(save).AddCollection(PLAYER_DAO_AMMO_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_PLAYER_ID_FOREIGN_COLUMN, pPlayer->GetSteamIDAsUInt64())
		->AddIndexField("type", index);

	if (save)
	{
		pAmmoData->AddNormalField("count", count);
	}
}

CPlayersWeaponsSaveDAO::CPlayersWeaponsSaveDAO(CHL2Roleplayer* pPlayer, CBaseCombatWeapon* pWeapon, bool save)
{
	CRecordNodeDTO* pWeaponData = GetCorrectDatabase(save).AddCollection(PLAYER_DAO_WEAPON_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_PLAYER_ID_FOREIGN_COLUMN, pPlayer->GetSteamIDAsUInt64())
		->AddIndexField("weapon", pWeapon->GetClassname());

	if (save)
	{
		pWeaponData->AddNormalField("clip1", pWeapon->Clip1());
		pWeaponData->AddNormalField("clip2", pWeapon->Clip2());
	}
}
