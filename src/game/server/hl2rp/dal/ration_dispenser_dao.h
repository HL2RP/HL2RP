#ifndef RATION_DISPENSER_DAO_H
#define RATION_DISPENSER_DAO_H
#pragma once

#include "idao.h"

class CRationDispenserProp;

class CDispenserDAO
{
protected:
	CRecordNodeDTO* AddFields(CRationDispenserProp*, CRecordNodeDTO*, bool save);
};

class CDispenserInsertDAO : public CAutoIncrementInsertDAO, CDispenserDAO
{
	void HandleCompletion() OVERRIDE;

	CHandle<CRationDispenserProp> mhDispenser;

public:
	CDispenserInsertDAO(CRationDispenserProp*);
};

class CDispensersSaveDAO : public CSaveDeleteDAO, CDispenserDAO
{
public:
	CDispensersSaveDAO(CRationDispenserProp*, bool save = true);
	CDispensersSaveDAO(CRationDispenserProp*, const char* pOldMapAlias); // For map/group re-linking
};

#endif // !RATION_DISPENSER_DAO_H
