// DTO - Data Transfer Object(s)
// Provides intermediate generic data to be transferred between the local game state and the database

#ifndef IDTO_H
#define IDTO_H 
#pragma once

#include "isql_driver.h"
#include <fmtstr.h>
#include <generic.h>
#include <platform.h>
#include <utldict.h>
#include <utlhashtable.h>
#include <utlmultilist.h>
#include <utlstring.h>
#include <utlvector.h>

#define IDTO_PRIMARY_COLUMN_NAME "id"

#define IDTO_INVALID_DATABASE_ID -1
#define IDTO_LOADING_DATABASE_ID 0 // ID is still invalid but it's being loaded, so it can't be requested again

class CNodeDTOHandler;
class CRecordNodeDTO;
class KeyValues;

enum class EFieldDTOType
{
	Integer,
	LongUnsignedInteger,
	Float,
	String
};

struct SFieldDTO
{
public:
	const char* ToString(CNumStr&& numberFormatter = CNumStr()) const;

	union
	{
		uint64 mUInt64;
		int mInt;
		float mFloat;
		double mDouble;
	};

	CUtlConstString mString;
	EFieldDTOType mType;
};

class CFieldDictionaryDTO : public CUtlDict<SFieldDTO>
{
	SFieldDTO& CreateField(const char* pName);

public:
	int GetInt(const char*);
	uint64 GetUInt64(const char*);
	float GetFloat(const char*);
	const char* GetString(const char*);
	void SetField(const char* pName, int);
	void SetField(const char* pName, uint64);
	void SetField(const char* pName, float);
	void SetField(const char* pName, const char*);
	void MergeFrom(CFieldDictionaryDTO&);
	void TransferDistinctFrom(CFieldDictionaryDTO&);
	bool FieldsEqual(KeyValues*, int start = 0);
	bool MatchesFieldAt(KeyValues*, int);
	void PopulateOther(KeyValues*);
};

inline SFieldDTO& CFieldDictionaryDTO::CreateField(const char* pName)
{
	return Element(Insert(pName));
}

inline void CFieldDictionaryDTO::SetField(const char* pName, int value)
{
	SFieldDTO& field = CreateField(pName);
	field.mInt = value;
	field.mType = EFieldDTOType::Integer;
}

inline void CFieldDictionaryDTO::SetField(const char* pName, uint64 value)
{
	SFieldDTO& field = CreateField(pName);
	field.mUInt64 = value;
	field.mType = EFieldDTOType::LongUnsignedInteger;
}

inline void CFieldDictionaryDTO::SetField(const char* pName, float value)
{
	SFieldDTO& field = CreateField(pName);
	field.mFloat = value;
	field.mType = EFieldDTOType::Float;
}

inline void CFieldDictionaryDTO::SetField(const char* pName, const char* pValue)
{
	SFieldDTO& field = CreateField(pName);
	field.mString = pValue;
	field.mType = EFieldDTOType::String;
}

class CRecordListDTO : public CUtlVector<CFieldDictionaryDTO>
{
public:
	CFieldDictionaryDTO& CreateRecord()
	{
		return Element(AddToTail());
	}
};

// A dictionary mapping collection names to record lists
typedef CUtlStableHashtable<const char*, CRecordListDTO> CLinearDatabaseDTO;

class CNodeDTO : public CAutoPurgeAdapter<CUtlDict<CRecordNodeDTO*>>
{
public:
	void TransferDistinctFrom(CNodeDTO&);
	void RemoveEqualFrom(CNodeDTO&);

protected:
	CRecordNodeDTO* CreateNode(const char* pName, bool force);
};

// A node which can both map index values to sub-nodes, and contain a flat record
class CRecordNodeDTO : public CNodeDTO
{
	CRecordNodeDTO* AddIndexField(const char*, bool force);

public:
	CRecordNodeDTO* AddIndexField(const char* pName, const char*, bool force = false);
	void TransferDistinctFrom(CRecordNodeDTO*);
	void TraverseAndHandleLeafRecords(CNodeDTOHandler*, const char* pCollection);

	template<typename T>
	CRecordNodeDTO* AddIndexField(const char* pName, T value, bool force = false)
	{
		CRecordNodeDTO* pNode = AddIndexField(CNumStr(value), force);
		pNode->mIndexFieldByName.SetField(pName, value);
		return pNode;
	}

	template<typename T = int>
	void AddNormalField(const char* pName, T value)
	{
		mNormalFieldByName.SetField(pName, value);
	}

	CFieldDictionaryDTO mIndexFieldByName, mNormalFieldByName;
};

#ifdef GAME_DLL
struct SDatabaseIdDTO
{
	SDatabaseIdDTO(int id = IDTO_INVALID_DATABASE_ID);

	operator int();
	bool IsValid();
	bool SetForLoading(); // Returns true if ID was fully invalid

private:
	int mId;
};

// A dictionary mapping collection names to record nodes
class CDatabaseNodeDTO : public CNodeDTO
{
public:
	CRecordNodeDTO* GetCollection(const char* pName);
	CRecordNodeDTO* AddCollection(const char* pName);
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
