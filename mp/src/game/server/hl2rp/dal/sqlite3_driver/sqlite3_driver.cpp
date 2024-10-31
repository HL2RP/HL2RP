// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <isql_driver.h>
#include <idao.h>
#include <sqlite3.h>
#include <smartptr.h>

class CSQLite3IOException : public CDatabaseIOException
{
public:
	CSQLite3IOException(sqlite3* pConnection, const char* pQuery)
	{
		Warning("SQLite3 error on query \"%s\": %s [Code: %i]\n", pQuery,
			sqlite3_errmsg(pConnection), sqlite3_errcode(pConnection));
	}
};

class CSQLite3PreparedStatement : public ISQLPreparedStatement
{
	~CSQLite3PreparedStatement() OVERRIDE
	{
		sqlite3_finalize(mpStmt);
	}

	void BindString(uint, const char*) OVERRIDE;

	sqlite3* mpConnection;
	sqlite3_stmt* mpStmt;

public:
	CSQLite3PreparedStatement(sqlite3* pConnection, sqlite3_stmt* pStmt) : mpConnection(pConnection), mpStmt(pStmt) {}

	void Execute(CRecordListDTO*) OVERRIDE;
};

void CSQLite3PreparedStatement::BindString(uint position, const char* pValue)
{
	if (sqlite3_bind_text(mpStmt, position, pValue, -1, NULL) != SQLITE_OK)
	{
		throw CSQLite3IOException(mpConnection, sqlite3_sql(mpStmt));
	}
}

void CSQLite3PreparedStatement::Execute(CRecordListDTO* pDestResults)
{
	switch (sqlite3_step(mpStmt))
	{
	case SQLITE_ROW:
	{
		if (pDestResults != NULL)
		{
			do
			{
				CFieldDictionaryDTO& destResult = pDestResults->CreateRecord();

				for (int i = sqlite3_column_count(mpStmt); --i >= 0;)
				{
					const char* pColumnName = sqlite3_column_name(mpStmt, i);

					switch (sqlite3_column_type(mpStmt, i))
					{
					case SQLITE_INTEGER:
					{
						destResult.Insert(pColumnName, (uint64)sqlite3_column_int64(mpStmt, i));
						break;
					}
					case SQLITE_FLOAT:
					{
						destResult.Insert(pColumnName, (float)sqlite3_column_double(mpStmt, i));
						break;
					}
					case SQLITE_TEXT:
					{
						destResult.Insert(pColumnName, (char*)sqlite3_column_text(mpStmt, i));
						break;
					}
					}
				}
			} while (sqlite3_step(mpStmt) == SQLITE_ROW);
		}
	}
	case SQLITE_OK:
	case SQLITE_DONE:
	{
		return;
	}
	}

	throw CSQLite3IOException(mpConnection, sqlite3_sql(mpStmt));
}

class CSQLite3Driver : public ISQLDriver
{
	bool Connect(const char*, const char*, const char*, const char*, int) OVERRIDE;
	void DispatchExecuteIO(IDAO*) OVERRIDE;
	void ExecuteQuery(const char*, CRecordListDTO* = NULL) OVERRIDE;
	ISQLPreparedStatement* PrepareStatement(const char*) OVERRIDE;
	void GetFeatures(SSQLDriverFeatures&) OVERRIDE;
	const char* GetTableColumnNames(const char*, CRecordListDTO*) OVERRIDE;
	void GetDuplicateKeyConflictInfo(CRecordNodeDTO*, CSQLQuery&, SSQLDuplicateKeyConflictInfo&) OVERRIDE;

	sqlite3* mpConnection = NULL;

public:
	~CSQLite3Driver() OVERRIDE
	{
		sqlite3_close_v2(mpConnection);
	}
};

bool CSQLite3Driver::Connect(const char* pDatabaseName, const char* pHostName,
	const char* pUserName, const char* pPassword, int port)
{
	if (sqlite3_open(pDatabaseName, &mpConnection) == SQLITE_OK)
	{
		if (IsRelease())
		{
			try
			{
				// Enable Write Ahead Logging (WAL). Makes execution faster and reduces concurrent access lock errors.
				// This disposes an intermediate file where data is written faster than usual, before being flushed
				// to the main file each N-byte blocks. Since this file is unsupported by inspection tools and it
				// can take some time for the main file to be updated, we'll only enable WAL in release mode.
				CSQLite3Driver::ExecuteQuery("PRAGMA journal_mode = WAL;");
			}
			catch (...)
			{

			}
		}

		return true;
	}

	Warning("SQLite3 error: %s [Code: %i]\n", sqlite3_errmsg(mpConnection), sqlite3_errcode(mpConnection));
	return false;
}

void CSQLite3Driver::DispatchExecuteIO(IDAO* pDAO)
{
	try
	{
		CSQLite3Driver::ExecuteQuery(SQL_TRANSACTION_BEGIN_QUERY);
		pDAO->ExecuteIO(this);
		CSQLite3Driver::ExecuteQuery(SQL_TRANSACTION_END_QUERY);
	}
	catch (const CSQLite3IOException&)
	{
		// SQLite3 doesn't terminate transactions on failure. End it, otherwise next one would fail on BEGIN.
		CSQLite3Driver::ExecuteQuery(SQL_TRANSACTION_END_QUERY);
		throw;
	}
}

void CSQLite3Driver::ExecuteQuery(const char* pQuery, CRecordListDTO* pDestResults)
{
	CPlainAutoPtr<ISQLPreparedStatement> preparedStatement(CSQLite3Driver::PrepareStatement(pQuery));
	preparedStatement->Execute(pDestResults);
}

ISQLPreparedStatement* CSQLite3Driver::PrepareStatement(const char* pQuery)
{
	sqlite3_stmt* pStmt;

	if (sqlite3_prepare_v2(mpConnection, pQuery, -1, &pStmt, NULL) != SQLITE_OK)
	{
		throw CSQLite3IOException(mpConnection, pQuery);
	}

	return (new CSQLite3PreparedStatement(mpConnection, pStmt));
}

void CSQLite3Driver::GetFeatures(SSQLDriverFeatures& features)
{
	features.mpUInt64ColumnKeyword = "UNSIGNED BIG INT";
}

const char* CSQLite3Driver::GetTableColumnNames(const char* pTableName, CRecordListDTO* pColumnNames)
{
	char query[SQL_QUERY_MAX_SIZE];
	V_sprintf_safe(query, "PRAGMA TABLE_INFO (`%s`);", pTableName);
	CSQLite3Driver::ExecuteQuery(query, pColumnNames);
	return "name";
}

void CSQLite3Driver::GetDuplicateKeyConflictInfo(CRecordNodeDTO* pRecord,
	CSQLQuery& query, SSQLDuplicateKeyConflictInfo& info)
{
	query += " ON CONFLICT";
	info.mpExtraClause = " DO NOTHING";
	info.mpAutoDeducedValueFormat = "excluded.`%s`";

	// If normal fields are present, we want to update them on conflict.
	// For this, SQLite3 requires the "conflict target" to be manually specified, so, inform about it.
	if (pRecord->mNormalFieldByName.Count() > 0)
	{
		info.mAppendIndexNames = true;
		info.mpExtraClause = " DO UPDATE SET ";
	}
}

EXPOSE_SINGLE_INTERFACE(CSQLite3Driver, ISQLDriver, SQLITE3_DRIVER_IMPL_VERSION_STRING);
