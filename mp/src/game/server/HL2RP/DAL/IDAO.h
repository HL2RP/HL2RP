#ifndef IDAO_H
#define IDAO_H
#pragma once

#include <platform.h>
#include "HL2RPDAL.h"

#define IDAO_INVALID_DATABASE_ID	-1
#define IDAO_SQL_VARCHAR_KEY_SIZE	255

class KeyValues;
class CSQLEngine;
class CKeyValuesEngine;
class CSQLiteEngine;
class CMySQLEngine;

class IDAO
{
public:
	virtual CDAOThread::EDAOCollisionResolution Collide(IDAO* pDAO);
	virtual CDAOThread::EDAOCollisionResolution Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO);
	virtual CDAOThread::EDAOCollisionResolution Collide(CSQLEngine* pSQL, IDAO* pDAO);

	virtual void Execute(CKeyValuesEngine* pKVEngine) { }
	virtual void Execute(CSQLEngine* pSQL) { } // For common SQL Engine execution
	virtual void Execute(CSQLiteEngine* pSQLite);
	virtual void Execute(CMySQLEngine* pMySQL);

	virtual void HandleResults(KeyValues* pResults) { }
	virtual void HandleResults(CKeyValuesEngine* pKVEngine, KeyValues* pResults);
	virtual void HandleResults(CSQLEngine* pKVEngine, KeyValues* pResults);

	// Casts to a DAO class in less code
	template<class D> D* ToClass(const D* pDAO = NULL)
	{
		return dynamic_cast<D*>(this);
	}

protected:
	CDAOThread::EDAOCollisionResolution TryResolveReplacement(bool condition);
	CDAOThread::EDAOCollisionResolution TryResolveRemoval(bool condition);
};

class CDAOException
{
public:
	// Whether the DAO should be inserted again to the DAO list
	virtual bool ShouldRetry()
	{
		return false;
	}
};

#endif // !IDAO_H
