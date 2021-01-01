#ifndef ISQL_DRIVER_H
#define ISQL_DRIVER_H
#pragma once

#include "idatabase_driver.h"
#include <fmtstr.h>
#include <platform.h>

#define SQLITE3_DRIVER_IMPL_VERSION_STRING "SQLite3Driver001"
#define MYSQL_DRIVER_IMPL_VERSION_STRING   "MySQLDriver001"

#define SQL_TRANSACTION_BEGIN_QUERY "BEGIN;"
#define SQL_TRANSACTION_END_QUERY   "COMMIT;"

#define SQL_MAX_QUERY_SIZE 1024

class CRecordListDTO;
class CRecordNodeDTO;

typedef CFmtStrN<SQL_MAX_QUERY_SIZE> CSQLQuery;

abstract_class ISQLPreparedStatement
{
public:
	virtual ~ISQLPreparedStatement() {};

	virtual void BindString(uint position, const char*) = 0;
	virtual void Execute(CRecordListDTO* pDestResults = NULL) = 0;
};

struct SSQLDriverFeatures
{
	const char* mpUInt64ColumnKeyword, * mpAutoIncrementKeyword = "";
	bool mUsesStrictForeignKeyChecks = false; // Used for multiple tests (if true, targets MySQL in practice)
};

struct SSQLDuplicateKeyConflictInfo
{
	bool mAppendIndexNames = false;
	const char* mpExtraClause = "", * mpAutoDeducedValueFormat;
};

abstract_class ISQLDriver : public IDatabaseDriver
{
public:
	virtual bool Connect(const char* pDatabaseName, const char* pHostName,
		const char* pUserName, const char* pPassword, int port) = 0;
	virtual void ExecuteQuery(const char* pQuery, CRecordListDTO* pDestResults = NULL) = 0;
	virtual ISQLPreparedStatement* PrepareStatement(const char* pQuery) = 0;
	virtual void GetFeatures(SSQLDriverFeatures&) = 0;
	virtual void GetDuplicateKeyConflictInfo(CRecordNodeDTO*, CSQLQuery&, SSQLDuplicateKeyConflictInfo&) = 0;

	// Retrieves the preexisting table column names, saving the results in the parameter.
	// Returns a driver-fixed column name that contains the retrieved table column names.
	virtual const char* GetTableColumnNames(const char* pTableName, CRecordListDTO* pColumnNames) = 0;

	void ExecuteFormattedQuery(CRecordListDTO* pDestResults, const char* pQueryFormat, ...);
};

#endif // !ISQL_DRIVER_H
