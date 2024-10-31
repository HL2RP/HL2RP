// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "idao.h"
#include "dal.h"
#include "keyvalues_driver.h"
#include <hl2rp_gamerules.h>
#include <smartptr.h>

SCOPED_ENUM(ESQLRecordAppendFlag,
	AppendNames = 1,
	AppendValues = 2,
	AssignFields = AppendValues | AppendNames,
	FilterFields = AssignFields | 4, // Default to 'AND' separate multiple fields
	FilterFieldsInclusive = FilterFields | 8, // Force to 'OR' separate multiple fields
	AutoDeduceValues = AssignFields | 16 // Use driver-dependent automatic value deducing format
)

static CDatabaseSetupDAOFactory& DatabaseSetupDAOFactory()
{
	static CDatabaseSetupDAOFactory sFactory;
	return sFactory;
}

bool IDAO::MustBeRemovedOnLevelShutdown()
{
	return false;
}

bool IDAO::MergeFrom(IDAO* pOther)
{
	return true;
}

bool ILevelInitDAOFactory::CLess::Less(ILevelInitDAOFactory* pLeft, ILevelInitDAOFactory* pRight, void*)
{
	return (pLeft->mType < pRight->mType);
}

ILevelInitDAOFactory::ILevelInitDAOFactory(ELevelInitDAOType priority) : mType(priority)
{
	DAL().mLevelInitDAOFactories.Insert(this);
}

ISQLTableSetupDTOFactory::ISQLTableSetupDTOFactory()
{
	DatabaseSetupDAOFactory().mSQLTableSetupDTOFactories.AddToTail(this);
}

void CDatabaseSetupDAO::ExecuteIO(ISQLDriver* pDriver)
{
	CUtlVectorAutoPurge<CSQLTableSetupDTO*> tableSetupDTOs;
	SSQLDriverFeatures features;
	pDriver->GetFeatures(features);

	if (features.mUsesStrictForeignKeyChecks)
	{
		pDriver->ExecuteQuery(NULL, "SET FOREIGN_KEY_CHECKS = 0;");
	}

	for (auto pTableSetupDTOFactory : DatabaseSetupDAOFactory().mSQLTableSetupDTOFactories)
	{
		int index = tableSetupDTOs.AddToTail(pTableSetupDTOFactory->Create());
		tableSetupDTOs[index]->CreateTable(pDriver, features);
	}

	for (auto pTableSetupDTO : tableSetupDTOs)
	{
		pTableSetupDTO->AddNewColumnsToTable(pDriver, features);
	}

	if (features.mUsesStrictForeignKeyChecks)
	{
		pDriver->ExecuteQuery(NULL, "SET FOREIGN_KEY_CHECKS = 1;");
	}
}

CKeyValuesDTOHandler::CKeyValuesDTOHandler(CKeyValuesDriver* pDriver) : mpDriver(pDriver)
{

}

bool CKeyValuesDTOHandler::OnCollectionStarted(CRecordNodeDTO* pCollection, const char* pName)
{
	mpCurIOCollection = pCollection;

	if (pCollection->Count() > 0)
	{
		FOR_EACH_DICT_FAST(*pCollection, i)
		{
			mpDriver->LoadFromFile(pName, pCollection->GetElementName(i), this);
		}
	}
	else if (pCollection->mIndexFieldByName.IsValidIndex(0))
	{
		mpDriver->LoadFromFile(pName, pCollection->mIndexFieldByName[0].ToString(), this);
	}
	else
	{
		OnIndexLessCollectionStarted(pName);
	}

	return false;
}

void CKeyValuesDTOHandler::OnIndexLessCollectionStarted(const char* pName)
{
	mpDriver->LoadFromDirectory(pName, this);
}

void CKeyValuesDTOHandler::HandleLoadedFileData(KeyValues* pFileData, const char*)
{
	if (pFileData->GetFirstTrueSubKey() == NULL)
	{
		return HandleLoadedRecord(pFileData, pFileData);
	}

	// Safely loop all true subkeys, accounting for the case some of them get removed
	for (KeyValues* pCurRecord = pFileData->GetFirstTrueSubKey(), *pNextRecord;
		pCurRecord != NULL; pCurRecord = pNextRecord)
	{
		pNextRecord = pCurRecord->GetNextTrueSubKey();
		HandleLoadedRecord(pFileData, pCurRecord);
	}
}

void CKeyValuesDTOHandler::HandleLoadedRecord(KeyValues* pFileData, KeyValues* pLoadedRecord)
{
	FindMatchingLocalRecord(pFileData, pLoadedRecord, mpCurIOCollection, 0);
}

bool CKeyValuesDTOHandler::FindMatchingLocalRecord(KeyValues* pFileData,
	KeyValues* pLoadedRecord, CRecordNodeDTO* pSearchRecord, int depth)
{
	if (pSearchRecord->Count() < 1)
	{
		return (pSearchRecord->mIndexFieldByName.FieldsEqual(pLoadedRecord, depth)
			&& OnLoadedRecordMatching(pFileData, pLoadedRecord, pSearchRecord));
	}

	FOR_EACH_DICT_FAST(*pSearchRecord, i)
	{
		if (pSearchRecord->Element(i)->mIndexFieldByName.MatchesFieldAt(pLoadedRecord, depth)
			&& FindMatchingLocalRecord(pFileData, pLoadedRecord, pSearchRecord->Element(i), depth + 1))
		{
			return true;
		}
	}

	return false;
}

class CLoadKeyValuesDTOHandler : public CKeyValuesDTOHandler
{
	bool OnCollectionStarted(CRecordNodeDTO* pCollection, const char* pName) OVERRIDE
	{
		if (!mResultDatabase.HasElement(pName))
		{
			mResultDatabase.Insert(pName, new CRecordListDTO);
		}

		return CKeyValuesDTOHandler::OnCollectionStarted(pCollection, pName);
	}

	bool OnLoadedRecordMatching(KeyValues*, KeyValues*, CRecordNodeDTO*) OVERRIDE;

	CLinearDatabaseDTO& mResultDatabase;

public:
	CLoadKeyValuesDTOHandler(CKeyValuesDriver* pDriver, CLinearDatabaseDTO& resultDatabase)
		: CKeyValuesDTOHandler(pDriver), mResultDatabase(resultDatabase) {}
};

bool CLoadKeyValuesDTOHandler::OnLoadedRecordMatching(KeyValues*,
	KeyValues* pLoadedRecord, CRecordNodeDTO* pMatchingRecord)
{
	// Match also against all selected normal fields and create the result record on success
	if (pMatchingRecord->mNormalFieldByName.FieldsEqual(pLoadedRecord))
	{
		CFieldDictionaryDTO& destResult = mResultDatabase[mResultDatabase.Count() - 1]->CreateRecord();

		FOR_EACH_VALUE(pLoadedRecord, pField)
		{
			switch (pField->GetDataType())
			{
			case KeyValues::TYPE_INT:
			{
				destResult.Insert(pField->GetName(), pField->GetInt());
				continue;
			}
			case KeyValues::TYPE_UINT64:
			{
				destResult.Insert(pField->GetName(), pField->GetUint64());
				continue;
			}
			case KeyValues::TYPE_FLOAT:
			{
				destResult.Insert(pField->GetName(), pField->GetFloat());
				continue;
			}
			}

			destResult.Insert(pField->GetName(), pField->GetString());
		}

		return true;
	}

	return false;
}

class CSaveKeyValuesDTOHandler : public CKeyValuesDTOHandler
{
	void HandleLoadedFileData(KeyValues*, const char*) OVERRIDE;
	bool OnLoadedRecordMatching(KeyValues*, KeyValues*, CRecordNodeDTO*) OVERRIDE;
	void HandleLeafRecord(CRecordNodeDTO*, const char*) OVERRIDE;

	KeyValues* mpFileData = NULL;

public:
	CSaveKeyValuesDTOHandler(CKeyValuesDriver* pDriver) : CKeyValuesDTOHandler(pDriver) {}
};

void CSaveKeyValuesDTOHandler::HandleLoadedFileData(KeyValues* pFileData, const char* pBaseFileName)
{
	CKeyValuesDTOHandler::HandleLoadedFileData(pFileData, pBaseFileName);

	// Set index-matching node associated to the file for final save traversal, and save to file
	int index = mpCurIOCollection->Find(pBaseFileName);
	CRecordNodeDTO* pIndexRecord = mpCurIOCollection->IsValidIndex(index) ?
		mpCurIOCollection->Element(index) : mpCurIOCollection;
	mpFileData = pFileData;
	pIndexRecord->TraverseAndHandleLeafRecords(this, pFileData->GetName());
	mpDriver->SaveToFile(pFileData, pFileData->GetName(), pBaseFileName);
}

bool CSaveKeyValuesDTOHandler::OnLoadedRecordMatching(KeyValues*,
	KeyValues* pLoadedRecord, CRecordNodeDTO* pMatchingRecord)
{
	if (pMatchingRecord->mIndexFieldByName.Count() > 0) // Is the record still valid for saving?
	{
		// Update target save KeyValues
		pMatchingRecord->mNormalFieldByName.PopulateOther(pLoadedRecord);

		// "Invalidate" local record to prevent it from resulting in a duplicated KeyValue on saving
		pMatchingRecord->mIndexFieldByName.Purge();

		return true;
	}

	return false;
}

void CSaveKeyValuesDTOHandler::HandleLeafRecord(CRecordNodeDTO* pRecord, const char*)
{
	if (pRecord->mIndexFieldByName.Count() > 0) // Is the record still valid for saving?
	{
		KeyValues* pSaveRecord = (pRecord->mIndexFieldByName.Count() > 1) ? mpFileData->CreateNewKey() : mpFileData;
		pRecord->mIndexFieldByName.PopulateOther(pSaveRecord);
		pRecord->mNormalFieldByName.PopulateOther(pSaveRecord);
	}
}

class CAutoIncrementLoadKeyValuesDTOHandler : public CKeyValuesDTOHandler
{
	bool OnCollectionStarted(CRecordNodeDTO*, const char*) OVERRIDE;
	void HandleLoadedRecord(KeyValues*, KeyValues*) OVERRIDE;
	bool OnLoadedRecordMatching(KeyValues*, KeyValues*, CRecordNodeDTO*) OVERRIDE;

	void HandleLeafRecord(CRecordNodeDTO* pRecord, const char*) OVERRIDE
	{
		pRecord->AddIndexField(mpInsertDAO->mpAutoIncrementField, mpInsertDAO->mResultId);
	}

	CAutoIncrementInsertDAO* mpInsertDAO;

public:
	CAutoIncrementLoadKeyValuesDTOHandler(CKeyValuesDriver* pDriver, CAutoIncrementInsertDAO* pInsertDAO)
		: CKeyValuesDTOHandler(pDriver), mpInsertDAO(pInsertDAO) {}
};

bool CAutoIncrementLoadKeyValuesDTOHandler::OnCollectionStarted(CRecordNodeDTO* pCollection, const char* pName)
{
	try
	{
		mpCurIOCollection = pCollection;
		mpDriver->LoadFromDirectory(pName, this);
	}
	catch (const CDatabaseIOException&)
	{
		// Some loaded record matched the local one
	}

	return true;
}

void CAutoIncrementLoadKeyValuesDTOHandler::HandleLoadedRecord(KeyValues* pFileData, KeyValues* pRecord)
{
	int oldResultId = mpInsertDAO->mResultId;
	mpInsertDAO->mResultId = pRecord->GetInt(mpInsertDAO->mpAutoIncrementField);
	CKeyValuesDTOHandler::HandleLoadedRecord(pFileData, pRecord);
	mpInsertDAO->mResultId = Max(oldResultId, mpInsertDAO->mResultId + 1);
}

bool CAutoIncrementLoadKeyValuesDTOHandler::OnLoadedRecordMatching(KeyValues*,
	KeyValues*, CRecordNodeDTO* pMatchingRecord)
{
	// If loaded record matches our real "unique key" identifiers, stop the process
	if (pMatchingRecord->mIndexFieldByName.Count() > 1 || mpInsertDAO->mIsFirstIndexWithinUniqueKey)
	{
		throw CDatabaseIOException();
	}

	return true;
}

class CDeleteKeyValuesDTOHandler : public CKeyValuesDTOHandler
{
	void OnIndexLessCollectionStarted(const char* pName) OVERRIDE
	{
		mpDriver->DeleteDirectory(pName);
	}

	void HandleLoadedFileData(KeyValues*, const char*) OVERRIDE;
	bool OnLoadedRecordMatching(KeyValues*, KeyValues*, CRecordNodeDTO*) OVERRIDE;

public:
	CDeleteKeyValuesDTOHandler(CKeyValuesDriver* pDriver) : CKeyValuesDTOHandler(pDriver) {}
};

void CDeleteKeyValuesDTOHandler::HandleLoadedFileData(KeyValues* pFileData, const char* pBaseFileName)
{
	CKeyValuesDTOHandler::HandleLoadedFileData(pFileData, pBaseFileName);

	if (pFileData->IsEmpty())
	{
		// Delete file to save filesystem space, which wouldn't have any record
		return mpDriver->DeleteFile(pFileData->GetName(), pBaseFileName);
	}

	mpDriver->SaveToFile(pFileData, pFileData->GetName(), pBaseFileName);
}

bool CDeleteKeyValuesDTOHandler::OnLoadedRecordMatching(KeyValues* pFileData,
	KeyValues* pLoadedRecord, CRecordNodeDTO* pMatchingRecord)
{
	if (pMatchingRecord->mNormalFieldByName.FieldsEqual(pLoadedRecord))
	{
		(pLoadedRecord == pFileData) ? pFileData->Clear() : pFileData->RemoveSubKey(pLoadedRecord);
		return true;
	}

	return false;
}

class CSQLDTOHandler : public CNodeDTOHandler
{
protected:
	CSQLDTOHandler(ISQLDriver* pDriver) : mpDriver(pDriver) {}

	void AppendRecord(CRecordNodeDTO*, ESQLRecordAppendFlag, const char* pAutoDeducedValueFormat = NULL);
	const char* AppendFields(const CFieldDictionaryDTO&, ESQLRecordAppendFlag, const char* pSeparator = "",
		const char* pAutoDeducedValueFormat = NULL, int start = 0); // Returns updated separator
	const char* AppendFieldAt(const CFieldDictionaryDTO&, int index, ESQLRecordAppendFlag,
		const char* pSeparator = "", const char* pAutoDeducedValueFormat = NULL); // Returns updated separator
	const char* AppendField(const SUtlField&, const char* pName, ESQLRecordAppendFlag,
		const char* pSeparator = "", const char* pAutoDeducedValueFormat = NULL); // Returns updated separator
	void Execute(CRecordListDTO* pResults);

	ISQLDriver* mpDriver;
	CSQLQuery mQuery;
	CUtlVector<const char*> mStringValues;
};

void CSQLDTOHandler::AppendRecord(CRecordNodeDTO* pRecord, ESQLRecordAppendFlag flags,
	const char* pAutoDeducedValueFormat)
{
	const char* pSeparator = AppendFields(pRecord->mIndexFieldByName, flags, "", pAutoDeducedValueFormat);
	AppendFields(pRecord->mNormalFieldByName, flags, pSeparator, pAutoDeducedValueFormat);
}

const char* CSQLDTOHandler::AppendFields(const CFieldDictionaryDTO& fieldByName, ESQLRecordAppendFlag flags,
	const char* pSeparator, const char* pAutoDeducedValueFormat, int start)
{
	for (; start < fieldByName.MaxElement(); ++start)
	{
		if (fieldByName.IsValidIndex(start))
		{
			pSeparator = AppendFieldAt(fieldByName, start, flags, pSeparator, pAutoDeducedValueFormat);
		}
	}

	return pSeparator;
}

const char* CSQLDTOHandler::AppendFieldAt(const CFieldDictionaryDTO& fieldByName, int index,
	ESQLRecordAppendFlag flags, const char* pSeparator, const char* pAutoDeducedValueFormat)
{
	return AppendField(fieldByName[index], fieldByName.GetElementName(index),
		flags, pSeparator, pAutoDeducedValueFormat);
}

const char* CSQLDTOHandler::AppendField(const SUtlField& field, const char* pName, ESQLRecordAppendFlag flags,
	const char* pSeparator, const char* pAutoDeducedValueFormat)
{
	mQuery += pSeparator;
	pSeparator = ", ";

	if (FBitSet(flags, ESQLRecordAppendFlag::AppendNames))
	{
		mQuery.AppendFormat("`%s`", pName);

		if (FBitSet(flags, ESQLRecordAppendFlag::AppendValues))
		{
			mQuery += " = ";

			if (flags == ESQLRecordAppendFlag::FilterFields)
			{
				pSeparator = " AND ";
			}
			else if (flags == ESQLRecordAppendFlag::FilterFieldsInclusive)
			{
				pSeparator = " OR ";
			}
		}
	}

	if (FBitSet(flags, ESQLRecordAppendFlag::AppendValues))
	{
		if (flags == ESQLRecordAppendFlag::AutoDeduceValues)
		{
			mQuery.AppendFormat(pAutoDeducedValueFormat, pName);
		}
		else if (field.mType == SUtlField::EType::String)
		{
			mQuery += "?";
			mStringValues.AddToTail(field);
		}
		else
		{
			mQuery += (field.mType == SUtlField::EType::Null) ? "NULL" : field.ToString();
		}
	}

	return pSeparator;
}

void CSQLDTOHandler::Execute(CRecordListDTO* pResults)
{
	mQuery += ";";

	if (mStringValues.IsEmpty())
	{
		mpDriver->ExecuteQuery(mQuery, pResults);
	}
	else
	{
		CPlainAutoPtr<ISQLPreparedStatement> preparedStatement(mpDriver->PrepareStatement(mQuery));

		FOR_EACH_VEC(mStringValues, i)
		{
			preparedStatement->BindString(i + 1, mStringValues[i]);
		}

		preparedStatement->Execute(pResults);
	}

	mQuery.Clear();
	mStringValues.Purge();
}

class CFilteringSQLDTOHandler : public CSQLDTOHandler
{
	// Appends our filters grouping them with parenthesized 'IN' clauses when possible in order to optimize the query size
	void AppendRecords(CRecordNodeDTO*, const char* pSeparator, int depth = 0);

protected:
	CFilteringSQLDTOHandler(ISQLDriver* pDriver) : CSQLDTOHandler(pDriver) {}

	bool OnCollectionStarted(CRecordNodeDTO* pCollection, const char*) OVERRIDE
	{
		AppendRecords(pCollection, " WHERE ");
		return false;
	}

	void Execute(CRecordListDTO* pResults)
	{
		CSQLDTOHandler::Execute(pResults);
	}
};

void CFilteringSQLDTOHandler::AppendRecords(CRecordNodeDTO* pNode, const char* pSeparator, int depth)
{
	if (pNode->Count() < 1)
	{
		if (depth > 0)
		{
			pSeparator = " AND ";
		}

		pSeparator = AppendFields(pNode->mIndexFieldByName, ESQLRecordAppendFlag::FilterFields, pSeparator, NULL, depth);
		AppendFields(pNode->mNormalFieldByName, ESQLRecordAppendFlag::FilterFields, pSeparator);
		return;
	}
	else if (depth > 0)
	{
		mQuery += " AND (";
	}

	CUtlHashtable<const char*, CUtlVector<SUtlField*>> groupableFieldsByName; // Groupable fields into 'IN' clauses

	FOR_EACH_DICT_FAST(*pNode, i)
	{
		CRecordNodeDTO* pChild = pNode->Element(i);

		// If record has siblings, is a leaf node and doesn't contain normal fields, then we can group it
		if (pNode->Count() > 1 && pChild->Count() < 1 && pChild->mNormalFieldByName.Count() < 1)
		{
			const char* pFieldName = pChild->mIndexFieldByName.GetElementName(depth);
			groupableFieldsByName[groupableFieldsByName.Insert(pFieldName)]
				.AddToTail(&pChild->mIndexFieldByName[depth]);
			continue;
		}

		pSeparator = AppendFieldAt(pChild->mIndexFieldByName, depth,
			ESQLRecordAppendFlag::FilterFieldsInclusive, pSeparator);
		AppendRecords(pChild, "", depth + 1);
	}

	FOR_EACH_HASHTABLE(groupableFieldsByName, i)
	{
		mQuery.AppendFormat("%s%s IN (", pSeparator, groupableFieldsByName.Key(i));
		pSeparator = "";

		for (auto pField : groupableFieldsByName[i])
		{
			pSeparator = AppendField(*pField, groupableFieldsByName.Key(i),
				ESQLRecordAppendFlag::AppendValues, pSeparator);
		}

		mQuery += ")";
		pSeparator = " OR ";
	}

	if (depth > 0)
	{
		mQuery += ")";
	}
}

class CLoadSQLDTOHandler : public CFilteringSQLDTOHandler
{
	bool OnCollectionStarted(CRecordNodeDTO* pCollection, const char* pName) OVERRIDE
	{
		mQuery.AppendFormat("SELECT * FROM `%s`", pName);
		bool traverse = CFilteringSQLDTOHandler::OnCollectionStarted(pCollection, pName);
		int index = mResultDatabase.Find(pName);

		if (!mResultDatabase.IsValidIndex(index))
		{
			index = mResultDatabase.Insert(pName, new CRecordListDTO);
		}

		Execute(mResultDatabase[index]);
		return traverse;
	}

	CLinearDatabaseDTO& mResultDatabase;

public:
	CLoadSQLDTOHandler(ISQLDriver* pDriver, CLinearDatabaseDTO& resultDatabase)
		: CFilteringSQLDTOHandler(pDriver), mResultDatabase(resultDatabase) {}
};

class CSaveSQLDTOHandler : public CSQLDTOHandler
{
	void HandleLeafRecord(CRecordNodeDTO* pRecord, const char* pCollection) OVERRIDE
	{
		// If the first index isn't really part of an unique key, move it to the normal fields
		if (!mIsFirstIndexWithinUniqueKey && pRecord->mIndexFieldByName.IsValidIndex(0))
		{
			pRecord->mNormalFieldByName.Insert(pRecord->mIndexFieldByName.GetElementName(0),
				pRecord->mIndexFieldByName[0]);
			pRecord->mIndexFieldByName.RemoveAt(0);
		}

		mQuery.AppendFormat("INSERT INTO `%s` (", pCollection);
		AppendRecord(pRecord, ESQLRecordAppendFlag::AppendNames);
		mQuery += ") VALUES (";
		AppendRecord(pRecord, ESQLRecordAppendFlag::AppendValues);
		mQuery += ")";

		if (pRecord->mIndexFieldByName.Count() > 0)
		{
			SSQLDuplicateKeyConflictInfo info;
			mpDriver->GetDuplicateKeyConflictInfo(pRecord, mQuery, info);

			if (info.mAppendIndexNames)
			{
				mQuery += " (";
				AppendFields(pRecord->mIndexFieldByName, ESQLRecordAppendFlag::AppendNames);
				mQuery += ")";
			}

			mQuery += info.mpExtraClause;
			AppendFields(pRecord->mNormalFieldByName, ESQLRecordAppendFlag::AutoDeduceValues,
				"", info.mpAutoDeducedValueFormat);
		}

		Execute(NULL);
	}

	bool mIsFirstIndexWithinUniqueKey;

public:
	CSaveSQLDTOHandler(ISQLDriver* pDriver, bool mIsFirstIndexWithinUniqueKey)
		: CSQLDTOHandler(pDriver), mIsFirstIndexWithinUniqueKey(mIsFirstIndexWithinUniqueKey) {}
};

class CDeleteSQLDTOHandler : public CFilteringSQLDTOHandler
{
	bool OnCollectionStarted(CRecordNodeDTO* pCollection, const char* pName) OVERRIDE
	{
		mQuery.AppendFormat("DELETE FROM `%s`", pName);
		bool traverse = CFilteringSQLDTOHandler::OnCollectionStarted(pCollection, pName);
		Execute(NULL);
		return traverse;
	}

public:
	CDeleteSQLDTOHandler(ISQLDriver* pDriver) : CFilteringSQLDTOHandler(pDriver) {}
};

bool CLoadDAO::MustBeRemovedOnLevelShutdown()
{
	return true;
}

void CLoadDAO::ExecuteIO(CKeyValuesDriver* pDriver)
{
	CLoadKeyValuesDTOHandler(pDriver, mResultDatabase).TraverseAndHandleLeafRecords(mQueryDatabase);
}

void CLoadDAO::ExecuteIO(ISQLDriver* pDriver)
{
	CLoadSQLDTOHandler(pDriver, mResultDatabase).TraverseAndHandleLeafRecords(mQueryDatabase);
}

void CLevelLoadDAO::AddCollection(const char* pCollectionName)
{
	CRecordNodeDTO* pCollection = mQueryDatabase.CreateCollection(pCollectionName);
	pCollection->AddIndexField(HL2RP_MAP_ALIAS_FIELD_NAME, gpGlobals->mapname);

	FOR_EACH_DICT_FAST(HL2RPRules()->mMapGroups, i)
	{
		pCollection->AddIndexField(HL2RP_MAP_ALIAS_FIELD_NAME, HL2RPRules()->mMapGroups[i]);
	}
}

CSaveDAO::CSaveDAO(bool isFirstIndexWithinUniqueKey) : mIsFirstIndexWithinUniqueKey(isFirstIndexWithinUniqueKey)
{

}

bool CSaveDAO::MergeFrom(IDAO* pDAO)
{
	return MergeFrom(pDAO->As(this));
}

bool CSaveDAO::MergeFrom(CSaveDAO* pOther)
{
	mSaveDatabase.TransferDistinctFrom(pOther->mSaveDatabase);
	return true;
}

void CSaveDAO::ExecuteIO(CKeyValuesDriver* pDriver)
{
	CSaveKeyValuesDTOHandler(pDriver).TraverseAndHandleLeafRecords(mSaveDatabase);
}

void CSaveDAO::ExecuteIO(ISQLDriver* pDriver)
{
	CSaveSQLDTOHandler(pDriver, mIsFirstIndexWithinUniqueKey).TraverseAndHandleLeafRecords(mSaveDatabase);
}

CAutoIncrementInsertDAO::CAutoIncrementInsertDAO(const char* pCollection, const char* pAutoIncrementField,
	bool isFirstIndexWithinUniqueKey) : CSaveDAO(isFirstIndexWithinUniqueKey),
	mpRecord(mSaveDatabase.CreateCollection(pCollection)), mpAutoIncrementField(pAutoIncrementField)
{

}

bool CAutoIncrementInsertDAO::MergeFrom(IDAO* pDAO)
{
	return false; // Keep it simple - Supporting multiple records would add some messy logic
}

void CAutoIncrementInsertDAO::ExecuteIO(CKeyValuesDriver* pDriver)
{
	CAutoIncrementLoadKeyValuesDTOHandler handler(pDriver, this);
	handler.TraverseAndHandleLeafRecords(mSaveDatabase);
	CSaveDAO::ExecuteIO(pDriver);
}

void CAutoIncrementInsertDAO::ExecuteIO(ISQLDriver* pDriver)
{
	CSaveDAO::ExecuteIO(pDriver);
	CRecordListDTO results;
	pDriver->ExecuteQuery(&results, "SELECT MAX(`%s`) AS `%s` FROM `%s`;", mpAutoIncrementField,
		mpAutoIncrementField, mSaveDatabase.GetElementName(0));

	if (!results.IsEmpty())
	{
		mResultId = results[0].GetInt(mpAutoIncrementField);
	}
}

CSaveDeleteDAO::CSaveDeleteDAO(bool isFirstIndexWithinUniqueKey) : CSaveDAO(isFirstIndexWithinUniqueKey)
{

}

CDatabaseNodeDTO& CSaveDeleteDAO::GetCorrectDatabase(bool isForSaving)
{
	return (isForSaving ? mSaveDatabase : mDeleteDatabase);
}

bool CSaveDeleteDAO::MergeFrom(IDAO* pDAO)
{
	CSaveDeleteDAO* pSaveDeleteDAO = pDAO->As(this);
	mSaveDatabase.RemoveEqualFrom(pSaveDeleteDAO->mDeleteDatabase);
	mDeleteDatabase.RemoveEqualFrom(pSaveDeleteDAO->mSaveDatabase);
	mDeleteDatabase.TransferDistinctFrom(pSaveDeleteDAO->mDeleteDatabase);
	return CSaveDAO::MergeFrom(pSaveDeleteDAO);
}

void CSaveDeleteDAO::ExecuteIO(CKeyValuesDriver* pDriver)
{
	CSaveDAO::ExecuteIO(pDriver);
	mSaveDatabase.RemoveEqualFrom(mDeleteDatabase); // Resolve cases where identical records may be in both maps
	CDeleteKeyValuesDTOHandler(pDriver).TraverseAndHandleLeafRecords(mDeleteDatabase);
}

void CSaveDeleteDAO::ExecuteIO(ISQLDriver* pDriver)
{
	CSaveDAO::ExecuteIO(pDriver);
	mSaveDatabase.RemoveEqualFrom(mDeleteDatabase); // Resolve cases where identical records may be in both maps
	CDeleteSQLDTOHandler(pDriver).TraverseAndHandleLeafRecords(mDeleteDatabase);
}
