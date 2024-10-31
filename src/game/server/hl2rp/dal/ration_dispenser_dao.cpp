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
		CreateVarCharColumn(HL2RP_MAP_ALIAS_FIELD_NAME, MAX_PATH);
		CreateIntColumn("spawnflags");
		CreateIntColumn("rations");
		CreateFloatColumn("x");
		CreateFloatColumn("y");
		CreateFloatColumn("z");
		CreateFloatColumn("yaw");
	}
};

REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLDispenserTableSetupDTO);

class CDispensersLoadDAO : public CLevelLoadDAO
{
	void HandleCompletion() OVERRIDE;

public:
	CDispensersLoadDAO() : CLevelLoadDAO(RATION_DISPENSER_DAO_COLLECTION_NAME) {}
};

void CDispensersLoadDAO::HandleCompletion()
{
	CAutoLessFuncAdapter<CUtlRBTree<int>> dispenserIds;

	for (auto& dispenserData : *mResultDatabase.GetList(RATION_DISPENSER_DAO_COLLECTION_NAME))
	{
		int index = dispenserIds.InsertIfNotFound(dispenserData.GetInt(IDTO_PRIMARY_COLUMN_NAME));

		if (dispenserIds.IsValidIndex(index))
		{
			Vector origin(dispenserData.GetFloat("x"), dispenserData.GetFloat("y"), dispenserData.GetFloat("z"));
			QAngle angles(0.0f, dispenserData.GetFloat("yaw"), 0.0f);
			CRationDispenserProp* pDispenser = static_cast<CRationDispenserProp*>
				(CBaseEntity::CreateNoSpawn("prop_ration_dispenser", origin, angles));

			if (pDispenser != NULL)
			{
				pDispenser->mDatabaseId = dispenserIds[index];
				pDispenser->mpMapAlias = HL2RPRules()->mMapGroups.GetElementOrDefault(dispenserData
					.GetString(HL2RP_MAP_ALIAS_FIELD_NAME), STRING(gpGlobals->mapname));
				pDispenser->AddSpawnFlags(dispenserData.GetInt("spawnflags"));
				pDispenser->mRationsAmmo = dispenserData.GetInt("rations");
				DispatchSpawn(pDispenser);
			}
		}
	}
}

REGISTER_LEVEL_INIT_DAO_FACTORY(CDispensersLoadDAO, ELevelInitDAOType::Load)

CRecordNodeDTO* CDispenserDAO::AddFields(CRationDispenserProp* pDispenser, CRecordNodeDTO* pCollection, bool save)
{
	CRecordNodeDTO* pDispenserData = pCollection->AddIndexField(HL2RP_MAP_ALIAS_FIELD_NAME, pDispenser->mpMapAlias);

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
	: CDispensersSaveDAO(pDispenser)
{
	// Set for deletion to ensure old record is erased from the initial map/group file under KeyValues
	mDeleteDatabase.CreateCollection(RATION_DISPENSER_DAO_COLLECTION_NAME)
		->AddIndexField(HL2RP_MAP_ALIAS_FIELD_NAME, pOldMapAlias)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pDispenser->mDatabaseId);
}
