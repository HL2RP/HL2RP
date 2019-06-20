#include <cbase.h>
#include "JobDAO.h"
#include "DALEngine.h"
#include <HL2RPRules.h>

#define JOBDAO_COLLECTION_NAME	"Job"
#define JOBDAO_KV_FILENAME	(JOBDAO_COLLECTION_NAME ".txt")

// Note: Only make a composite key of <Type, Alias>, to relax program checks and gain flexibility
#define JOBDAO_SQL_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (type INTEGER, alias VARCHAR (%i) NOT NULL, model_path VARCHAR (%i) NOT NULL,"	\
" PRIMARY KEY (type, alias), CONSTRAINT type CHECK (type >= 0 AND type < %i));"

#define JOBDAO_SQL_LOAD_QUERY	"SELECT * FROM " JOBDAO_COLLECTION_NAME ";"

#define JOBDAO_SQLITE_UPDATE_QUERY	\
"UPDATE " JOBDAO_COLLECTION_NAME " SET model_path = ? WHERE type = ? AND alias = ?;"

#define JOBDAO_SQLITE_INSERT_QUERY	\
"INSERT OR IGNORE INTO " JOBDAO_COLLECTION_NAME " (type, alias, model_path) VALUES (?, ?, ?);"

#define JOBDAO_MYSQL_SAVE_QUERY	\
"INSERT INTO " JOBDAO_COLLECTION_NAME " (type, alias, model_path) VALUES (?, ?, ?)"	\
" ON DUPLICATE KEY UPDATE model_path = ?;"

#define JOBDAO_SQL_DELETE_QUERY	"DELETE FROM " JOBDAO_COLLECTION_NAME " WHERE alias = ?;"

bool CJobsInitDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return (pDAO->ToClass(this) != NULL);
}

void CJobsInitDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteFormatQuery(JOBDAO_SQL_SETUP_QUERY, NULL, JOBDAO_COLLECTION_NAME,
		IDAO_SQL_VARCHAR_KEY_SIZE, IDAO_SQL_VARCHAR_KEY_SIZE, JobTeamIndex_Max);
}

void CJobsLoadDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	pKeyValues->LoadFromFile(JOBDAO_KV_FILENAME, JOBDAO_COLLECTION_NAME);
}

void CJobsLoadDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	pSQL->ExecuteQuery(JOBDAO_SQL_LOAD_QUERY, JOBDAO_COLLECTION_NAME);
}

void CJobsLoadDAO::HandleResults(KeyValues* pResults)
{
	KeyValues* pResult = pResults->FindKey(JOBDAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		FOR_EACH_SUBKEY(pResult, pJobData)
		{
			const char* pAlias = pJobData->GetString("alias");
			int type = pJobData->GetInt("type");

			if (HL2RPRules()->m_JobTeamsModels[type].HasElement(pAlias))
			{
				continue;
			}

			const char* pModelPath = pJobData->GetString("model_path");

			if (type > EJobTeamIndex::Invalid && type < JobTeamIndex_Max
				&& filesystem->FileExists(pModelPath))
			{
				CBaseEntity::PrecacheModel(pModelPath);
				SJob auxJob(pModelPath);
				HL2RPRules()->m_JobTeamsModels[type].Insert(pAlias, auxJob);
			}
		}
	}
}

CJobDAO::CJobDAO(const char* pModelIdText)
{
	Q_strncpy(m_ModelIdText, pModelIdText, sizeof(m_ModelIdText));
}

// Replaces a derived DAO with anothers. It assumes both are safe candidates!
bool CJobDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	CJobDAO* pJobDAO = pDAO->ToClass(this);
	return ((pJobDAO != NULL) && !Q_strcmp(pJobDAO->m_ModelIdText, m_ModelIdText));
}

CJobSaveDAO::CJobSaveDAO(const char* pModelIdText, const char* pModelPath, int modelType)
	: m_iModelType(modelType), CJobDAO(pModelIdText)
{
	Q_strcpy(m_ModelPath, pModelPath);
}

void CJobSaveDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	KeyValues* pResult = pKeyValues->LoadFromFile(JOBDAO_KV_FILENAME, JOBDAO_COLLECTION_NAME, true);
	KeyValues* pJobData = pResult->FindKey(m_ModelIdText, true);
	pJobData->SetString("alias", m_ModelIdText);
	pJobData->SetString("model_path", m_ModelPath);
	pJobData->SetInt("type", m_iModelType);
	pKeyValues->SaveToFile(pResult, JOBDAO_KV_FILENAME);
}

void CJobSaveDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	ExecuteSQLPreparedStatement(pSQL, JOBDAO_SQLITE_UPDATE_QUERY, 2, 3, 1);
	ExecuteSQLPreparedStatement(pSQL, JOBDAO_SQLITE_INSERT_QUERY, 1, 2, 3);
}

void CJobSaveDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	void* pStmt = PrepareSQLPreparedStatement(pSQL, JOBDAO_MYSQL_SAVE_QUERY, 1, 2, 3);
	pSQL->BindString(pStmt, 4, m_ModelPath);
	pSQL->ExecutePreparedStatement(pStmt);
}

void* CJobSaveDAO::PrepareSQLPreparedStatement(CSQLEngine* pSQL, const char* pQueryText,
	int modelTypePos, int modelAliasPos, int modelPathPos)
{
	void* pStmt = pSQL->PrepareStatement(pQueryText);
	pSQL->BindInt(pStmt, modelTypePos, m_iModelType);
	pSQL->BindString(pStmt, modelAliasPos, m_ModelIdText);
	pSQL->BindString(pStmt, modelPathPos, m_ModelPath);
	return pStmt;
}

void CJobSaveDAO::ExecuteSQLPreparedStatement(CSQLEngine* pSQL, const char* pQueryText,
	int modelTypePos, int modelAliasPos, int modelPathPos)
{
	void* pStmt = PrepareSQLPreparedStatement(pSQL, pQueryText, modelAliasPos, modelPathPos,
		modelTypePos);
	pSQL->ExecutePreparedStatement(pStmt);
}

CJobDeleteDAO::CJobDeleteDAO(const char* pModelIdText) : CJobDAO(pModelIdText)
{

}

void CJobDeleteDAO::ExecuteKeyValues(CKeyValuesEngine* pKeyValues)
{
	KeyValues* pResult = pKeyValues->LoadFromFile(JOBDAO_KV_FILENAME, JOBDAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		KeyValues* pJobData = pResult->FindKey(m_ModelIdText);

		if (pJobData != NULL)
		{
			pResult->RemoveSubKey(pJobData);
			pKeyValues->SaveToFile(pResult, JOBDAO_KV_FILENAME);
		}
	}
}

void CJobDeleteDAO::ExecuteSQL(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->PrepareStatement(JOBDAO_SQL_DELETE_QUERY);
	pSQL->BindString(pStmt, 1, m_ModelIdText);
	pSQL->ExecutePreparedStatement(pStmt);
}
