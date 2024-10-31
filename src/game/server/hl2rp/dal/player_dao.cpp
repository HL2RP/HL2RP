// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "player_dao.h"
#include "dal.h"
#include <hl2_roleplayer.h>
#include <hl2rp_gamerules.h>

#define PLAYER_DAO_AMMO_COLLECTION_NAME   "PlayerAmmo"
#define PLAYER_DAO_WEAPON_COLLECTION_NAME "PlayerWeapon"

#define PLAYER_DAO_ID_FOREIGN_COLUMN "playerId"

// Max. delay since player loads to print welcome message (client may not display too early messages)
#define PLAYER_DAO_WELCOME_PRINT_MAX_DELAY 2.0f

class CSQLPlayerMainTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPlayerMainTableSetupDTO() : CSQLTableSetupDTO(PLAYER_DAO_MAIN_COLLECTION_NAME)
	{
		mPrimaryKeyColumnIndices.AddToTail(CreateUInt64Column(IDTO_PRIMARY_COLUMN_NAME));
		CreateVarCharColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::Name], MAX_PLAYER_NAME_LENGTH);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::Seconds]);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::Crime]);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::Faction]);
		CreateTextColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::Job]);
		CreateTextColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::ModelGroup]);
		CreateTextColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::ModelAlias]);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::Health]);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::Armor]);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::AccessFlags]);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::MiscFlags]);
		CreateIntColumn(gPlayerDatabasePropNames[EPlayerDatabasePropType::LearnedHUDHints]);
	}
};

class CSQLPlayerAmmoTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPlayerAmmoTableSetupDTO() : CSQLTableSetupDTO(PLAYER_DAO_AMMO_COLLECTION_NAME)
	{
		int index = mPrimaryKeyColumnIndices.AddToTail(CreateUInt64Column(PLAYER_DAO_ID_FOREIGN_COLUMN));
		AddForeignKey(mPrimaryKeyColumnIndices[index], PLAYER_DAO_MAIN_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME);
		mPrimaryKeyColumnIndices.AddToTail(CreateIntColumn("type"));
		CreateIntColumn("count");
	}
};

class CSQLPlayerWeaponTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPlayerWeaponTableSetupDTO() : CSQLTableSetupDTO(PLAYER_DAO_WEAPON_COLLECTION_NAME)
	{
		int index = mPrimaryKeyColumnIndices.AddToTail(CreateUInt64Column(PLAYER_DAO_ID_FOREIGN_COLUMN));
		AddForeignKey(mPrimaryKeyColumnIndices[index], PLAYER_DAO_MAIN_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME);
		mPrimaryKeyColumnIndices.AddToTail(CreateVarCharColumn("weapon", MAX_WEAPON_STRING));
		CreateIntColumn("clip1");
		CreateIntColumn("clip2");
	}
};

REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPlayerMainTableSetupDTO)
REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPlayerAmmoTableSetupDTO)
REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPlayerWeaponTableSetupDTO)

CPlayerLoadDAO::CPlayerLoadDAO(CBasePlayer* pPlayer) : mSteamIdNumber(pPlayer->GetSteamIDAsUInt64())
{
	mQueryDatabase.CreateCollection(PLAYER_DAO_MAIN_COLLECTION_NAME)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, mSteamIdNumber);
	mQueryDatabase.CreateCollection(PLAYER_DAO_AMMO_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_ID_FOREIGN_COLUMN, mSteamIdNumber);
	mQueryDatabase.CreateCollection(PLAYER_DAO_WEAPON_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_ID_FOREIGN_COLUMN, mSteamIdNumber);
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

	bool updateMainData = pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::UpdateMainDataOnLoaded);
	CRecordListDTO* pMainData = mResultDatabase.GetList(PLAYER_DAO_MAIN_COLLECTION_NAME);

	if (pMainData->IsEmpty())
	{
		pPlayer->mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::IsNewCitizenPrintPending);
	}
	else
	{
		CFieldDictionaryDTO& mainData = pMainData->Head();
		pPlayer->mSeconds += mainData.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::Seconds]);
		pPlayer->mCrime += mainData.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::Crime]);
		int faction = Clamp(mainData.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::Faction]),
			0, EFaction::_Count - 1), jobHealth = 0;
		pPlayer->mAccessFlags.SetFlag(mainData.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::AccessFlags]));
		pPlayer->mMiscFlags.SetFlag(mainData.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::MiscFlags]));
		pPlayer->mRestorableEquipment.Delete(); // Delete old, just in case (avoid merging, for now)
		CRestorablePlayerEquipment* pEquipment = new CRestorablePlayerEquipment(pPlayer, false);
		pPlayer->mRestorableEquipment.Attach(pEquipment);
		pEquipment->mHealth = mainData.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::Health]);
		pEquipment->mArmor = mainData.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::Armor]);

		for (auto& ammo : *mResultDatabase.GetList(PLAYER_DAO_AMMO_COLLECTION_NAME))
		{
			pEquipment->mAmmoCountByIndex.InsertOrReplace(ammo.GetInt("type"), ammo.GetInt("count"));
		}

		for (auto& weapon : *mResultDatabase.GetList(PLAYER_DAO_WEAPON_COLLECTION_NAME))
		{
			pEquipment->AddWeapon(weapon.GetString("weapon"), weapon.GetInt("clip1"), weapon.GetInt("clip2"));
		}

		if (!updateMainData && Q_strcmp(pPlayer->GetPlayerName(),
			mainData.GetString(gPlayerDatabasePropNames[EPlayerDatabasePropType::Name])) != 0)
		{
			DAL().AddDAO(new CPlayersMainDataSaveDAO(pPlayer, pPlayer->GetPlayerName(), EPlayerDatabasePropType::Name));
		}

		// Override faction/job only if player wasn't explicitly assigned a job during load, for fairness
		if (pPlayer->mJobName.Get() == NULL_STRING && pPlayer->HasFactionAccess(faction))
		{
			pPlayer->mFaction = faction;
			CUtlString jobName = mainData.GetString(gPlayerDatabasePropNames[EPlayerDatabasePropType::Job]);
			pPlayer->mJobName = MAKE_STRING(jobName);
			auto& jobs = HL2RPRules()->mJobByName[faction];
			int index = pPlayer->CheckCurrentJobAccess();

			if (jobs.IsValidIndex(index))
			{
				pPlayer->mJobName = MAKE_STRING(jobs.Key(index)); // Ensure pointing to original memory

				if (pPlayer->IsAlive())
				{
					jobHealth = jobs[index]->mHealth;
					jobs[index]->Equip(pPlayer);
				}
			}
		}

		if (pPlayer->mFaction == EFaction::Citizen && pPlayer->IsAlive())
		{
			pEquipment->Restore(pPlayer, jobHealth);
		}

		if (pPlayer->mModelGroup.Get() == NULL_STRING)
		{
			auto& models = HL2RPRules()->mPlayerModelsByGroup;
			int index = models.Find(mainData.GetString(gPlayerDatabasePropNames[EPlayerDatabasePropType::ModelGroup]));

			if (models.IsValidIndex(index) && pPlayer->HasModelGroupAccess(index))
			{
				// Search last player model in the registry, to keep if possible. Otherwise, set a random one.
				int aliasIndex = models[index]->Find(mainData
					.GetString(gPlayerDatabasePropNames[EPlayerDatabasePropType::ModelAlias]));

				if (!models[index]->IsValidIndex(aliasIndex))
				{
					aliasIndex = RandomInt(0, models[index]->Count() - 1);
				}

				pPlayer->SetModel(index, aliasIndex);
			}
		}

#ifdef HL2RP_LEGACY
		pPlayer->mLearnedHUDHints.SetFlag(mainData
			.GetInt(gPlayerDatabasePropNames[EPlayerDatabasePropType::LearnedHUDHints]));
#endif // HL2RP_LEGACY
	}

	if (updateMainData)
	{
		DAL().AddDAO(new CPlayersMainDataSaveDAO(pPlayer));
		pPlayer->mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::UpdateMainDataOnLoaded);
	}

	if (!pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled) && pPlayer->IsAlive())
	{
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
		}
	}

	// Fix team here as player must be alive for the above updates to work correctly
	if (pPlayer->GetTeamNumber() != TEAM_SPECTATOR)
	{
		pPlayer->ChangeTeam();
	}

	pPlayer->mDatabaseIOFlags.ClearBits(EPlayerDatabaseIOFlag::UpdateAmmunitionOnLoaded,
		EPlayerDatabaseIOFlag::UpdateWeaponsOnLoaded);
	pPlayer->mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::IsLoaded);
	pPlayer->SetContextThink(&CHL2Roleplayer::PrintWelcomeInfo,
		pPlayer->mFirstSpawnTime + PLAYER_DAO_WELCOME_PRINT_MAX_DELAY, "WelcomePrintThink");
}

CPlayersMainDataSaveDAO::CPlayersMainDataSaveDAO(CHL2Roleplayer* pPlayer)
{
	CRecordNodeDTO* pMainData = CreateRecord(pPlayer);
	AddField(pMainData, pPlayer->GetPlayerName(), EPlayerDatabasePropType::Name);
	AddField(pMainData, pPlayer->mSeconds->Get(), EPlayerDatabasePropType::Seconds);
	AddField(pMainData, pPlayer->mCrime->Get(), EPlayerDatabasePropType::Crime);
	AddField(pMainData, pPlayer->mFaction->Get(), EPlayerDatabasePropType::Faction);
	AddField(pMainData, pPlayer->mJobName.Get(), EPlayerDatabasePropType::Job);
	AddField(pMainData, pPlayer->mModelGroup.Get(), EPlayerDatabasePropType::ModelGroup);
	AddField(pMainData, pPlayer->mModelAlias.Get(), EPlayerDatabasePropType::ModelAlias);
	AddField(pMainData, pPlayer->mAccessFlags.Get(), EPlayerDatabasePropType::AccessFlags);
	AddField(pMainData, pPlayer->mMiscFlags.Get(), EPlayerDatabasePropType::MiscFlags);
	AddField(pMainData, pPlayer->mLearnedHUDHints.Get(), EPlayerDatabasePropType::LearnedHUDHints);

	if (!pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled) && pPlayer->IsAlive())
	{
		AddField(pMainData, pPlayer->GetHealth(), EPlayerDatabasePropType::Health);
		AddField(pMainData, pPlayer->ArmorValue(), EPlayerDatabasePropType::Armor);
	}
}

CPlayersMainDataSaveDAO::CPlayersMainDataSaveDAO(CBasePlayer* pPlayer,
	const SUtlField& field, EPlayerDatabasePropType type)
{
	AddField(CreateRecord(pPlayer), field, type);
}

CRecordNodeDTO* CPlayersMainDataSaveDAO::CreateRecord(CBasePlayer* pPlayer)
{
	return mSaveDatabase.CreateCollection(PLAYER_DAO_MAIN_COLLECTION_NAME)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pPlayer->GetSteamIDAsUInt64());
}

void CPlayersMainDataSaveDAO::AddField(CRecordNodeDTO* pMainData, const SUtlField& field, EPlayerDatabasePropType type)
{
	pMainData->AddNormalField(gPlayerDatabasePropNames[type], field);
}

CPlayersAmmunitionSaveDAO::CPlayersAmmunitionSaveDAO(CBasePlayer* pPlayer, int index, int count)
{
	bool save = count > 0;
	CRecordNodeDTO* pAmmoData = GetCorrectDatabase(save).CreateCollection(PLAYER_DAO_AMMO_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_ID_FOREIGN_COLUMN, pPlayer->GetSteamIDAsUInt64())->AddIndexField("type", index);

	if (save)
	{
		pAmmoData->AddNormalField("count", count);
	}
}

CPlayersWeaponsSaveDAO::CPlayersWeaponsSaveDAO(CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon, bool save)
{
	CRecordNodeDTO* pWeaponData = GetCorrectDatabase(save).CreateCollection(PLAYER_DAO_WEAPON_COLLECTION_NAME)
		->AddIndexField(PLAYER_DAO_ID_FOREIGN_COLUMN, pPlayer->GetSteamIDAsUInt64())
		->AddIndexField("weapon", pWeapon->GetClassname());

	if (save)
	{
		pWeaponData->AddNormalField("clip1", pWeapon->Clip1());
		pWeaponData->AddNormalField("clip2", pWeapon->Clip2());
	}
}
