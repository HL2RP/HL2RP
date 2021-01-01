#ifndef HL2RP_PROPERTY_DAO_H
#define HL2RP_PROPERTY_DAO_H
#pragma once

#include "idao.h"

class CHL2RP_Property;

class CPropertyInsertDAO : public CAutoIncrementInsertDAO
{
	void HandleCompletion() OVERRIDE;

	CHL2RP_Property* mpProperty;

public:
	CPropertyInsertDAO(CHL2RP_Property*);
};

class CPropertiesSaveDAO : public CSaveDeleteDAO
{
public:
	CPropertiesSaveDAO(CHL2RP_Property*, bool save = true);
	CPropertiesSaveDAO(CHL2RP_Property*, const char* pOldMapAlias); // For map/group re-linking
};

class CPropertyGrantsSaveDAO : public CSaveDeleteDAO
{
public:
	CPropertyGrantsSaveDAO(CHL2RP_Property*, uint64, bool save = false);
};

class CPropertyDoorDAO
{
protected:
	CRecordNodeDTO* AddFields(CBaseEntity*, CRecordNodeDTO*, bool save);
};

class CPropertyDoorInsertDAO : public CAutoIncrementInsertDAO, CPropertyDoorDAO
{
	void HandleCompletion() OVERRIDE;

	EHANDLE mhDoor;

public:
	CPropertyDoorInsertDAO(CBaseEntity*);
};

class CPropertyDoorsSaveDAO : public CSaveDeleteDAO, CPropertyDoorDAO
{
public:
	CPropertyDoorsSaveDAO(CBaseEntity*, bool save = true);
};

#endif // !HL2RP_PROPERTY_DAO_H
