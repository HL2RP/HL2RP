// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "idto.h"

#define SQL_INT_COLUMN_TYPE     "INTEGER"
#define SQL_FLOAT_COLUMN_TYPE   "DECIMAL"
#define SQL_VARCHAR_COLUMN_TYPE "VARCHAR"

const char* SFieldDTO::ToString(CNumStr&& numberFormatter) const
{
	switch (mType)
	{
	case EFieldDTOType::Integer:
	{
		numberFormatter.SetInt32(mInt);
		break;
	}
	case EFieldDTOType::LongUnsignedInteger:
	{
		numberFormatter.SetUint64(mUInt64);
		break;
	}
	case EFieldDTOType::Float:
	{
		numberFormatter.SetFloat(mFloat);
		break;
	}
	default: // EFieldDTOType::String
	{
		return mString;
	}
	}

	return numberFormatter;
}

int CFieldDictionaryDTO::GetInt(const char* pName)
{
	int index = Find(pName);

	if (IsValidIndex(index))
	{
		if (Element(index).mType == EFieldDTOType::String) // Support loaded KeyValues
		{
			return Q_atoi(Element(index).mString);
		}

		return Element(index).mInt;
	}

	return 0;
}

uint64 CFieldDictionaryDTO::GetUInt64(const char* pName)
{
	int index = Find(pName);

	if (IsValidIndex(index))
	{
		if (Element(index).mType == EFieldDTOType::String) // Support loaded KeyValues
		{
			return Q_atoui64(Element(index).mString);
		}

		return Element(index).mUInt64;
	}

	return 0;
}

float CFieldDictionaryDTO::GetFloat(const char* pName)
{
	int index = Find(pName);

	if (IsValidIndex(index))
	{
		if (Element(index).mType == EFieldDTOType::String) // Support loaded KeyValues
		{
			return Q_atof(Element(index).mString);
		}

		return Element(index).mFloat;
	}

	return 0.0f;
}

const char* CFieldDictionaryDTO::GetString(const char* pName)
{
	int index = Find(pName);

	if (IsValidIndex(index))
	{
		return Element(index).mString;
	}

	return "";
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
		int index = Find(other.GetElementName(i));

		if (!IsValidIndex(index))
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
		const char* pFieldName = pKeyValues->GetString(GetElementName(index));

		switch (Element(index).mType)
		{
		case EFieldDTOType::Integer:
		{
			return (Q_atoi(pFieldName) == Element(index).mInt);
		}
		case EFieldDTOType::LongUnsignedInteger:
		{
			return (Q_atoui64(pFieldName) == Element(index).mUInt64);
		}
		case EFieldDTOType::Float:
		{
			return (Q_atof(pFieldName) == Element(index).mFloat);
		}
		}

		return (Q_strcmp(pFieldName, Element(index).mString) == 0); // EFieldDTOType::String
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

CRecordNodeDTO* CNodeDTO::CreateNode(const char* pName, bool force)
{
	int index;

	if (force || !IsValidIndex(index = Find(pName)))
	{
		index = Insert(pName, new CRecordNodeDTO);
	}

	return Element(index);
}

CRecordNodeDTO* CRecordNodeDTO::AddIndexField(const char* pValue, bool force)
{
	CRecordNodeDTO* pNode = CreateNode(pValue, force);
	pNode->mIndexFieldByName.MergeFrom(mIndexFieldByName);
	pNode->mNormalFieldByName.MergeFrom(mNormalFieldByName);
	return pNode;
}

CRecordNodeDTO* CRecordNodeDTO::AddIndexField(const char* pName, const char* pValue, bool force)
{
	CRecordNodeDTO* pNode = AddIndexField(pValue, force);
	pNode->mIndexFieldByName.SetField(pName, pValue);
	return pNode;
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

SDatabaseIdDTO::SDatabaseIdDTO(int id) : mId(id)
{

}

SDatabaseIdDTO::operator int()
{
	return mId;
}

bool SDatabaseIdDTO::IsValid()
{
	return (mId > IDTO_LOADING_DATABASE_ID);
}

bool SDatabaseIdDTO::SetForLoading()
{
	if (mId < IDTO_LOADING_DATABASE_ID)
	{
		mId = IDTO_LOADING_DATABASE_ID;
		return true;
	}

	return false;
}

CRecordNodeDTO* CDatabaseNodeDTO::GetCollection(const char* pName)
{
	int index = Find(pName);
	return (IsValidIndex(index) ? Element(index) : NULL);
}

CRecordNodeDTO* CDatabaseNodeDTO::AddCollection(const char* pName)
{
	return CreateNode(pName, false);
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

class CIntegerSQLColumnDTO : public ISQLColumnDTO
{
	void AppendTo(CSQLQuery& query, const SSQLDriverFeatures&) OVERRIDE
	{
		query += SQL_INT_COLUMN_TYPE;
	}
};

class CLongUnsignedIntegerSQLColumnDTO : public ISQLColumnDTO
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
	return mColumnByName.Insert(pName, new CIntegerSQLColumnDTO);
}

int CSQLTableSetupDTO::CreateUInt64Column(const char* pName)
{
	return mColumnByName.Insert(pName, new CLongUnsignedIntegerSQLColumnDTO);
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
	if (mPrimaryKeyColumnIndices.Count() > 0)
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

	// Add the foreign key constraints just for index columns, which we're already defining
	// for simplicity since some drivers may not support adding unique keys if table exists
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
		const char* pColumnName = column.GetString(pColumnsFieldName);
		int index = mColumnByName.Find(pColumnName);

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
