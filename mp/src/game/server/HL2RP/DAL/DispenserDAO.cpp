#include <cbase.h>
#include "DispenserDAO.h"
#include "DALEngine.h"
#include <PropRationDispenser.h>

#define DISPENSERDAO_COLLECTION_NAME	"Dispenser"

#define DISPENSERDAO_SQLITE_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (id INTEGER NOT NULL, map VARCHAR (%i) NOT NULL, x DOUBLE, y DOUBLE, z DOUBLE, yaw DOUBLE,"	\
" PRIMARY KEY (id), CONSTRAINT id CHECK (id >= 0));"

#define DISPENSERDAO_MYSQL_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (id INTEGER NOT NULL AUTO_INCREMENT, map VARCHAR (%i) NOT NULL, x DOUBLE, y DOUBLE, z DOUBLE, yaw DOUBLE,"	\
" PRIMARY KEY (id), CONSTRAINT id CHECK (id >= 0));"

#define DISPENSERDAO_SQL_LOAD_QUERY	"SELECT * FROM " DISPENSERDAO_COLLECTION_NAME " WHERE map = ?;"

#define DISPENSERDAO_SQL_INSERT_QUERY	\
"INSERT INTO " DISPENSERDAO_COLLECTION_NAME " (map, x, y, z, yaw) VALUES (?, ?, ?, ?, ?);"

#define DISPENSERDAO_SQL_GET_ID_QUERY	"SELECT id FROM " DISPENSERDAO_COLLECTION_NAME " ORDER BY id DESC LIMIT 1;"

#define DISPENSERDAO_SQLITE_UPDATE_QUERY	\
"UPDATE " DISPENSERDAO_COLLECTION_NAME " SET x = ?, y = ?, z = ?, yaw = ? WHERE id = ? AND map = ?;"

#define DISPENSERDAO_SQLITE_INSERT_QUERY	\
"INSERT OR IGNORE INTO " DISPENSERDAO_COLLECTION_NAME " (id, map, x, y, z, yaw)"	\
" VALUES (?, ?, ?, ?, ?, ?);"

#define DISPENSERDAO_MYSQL_SAVE_QUERY	\
"INSERT INTO " DISPENSERDAO_COLLECTION_NAME " (id, map, x, y, z, yaw)"	\
" VALUES (?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE x = ?, y = ?, z = ?, yaw = ?;"

#define DISPENSERDAO_SQL_DELETE_QUERY	"DELETE FROM " DISPENSERDAO_COLLECTION_NAME " WHERE id = ?;"

bool CDispensersInitDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return (pDAO->ToClass(this) != NULL);
}

void CDispensersInitDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	pSQL->ExecuteFormatQuery(DISPENSERDAO_SQLITE_SETUP_QUERY, NULL, DISPENSERDAO_COLLECTION_NAME,
		MAX_MAP_NAME);
}

void CDispensersInitDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteFormatQuery(DISPENSERDAO_MYSQL_SETUP_QUERY, NULL, DISPENSERDAO_COLLECTION_NAME,
		MAX_MAP_NAME);
}

CMapDispenserDAO::CMapDispenserDAO()
{
	Q_strcpy(m_MapName, STRING(gpGlobals->mapname));
}

bool CMapDispenserDAO::ShouldBeReplacedBy(CMapDispenserDAO* pDAO)
{
	return (pDAO != NULL && !Q_strcmp(pDAO->m_MapName, m_MapName));
}

void CMapDispenserDAO::BuildKeyValuesPath(char* pDest, int destSize)
{
	Q_snprintf(pDest, destSize, DISPENSERDAO_COLLECTION_NAME "/%s.txt", m_MapName);
}

void CMapDispenserDAO::BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int mapNamePos)
{
	pSQL->BindString(pStmt, mapNamePos, m_MapName);
}

bool CDispensersLoadDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	// If it's a CDispensersLoadDAO, there are no identificators to compare.
	// The incoming DAO should have been created along a rapid map change, so it has priority:
	return (pDAO->ToClass(this) != NULL);
}

void CDispensersLoadDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	pKeyValues->LoadFromFile(path, DISPENSERDAO_COLLECTION_NAME);
}

void CDispensersLoadDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->PrepareStatement(DISPENSERDAO_SQL_LOAD_QUERY);
	BindSQLDTO(pSQL, pStmt, 1);
	pSQL->ExecutePreparedStatement(pStmt, DISPENSERDAO_COLLECTION_NAME);
}

void CDispensersLoadDAO::HandleResults(KeyValues* pResults)
{
	KeyValues* pResult = pResults->FindKey(DISPENSERDAO_COLLECTION_NAME);

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

bool CDispenserHandleDAO::ShouldBeReplacedBy(CDispenserHandleDAO* pDAO)
{
	// BaseClass should validate the pointer (so it's safe on the comparison)
	return (CMapDispenserDAO::ShouldBeReplacedBy(pDAO)
		&& pDAO->m_DispenserHandle == m_DispenserHandle);
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

void* CDispenserCoordsDAO::PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int mapNamePos,
	int xPos, int yPos, int zPos, int yawPos)
{
	void* pStmt = pSQL->PrepareStatement(pQueryText);
	BindSQLDTO(pSQL, pStmt, mapNamePos);
	pSQL->BindFloat(pStmt, xPos, m_vecCoords[X_INDEX]);
	pSQL->BindFloat(pStmt, yPos, m_vecCoords[Y_INDEX]);
	pSQL->BindFloat(pStmt, zPos, m_vecCoords[Z_INDEX]);
	pSQL->BindFloat(pStmt, yawPos, m_flYaw);
	return pStmt;
}

CDispenserFirstSaveDAO::CDispenserFirstSaveDAO(CPropRationDispenser* pDispenser)
	: CDispenserCoordsDAO(pDispenser)
{

}

bool CDispenserFirstSaveDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return CDispenserCoordsDAO::ShouldBeReplacedBy(pDAO->ToClass(this));
}

bool CDispenserFirstSaveDAO::ShouldRemoveBoth(IDAO* pDAO)
{
	// An incoming Delete DAO may safely cause to remove an Insert DAO and itself
	return CDispenserCoordsDAO::ShouldBeReplacedBy(pDAO->ToClass<CDispenserDeleteDAO>());
}

void CDispenserFirstSaveDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	KeyValues* pResult = pKeyValues->LoadFromFile(path, DISPENSERDAO_COLLECTION_NAME, true);

	// Append to the Dispensers keys
	KeyValues* pDispenserData = pResult->CreateNewKey();
	pDispenserData->SetString("id", pDispenserData->GetName());
	pDispenserData->SetFloat("x", m_vecCoords[X_INDEX]);
	pDispenserData->SetFloat("y", m_vecCoords[Y_INDEX]);
	pDispenserData->SetFloat("z", m_vecCoords[Z_INDEX]);
	pDispenserData->SetFloat("yaw", m_flYaw);
	pKeyValues->SaveToFile(pResult, path);
}

void CDispenserFirstSaveDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->BeginTxn();
	void* pStmt = PrepareSQLStatement(pSQL, DISPENSERDAO_SQL_INSERT_QUERY, 1, 2, 3, 4, 5);
	pSQL->ExecutePreparedStatement(pStmt);
	pSQL->ExecuteQuery(DISPENSERDAO_SQL_GET_ID_QUERY, DISPENSERDAO_COLLECTION_NAME);
	pSQL->EndTxn();
}

void CDispenserFirstSaveDAO::HandleResults(KeyValues* pResults)
{
	if (m_DispenserHandle != NULL)
	{
		KeyValues* pResult = pResults->FindKey(DISPENSERDAO_COLLECTION_NAME);

		if (pResult != NULL)
		{
			// This should correspond to the dispenser inserted
			KeyValues* pDispenserData = pResult->FindLastSubKey();
			m_DispenserHandle->m_iDatabaseId = pDispenserData->GetInt("id");
		}
	}
}

CDispenserFurtherSaveDAO::CDispenserFurtherSaveDAO(CPropRationDispenser* pDispenser)
	: CDispenserCoordsDAO(pDispenser), m_DispenserDatabaseIdDto(pDispenser)
{

}

bool CDispenserFurtherSaveDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return CDispenserCoordsDAO::ShouldBeReplacedBy(pDAO->ToClass(this));
}

void CDispenserFurtherSaveDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	KeyValues* pResult = pKeyValues->LoadFromFile(path, DISPENSERDAO_COLLECTION_NAME, true);
	pResult->SetInt(NULL, m_DispenserDatabaseIdDto.m_Id);
	KeyValues* pDispenserData = pResult->FindKey(pResult->GetString(), true);
	pDispenserData->SetString("id", pResult->GetName());
	pDispenserData->SetFloat("x", m_vecCoords[X_INDEX]);
	pDispenserData->SetFloat("y", m_vecCoords[Y_INDEX]);
	pDispenserData->SetFloat("z", m_vecCoords[Z_INDEX]);
	pDispenserData->SetFloat("yaw", m_flYaw);
	pKeyValues->SaveToFile(pResult, path);
}

void CDispenserFurtherSaveDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	pSQL->BeginTxn();
	void* pStmt = PrepareSQLStatement(pSQL, DISPENSERDAO_SQLITE_UPDATE_QUERY,
		6, 1, 2, 3, 4);
	pSQL->BindInt(pStmt, 5, m_DispenserDatabaseIdDto.m_Id);
	pSQL->ExecutePreparedStatement(pStmt);
	pStmt = PrepareSQLStatement(pSQL, DISPENSERDAO_SQLITE_INSERT_QUERY,
		2, 3, 4, 5, 6);
	pSQL->BindInt(pStmt, 1, m_DispenserDatabaseIdDto.m_Id);
	pSQL->ExecutePreparedStatement(pStmt);
	pSQL->EndTxn();
}

void CDispenserFurtherSaveDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	void* pStmt = PrepareSQLStatement(pSQL, DISPENSERDAO_MYSQL_SAVE_QUERY,
		2, 3, 4, 5, 6);
	pSQL->BindInt(pStmt, 1, m_DispenserDatabaseIdDto.m_Id);
	pSQL->BindFloat(pStmt, 7, m_vecCoords[X_INDEX]);
	pSQL->BindFloat(pStmt, 8, m_vecCoords[Y_INDEX]);
	pSQL->BindFloat(pStmt, 9, m_vecCoords[Z_INDEX]);
	pSQL->BindFloat(pStmt, 10, m_flYaw);
	pSQL->ExecutePreparedStatement(pStmt);
}

CDispenserDeleteDAO::CDispenserDeleteDAO(CPropRationDispenser* pDispenser)
	: CDispenserHandleDAO(pDispenser), m_DispenserDatabaseIdDto(pDispenser)
{

}

void CDispenserDeleteDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	char path[MAX_PATH];
	BuildKeyValuesPath(path, sizeof(path));
	KeyValues* pResult = pKeyValues->LoadFromFile(path, DISPENSERDAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		KeyValues* pDispenserData = pResult->FindKey(m_DispenserDatabaseIdDto.m_Id);

		if (pDispenserData != NULL)
		{
			pResult->RemoveSubKey(pDispenserData);
			pKeyValues->SaveToFile(pResult, path);
		}
	}
}

void CDispenserDeleteDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->PrepareStatement(DISPENSERDAO_SQL_DELETE_QUERY);
	pSQL->BindInt(pStmt, 1, m_DispenserDatabaseIdDto.m_Id);
	pSQL->ExecutePreparedStatement(pStmt);
}
