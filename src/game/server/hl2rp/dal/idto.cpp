// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "idto.h"

#define SQL_INT_COLUMN_TYPE     "INTEGER"
#define SQL_FLOAT_COLUMN_TYPE   "FLOAT"
#define SQL_VARCHAR_COLUMN_TYPE "VARCHAR"

int CFieldDictionaryDTO::GetInt(const char* pName)
{
	return GetUInt64(pName);
}

uint64 CFieldDictionaryDTO::GetUInt64(const char* pName)
{
	return GetField(pName).ToUInt64();
}

float CFieldDictionaryDTO::GetFloat(const char* pName)
{
	return GetField(pName).ToFloat();
}

CUtlString CFieldDictionaryDTO::GetString(const char* pName)
{
	return GetField(pName).ToString();
}

SUtlField CFieldDictionaryDTO::GetField(const char* pName)
{
	return GetElementOrDefault<const char*, SUtlField>(pName);
}

void CFieldDictionaryDTO::MergeFrom(CFieldDictionaryDTO& other)
{
	FOR_EACH_DICT_FAST(other, i)
	{
		Insert(other.GetElementName(i), other[i]);
	}
}

void CFieldDictionaryDTO::TransferDistinctFrom(CFieldDictionaryDTO& other)
{
	FOR_EACH_DICT_FAST(other, i)
	{
		if (!HasElement(other.GetElementName(i)))
		{
			Insert(other.GetElementName(i), other[i]);
		}
	}
}

bool CFieldDictionaryDTO::FieldsEqual(KeyValues* pKeyValues, int start)
{
	for (; start < MaxElement(); ++start)
	{
		if (!MatchesFieldAt(pKeyValues, start))
		{
			return false;
		}
	}

	return true;
}

bool CFieldDictionaryDTO::MatchesFieldAt(KeyValues* pKeyValues, int index)
{
	if (IsValidIndex(index))
	{
		SUtlField& field = Element(index);
		const char* pFieldName = GetElementName(index);

		switch (field.mType)
		{
		case SUtlField::EType::Int:
		case SUtlField::EType::UInt64:
		{
			return (pKeyValues->GetUint64(pFieldName) == field.mUInt64);
		}
		case SUtlField::EType::Float:
		{
			return (pKeyValues->GetFloat(pFieldName) == field.mFloat);
		}
		}

		return (pKeyValues->GetString(pFieldName) == field.mString);
	}

	return true;
}

void CFieldDictionaryDTO::PopulateOther(KeyValues* pKeyValues)
{
	FOR_EACH_MAP_FAST(m_Elements, i)
	{
		pKeyValues->SetString(GetElementName(i), Element(i).ToString());
	}
}

CRecordListDTO* CLinearDatabaseDTO::GetList(const char* pCollection)
{
	return GetElementOrDefault<const char*, CRecordListDTO*>(pCollection);
}

void CNodeDTO::TransferDistinctFrom(CNodeDTO& other)
{
	FOR_EACH_DICT_FAST(other, i)
	{
		int index = Find(other.GetElementName(i));

		if (IsValidIndex(index))
		{
			Element(index)->TransferDistinctFrom(other[i]);
			continue;
		}

		Insert(other.GetElementName(i), other[i]);
		other.RemoveAt(i);
	}
}

void CNodeDTO::RemoveEqualFrom(CNodeDTO& other)
{
	FOR_EACH_MAP_FAST(m_Elements, i)
	{
		int index = other.Find(GetElementName(i));

		if (other.IsValidIndex(index))
		{
			Element(i)->CNodeDTO::RemoveEqualFrom(*other[index]);

			if (other[index]->Count() < 1)
			{
				delete other[index];
				other.RemoveAt(index);
			}
		}
	}
}

CRecordNodeDTO* CNodeDTO::CreateChild(const char* pName)
{
	int index = Insert(pName, new CRecordNodeDTO);
	return Element(index);
}

CRecordNodeDTO* CRecordNodeDTO::AddIndexField(const char* pName, const SUtlField& field)
{
	CRecordNodeDTO* pChild = CreateChild(field.ToString());
	pChild->mIndexFieldByName.MergeFrom(mIndexFieldByName);
	pChild->mNormalFieldByName.MergeFrom(mNormalFieldByName);
	pChild->mIndexFieldByName.Insert(pName, field);
	return pChild;
}

void CRecordNodeDTO::TransferDistinctFrom(CRecordNodeDTO* pOther)
{
	if (pOther->Count() > 0)
	{
		return CNodeDTO::TransferDistinctFrom(*pOther);
	}

	mIndexFieldByName.TransferDistinctFrom(pOther->mIndexFieldByName);
	mNormalFieldByName.TransferDistinctFrom(pOther->mNormalFieldByName);
}

void CRecordNodeDTO::TraverseAndHandleLeafRecords(CNodeDTOHandler* pHandler, const char* pCollection)
{
	if (Count() < 1)
	{
		return pHandler->HandleLeafRecord(this, pCollection);
	}

	FOR_EACH_MAP_FAST(m_Elements, i)
	{
		Element(i)->TraverseAndHandleLeafRecords(pHandler, pCollection);
	}
}

CRecordNodeDTO* CDatabaseNodeDTO::GetCollection(const char* pName)
{
	return GetElementOrDefault<const char*, CRecordNodeDTO*>(pName);
}

CRecordNodeDTO* CDatabaseNodeDTO::CreateCollection(const char* pName)
{
	return CreateChild(pName);
}

bool CNodeDTOHandler::OnCollectionStarted(CRecordNodeDTO*, const char*)
{
	return true;
}

void CNodeDTOHandler::TraverseAndHandleLeafRecords(CDatabaseNodeDTO& database)
{
	FOR_EACH_DICT_FAST(database, i)
	{
		if (OnCollectionStarted(database[i], database.GetElementName(i)))
		{
			database[i]->TraverseAndHandleLeafRecords(this, database.GetElementName(i));
		}
	}
}

class CIntSQLColumnDTO : public ISQLColumnDTO
{
	void AppendTo(CSQLQuery& query, const SSQLDriverFeatures&) OVERRIDE
	{
		query += SQL_INT_COLUMN_TYPE;
	}
};

class CUInt64SQLColumnDTO : public ISQLColumnDTO
{
	void AppendTo(CSQLQuery& query, const SSQLDriverFeatures& driverFeatures) OVERRIDE
	{
		query += driverFeatures.mpUInt64ColumnKeyword;
	}
};

class CFloatSQLColumnDTO : public ISQLColumnDTO
{
	void AppendTo(CSQLQuery& query, const SSQLDriverFeatures&) OVERRIDE
	{
		query += SQL_FLOAT_COLUMN_TYPE;
	}
};

class CVarCharSQLColumnDTO : public ISQLColumnDTO
{
	void AppendTo(CSQLQuery& query, const SSQLDriverFeatures&) OVERRIDE
	{
		query.AppendFormat("%s (%i)", SQL_VARCHAR_COLUMN_TYPE, mSize);
	}

public:
	CVarCharSQLColumnDTO(int size) : mSize(size) {}

	int mSize;
};

class CTextSQLColumnDTO : public ISQLColumnDTO
{
	void AppendTo(CSQLQuery& query, const SSQLDriverFeatures&) OVERRIDE
	{
		query += "TEXT";
	}
};

CSQLTableSetupDTO::CSQLTableSetupDTO(const char* pTableName, bool autoIncrementPrimaryKey)
	: mpTableName(pTableName), mAutoIncrementPrimaryKey(autoIncrementPrimaryKey)
{
	mColumnByName.SetLessFunc(CaselessStringLessThan);
}

int CSQLTableSetupDTO::CreateIntColumn(const char* pName)
{
	return mColumnByName.Insert(pName, new CIntSQLColumnDTO);
}

int CSQLTableSetupDTO::CreateUInt64Column(const char* pName)
{
	return mColumnByName.Insert(pName, new CUInt64SQLColumnDTO);
}

int CSQLTableSetupDTO::CreateFloatColumn(const char* pName)
{
	return mColumnByName.Insert(pName, new CFloatSQLColumnDTO);
}

int CSQLTableSetupDTO::CreateVarCharColumn(const char* pName, int maxLen)
{
	return mColumnByName.Insert(pName, new CVarCharSQLColumnDTO(maxLen - 1));
}

int CSQLTableSetupDTO::CreateTextColumn(const char* pName)
{
	return mColumnByName.Insert(pName, new CTextSQLColumnDTO);
}

int CSQLTableSetupDTO::CreateUniqueKeyColumnList(int firstColumnIndex)
{
	int index = mUniqueKeysColumnIndices.CreateList();
	mUniqueKeysColumnIndices.AddToTail(index, firstColumnIndex);
	return index;
}

void CSQLTableSetupDTO::AddForeignKey(int localColumnIndex, const char* pReferencedTable,
	const char* pReferencedColumn, bool cascadeOnDelete)
{
	int index = mForeignKeyByColumnName.Insert(mColumnByName.Key(localColumnIndex));
	mForeignKeyByColumnName[index].mpReferencedTable = pReferencedTable;
	mForeignKeyByColumnName[index].mpReferencedColumn = pReferencedColumn;
	mForeignKeyByColumnName[index].mpOnDeleteClause = cascadeOnDelete ? "ON DELETE CASCADE" : "ON DELETE SET NULL";
}

void CSQLTableSetupDTO::CreateTable(ISQLDriver* pDriver, const SSQLDriverFeatures& driverFeatures)
{
	CSQLQuery query("CREATE TABLE IF NOT EXISTS `%s` (", mpTableName);
	const char* pSeparator = "";

	// Add primary key columns
	for (auto index : mPrimaryKeyColumnIndices)
	{
		query.AppendFormat("%s`%s` ", pSeparator, mColumnByName.Key(index));
		mColumnByName[index]->AppendTo(query, driverFeatures);
		pSeparator = ", ";

		if (mAutoIncrementPrimaryKey)
		{
			query += " ";
			query += driverFeatures.mpAutoIncrementKeyword;
			break; // Stop since auto increment is usually restricted to a single column
		}
	}

	// Add unique keys columns
	for (int i = 0; i < mUniqueKeysColumnIndices.TotalCount(); pSeparator = ", ", ++i)
	{
		query.AppendFormat("%s`%s` ", pSeparator, mColumnByName.Key(mUniqueKeysColumnIndices[i]));
		mColumnByName[mUniqueKeysColumnIndices[i]]->AppendTo(query, driverFeatures);
	}

	// Add primary key constraint
	if (!mPrimaryKeyColumnIndices.IsEmpty())
	{
		query += ", PRIMARY KEY (";
		pSeparator = "";

		for (auto index : mPrimaryKeyColumnIndices)
		{
			query.AppendFormat("%s`%s`", pSeparator, mColumnByName.Key(index));
			delete mColumnByName[index];
			mColumnByName.RemoveAt(index);
			pSeparator = ", ";
		}

		query += ")";
	}

	// Add unique keys constraints
	if (mUniqueKeysColumnIndices.TotalCount() > 0)
	{
		for (int i = 0; mUniqueKeysColumnIndices.IsValidList(i); ++i)
		{
			query += ", UNIQUE (";
			pSeparator = "";

			for (int j = mUniqueKeysColumnIndices.Head(i); mUniqueKeysColumnIndices.IsValidIndex(j);
				pSeparator = ", ", j = mUniqueKeysColumnIndices.Next(j))
			{
				query += pSeparator;
				query.AppendFormat("%s`%s`", pSeparator, mColumnByName.Key(mUniqueKeysColumnIndices[j]));
				delete mColumnByName[mUniqueKeysColumnIndices[j]];
				mColumnByName.RemoveAt(mUniqueKeysColumnIndices[j]);
			}

			query += ")";
		}
	}

	// Add the foreign key constraints just for index columns (which we're already defining),
	// since some drivers may not support adding unique keys if table exists
	FOR_EACH_MAP_FAST(mForeignKeyByColumnName, i)
	{
		const char* pLocalColumnName = mForeignKeyByColumnName.Key(i);

		if (!mColumnByName.IsValidIndex(mColumnByName.Find(pLocalColumnName)))
		{
			query.AppendFormat(", FOREIGN KEY (`%s`) REFERENCES `%s` (`%s`) ON UPDATE CASCADE %s",
				pLocalColumnName, mForeignKeyByColumnName[i].mpReferencedTable,
				mForeignKeyByColumnName[i].mpReferencedColumn, mForeignKeyByColumnName[i].mpOnDeleteClause);
			mForeignKeyByColumnName.RemoveAt(i);
		}
	}

	query += ");";
	pDriver->ExecuteQuery(query);
}

void CSQLTableSetupDTO::AddNewColumnsToTable(ISQLDriver* pDriver, const SSQLDriverFeatures& driverFeatures)
{
	CRecordListDTO existingDatabaseColumns;
	const char* pColumnsFieldName = pDriver->GetTableColumnNames(mpTableName, &existingDatabaseColumns);

	for (auto& column : existingDatabaseColumns)
	{
		int index = mColumnByName.Find(column.GetField(pColumnsFieldName));

		if (mColumnByName.IsValidIndex(index))
		{
			delete mColumnByName[index];
			mColumnByName.RemoveAt(index);
		}
	}

	// Add the new normal columns to the table, along any related foreign key constraint
	FOR_EACH_MAP_FAST(mColumnByName, i)
	{
		CSQLQuery query("ALTER TABLE `%s` ADD COLUMN `%s` ", mpTableName, mColumnByName.Key(i));
		mColumnByName[i]->AppendTo(query, driverFeatures);
		int index = mForeignKeyByColumnName.Find(mColumnByName.Key(i));

		if (mForeignKeyByColumnName.IsValidIndex(index))
		{
			// Check whether certain preceding syntax is needed, through this ambiguous condition for now
			if (driverFeatures.mUsesStrictForeignKeyChecks)
			{
				query.AppendFormat(", ADD FOREIGN KEY (`%s`)", mColumnByName.Key(i));
			}

			query.AppendFormat(" REFERENCES `%s` (`%s`) ON UPDATE CASCADE %s",
				mForeignKeyByColumnName[index].mpReferencedTable, mForeignKeyByColumnName[index].mpReferencedColumn,
				mForeignKeyByColumnName[index].mpOnDeleteClause);
		}

		query += ";";
		pDriver->ExecuteQuery(query);
	}
}
