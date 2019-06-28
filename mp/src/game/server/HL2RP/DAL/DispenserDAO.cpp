#include <cbase.h>
#include "DispenserDAO.h"
#include "IDALEngine.h"
#include <PropRationDispenser.h>

#define DISPENSER_DAO_COLLECTION_NAME	"Dispenser"

#define DISPENSER_DAO_SQLITE_SETUP_QUERY	"CREATE TABLE IF NOT EXISTS %s "	\
"(id INTEGER NOT NULL, map VARCHAR (%i) NOT NULL, x DOUBLE, y DOUBLE, z DOUBLE, yaw DOUBLE, "	\
"PRIMARY KEY (id), CONSTRAINT id CHECK (id >= 0));"

#define DISPENSER_DAO_MYSQL_SETUP_QUERY	"CREATE TABLE IF NOT EXISTS %s "	\
"(id INTEGER NOT NULL AUTO_INCREMENT, map VARCHAR (%i) NOT NULL, x DOUBLE, y DOUBLE, z DOUBLE, yaw DOUBLE, "	\
"PRIMARY KEY (id));"

CDAOThread::EDAOCollisionResolution CDispensersInitDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

void CDispensersInitDAO::Execute(CSQLiteEngine* pSQLite)
{
	pSQLite->FormatAndExecuteQuery(DISPENSER_DAO_SQLITE_SETUP_QUERY, DISPENSER_DAO_COLLECTION_NAME,
		MAX_MAP_NAME);
}

void CDispensersInitDAO::Execute(CMySQLEngine* pMySQL)
{
	pMySQL->FormatAndExecuteQuery(DISPENSER_DAO_MYSQL_SETUP_QUERY, DISPENSER_DAO_COLLECTION_NAME,
		MAX_MAP_NAME);
}

CMapDispenserDAO::CMapDispenserDAO()
{
	Q_strcpy(m_MapName, STRING(gpGlobals->mapname));
}

bool CMapDispenserDAO::Equals(CMapDispenserDAO* pDAO)
{
	return (pDAO != NULL && Q_strcmp(pDAO->m_MapName, m_MapName) == 0);
}

void CMapDispenserDAO::BuildKeyValuesPath(char* pDest, int destSize)
{
	Q_snprintf(pDest, destSize, DISPENSER_DAO_COLLECTION_NAME "/%s.txt", m_MapName);
}

void CMapDispenserDAO::BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int mapNamePos)
{
	pSQL->BindString(pStmt, mapNamePos, m_MapName);
}

CDAOThread::EDAOCollisionResolution CDispensersLoadDAO::Collide(IDAO* pDAO)
{
	// If it's a CDispensersLoadDAO, there are no identificators to compare.
	// The incoming DAO may have been created after rapid map changes, so it has priority.
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

void CDispensersLoadDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	pKVEngine->LoadFromFile(path, DISPENSER_DAO_COLLECTION_NAME);
}

void CDispensersLoadDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteSelectQuery("SELECT * FROM %s WHERE map = '%s';",
		DISPENSER_DAO_COLLECTION_NAME, DISPENSER_DAO_COLLECTION_NAME, m_MapName);
}

void CDispensersLoadDAO::HandleResults(KeyValues* pResults)
{
	KeyValues* pResult = pResults->FindKey(DISPENSER_DAO_COLLECTION_NAME);

	if (pResult != NULL && Q_strcmp(STRING(gpGlobals->mapname), m_MapName) == 0)
	{
		FOR_EACH_SUBKEY(pResult, pDispenserData)
		{
			const Vector origin(pDispenserData->GetFloat("x"),
				pDispenserData->GetFloat("y"), pDispenserData->GetFloat("z"));
			const QAngle angles(0.0f, pDispenserData->GetFloat("yaw"), 0.0f);
			CPropRationDispenser* pDispenser = static_cast<CPropRationDispenser*>
				(CBaseEntity::Create("prop_ration_dispenser", origin, angles));
			pDispenser->m_iDatabaseId = pDispenserData->GetInt("id");
		}
	}
}

CDispenserHandleDAO::CDispenserHandleDAO(CPropRationDispenser* pDispenser)
	: m_DispenserHandle(pDispenser->GetRefEHandle())
{

}

CDAOThread::EDAOCollisionResolution CDispenserHandleDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

bool CDispenserHandleDAO::Equals(CDispenserHandleDAO* pDAO)
{
	return (CMapDispenserDAO::Equals(pDAO) && pDAO->m_DispenserHandle == m_DispenserHandle);
}

CDispenserDatabaseIdDto::CDispenserDatabaseIdDto(CPropRationDispenser* pDispenser)
	: m_Id(pDispenser->m_iDatabaseId)
{

}

CDispenserCoordsDAO::CDispenserCoordsDAO(CPropRationDispenser* pDispenser)
	: CDispenserHandleDAO(pDispenser), m_vecCoords(pDispenser->GetAbsOrigin()),
	m_flYaw(pDispenser->GetAbsAngles().y)
{

}

CDispenserInsertSaveDAO::CDispenserInsertSaveDAO(CPropRationDispenser* pDispenser)
	: CDispenserCoordsDAO(pDispenser)
{

}

CDAOThread::EDAOCollisionResolution CDispenserInsertSaveDAO::Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO)
{
	if (Equals(pDAO->ToClass<CDispenserDeleteDAO>()))
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return CDispenserHandleDAO::Collide(pDAO);
}

CDAOThread::EDAOCollisionResolution CDispenserInsertSaveDAO::Collide(CSQLEngine* pSQL, IDAO* pDAO)
{
	if (Equals(pDAO->ToClass<CDispenserDeleteDAO>()))
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return TryResolveReplacement(Equals(pDAO->ToClass(this)));
}

void CDispenserInsertSaveDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	KeyValues* pResult = pKVEngine->LoadFromFile(path, DISPENSER_DAO_COLLECTION_NAME, true);

	// Append to the Dispensers keys
	KeyValues* pDispenserData = pResult->CreateNewKey();
	pDispenserData->SetString("id", pDispenserData->GetName());
	pDispenserData->SetFloat("x", m_vecCoords[X_INDEX]);
	pDispenserData->SetFloat("y", m_vecCoords[Y_INDEX]);
	pDispenserData->SetFloat("z", m_vecCoords[Z_INDEX]);
	pDispenserData->SetFloat("yaw", m_flYaw);
	pKVEngine->SaveToFile(pResult, path);
}

void CDispenserInsertSaveDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->BeginTxn();
	pSQL->FormatAndExecuteQuery("INSERT INTO %s (map, x, y, z, yaw) VALUES ('%s', %f, %f, %f, %f);",
		DISPENSER_DAO_COLLECTION_NAME, m_MapName, m_vecCoords[X_INDEX], m_vecCoords[Y_INDEX],
		m_vecCoords[Z_INDEX], m_flYaw);
	pSQL->FormatAndExecuteSelectQuery("SELECT id FROM %s ORDER BY id DESC LIMIT 1;",
		DISPENSER_DAO_COLLECTION_NAME, DISPENSER_DAO_COLLECTION_NAME);
	pSQL->EndTxn();
}

void CDispenserInsertSaveDAO::HandleResults(KeyValues* pResults)
{
	if (m_DispenserHandle != NULL)
	{
		KeyValues* pResult = pResults->FindKey(DISPENSER_DAO_COLLECTION_NAME);

		if (pResult != NULL)
		{
			// This should correspond to the dispenser inserted
			KeyValues* pDispenserData = pResult->FindLastSubKey();
			m_DispenserHandle->m_iDatabaseId = pDispenserData->GetInt("id");
		}
	}
}

CDispenserUpdateDAO::CDispenserUpdateDAO(CPropRationDispenser* pDispenser)
	: CDispenserCoordsDAO(pDispenser), m_DatabaseIdDto(pDispenser)
{

}

void CDispenserUpdateDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	KeyValues* pResult = pKVEngine->LoadFromFile(path, DISPENSER_DAO_COLLECTION_NAME, true);
	pResult->SetInt(NULL, m_DatabaseIdDto.m_Id);
	KeyValues* pDispenserData = pResult->FindKey(pResult->GetString(), true);
	pDispenserData->SetString("id", pResult->GetName());
	pDispenserData->SetFloat("x", m_vecCoords[X_INDEX]);
	pDispenserData->SetFloat("y", m_vecCoords[Y_INDEX]);
	pDispenserData->SetFloat("z", m_vecCoords[Z_INDEX]);
	pDispenserData->SetFloat("yaw", m_flYaw);
	pKVEngine->SaveToFile(pResult, path);
}

void CDispenserUpdateDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery("UPDATE %s SET x = %f, y = %f, z = %f, yaw = %f WHERE id = %i AND map = '%s';",
		DISPENSER_DAO_COLLECTION_NAME, m_vecCoords[X_INDEX], m_vecCoords[Y_INDEX], m_vecCoords[Z_INDEX],
		m_flYaw, m_DatabaseIdDto, m_MapName);
}

CDispenserDeleteDAO::CDispenserDeleteDAO(CPropRationDispenser* pDispenser)
	: CDispenserHandleDAO(pDispenser), m_DatabaseIdDto(pDispenser)
{

}

void CDispenserDeleteDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	KeyValues* pResult = pKVEngine->LoadFromFile(path, DISPENSER_DAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		KeyValues* pDispenserData = pResult->FindKey(m_DatabaseIdDto.m_Id);

		if (pDispenserData != NULL)
		{
			pResult->RemoveSubKey(pDispenserData);
			pKVEngine->SaveToFile(pResult, path);
		}
	}
}

void CDispenserDeleteDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery("DELETE FROM %s WHERE id = %i;",
		DISPENSER_DAO_COLLECTION_NAME, m_DatabaseIdDto.m_Id);
}
