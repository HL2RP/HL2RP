// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "ration_dispenser_dao.h"
#include <hl2rp_gamerules.h>
#include <prop_ration_dispenser.h>

#define RATION_DISPENSER_DAO_COLLECTION_NAME "Dispenser"

class CSQLDispenserTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLDispenserTableSetupDTO() : CSQLTableSetupDTO(RATION_DISPENSER_DAO_COLLECTION_NAME, true)
	{
		mPrimaryKeyColumnIndices.AddToTail(CreateIntColumn(IDTO_PRIMARY_COLUMN_NAME));
		CreateVarCharColumn("map", MAX_MAP_NAME_SAVE);
		CreateIntColumn("spawnflags");
		CreateIntColumn("rations");
		CreateFloatColumn("x");
		CreateFloatColumn("y");
		CreateFloatColumn("z");
		CreateFloatColumn("yaw");
	}
};

REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLDispenserTableSetupDTO);

class CDispensersLoadDAO : public CLoadDAO
{
	void HandleCompletion() OVERRIDE;

public:
	CDispensersLoadDAO()
	{
		CRecordNodeDTO* pCollection = mQueryDatabase.CreateCollection(RATION_DISPENSER_DAO_COLLECTION_NAME);
		pCollection->AddIndexField("map", STRING(gpGlobals->mapname));

		FOR_EACH_DICT_FAST(HL2RPRules()->mMapGroups, i)
		{
			pCollection->AddIndexField("map", HL2RPRules()->mMapGroups[i]);
		}
	}
};

void CDispensersLoadDAO::HandleCompletion()
{
	CRecordListDTO* pDispensersData = mResultDatabase.GetPtr(RATION_DISPENSER_DAO_COLLECTION_NAME);

	for (auto& dispenserData : *pDispensersData)
	{
		Vector origin(dispenserData.GetFloat("x"), dispenserData.GetFloat("y"), dispenserData.GetFloat("z"));
		QAngle angles(0.0f, dispenserData.GetFloat("yaw"), 0.0f);
		CRationDispenserProp* pDispenser = static_cast<CRationDispenserProp*>
			(CBaseEntity::CreateNoSpawn("prop_ration_dispenser", origin, angles));

		if (pDispenser != NULL)
		{
			pDispenser->mDatabaseId = dispenserData.GetInt(IDTO_PRIMARY_COLUMN_NAME);
			pDispenser->mpMapAlias = HL2RPRules()->mMapGroups
				.GetElementOrDefault(dispenserData.GetField("map"), STRING(gpGlobals->mapname));
			pDispenser->AddSpawnFlags(dispenserData.GetInt("spawnflags"));
			pDispenser->mRationsAmmo = dispenserData.GetInt("rations");
			DispatchSpawn(pDispenser);
		}
	}
}

REGISTER_LEVEL_INIT_DAO_FACTORY(CDispensersLoadDAO, ELevelInitDAOType::Load);

CRecordNodeDTO* CDispenserDAO::AddFields(CRationDispenserProp* pDispenser, CRecordNodeDTO* pCollection, bool save)
{
	CRecordNodeDTO* pDispenserData = pCollection->AddIndexField("map", pDispenser->mpMapAlias);

	if (save)
	{
		const Vector& origin = pDispenser->GetAbsOrigin();
		pDispenserData->AddNormalField("spawnflags", pDispenser->GetSpawnFlags());
		pDispenserData->AddNormalField("rations", pDispenser->mRationsAmmo);
		pDispenserData->AddNormalField("x", origin.x);
		pDispenserData->AddNormalField("y", origin.y);
		pDispenserData->AddNormalField("z", origin.z);
		pDispenserData->AddNormalField("yaw", pDispenser->GetAbsAngles().y);
	}

	return pDispenserData;
}

CDispenserInsertDAO::CDispenserInsertDAO(CRationDispenserProp* pDispenser)
	: CAutoIncrementInsertDAO(RATION_DISPENSER_DAO_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME), mhDispenser(pDispenser)
{
	AddFields(pDispenser, mpRecord, true);
}

void CDispenserInsertDAO::HandleCompletion()
{
	if (mhDispenser != NULL)
	{
		mhDispenser->mDatabaseId = mResultId;
	}
}

CDispensersSaveDAO::CDispensersSaveDAO(CRationDispenserProp* pDispenser, bool save) : CSaveDeleteDAO(false)
{
	AddFields(pDispenser, GetCorrectDatabase(save).CreateCollection(RATION_DISPENSER_DAO_COLLECTION_NAME), save)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pDispenser->mDatabaseId);
}

CDispensersSaveDAO::CDispensersSaveDAO(CRationDispenserProp* pDispenser, const char* pOldMapAlias)
	: CDispensersSaveDAO(pDispenser, true)
{
	// Set for deletion to ensure old record is erased from the initial map/group file under KeyValues
	mDeleteDatabase.CreateCollection(RATION_DISPENSER_DAO_COLLECTION_NAME)->AddIndexField("map", pOldMapAlias)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pDispenser->mDatabaseId);
}
