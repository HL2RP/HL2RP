#ifndef PROP_RATION_DISPENSER_H
#define PROP_RATION_DISPENSER_H

#include "props.h"
#include "CHL2RP_SQL.h"

class CWeaponCitizenPackage;
class CHL2RP_Player;

class CPropRationDispenser : public CDynamicProp
{
	DECLARE_CLASS(CPropRationDispenser, CDynamicProp)
	DECLARE_DATADESC();

public:
	CPropRationDispenser();

	void CHL2RP_PlayerUse(const CHL2RP_Player &player);
	void OnContainedRationPickup();

	int m_iDatabaseID;
	float m_flNextReadyTime;

private:
	void Spawn() OVERRIDE;
	void Precache() OVERRIDE;
	void InputDisableRationPickup(inputdata_t &inputdata);

	CHandle<CWeaponCitizenPackage> m_hContainedRation;
};

// Purpose: Share map identification among dispenser transactions
abstract_class CDispenserMapTxn : public CAsyncSQLTxn
{
protected:
	CDispenserMapTxn();

	CUtlString m_sDispenserMapName;
};

// Purpose: Share serial identification among dispenser transactions
abstract_class CDispenserSerialTxn : public CDispenserMapTxn
{
	DECLARE_CLASS(CDispenserSerialTxn, CDispenserMapTxn)

protected:
	CDispenserSerialTxn(CPropRationDispenser *pDispenser);
	bool IsValidDispenserTxnAndIDEquals(const ThisClass *pTxn) const;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;

	EHANDLE m_hDispenserHandle;
};

// Purpose: Share coordinates among dispenser transactions
abstract_class CDispenserCoordsTxn : public CDispenserSerialTxn
{
protected:
	CDispenserCoordsTxn(CPropRationDispenser *pDispenser);

	Vector m_vecCoords;
	vec_t m_flYaw;
};

// Purpose: Share database id identification among dispenser transactions
abstract_class CDispenserDatabaseIDTxn
{
protected:
	CDispenserDatabaseIDTxn(CPropRationDispenser *pDispenser);

	int m_iDispenserDatabaseID;
};

class CSetUpDispensersTxn : public CAsyncSQLTxn
{
public:
	CSetUpDispensersTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
	void BuildSQLiteQueries() OVERRIDE;
	void BuildMySQLQueries() OVERRIDE;
};

class CLoadDispensersTxn : public CDispenserMapTxn
{
	DECLARE_CLASS(CLoadDispensersTxn, CDispenserMapTxn)

public:
	CLoadDispensersTxn();

private:
	void BindParametersGeneric(int iQueryId, const void *pStmt) const OVERRIDE;
	void HandleQueryResults() const OVERRIDE;
	bool ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const OVERRIDE;
};

class CInsertDispenserTxn : public CDispenserCoordsTxn
{
	DECLARE_CLASS(CInsertDispenserTxn, CDispenserCoordsTxn)

public:
	CInsertDispenserTxn(CPropRationDispenser *pDispenser);

private:
	void BindParametersGeneric(int iQueryId, const void *pStmt) const OVERRIDE;
	void HandleQueryResults() const OVERRIDE;
	bool ShouldRemoveBoth(const CAsyncSQLTxn &txn) const OVERRIDE;
};

class CUpsertDispenserTxn : public CDispenserCoordsTxn, CDispenserDatabaseIDTxn
{
public:
	CUpsertDispenserTxn(CPropRationDispenser *pDispenser);

private:
	void BuildSQLiteQueries() OVERRIDE;
	void BuildMySQLQueries() OVERRIDE;
	void BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const OVERRIDE;
	void BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const OVERRIDE;
};

class CDeleteDispenserTxn : public CDispenserSerialTxn, CDispenserDatabaseIDTxn
{
	DECLARE_CLASS_NOBASE(CDeleteDispenserTxn)

public:
	CDeleteDispenserTxn(CPropRationDispenser *pDispenser);

private:
	void BindParametersGeneric(int iQueryId, const void *pStmt) const OVERRIDE;
};

#endif // PROP_RATION_DISPENSER_H
