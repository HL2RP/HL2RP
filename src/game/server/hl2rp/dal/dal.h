#ifndef DAL_H
#define DAL_H
#pragma once

#include "idao.h"
#include <typeinfo>
#include <utlhashtable.h>
#include <UtlSortVector.h>

class CDatabaseConnection;

// Data Access Layer - A multi-threaded processor for user-level database I/O requests
class CDAL : CAutoGameSystemPerFrame, CThread
{
	class CDAOList : public CAutoDeleteAdapter<CUtlLinkedList<IDAO*>>
	{
	public:
		CThreadFastMutex mMutex;
	};

	// Type wrapper and comparator for hash map lookup
	class CDAOTypeRef
	{
	public:
		CDAOTypeRef(IDAO*);

		bool operator==(const CDAOTypeRef&) const;

		const char* Name();

		const std::type_info& mType;
	};

	// Hash generator
	class CDAOHashFunctor
	{
	public:
		unsigned operator()(const CDAOTypeRef&) const;
	};

	class CHashedDAOList : public CDAOList
	{
	public:
		CHashedDAOList();

		// Maps a DAO type to a list of indices of the base list with equal DAO types.
		// Used to efficiently find and replace an existing DAO with an identical one.
		CUtlHashtable<CDAOTypeRef, CUtlLinkedList<int>, CDAOHashFunctor> mIndicesByType;
	};

	void LevelInitPostEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE;
	void FrameUpdatePostEntityThink() OVERRIDE;
	int Run() OVERRIDE;

	CDatabaseConnection* LoadDatabaseConfiguration();
	bool LoadSQLDriver(const char* pFileName, const char* pInterfaceName, CSysModule*&, void*&);

	CHashedDAOList mPendingDAOs;
	CDAOList mCompletedDAOs;

	// Used to request terminating the I/O thread at level shutdown if there aren't pending
	// DAOs left, or when the thread finishes processing last one from old level shutdown
	bool mMustIOThreadRun = false;

	// Points to the last pending DAO found at a past level shutdown, when value wasn't valid yet.
	// First, it acts as a finish marker, causing the I/O thread to stop once completing the DAO.
	// Then, since I/O thread will be re-created and a new connection will be made before processing
	// further DAOs, new DAOs may only replace already listed ones following this index, for security.
	int mOldLevelPendingDAOIndex = mPendingDAOs.InvalidIndex();

public:
	CDAL();

	void AddDAO(IDAO*);

	CUtlSortVector<ILevelInitDAOFactory*, ILevelInitDAOFactory::CLess> mLevelInitDAOFactories;
};

CDAL& DAL();

#endif // !DAL_H
