#ifndef ROLEPLAY_SQL_H
#define ROLEPLAY_SQL_H

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

// Forward declare MySQLCPPConn namespace classes for usage
namespace sql
{
	class Connection;
	class ResultSet;
	class PreparedStatement;
};

const int INVALID_DATABASE_ID = -1;
const int MAX_VARCHAR_KEY_LENGTH = 255;

using namespace sql;

class CAsyncSQLTxn
{
	DECLARE_CLASS_NOBASE(CAsyncSQLTxn)

public:
	CAsyncSQLTxn() { ; }
	CAsyncSQLTxn(const char *pszFirstQuery);

	void AddQuery(const char *pszQuery);
	void AddFormattedQuery(const char *pszQueryFormat, ...);

	// Purpose: Determine if a prepared statement should be used to execute the queries
	// (for caching and reusing the plan on driver), false if a traditional statement should be used instead
	virtual bool ShouldUsePreparedStatements() const { return true; }

	virtual bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const { return false; }
	virtual bool ShouldRemoveBoth(const CAsyncSQLTxn &txn) const { return false; }

	// Purpose: Handle query execution results.
	// To be called in CSQL's frame think of main thread to prevent race conditions
	virtual void HandleQueryResults() const { ; }

	// Below driver-specific functions are called with the correct driver from polymorphic CSQL functions
	virtual void BuildSQLiteQueries() { ; }
	virtual void BuildMySQLQueries() { ; }
	virtual void BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const;
	virtual void BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const;

	// Purpose: Shorthand for downcasting to derived transaction classes.
	// Can be correctly called either by passing a transaction or templatizing a class, for more ease
	template<class C>
	C *ToTxnClass(const C *pTxn = NULL) const;

	// NOTE: Use V_STRINGIFY with size macros within static queries rather than wasting allocation of formatted buffers
	CUtlStringList m_QueryList;

private:
	// Purpose: A fallback on derived transactions when the query parameters sequence is the same between drivers
	virtual void BindParametersGeneric(int iQueryId, const void *pStmt) const { ; }
};

// Purpose: Shorthand for downcasting to derived transaction classes.
// It can be correctly called without a transaction and only templatizing a class, for more ease
template<class C>
FORCEINLINE C *CAsyncSQLTxn::ToTxnClass(const C *pTxn) const
{
	return dynamic_cast<C *>((ThisClass *)this);
}

abstract_class CHL2RP_SQL
{
protected:
	CHL2RP_SQL();

public:
	static void Init();
	static void ServerActivate();
	static void Think();

	// Not static since database object should have been validated when calling it to prevent useless allocations
	void AddAsyncTxn(CAsyncSQLTxn *pTxn);

	virtual void BuildTxnQueries(CAsyncSQLTxn &txn) const = 0;
	virtual bool ExecuteTxnQuery(CAsyncSQLTxn &txn, int queryId) = 0;
	virtual void ExecuteRawQuery(const char *pszQuery) const = 0;
	virtual bool FetchNextRow() = 0;
	virtual int FetchInt(const char *pszName) const = 0;
	virtual float FetchFloat(const char *pszName) const = 0;
	virtual void FetchString(const char *pszName, char *psDest, int iMaxLen) const = 0;
	virtual void CloseResults() = 0;
	virtual void BindInt(const void *pStmt, int iValue, int iPosition) const = 0;
	virtual void BindFloat(const void *pStmt, float flValue, int iPosition) const = 0;
	virtual void BindString(const void *pStmt, const char *pszValue, int iPosition) const = 0;
	template<typename... Position> void BindInt(const void *pStmt, int iValue, Position&&... iPositions) const;
	template<typename... Position> void BindFloat(const void *pStmt, float flValue, Position&&... iPositions) const;
	template<typename... Position> void BindString(const void *pStmt, const char *pszValue, Position&&... iPositions) const;

private:
	static unsigned ProcessTxns(void *pSQL);

	CUtlPtrLinkedList<CAsyncSQLTxn *> m_Txns;
	volatile CThreadFastMutex m_TxnsMutex;
	CAsyncSQLTxn *volatile m_pHeadResultHandlingTxn; // To be extracted from queue to prevent wasted mutex lock contention
};

template<typename... Position>
void CHL2RP_SQL::BindInt(const void *pStmt, int iValue, Position&&... iPositions) const
{
	int iPositionsBuff[] = { iPositions... };

	for (int iPosition : iPositionsBuff)
	{
		BindInt(pStmt, iValue, iPosition);
	}
}

template<typename... Position>
void CHL2RP_SQL::BindFloat(const void *pStmt, float flValue, Position&&... iPositions) const
{
	int iPositionsBuff[] = { iPositions... };

	for (int iPosition : iPositionsBuff)
	{
		BindFloat(pStmt, flValue, iPosition);
	}
}

template<typename... Position>
void CHL2RP_SQL::BindString(const void *pStmt, const char *pszValue, Position&&... iPositions) const
{
	int iPositionsBuff[] = { iPositions... };

	for (int iPosition : iPositionsBuff)
	{
		BindString(pStmt, pszValue, iPosition);
	}
}

class CSQLite : public CHL2RP_SQL
{
public:
	CSQLite(sqlite3 *pConnection);

private:
	void BuildTxnQueries(CAsyncSQLTxn &txn) const OVERRIDE;
	bool ExecuteTxnQuery(CAsyncSQLTxn &txn, int queryId) OVERRIDE;
	void ExecuteRawQuery(const char *pszQuery) const OVERRIDE;
	bool FetchNextRow() OVERRIDE;
	int GetColumnIndex(const char *pszName);
	int FetchInt(const char *pszName) const OVERRIDE;
	float FetchFloat(const char *pszName) const OVERRIDE;
	void FetchString(const char *pszName, char *psDest, int iMaxLen) const OVERRIDE;
	void CloseResults() OVERRIDE;
	void BindInt(const void *pStmt, int iValue, int iPosition) const OVERRIDE;
	void BindFloat(const void *pStmt, float flValue, int iPosition) const OVERRIDE;
	void BindString(const void *pStmt, const char *pszValue, int iPosition) const OVERRIDE;

	sqlite3 *m_pConnection;
	KeyValues *m_pGlobalResults, *m_pGlobalCurrentResult;
};

class CMySQL : public CHL2RP_SQL
{
public:
	CMySQL(Connection *pConnection);

private:
	void BuildTxnQueries(CAsyncSQLTxn &txn) const OVERRIDE;
	bool ExecuteTxnQuery(CAsyncSQLTxn &txn, int queryId) OVERRIDE;
	void ExecuteRawQuery(const char *pszQuery) const OVERRIDE;
	bool FetchNextRow() OVERRIDE;
	int FetchInt(const char *pszName) const OVERRIDE;
	float FetchFloat(const char *pszName) const OVERRIDE;
	void FetchString(const char *pszName, char *psDest, int iMaxLen) const OVERRIDE;
	void CloseResults() OVERRIDE;
	void BindInt(const void *pStmt, int iValue, int iPosition) const OVERRIDE;
	void BindFloat(const void *pStmt, float flValue, int iPosition) const OVERRIDE;
	void BindString(const void *pStmt, const char *pszValue, int iPosition) const OVERRIDE;

	Connection *m_pConnection;
	ResultSet *m_pGlobalResults;
};

#endif // ROLEPLAY_SQL_H