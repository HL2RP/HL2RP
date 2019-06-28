#ifndef JOBDAO_H
#define JOBDAO_H
#pragma once

#include "IDAO.h"

class CJobsInitDAO : public IDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
};

class CJobsLoadDAO : public IDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;
};

// Shares DTO among derived DAOs
class CJobDAO : public IDAO
{
public:
	char m_ModelIdText[IDAO_SQL_VARCHAR_KEY_SIZE];

protected:
	CJobDAO(const char* pModelIdText);
};

// Shares DTO among derived DAOs
class CJobInsertDAO : public CJobDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

	char m_ModelPath[MAX_PATH];
	int m_iModelType;

public:
	CJobInsertDAO(const char* pModelIdText, const char* pModelPath, int modelType);
};

class CJobDeleteDAO : public CJobDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CJobDeleteDAO(const char* pModelIdText);
};

#endif // !JOBDAO_H
