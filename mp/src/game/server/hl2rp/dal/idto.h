// DTO - Data Transfer Object(s)
// Provides intermediate generic data to be transferred between the local game state and the database

#ifndef IDTO_H
#define IDTO_H 
#pragma once

#include "isql_driver.h"
#include <generic.h>
#include <string_t.h>
#include <utldict.h>
#include <utlhashtable.h>
#include <utlmultilist.h>
#include <utlstring.h>

#define IDTO_PRIMARY_COLUMN_NAME "id"

#define IDTO_INVALID_DATABASE_ID -1
#define IDTO_LOADING_DATABASE_ID  0 // ID is still invalid but it's being loaded, so it can't be requested again

class CNodeDTOHandler;
class KeyValues;

struct SFieldDTO
{
	enum class EType
	{
		Int,
		UInt64,
		Float,
		String
	};

	SFieldDTO(int value = 0) : mUInt64(value), mType(EType::Int) {}
	SFieldDTO(uint64 value) : mUInt64(value), mType(EType::UInt64) {}
	SFieldDTO(float value) : mFloat(value), mType(EType::Float) {}
	SFieldDTO(const char* pValue) : mString(pValue), mType(EType::String) {}
	SFieldDTO(const string_t& value) : SFieldDTO(STRING(value)) {}

	operator const char* () const;
	const char* ToString(CNumStr&& dest = {}) const;

	union
	{
		int mInt;
		uint64 mUInt64;
		float mFloat;
		double mDouble;
	};

	CUtlConstString mString;
	EType mType;
};

class CFieldDictionaryDTO : public CGenericAdapter<CUtlDict<SFieldDTO>>
{
public:
	int GetInt(const char*);
	uint64 GetUInt64(const char*);
	float GetFloat(const char*);
	SFieldDTO GetField(const char*);
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
using CLinearDatabaseDTO = CUtlStableHashtable<const char*, CRecordListDTO>;

class CNodeDTO : public CAutoPurgeAdapter<CUtlDict<CRecordNodeDTO*>>
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
	CRecordNodeDTO* AddIndexField(const char* pName, const SFieldDTO&);
	void TransferDistinctFrom(CRecordNodeDTO*);
	void TraverseAndHandleLeafRecords(CNodeDTOHandler*, const char* pCollection);

	void AddNormalField(const char* pName, const SFieldDTO& field)
	{
		mNormalFieldByName.Insert(pName, field);
	}

	CFieldDictionaryDTO mIndexFieldByName, mNormalFieldByName;
};

#ifdef GAME_DLL
struct SDatabaseIdDTO
{
	SDatabaseIdDTO(int id = IDTO_INVALID_DATABASE_ID);

	operator int();
	operator SFieldDTO();
	bool IsValid();
	bool SetForLoading(); // Returns true if ID was fully invalid

private:
	int mId;
};

// A dictionary mapping collection names to record nodes
class CDatabaseNodeDTO : public CGenericAdapter<CNodeDTO>
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
	CAutoPurgeAdapter<CUtlMap<const char*, ISQLColumnDTO*>> mColumnByName;
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
