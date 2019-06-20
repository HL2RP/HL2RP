#ifndef DISPENSERDAO_H
#define DISPENSERDAO_H
#pragma once

#include "IDAO.h"

class CPropRationDispenser;

class CDispensersInitDAO : public IDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO);
	void ExecuteSQLite(CSQLEngine* pSQL) OVERRIDE;
	void ExecuteMySQL(CSQLEngine* pSQL) OVERRIDE;
};

// Shares DTO among Dispenser DAOs
class CMapDispenserDAO : public IDAO
{
protected:
	CMapDispenserDAO();

	bool ShouldBeReplacedBy(CMapDispenserDAO* pThisClassDAO);
	void BuildKeyValuesPath(char* pDest, int destSize);
	void BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int mapNamePos);

	char m_MapName[MAX_MAP_NAME];
};

class CDispensersLoadDAO : public CMapDispenserDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO);
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;
};

// Shares DTO among Dispenser DAOs
class CDispenserHandleDAO : public CMapDispenserDAO
{
protected:
	CDispenserHandleDAO(CPropRationDispenser* pDispenser);

	bool ShouldBeReplacedBy(CDispenserHandleDAO* pThisClassDAO);

	CHandle<CPropRationDispenser> m_DispenserHandle;
};

// Shares DTO among Dispenser DAOs
class CDispenserDatabaseIdDto
{
public:
	CDispenserDatabaseIdDto(CPropRationDispenser* pDispenser);

	int m_Id;
};

// Shares DTO among Dispenser DAOs
class CDispenserCoordsDAO : public CDispenserHandleDAO
{
protected:
	CDispenserCoordsDAO(CPropRationDispenser* pDispenser);

	void* PrepareSQLStatement(CSQLEngine* pSQL, const char* pQueryText, int mapNamePos,
		int xPos, int yPos, int zPos, int yawPos);

	Vector m_vecCoords;
	vec_t m_flYaw;
};

class CDispenserFirstSaveDAO : public CDispenserCoordsDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;
	bool ShouldRemoveBoth(IDAO* pDAO) OVERRIDE;
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;

public:
	CDispenserFirstSaveDAO(CPropRationDispenser* pDispenser);
};

class CDispenserFurtherSaveDAO : public CDispenserCoordsDAO
{
	bool ShouldBeReplacedBy(IDAO* pDAO) OVERRIDE;
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQLite(CSQLEngine* pSQL) OVERRIDE;
	void ExecuteMySQL(CSQLEngine* pSQL) OVERRIDE;

public:
	CDispenserFurtherSaveDAO(CPropRationDispenser* pDispenser);

protected:
	CDispenserDatabaseIdDto m_DispenserDatabaseIdDto;
};

class CDispenserDeleteDAO : public CDispenserHandleDAO
{
	void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) OVERRIDE;
	void ExecuteSQL(CSQLEngine* pSQL) OVERRIDE;

	CDispenserDatabaseIdDto m_DispenserDatabaseIdDto;

public:
	CDispenserDeleteDAO(CPropRationDispenser* pDispenser);
};

#endif // !DISPENSERDAO_H
