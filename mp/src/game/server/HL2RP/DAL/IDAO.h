#ifndef IDAO_H
#define IDAO_H
#pragma once

#include <platform.h>

#define IDAO_INVALID_DATABASE_ID	-1
#define IDAO_SQL_VARCHAR_KEY_SIZE	255

class CKeyValuesEngine;
class CSQLEngine;
class KeyValues;

abstract_class IDAO
{
public:
	virtual bool ShouldBeReplacedBy(IDAO * pDAO);
	virtual bool ShouldRemoveBoth(IDAO* pDAO);
	virtual void ExecuteKeyValues(CKeyValuesEngine* pKeyValues) { }
	virtual void ExecuteSQL(CSQLEngine* pSQL) { } // For common SQL Engine execution
	virtual void ExecuteSQLite(CSQLEngine* pSQL);
	virtual void ExecuteMySQL(CSQLEngine* pSQL);
	virtual void HandleResults(KeyValues* pResults) { }
	virtual void HandleKeyValuesResults(KeyValues* pResults);
	virtual void HandleSQLResults(KeyValues* pResults);

	// Casts to explicit template DAO's class easier
	template<class D> D* ToClass();

	// Casts to passed DAO's class easier
	template<class D> D* ToClass(D* const pDAO);
};

template<class D>
FORCEINLINE D* IDAO::ToClass()
{
	return dynamic_cast<D*>(this);
}

template<class D>
FORCEINLINE D* IDAO::ToClass(D* const pDAO)
{
	return ToClass<D>();
}

class CDAOException
{
public:
	// Whether the DAO should be inserted again to the DAO list
	virtual bool ShouldRetry();
};

FORCEINLINE bool CDAOException::ShouldRetry()
{
	return false;
}

#endif // !IDAO_H
