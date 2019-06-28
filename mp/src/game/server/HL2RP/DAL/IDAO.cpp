#include <cbase.h>
#include "IDAO.h"
#include <HL2RPRules.h>
#include "IDALEngine.h"

CDAOThread::EDAOCollisionResolution IDAO::Collide(IDAO* pDAO)
{
	return CDAOThread::EDAOCollisionResolution::None;
}

CDAOThread::EDAOCollisionResolution IDAO::Collide(CKeyValuesEngine* pKVEngine, IDAO* pDAO)
{
	return Collide(pDAO);
}

CDAOThread::EDAOCollisionResolution IDAO::Collide(CSQLEngine* pSQite, IDAO* pDAO)
{
	return Collide(pDAO);
}

void IDAO::Execute(CSQLiteEngine* pSQLite)
{
	Execute(static_cast<CSQLEngine*>(pSQLite));
}

void IDAO::Execute(CMySQLEngine* pMySQL)
{
	Execute(static_cast<CSQLEngine*>(pMySQL));
}

void IDAO::HandleResults(CKeyValuesEngine* pKVEngine, KeyValues* pResults)
{
	HandleResults(pResults);
}

void IDAO::HandleResults(CSQLEngine* pKVEngine, KeyValues* pResults)
{
	HandleResults(pResults);
}

CDAOThread::EDAOCollisionResolution IDAO::TryResolveReplacement(bool condition)
{
	if (condition)
	{
		return CDAOThread::EDAOCollisionResolution::ReplaceQueued;
	}

	return CDAOThread::EDAOCollisionResolution::None;
}

CDAOThread::EDAOCollisionResolution TryResolveRemoval(bool condition)
{
	if (condition)
	{
		return CDAOThread::EDAOCollisionResolution::RemoveBothAndStop;
	}

	return CDAOThread::EDAOCollisionResolution::None;
}
