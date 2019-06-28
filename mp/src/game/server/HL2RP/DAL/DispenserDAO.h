#ifndef DISPENSERDAO_H
#define DISPENSERDAO_H
#pragma once

#include "IDAO.h"

class CPropRationDispenser;

class CDispensersInitDAO : public IDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CSQLiteEngine* pSQLite) OVERRIDE;
	void Execute(CMySQLEngine* pMySQL) OVERRIDE;
};

// Shares DTO among Dispenser DAOs
class CMapDispenserDAO : public IDAO
{
protected:
	CMapDispenserDAO();

	bool Equals(CMapDispenserDAO* pDAO);
	void BuildKeyValuesPath(char* pDest, int destSize);
	void BindSQLDTO(CSQLEngine* pSQL, void* pStmt, int mapNamePos);

	char m_MapName[MAX_MAP_NAME];
};

class CDispensersLoadDAO : public CMapDispenserDAO
{
	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;
};

// Shares DTO among Dispenser DAOs
class CDispenserHandleDAO : public CMapDispenserDAO
{
protected:
	CDispenserHandleDAO(CPropRationDispenser* pDispenser);

	CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO) OVERRIDE;

	bool Equals(CDispenserHandleDAO* pDAO);

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

	Vector m_vecCoords;
	vec_t m_flYaw;
};

class CDispenserInsertSaveDAO : public CDispenserCoordsDAO
{
	CDAOThread::EDAOCollisionResolution Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO) OVERRIDE;
	CDAOThread::EDAOCollisionResolution Collide(CSQLEngine* pSQL, IDAO* pDAO) OVERRIDE;
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;
	void HandleResults(KeyValues* pResults) OVERRIDE;

public:
	CDispenserInsertSaveDAO(CPropRationDispenser* pDispenser);
};

class CDispenserUpdateDAO : public CDispenserCoordsDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

public:
	CDispenserUpdateDAO(CPropRationDispenser* pDispenser);

protected:
	CDispenserDatabaseIdDto m_DatabaseIdDto;
};

class CDispenserDeleteDAO : public CDispenserHandleDAO
{
	void Execute(CKeyValuesEngine* pKVEngine) OVERRIDE;
	void Execute(CSQLEngine* pSQL) OVERRIDE;

	CDispenserDatabaseIdDto m_DatabaseIdDto;

public:
	CDispenserDeleteDAO(CPropRationDispenser* pDispenser);
};

#endif // !DISPENSERDAO_H
