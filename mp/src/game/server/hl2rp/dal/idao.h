#ifndef IDAO_H
#define IDAO_H
#pragma once

#include "idto.h"

class CKeyValuesDriver;

// Data Access Object - Interface for user-level I/O requests to/from the database
abstract_class IDAO
{
public:
	virtual ~IDAO() = default;

	virtual bool MustBeRemovedOnLevelShutdown();
	virtual bool MergeFrom(IDAO*); // NOTE: The calling DAL code guarantees that the passed DAO type equals current
	virtual void ExecuteIO(CKeyValuesDriver*) {}
	virtual void ExecuteIO(ISQLDriver*) = 0;
	virtual void HandleCompletion() {}

	// This is mainly aimed to downcast the passed DAO to the current class at MergeFrom() calls and compare data
	template<class T>
	T* As(T*)
	{
		return static_cast<T*>(this);
	}
};

#ifdef GAME_DLL
enum class ELevelInitDAOType // Prioritized in descending order
{
	DatabaseSetup,
	Load
};

abstract_class ILevelInitDAOFactory
{
	ELevelInitDAOType mType;

public:
	class CLess
	{
	public:
		bool Less(ILevelInitDAOFactory*, ILevelInitDAOFactory*, void*);
	};

	virtual IDAO* Create() = 0;

protected:
	ILevelInitDAOFactory(ELevelInitDAOType);
};

template<class T, ELevelInitDAOType type>
class CLevelInitDAOFactory : ILevelInitDAOFactory
{
	IDAO* Create() OVERRIDE
	{
		return (new T);
	}

public:
	CLevelInitDAOFactory() : ILevelInitDAOFactory(type) {}
};

abstract_class ISQLTableSetupDTOFactory
{
public:
	virtual CSQLTableSetupDTO * Create() = 0;

protected:
	ISQLTableSetupDTOFactory();
};

template<class T>
class CSQLTableSetupDTOFactory : ISQLTableSetupDTOFactory
{
	CSQLTableSetupDTO* Create() OVERRIDE
	{
		return (new T);
	}
};

#define REGISTER_LEVEL_INIT_DAO_FACTORY(Class, type) \
	static CLevelInitDAOFactory<Class, type> s##Class##Factory;
#define REGISTER_SQL_TABLE_SETUP_DTO_FACTORY(Class) static CSQLTableSetupDTOFactory<Class> s##Class##Factory;

class CDatabaseSetupDAO : public IDAO
{
	void ExecuteIO(ISQLDriver*) OVERRIDE;
};

class CDatabaseSetupDAOFactory : CLevelInitDAOFactory<CDatabaseSetupDAO, ELevelInitDAOType::DatabaseSetup>
{
public:
	CUtlVector<ISQLTableSetupDTOFactory*> mSQLTableSetupDTOFactories;
};

abstract_class CKeyValuesDTOHandler : public CNodeDTOHandler
{
	virtual void OnIndexLessCollectionStarted(const char*); // Collection without index fields is visited

// Called when a loaded record matches the identifiers of a local one (leaf).
// Returns whether it fully matches from the concrete handler consideration.
virtual bool OnLoadedRecordMatching(KeyValues* pFileData, KeyValues* pLoadedRecord, CRecordNodeDTO*) = 0;

bool FindMatchingLocalRecord(KeyValues* pFileData, KeyValues* pLoadedRecord, CRecordNodeDTO*, int depth);

public:
	virtual void HandleLoadedFileData(KeyValues* pFileData, const char* pBaseFileName);

protected:
	CKeyValuesDTOHandler(CKeyValuesDriver*);

	bool OnCollectionStarted(CRecordNodeDTO*, const char*) OVERRIDE;
	virtual void HandleLoadedRecord(KeyValues* pFileData, KeyValues* pRecord);

	CKeyValuesDriver* mpDriver;
	CRecordNodeDTO* mpCurIOCollection = NULL; // Current collection for corresponding directory I/O handling phase
};

// General records-loading class, which retrieves all fields
class CLoadDAO : public IDAO
{
	bool MustBeRemovedOnLevelShutdown() OVERRIDE;

protected:
	void ExecuteIO(CKeyValuesDriver*) OVERRIDE;
	void ExecuteIO(ISQLDriver*) OVERRIDE;

	CDatabaseNodeDTO mQueryDatabase;
	CLinearDatabaseDTO mResultDatabase;
};

class CLevelLoadDAO : public CLoadDAO
{
	void AddCollection(const char*);

protected:
	template<typename... T>
	CLevelLoadDAO(T... collections)
	{
		for (auto pCollection : { collections... })
		{
			AddCollection(pCollection);
		}
	}
};

class CSaveDAO : public IDAO
{
	bool MergeFrom(IDAO*) OVERRIDE;

public:
	// When false, it means that the first index is only required as the file
	// name to use on KeyValues driver, while not being part of an unique key
	bool mIsFirstIndexWithinUniqueKey;

protected:
	CSaveDAO(bool isFirstIndexWithinUniqueKey = true);

	bool MergeFrom(CSaveDAO*);
	void ExecuteIO(CKeyValuesDriver*) OVERRIDE;
	void ExecuteIO(ISQLDriver*) OVERRIDE;

	CDatabaseNodeDTO mSaveDatabase;
};

// Creates a single auto incremented record, then retrieves its generated index field value
class CAutoIncrementInsertDAO : public CSaveDAO
{
	bool MergeFrom(IDAO*) OVERRIDE;
	void ExecuteIO(CKeyValuesDriver*) OVERRIDE;
	void ExecuteIO(ISQLDriver*) OVERRIDE;

public:
	const char* mpAutoIncrementField;
	int mResultId = 1;

protected:
	CAutoIncrementInsertDAO(const char* pCollection, const char* pAutoIncrementField,
		bool isFirstIndexWithinUniqueKey = false);

	CRecordNodeDTO* mpRecord;
};

// Saves (updating existing) or deletes records for multiple collections
class CSaveDeleteDAO : public CSaveDAO
{
	bool MergeFrom(IDAO*) OVERRIDE;
	void ExecuteIO(CKeyValuesDriver*) OVERRIDE;
	void ExecuteIO(ISQLDriver*) OVERRIDE;

protected:
	CSaveDeleteDAO(bool isFirstIndexWithinUniqueKey = true);

	CDatabaseNodeDTO& GetCorrectDatabase(bool isForSaving);

	CDatabaseNodeDTO mDeleteDatabase;
};
#endif // GAME_DLL

#endif // !IDAO_H
