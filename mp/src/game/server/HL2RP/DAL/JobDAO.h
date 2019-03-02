#ifndef JOBDAO_H
#define JOBDAO_H
#pragma once

#include "IDAO.h"

class CJobsInitDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO);
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
};

class CJobsLoadDAO : public IDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;
};

// Shares DTO among derived DAOs
class CJobDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;

protected:
	CJobDAO(const char* pModelIdText);

	char m_ModelIdText[IDAO_SQL_VARCHAR_KEY_SIZE];
};

// Shares DTO among derived DAOs
class CJobSaveDAO : public CJobDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQLite(CSQLEngine* pSQL) OVERRIDE;
	void ExecuteMySQL(CSQLEngine* pSQL) OVERRIDE;

	void* PrepareSQLPreparedStatement(CSQLEngine* pSQL, const char* pQueryText, int modelTypePos,
		int modelAliasPos, int modelPathPos);
	void ExecuteSQLPreparedStatement(CSQLEngine* pSQL, const char* pQueryText, int modelTypePos,
		int modelAliasPos, int modelPathPos);

	char m_ModelPath[MAX_PATH];
	int m_iModelType;

public:
	CJobSaveDAO(const char* pModelIdText, const char* pModelPath, int modelType);
};

class CJobDeleteDAO : public CJobDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;

public:
	CJobDeleteDAO(const char* pModelIdText);
};

#endif // !JOBDAO_H
