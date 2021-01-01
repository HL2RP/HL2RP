#ifndef RATION_DISPENSER_DAO_H
#define RATION_DISPENSER_DAO_H
#pragma once

#include "idao.h"
#include <generic.h>

class CDatabaseNodeDTO;
class CRationDispenserProp;
class CRecordNodeDTO;

class CDispenserDAO
{
protected:
	CRecordNodeDTO* AddFields(CRationDispenserProp*, CDatabaseNodeDTO&, bool save);
};

class CDispenserInsertDAO : public CAutoIncrementInsertDAO, CDispenserDAO
{
	bool MergeFrom(IDAO*) OVERRIDE;
	void HandleCompletion() OVERRIDE;

	CHandle<CRationDispenserProp> mhDispenser;

public:
	CDispenserInsertDAO(CRationDispenserProp*);
};

class CDispensersSaveDeleteDAO : public CSaveDeleteDAO, CDispenserDAO
{
public:
	CDispensersSaveDeleteDAO(CRationDispenserProp*, bool save);
};

#endif // !RATION_DISPENSER_DAO_H
