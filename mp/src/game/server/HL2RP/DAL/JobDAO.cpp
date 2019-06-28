#include <cbase.h>
#include "JobDAO.h"
#include <HL2RPRules.h>
#include "IDALEngine.h"

#define JOB_DAO_COLLECTION_NAME	"Job"
#define JOB_DAO_KV_FILENAME	(JOB_DAO_COLLECTION_NAME ".txt")

// Note: Only make a composite key of <Type, Alias>, to relax program checks and gain flexibility
#define JOB_DAO_SQL_SETUP_QUERY	\
"CREATE TABLE IF NOT EXISTS %s (type INTEGER, alias VARCHAR (%i) NOT NULL, modelPath VARCHAR (%i) NOT NULL, "	\
"PRIMARY KEY (type, alias), CONSTRAINT type CHECK (type >= 0 AND type < %i));"

CDAOThread::EDAOCollisionResolution CJobsInitDAO::Collide(IDAO* pDAO)
{
	return TryResolveReplacement(pDAO->ToClass(this) != NULL);
}

void CJobsInitDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery(JOB_DAO_SQL_SETUP_QUERY, JOB_DAO_COLLECTION_NAME,
		IDAO_SQL_VARCHAR_KEY_SIZE, IDAO_SQL_VARCHAR_KEY_SIZE, JobTeamIndex_Max);
}

void CJobsLoadDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	pKVEngine->LoadFromFile(JOB_DAO_KV_FILENAME, JOB_DAO_COLLECTION_NAME);
}

void CJobsLoadDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->FormatAndExecuteQuery("SELECT * FROM %s;", JOB_DAO_COLLECTION_NAME);
}

void CJobsLoadDAO::HandleResults(KeyValues* pResults)
{
	KeyValues* pResult = pResults->FindKey(JOB_DAO_COLLECTION_NAME);

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

			const char* pModelPath = pJobData->GetString("modelPath");

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

CJobInsertDAO::CJobInsertDAO(const char* pModelIdText, const char* pModelPath, int modelType)
	: m_iModelType(modelType), CJobDAO(pModelIdText)
{
	Q_strcpy(m_ModelPath, pModelPath);
}

CDAOThread::EDAOCollisionResolution CJobInsertDAO::Collide(IDAO* pDAO)
{
	CJobDeleteDAO* pDeleteDAO = pDAO->ToClass<CJobDeleteDAO>();

	if (pDeleteDAO != NULL && Q_strcmp(pDeleteDAO->m_ModelIdText, m_ModelIdText) == 0)
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return CDAOThread::EDAOCollisionResolution::None;
}

void CJobInsertDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	KeyValues* pResult = pKVEngine->LoadFromFile(JOB_DAO_KV_FILENAME, JOB_DAO_COLLECTION_NAME, true);
	KeyValues* pJobData = pResult->FindKey(m_ModelIdText, true);
	pJobData->SetString("alias", m_ModelIdText);
	pJobData->SetString("modelPath", m_ModelPath);
	pJobData->SetInt("type", m_iModelType);
	pKVEngine->SaveToFile(pResult, JOB_DAO_KV_FILENAME);
}

void CJobInsertDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("INSERT INTO %s (type, alias, modelPath) "
		"VALUES (%i, ? , ? ); ", JOB_DAO_COLLECTION_NAME, m_iModelType);
	pSQL->BindString(pStmt, 1, m_ModelIdText);
	pSQL->BindString(pStmt, 2, m_ModelPath);
	pSQL->ExecutePreparedStatement(pStmt);
}

CJobDeleteDAO::CJobDeleteDAO(const char* pModelIdText) : CJobDAO(pModelIdText)
{

}

void CJobDeleteDAO::Execute(CKeyValuesEngine* pKVEngine)
{
	KeyValues* pResult = pKVEngine->LoadFromFile(JOB_DAO_KV_FILENAME, JOB_DAO_COLLECTION_NAME);

	if (pResult != NULL)
	{
		KeyValues* pJobData = pResult->FindKey(m_ModelIdText);

		if (pJobData != NULL)
		{
			pResult->RemoveSubKey(pJobData);
			pKVEngine->SaveToFile(pResult, JOB_DAO_KV_FILENAME);
		}
	}
}

void CJobDeleteDAO::Execute(CSQLEngine* pSQL)
{
	void* pStmt = pSQL->FormatAndPrepareStatement("DELETE FROM %s WHERE alias = ?;", JOB_DAO_COLLECTION_NAME);
	pSQL->BindString(pStmt, 1, m_ModelIdText);
	pSQL->ExecutePreparedStatement(pStmt);
}
