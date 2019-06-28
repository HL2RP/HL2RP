#ifndef HL2RP_DAL_H
#define HL2RP_DAL_H
#pragma once

#include <refcount.h>
#include <utllinkedlist.h>

class IDALEngine;
class IDAO;

// NOTE: Never invoke CallWorker with a zeroed timeout, it turns the execution unsafe!
class CDAOThread : public CWorkerThread
{
	// Requests to the worker thread
	enum ECallRequest
	{
		CallRequest_ProcessDAOList,
		CallRequest_AckResultsHandled,
		Stop
	};

	// Responses from the worker thread
	enum ECallResponse
	{
		CallResponse_None,
		ResultsClosed,
		Stopped,
	};

	enum EWorkerWaitingState
	{
		WorkerWaitingState_None,
		WorkerWaitingState_ProcessDAOList,
		WorkerWaitingState_AckResultsHandled
	};

	bool Init() OVERRIDE;
	int Run() OVERRIDE;

	void InsertDAO(IDAO* pDAO);
	void ProcessDAOList();

	EWorkerWaitingState m_WorkerWaitingState;
	CUtlFixedLinkedList<IDAO*> m_DAOList;

	// This mutex is used to protect the DAO list against shared access by both threads
	CThreadMutex m_DAOListMutex;

	// The DAO whose state is between (1) about to do the I/O threaded work and
	// (2) just finished handling results at the main thread.
	// Participant threads must synchronize the shared access, preferably via m_WorkerWaitingState checks.
	IDAO* m_pHandlingDAO;

public:
	enum EDAOCollisionResolution
	{
		None,
		ReplaceQueued,
		RemoveBothAndStop
	};

	CDAOThread();

	void AddAsyncDAO(IDAO* pDAO);
	bool TryHandleDAOResults();
	void ProcessDAOListAndStop();
};

class CHL2RPDAL
{
	CDAOThread m_DAOThread;

public:
	~CHL2RPDAL();

	void Init();
	void LevelInitPostEntity();
	void LevelShutdown();
	void Think();

	// Adds a DAO to the threaded DAO processor, colliding with DAO list for optimization
	void AddAsyncDAO(IDAO* pDAO)
	{
		m_DAOThread.AddAsyncDAO(pDAO);
	}

	CRefPtr<IDALEngine> m_EngineRef;
};

CHL2RPDAL& GetHL2RPDAL();

FORCEINLINE IDALEngine* GetHL2RPDALEngine()
{
	return GetHL2RPDAL().m_EngineRef;
}

// Input: T - A DAO subclass. U - Arguments for T's constructor.
template<class T, typename... U>
void CreateAsyncDAO(U... args)
{
	Assert(GetHL2RPDALEngine() != NULL);
	IDAO* pDAO = new T(args...);
	GetHL2RPDAL().AddAsyncDAO(pDAO);
}

// Attempts to create and queue up an asynchronous DAO, if conditions pass
template<class T, typename... U>
void TryCreateAsyncDAO(U... args)
{
	if (GetHL2RPDALEngine() != NULL)
	{
		CreateAsyncDAO<T>(args...);
	}
}

#endif // !HL2RP_DAL_H