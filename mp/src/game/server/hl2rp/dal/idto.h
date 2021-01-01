// DTO - Data Transfer Object(s)
// Provides intermediate generic data to be transferred between the local game state and the database

#ifndef IDTO_H
#define IDTO_H 
#pragma once

#include "isql_driver.h"
#include <hl2rp_util.h>
#include <utldict.h>
#include <utlmultilist.h>

#define IDTO_PRIMARY_COLUMN_NAME "id"

class CNodeDTOHandler;
class KeyValues;

class CFieldDictionaryDTO : public CDefaultGetAdapter<CUtlDict<SUtlField>>
{
public:
	int GetInt(const char*);
	uint64 GetUInt64(const char*);
	float GetFloat(const char*);
	CUtlString GetString(const char*);
	SUtlField GetField(const char*);
	void MergeFrom(CFieldDictionaryDTO&);
	void TransferDistinctFrom(CFieldDictionaryDTO&);
	bool FieldsEqual(KeyValues*, int start = 0);
	bool MatchesFieldAt(KeyValues*, int);
	void PopulateOther(KeyValues*);
};

class CRecordListDTO : public CUtlVector<CFieldDictionaryDTO>
{
public:
	CFieldDictionaryDTO& CreateRecord()
	{
		return Element(AddToTail());
	}
};

// A dictionary mapping collection names to record lists
class CLinearDatabaseDTO : public CAutoDeleteAdapter<CUtlDict<CRecordListDTO*>>
{
public:
	CRecordListDTO* GetList(const char* pCollection);
};

class CNodeDTO : public CAutoDeleteAdapter<CUtlDict<CRecordNodeDTO*>>
{
public:
	void TransferDistinctFrom(CNodeDTO&);
	void RemoveEqualFrom(CNodeDTO&);

protected:
	CRecordNodeDTO* CreateChild(const char* pName);
};

// A node which can both map index values to sub-nodes, and contain a flat record
class CRecordNodeDTO : public CNodeDTO
{
public:
	CRecordNodeDTO* AddIndexField(const char* pName, const SUtlField&);
	void TransferDistinctFrom(CRecordNodeDTO*);
	void TraverseAndHandleLeafRecords(CNodeDTOHandler*, const char* pCollection);

	void AddNormalField(const char* pName, const SUtlField& field)
	{
		mNormalFieldByName.Insert(pName, field);
	}

	CFieldDictionaryDTO mIndexFieldByName, mNormalFieldByName;
};

#ifdef GAME_DLL
// A dictionary mapping collection names to record nodes
class CDatabaseNodeDTO : public CNodeDTO
{
public:
	CRecordNodeDTO* GetCollection(const char* pName);
	CRecordNodeDTO* CreateCollection(const char* pName);
};

class CNodeDTOHandler
{
public:
	virtual bool OnCollectionStarted(CRecordNodeDTO*, const char* pName); // Returns whether to proceed traversing
	virtual void HandleLeafRecord(CRecordNodeDTO*, const char* pCollection) {}

	void TraverseAndHandleLeafRecords(CDatabaseNodeDTO&);
};

abstract_class ISQLColumnDTO
{
public:
	virtual void AppendTo(CSQLQuery&, const SSQLDriverFeatures&) = 0;
};

class CSQLTableSetupDTO
{
	struct SForeignKey
	{
		const char* mpReferencedTable, * mpReferencedColumn;
		const char* mpOnDeleteClause; // The 'ON DELETE' clause - Enabling 'CASCADE' may not be always desired
	};

	const char* mpTableName;
	CAutoDeleteAdapter<CUtlMap<const char*, ISQLColumnDTO*>> mColumnByName;
	CAutoLessFuncAdapter<CUtlMap<const char*, SForeignKey>> mForeignKeyByColumnName;

public:
	void CreateTable(ISQLDriver*, const SSQLDriverFeatures&);
	void AddNewColumnsToTable(ISQLDriver*, const SSQLDriverFeatures&);

protected:
	CSQLTableSetupDTO(const char* pTableName, bool autoIncrementPrimaryKey = false);

	int CreateIntColumn(const char*);
	int CreateUInt64Column(const char*);
	int CreateFloatColumn(const char*);
	int CreateVarCharColumn(const char*, int maxLen);
	int CreateTextColumn(const char*);
	int CreateUniqueKeyColumnList(int firstColumnIndex);
	void AddForeignKey(int localColumnIndex, const char* pReferencedTable,
		const char* pReferencedColumn, bool cascadeOnDelete = true);

	bool mAutoIncrementPrimaryKey;
	CUtlVector<int> mPrimaryKeyColumnIndices;
	CUtlMultiList<int, int> mUniqueKeysColumnIndices; // Multi-list of column index sublists, each making up a key
};
#endif // GAME_DLL

#endif // !IDTO_H
