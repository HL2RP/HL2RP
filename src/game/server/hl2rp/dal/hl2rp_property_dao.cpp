// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_property_dao.h"
#include "dal.h"
#include "player_dao.h"
#include <hl2_roleplayer.h>
#include <hl2rp_gamerules.h>
#include <hl2rp_property.h>
#include <toolframework/itoolentity.h>

#define HL2RP_PROPERTY_DAO_COLLECTION_NAME       "Property"
#define HL2RP_PROPERTY_DAO_GRANT_COLLECTION_NAME "PropertyGrant"
#define HL2RP_PROPERTY_DAO_DOOR_COLLECTION_NAME  "PropertyDoor"

#define HL2RP_PROPERTY_DAO_FOREIGN_COLUMN "propertyId"

class CSQLPropertyTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPropertyTableSetupDTO() : CSQLTableSetupDTO(HL2RP_PROPERTY_DAO_COLLECTION_NAME, true)
	{
		mPrimaryKeyColumnIndices.AddToTail(CreateIntColumn(IDTO_PRIMARY_COLUMN_NAME));
		CreateVarCharColumn(HL2RP_MAP_ALIAS_FIELD_NAME, MAX_PATH);
		CreateIntColumn("type");
		CreateVarCharColumn("name", HL2RP_PROPERTY_NAME_SIZE);
		AddForeignKey(CreateUInt64Column("ownerId"), PLAYER_DAO_MAIN_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME, false);
		CreateIntColumn("ownerLastSeenTime");
		CreateIntColumn("hasZone");
		CreateTextColumn("zoneTargetName");
		CreateFloatColumn("zoneX");
		CreateFloatColumn("zoneY");
		CreateFloatColumn("zoneZ");
	}
};

class CSQLPropertyGrantTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPropertyGrantTableSetupDTO() : CSQLTableSetupDTO(HL2RP_PROPERTY_DAO_GRANT_COLLECTION_NAME)
	{
		int index = mPrimaryKeyColumnIndices.AddToTail(CreateIntColumn(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN));
		AddForeignKey(mPrimaryKeyColumnIndices[index], HL2RP_PROPERTY_DAO_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME);
		index = mPrimaryKeyColumnIndices.AddToTail(CreateUInt64Column("grantedId"));
		AddForeignKey(mPrimaryKeyColumnIndices[index], PLAYER_DAO_MAIN_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME);
	}
};

class CSQLPropertyDoorTableSetupDTO : public CSQLTableSetupDTO
{
public:
	CSQLPropertyDoorTableSetupDTO() : CSQLTableSetupDTO(HL2RP_PROPERTY_DAO_DOOR_COLLECTION_NAME, true)
	{
		mPrimaryKeyColumnIndices.AddToTail(CreateIntColumn(IDTO_PRIMARY_COLUMN_NAME));
		AddForeignKey(CreateIntColumn(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN),
			HL2RP_PROPERTY_DAO_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME);
		CreateVarCharColumn("origMap", MAX_MAP_NAME_SAVE);
		CreateVarCharColumn("name", HL2RP_PROPERTY_NAME_SIZE);
		CreateIntColumn("isLocked");
		CreateIntColumn("hammerId");
		CreateTextColumn("targetName");
		CreateFloatColumn("x");
		CreateFloatColumn("y");
		CreateFloatColumn("z");
	}
};

REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPropertyTableSetupDTO)
REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPropertyGrantTableSetupDTO)
REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(CSQLPropertyDoorTableSetupDTO)

static CCityZone* FindZone(const CUtlString& targetName, const Vector& samplePoint)
{
	CCityZone* pZone = NULL;
	ECityZoneType maxType = ECityZoneType::None;

	for (CCityZone* pAuxZone = NULL; (pAuxZone = gEntList.NextEntByClass(pAuxZone)) != NULL;)
	{
		// NOTE: To avoid too detailed logic, check point intersection regardless of targetname presence
		if (pAuxZone->mType > maxType && (targetName.IsEmpty() || pAuxZone->NameMatches(targetName))
			&& pAuxZone->IsPointWithin(samplePoint))
		{
			pZone = pAuxZone;
			maxType = pZone->mType;
		}
	}

	return pZone;
}

static const Vector& GetDoorSearchOrigin(CBaseEntity* pDoor)
{
	CBaseToggle* pBaseToggle = dynamic_cast<CBaseToggle*>(pDoor);
	return (pBaseToggle != NULL ? pBaseToggle->m_vecPosition1 : pDoor->GetAbsOrigin());
}

static bool FindDoor(const CUtlString& targetName, const Vector& origin, CBaseEntity*& pDoor)
{
	pDoor = NULL;
	vec_t minDist = FLT_MAX;

	for (CBaseEntity* pEntity = NULL; (pEntity = gEntList.NextEnt(pEntity)) != NULL;)
	{
		if (UTIL_IsPropertyDoor(pEntity) && (targetName.IsEmpty() || pEntity->NameMatches(targetName)))
		{
			vec_t dist = origin.DistToSqr(GetDoorSearchOrigin(pEntity));

			if (dist < minDist)
			{
				minDist = dist;
				pDoor = pEntity;
			}
		}
	}

	return (pDoor != NULL && (!targetName.IsEmpty() || minDist < HL2RP_ENTITY_SEARCH_RADIUS_SQR));
}

class CPropertiesLoadDAO : public CLevelLoadDAO
{
	void ExecuteIO(CKeyValuesDriver*) OVERRIDE;
	void ExecuteIO(ISQLDriver*) OVERRIDE;
	void HandleCompletion() OVERRIDE;

	template<class T>
	void InternalExecuteIO(T* pDriver)
	{
		CLoadDAO::ExecuteIO(pDriver);
		AddSubDataToQuery();
		CLoadDAO::ExecuteIO(pDriver);
		AddGrantedPlayersToQuery();
		CLoadDAO::ExecuteIO(pDriver);
	}

	void AddSubDataToQuery();
	void AddGrantedPlayersToQuery();

public:
	CPropertiesLoadDAO() : CLevelLoadDAO(HL2RP_PROPERTY_DAO_COLLECTION_NAME) {}
};

void CPropertiesLoadDAO::ExecuteIO(CKeyValuesDriver* pDriver)
{
	InternalExecuteIO(pDriver);
}

void CPropertiesLoadDAO::ExecuteIO(ISQLDriver* pDriver)
{
	InternalExecuteIO(pDriver);
}

void CPropertiesLoadDAO::AddSubDataToQuery()
{
	mQueryDatabase.PurgeAndDeleteElements();
	CRecordListDTO* pPropertiesData = mResultDatabase.GetList(HL2RP_PROPERTY_DAO_COLLECTION_NAME);

	if (!pPropertiesData->IsEmpty())
	{
		CRecordNodeDTO* pOwnersData = mQueryDatabase.CreateCollection(PLAYER_DAO_MAIN_COLLECTION_NAME),
			* pGrantsData = mQueryDatabase.CreateCollection(HL2RP_PROPERTY_DAO_GRANT_COLLECTION_NAME),
			* pDoorsData = mQueryDatabase.CreateCollection(HL2RP_PROPERTY_DAO_DOOR_COLLECTION_NAME);

		for (auto& propertyData : *pPropertiesData)
		{
			int propertyId = propertyData.GetInt(IDTO_PRIMARY_COLUMN_NAME);
			pOwnersData->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, propertyData.GetField("ownerId"));
			pGrantsData->AddIndexField(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN, propertyId);
			pDoorsData->AddIndexField(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN, propertyId);
		}
	}
}

void CPropertiesLoadDAO::AddGrantedPlayersToQuery()
{
	mQueryDatabase.PurgeAndDeleteElements();
	CRecordListDTO* pGrantsData = mResultDatabase.GetList(HL2RP_PROPERTY_DAO_GRANT_COLLECTION_NAME);

	if (pGrantsData != NULL && !pGrantsData->IsEmpty())
	{
		CRecordNodeDTO* pGrantedPlayersData = mQueryDatabase.CreateCollection(PLAYER_DAO_MAIN_COLLECTION_NAME);

		for (auto& grantData : *pGrantsData)
		{
			pGrantedPlayersData->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, grantData.GetField("grantedId"));
		}
	}
}

void CPropertiesLoadDAO::HandleCompletion()
{
	HL2RPRules()->mDatabaseIOFlags.SetBit(EHL2RPDatabaseIOFlag::ArePropertiesLoaded);
	CRecordListDTO* pPropertiesData = mResultDatabase.GetList(HL2RP_PROPERTY_DAO_COLLECTION_NAME);

	if (!pPropertiesData->IsEmpty()) // Implicitly check if all other related result lists exist
	{
		CAutoLessFuncAdapter<CUtlMap<int, CHL2RP_Property*>> propertyById;
		IServerTools* pTools = static_cast<IServerTools*>(CreateInterface(VSERVERTOOLS_INTERFACE_VERSION, NULL));

		for (auto& propertyData : *pPropertiesData)
		{
			int propertyId = propertyData.GetInt(IDTO_PRIMARY_COLUMN_NAME);

			if (!propertyById.IsValidIndex(propertyById.Find(propertyId)))
			{
				CHL2RP_Property* pProperty = new CHL2RP_Property(HL2RPRules()->mMapGroups
					.GetElementOrDefault(propertyData.GetString(HL2RP_MAP_ALIAS_FIELD_NAME),
						STRING(gpGlobals->mapname)), propertyData.GetInt("type"));
				pProperty->mDatabaseId = propertyId;
				V_strcpy_safe(pProperty->mName, UTIL_TrimQuotableString(propertyData.GetString("name")));
				propertyById.Insert(propertyId, pProperty);
				HL2RPRules()->mProperties.Insert(pProperty);

				// For the shake of further consistency and efficiency, limit owner data to home properties
				if (pProperty->mType == EHL2RP_PropertyType::Home)
				{
					pProperty->mOwnerSteamIdNumber = propertyData.GetUInt64("ownerId");
					pProperty->mOwnerLastSeenTime = propertyData.GetInt("ownerLastSeenTime");
				}

				if (propertyData.GetInt("hasZone") > 0)
				{
					Vector samplePoint(propertyData.GetFloat("zoneX"),
						propertyData.GetFloat("zoneY"), propertyData.GetFloat("zoneZ"));
					CUtlString targetName = propertyData.GetString("zoneTargetName");
					targetName.Trim();
					CCityZone* pZone = FindZone(targetName, samplePoint);

					if ((pZone != NULL || (!targetName.IsEmpty()
						&& (pZone = FindZone("", samplePoint)) != NULL)) && pZone->mpProperty == NULL)
					{
						pProperty->LinkZone(pZone, samplePoint);
					}
				}

				pProperty->Synchronize(true, false);
			}
		}

		for (auto& playerData : *mResultDatabase.GetList(PLAYER_DAO_MAIN_COLLECTION_NAME))
		{
			HL2RPRules()->AddPlayerName(playerData.GetUInt64(IDTO_PRIMARY_COLUMN_NAME), playerData.GetString("name"));
		}

		for (auto& grantData : *mResultDatabase.GetList(HL2RP_PROPERTY_DAO_GRANT_COLLECTION_NAME))
		{
			uint64 grantedId = grantData.GetUInt64("grantedId");
			int index = propertyById.Find(grantData.GetInt(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN));

			if (propertyById.IsValidIndex(index) && propertyById[index]->HasOwner()
				&& propertyById[index]->mOwnerSteamIdNumber != grantedId)
			{
				int grantIndex = propertyById[index]->mGrantedSteamIdNumbers.InsertIfNotFound(grantedId);

				if (propertyById[index]->mGrantedSteamIdNumbers.IsValidIndex(grantIndex))
				{
					propertyById[index]->SendSteamIdGrantToPlayers(grantedId);
				}
			}
		}

		for (auto& doorData : *mResultDatabase.GetList(HL2RP_PROPERTY_DAO_DOOR_COLLECTION_NAME))
		{
			int index = propertyById.Find(doorData.GetInt(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN));

			if (propertyById.IsValidIndex(index))
			{
				CBaseEntity* pDoor = NULL;
				Vector origin(doorData.GetFloat("x"), doorData.GetFloat("y"), doorData.GetFloat("z"));

				// First, search door by Hammer ID only if we're in the original map that door was saved at.
				// If we fail, then try via targetname and finally via origin within a small radius.
				if (Q_stricmp(doorData.GetString("origMap"), STRING(gpGlobals->mapname)) != 0
					|| !UTIL_IsPropertyDoor(pDoor = pTools->FindEntityByHammerID(doorData.GetInt("hammerId"))))
				{
					CUtlString targetName = doorData.GetString("targetName");
					targetName.Trim();

					if (!FindDoor(targetName, origin, pDoor) && (targetName.IsEmpty() || !FindDoor("", origin, pDoor)))
					{
						continue;
					}
				}

				CHL2RP_PropertyDoorData* pPropertyData = pDoor->GetPropertyDoorData();

				if (pPropertyData->mProperty == NULL)
				{
					propertyById[index]->LinkDoor(pDoor);
					pPropertyData->mDatabaseId = doorData.GetInt(IDTO_PRIMARY_COLUMN_NAME);
					Q_strncpy(pPropertyData->mName.GetForModify(),
						UTIL_TrimQuotableString(doorData.GetString("name")), sizeof(pPropertyData->mName.m_Value));
					UTIL_SetDoorLockState(pDoor, NULL, doorData.GetInt("isLocked") > 0, false);
				}
			}
		}

		FOR_EACH_DICT_FAST(HL2RPRules()->mProperties, i)
		{
			CHL2RP_Property* pProperty = HL2RPRules()->mProperties[i];

			if (pProperty->mDoors.Count() < 1)
			{
				// Delete orphan property
				pProperty->Synchronize(false, false);
				delete pProperty;
				HL2RPRules()->mProperties.RemoveAt(i);
			}
			else if (pProperty->HasOwner())
			{
				CHL2Roleplayer* pOwner = ToHL2Roleplayer(UTIL_PlayerBySteamID(pProperty->mOwnerSteamIdNumber));

				if (pOwner != NULL)
				{
					pOwner->mHomes.Insert(pProperty);
					VCRHook_Time(&pProperty->mOwnerLastSeenTime);
				}
			}
		}
	}
}

REGISTER_LEVEL_INIT_DAO_FACTORY(CPropertiesLoadDAO, ELevelInitDAOType::Load)

CPropertyInsertDAO::CPropertyInsertDAO(CHL2RP_Property* pProperty)
	: CAutoIncrementInsertDAO(HL2RP_PROPERTY_DAO_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME), mpProperty(pProperty)
{
	mpRecord->AddIndexField(HL2RP_MAP_ALIAS_FIELD_NAME, pProperty->mpMapAlias)->AddNormalField("type", pProperty->mType);
}

void CPropertyInsertDAO::HandleCompletion()
{
	if (HL2RPRules()->mProperties.HasElement(mpProperty))
	{
		mpProperty->mDatabaseId = mResultId;
		mpProperty->Synchronize(true, false);

		if (mpProperty->mDoors.IsValidIndex(0) && mpProperty->mDoors[0] != NULL)
		{
			DAL().AddDAO(new CPropertyDoorInsertDAO(mpProperty->mDoors[0]));
		}
	}
}

CPropertiesSaveDAO::CPropertiesSaveDAO(CHL2RP_Property* pProperty, bool save) : CSaveDeleteDAO(false)
{
	CRecordNodeDTO* pPropertyData = GetCorrectDatabase(save).CreateCollection(HL2RP_PROPERTY_DAO_COLLECTION_NAME)
		->AddIndexField(HL2RP_MAP_ALIAS_FIELD_NAME, pProperty->mpMapAlias)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pProperty->mDatabaseId);

	if (save)
	{
		pPropertyData->AddNormalField("type", pProperty->mType);
		pPropertyData->AddNormalField("name", pProperty->mName);
		pPropertyData->AddNormalField("ownerId", pProperty->HasOwner() ? pProperty->mOwnerSteamIdNumber : SUtlField());
		pPropertyData->AddNormalField("ownerLastSeenTime", (int)pProperty->mOwnerLastSeenTime);
		pPropertyData->AddNormalField("hasZone", pProperty->mhZone.IsValid());
		pPropertyData->AddNormalField("zoneTargetName",
			pProperty->mhZone != NULL ? pProperty->mhZone->GetEntityName() : NULL_STRING);
		pPropertyData->AddNormalField("zoneX", pProperty->mSampleZonePoint.x);
		pPropertyData->AddNormalField("zoneY", pProperty->mSampleZonePoint.y);
		pPropertyData->AddNormalField("zoneZ", pProperty->mSampleZonePoint.z);
	}
}

CPropertiesSaveDAO::CPropertiesSaveDAO(CHL2RP_Property* pProperty, const char* pOldMapAlias)
	: CPropertiesSaveDAO(pProperty)
{
	// Set for deletion to ensure old record is erased from the initial map/group file under KeyValues
	mDeleteDatabase.CreateCollection(HL2RP_PROPERTY_DAO_COLLECTION_NAME)
		->AddIndexField(HL2RP_MAP_ALIAS_FIELD_NAME, pOldMapAlias)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pProperty->mDatabaseId);
}

CPropertyGrantsSaveDAO::CPropertyGrantsSaveDAO(CHL2RP_Property* pProperty, uint64 grantedId, bool save)
{
	GetCorrectDatabase(save).CreateCollection(HL2RP_PROPERTY_DAO_GRANT_COLLECTION_NAME)
		->AddIndexField(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN, pProperty->mDatabaseId)
		->AddIndexField("grantedId", grantedId);
}

CRecordNodeDTO* CPropertyDoorDAO::AddFields(CBaseEntity* pDoor, CRecordNodeDTO* pCollection, bool save)
{
	CHL2RP_PropertyDoorData* pPropertyData = pDoor->GetPropertyDoorData();
	CRecordNodeDTO* pDoorData = pCollection->AddIndexField(HL2RP_PROPERTY_DAO_FOREIGN_COLUMN,
		pPropertyData->mProperty.Get()->mDatabaseId);

	if (save)
	{
		const Vector& origin = GetDoorSearchOrigin(pDoor);
		pDoorData->AddNormalField("origMap", gpGlobals->mapname);
		pDoorData->AddNormalField("name", pPropertyData->mName.Get());
		pDoorData->AddNormalField("isLocked", pDoor->IsLocked());
		pDoorData->AddNormalField("hammerId", pDoor->m_iHammerID);
		pDoorData->AddNormalField("targetName", pDoor->GetEntityName());
		pDoorData->AddNormalField("x", origin.x);
		pDoorData->AddNormalField("y", origin.y);
		pDoorData->AddNormalField("z", origin.z);
	}

	return pDoorData;
}

CPropertyDoorInsertDAO::CPropertyDoorInsertDAO(CBaseEntity* pDoor)
	: CAutoIncrementInsertDAO(HL2RP_PROPERTY_DAO_DOOR_COLLECTION_NAME, IDTO_PRIMARY_COLUMN_NAME), mhDoor(pDoor)
{
	AddFields(pDoor, mpRecord, true);
}

void CPropertyDoorInsertDAO::HandleCompletion()
{
	CHL2RP_PropertyDoorData* pPropertyData = UTIL_GetPropertyDoorData(mhDoor);

	if (pPropertyData != NULL && pPropertyData->mProperty != NULL)
	{
		pPropertyData->mDatabaseId = mResultId;
		pPropertyData->mProperty.GetForModify(); // Create property at client via SendProxy_DoorProperty
	}
}

CPropertyDoorsSaveDAO::CPropertyDoorsSaveDAO(CBaseEntity* pDoor, bool save) : CSaveDeleteDAO(false)
{
	AddFields(pDoor, GetCorrectDatabase(save).CreateCollection(HL2RP_PROPERTY_DAO_DOOR_COLLECTION_NAME), save)
		->AddIndexField(IDTO_PRIMARY_COLUMN_NAME, pDoor->GetPropertyDoorData()->mDatabaseId);
}
