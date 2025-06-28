// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <isql_driver.h>
#include <idao.h>
#include <mysql.h>
#include <utlbinaryblock.h>

class CMySQLIOException : public CDatabaseIOException
{
	void PrintNativeError(const char* pMessage, int code)
	{
		Warning("MySQL error: %s [Code: %i]\n", pMessage, code);
	}

public:
	CMySQLIOException(MYSQL* pConnection)
	{
		PrintNativeError(mysql_error(pConnection), mysql_errno(pConnection));
	}

	CMySQLIOException(MYSQL_STMT* pStmt)
	{
		PrintNativeError(mysql_stmt_error(pStmt), mysql_stmt_errno(pStmt));
	}

	CMySQLIOException(PRINTF_FORMAT_STRING const char* pErrorFormat, ...) FMTFUNCTION(2, 3)
	{
		Warning("MySQL error: ");
		va_list args;
		va_start(args, pErrorFormat);
		WarningV(pErrorFormat, args);
		va_end(args);
		Warning("\n");
	}
};

class CMySQLPreparedStatement : public ISQLPreparedStatement
{
	~CMySQLPreparedStatement() OVERRIDE;

	void BindString(uint, const char*) OVERRIDE;
	void Execute(CRecordListDTO*) OVERRIDE;

	MYSQL_STMT* mpStmt;

	// A MySQL binds strings cache. The binds are later built into a contiguous vector to conform to the MySQL API.
	// It also allows to set bind positions in an unspecified order, and easily check for missing required ones.
	CAutoLessFuncAdapter<CUtlMap<uint, const char*>> mBindStringByIndex;

public:
	CMySQLPreparedStatement(MYSQL_STMT*);
};

CMySQLPreparedStatement::CMySQLPreparedStatement(MYSQL_STMT* pStmt) : mpStmt(pStmt)
{

}

CMySQLPreparedStatement::~CMySQLPreparedStatement()
{
	mysql_stmt_close(mpStmt);
}

void CMySQLPreparedStatement::BindString(uint position, const char* pValue)
{
	uint count = mysql_stmt_param_count(mpStmt);

	if (position < 1 || position > count)
	{
		throw CMySQLIOException("Found invalid position %u while binding a prepared statement."
			" Expected position within range: 1-%u.", position, count);
	}

	mBindStringByIndex.InsertOrReplace(position, pValue);
}

void CMySQLPreparedStatement::Execute(CRecordListDTO* pDestResults)
{
	uint count = mysql_stmt_param_count(mpStmt);

	if (count > 0)
	{
		CUtlVector<MYSQL_BIND> binds;

		// Check for missing required binds
		for (uint position = 1; position <= count; ++position)
		{
			int stringIndex = mBindStringByIndex.Find(position);

			if (!mBindStringByIndex.IsValidIndex(stringIndex))
			{
				throw CMySQLIOException("Found parameter position %u without required bind"
					" while executing a prepared statement", position);
			}

			int bindIndex = binds.AddToTail();
			binds[bindIndex].buffer_type = MYSQL_TYPE_STRING;
			binds[bindIndex].buffer = (void*)mBindStringByIndex[stringIndex];
			binds[bindIndex].buffer_length = Q_strlen(mBindStringByIndex[stringIndex]);
			binds[bindIndex].length = NULL;
			binds[bindIndex].is_null = NULL;
		}

		if (mysql_stmt_bind_param(mpStmt, binds.Base()) != 0)
		{
			throw CMySQLIOException(mpStmt);
		}
	}

	if (mysql_stmt_execute(mpStmt) != 0)
	{
		throw CMySQLIOException(mpStmt);
	}
	else if (pDestResults != NULL)
	{
		MYSQL_RES* pResultMetadata = mysql_stmt_result_metadata(mpStmt);

		if (pResultMetadata != NULL)
		{
			CUtlVector<MYSQL_BIND> resultBinds;
			resultBinds.SetSize(mysql_stmt_field_count(mpStmt));
			CUtlVector<CUtlBinaryBlock> resultBuffers;
			resultBuffers.SetSize(resultBinds.Size());

			FOR_EACH_VEC(resultBinds, i)
			{
				MYSQL_FIELD* pField = mysql_fetch_field_direct(pResultMetadata, i);
				resultBinds[i].buffer_type = pField->type;
				resultBinds[i].is_null = &resultBinds[i].is_null_value;
				resultBinds[i].error = NULL;

				switch (pField->type)
				{
				case MYSQL_TYPE_NEWDECIMAL:
				case MYSQL_TYPE_BLOB:
				case MYSQL_TYPE_STRING:
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_VARCHAR:
				{
					// Set remaining to delay buffer allocation until retrieving actual field length at mysql_stmt_fetch
					resultBinds[i].buffer = NULL;
					resultBinds[i].buffer_length = 0; // NOTE: Must be zero even when buffer is NULL to safely call function
					resultBinds[i].length = &resultBinds[i].length_value;
					continue;
				}
				}

				resultBuffers[i].SetLength(pField->length);
				resultBinds[i].buffer = resultBuffers[i].Get();
				resultBinds[i].buffer_length = pField->length;
				resultBinds[i].length = NULL;
			}

			if (mysql_stmt_bind_result(mpStmt, resultBinds.Base()) != 0 || mysql_stmt_store_result(mpStmt) != 0)
			{
				mysql_free_result(pResultMetadata);
				throw CMySQLIOException(mpStmt);
			}

			while (mysql_stmt_fetch(mpStmt) == 0)
			{
				CFieldDictionaryDTO& destResult = pDestResults->CreateRecord();

				FOR_EACH_VEC(resultBinds, i)
				{
					if (!resultBinds[i].is_null_value)
					{
						const char* pFieldName = mysql_fetch_field_direct(pResultMetadata, i)->name;

						// Check to allocate and fill pending string column buffer for the current field. By providing
						// an extra length slot for the null terminator, mysql_stmt_fetch_column will initialize it.
						if (resultBinds[i].buffer == NULL)
						{
							resultBuffers[i].SetLength(resultBinds[i].length_value + 1);
							resultBinds[i].buffer = resultBuffers[i].Get();
							resultBinds[i].buffer_length = resultBuffers[i].Length();
							mysql_stmt_fetch_column(mpStmt, resultBinds.Base() + i, i, 0);
							destResult.Insert(pFieldName, (char*)resultBinds[i].buffer);
							resultBinds[i].buffer = NULL; // Allow fetching next string-like value
							continue;
						}

						switch (resultBinds[i].buffer_type)
						{
						case MYSQL_TYPE_LONG:
						{
							destResult.Insert(pFieldName, *(int*)resultBinds[i].buffer);
							break;
						}
						case MYSQL_TYPE_LONGLONG:
						{
							destResult.Insert(pFieldName, *(uint64*)resultBinds[i].buffer);
							break;
						}
						case MYSQL_TYPE_FLOAT:
						{
							destResult.Insert(pFieldName, *(float*)resultBinds[i].buffer);
							break;
						}
						case MYSQL_TYPE_DOUBLE:
						{
							destResult.Insert(pFieldName, (float)*(double*)resultBinds[i].buffer);
							break;
						}
						}
					}
				}
			}

			mysql_stmt_free_result(mpStmt);
			mysql_free_result(pResultMetadata);
		}
	}
}

class CMySQLDriver : public ISQLDriver
{
	bool Connect(const char*, const char*, const char*, const char*, int) OVERRIDE;
	void Close() OVERRIDE;
	void DispatchExecuteIO(IDAO*) OVERRIDE;
	void ExecuteQuery(const char*, CRecordListDTO* = NULL) OVERRIDE;
	ISQLPreparedStatement* PrepareStatement(const char*) OVERRIDE;
	void GetFeatures(SSQLDriverFeatures&) OVERRIDE;
	const char* GetTableColumnNames(const char*, CRecordListDTO*) OVERRIDE;
	void GetDuplicateKeyConflictInfo(CRecordNodeDTO*, CSQLQuery&, SSQLDuplicateKeyConflictInfo&) OVERRIDE;

	MYSQL mConnection;

public:
	CMySQLDriver()
	{
		mysql_init(&mConnection);
	}

	~CMySQLDriver() OVERRIDE
	{
		CMySQLDriver::Close();
		mysql_library_end();
	}
};

bool CMySQLDriver::Connect(const char* pDatabaseName, const char* pHostName,
	const char* pUserName, const char* pPassword, int port)
{
	bool autoReconnect = true, reportDataTruncation = false;
	mysql_options(&mConnection, MYSQL_OPT_RECONNECT, &autoReconnect);
	mysql_options(&mConnection, MYSQL_REPORT_DATA_TRUNCATION, &reportDataTruncation);
	mysql_options(&mConnection, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	if (mysql_real_connect(&mConnection, pHostName, pUserName, pPassword, pDatabaseName, port, NULL, 0) != NULL)
	{
		return true;
	}

	CMySQLIOException e(&mConnection);
	return false;
}

void CMySQLDriver::Close()
{
	mysql_close(&mConnection);
}

void CMySQLDriver::DispatchExecuteIO(IDAO* pDAO)
{
	CMySQLDriver::ExecuteQuery(SQL_TRANSACTION_BEGIN_QUERY);
	pDAO->ExecuteIO(this);
	CMySQLDriver::ExecuteQuery(SQL_TRANSACTION_END_QUERY);
}

void CMySQLDriver::ExecuteQuery(const char* pQuery, CRecordListDTO* pDestResults)
{
	if (mysql_query(&mConnection, pQuery) != 0)
	{
		throw CMySQLIOException(&mConnection);
	}
	else if (pDestResults != NULL)
	{
		MYSQL_RES* pSrcResult = mysql_store_result(&mConnection);

		if (pSrcResult != NULL)
		{
			for (MYSQL_ROW srcRow; (srcRow = mysql_fetch_row(pSrcResult)) != NULL;)
			{
				CFieldDictionaryDTO& destResult = pDestResults->CreateRecord();

				for (uint i = 0; i < mysql_num_fields(pSrcResult); ++i)
				{
					if (srcRow[i] != NULL)
					{
						switch (pSrcResult->fields[i].type)
						{
						case MYSQL_TYPE_LONG:
						{
							destResult.Insert(pSrcResult->fields[i].name, Q_atoi(srcRow[i]));
							continue;
						}
						case MYSQL_TYPE_LONGLONG:
						{
							destResult.Insert(pSrcResult->fields[i].name, Q_atoui64(srcRow[i]));
							continue;
						}
						case MYSQL_TYPE_FLOAT:
						{
							destResult.Insert(pSrcResult->fields[i].name, Q_atof(srcRow[i]));
							continue;
						}
						}

						destResult.Insert(pSrcResult->fields[i].name, srcRow[i]);
					}
				}
			}

			mysql_free_result(pSrcResult);
		}
	}
}

ISQLPreparedStatement* CMySQLDriver::PrepareStatement(const char* pQuery)
{
	MYSQL_STMT* pStmt = mysql_stmt_init(&mConnection);

	if (pStmt == NULL)
	{
		throw CMySQLIOException(&mConnection);
	}
	else if (mysql_stmt_prepare(pStmt, pQuery, Q_strlen(pQuery)) != 0)
	{
		throw CMySQLIOException(pStmt);
	}

	return (new CMySQLPreparedStatement(pStmt));
}

void CMySQLDriver::GetFeatures(SSQLDriverFeatures& features)
{
	features.mpUInt64ColumnKeyword = "BIGINT UNSIGNED";
	features.mpAutoIncrementKeyword = "AUTO_INCREMENT";
	features.mUsesStrictForeignKeyChecks = true;
}

const char* CMySQLDriver::GetTableColumnNames(const char* pTableName, CRecordListDTO* pColumnNames)
{
	char query[SQL_QUERY_MAX_SIZE];
	V_sprintf_safe(query, "SHOW FIELDS FROM `%s`;", pTableName);
	CMySQLDriver::ExecuteQuery(query, pColumnNames);
	return "Field";
}

void CMySQLDriver::GetDuplicateKeyConflictInfo(CRecordNodeDTO* pRecord,
	CSQLQuery& query, SSQLDuplicateKeyConflictInfo& info)
{
	query += " ON DUPLICATE KEY UPDATE ";
	info.mpAutoDeducedValueFormat = "VALUES (`%s`)";

	// In case the normal fields are missing, provide a single redundant index field update to succeed
	if (pRecord->mNormalFieldByName.Count() < 1)
	{
		const char* pIndexName = pRecord->mIndexFieldByName.GetElementName(0);
		query.AppendFormat("`%s` = `%s`", pIndexName, pIndexName);
	}
}

EXPOSE_SINGLE_INTERFACE(CMySQLDriver, ISQLDriver, MYSQL_DRIVER_IMPL_VERSION_STRING);
