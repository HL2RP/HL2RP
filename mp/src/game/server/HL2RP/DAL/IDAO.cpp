#include <cbase.h>
#include "IDAO.h"
#include <HL2RPRules.h>

bool IDAO::ShouldBeReplacedBy(IDAO* pDAO)
{
	return false;
}

bool IDAO::ShouldRemoveBoth(IDAO* pDAO)
{
	return false;
}

void IDAO::ExecuteSQLite(CSQLEngine* pSQL)
{
	// If derived DAO didn't implement this, it may define common SQL Engine execution
	ExecuteSQL(pSQL);
}

void IDAO::ExecuteMySQL(CSQLEngine* pSQL)
{
	// If derived DAO didn't implement this, it may define common SQL Engine execution
	ExecuteSQL(pSQL);
}

void IDAO::HandleKeyValuesResults(KeyValues* pResults)
{
	HandleResults(pResults);
}

void IDAO::HandleSQLResults(KeyValues* pResults)
{
	HandleResults(pResults);
}
