#ifndef HL2RP_PROPERTY_H
#define HL2RP_PROPERTY_H
#pragma once

class CHL2RPProperty;

class CHL2RPPropertyDoor : public CLogicalEntity
{
	DECLARE_CLASS(CHL2RPPropertyDoor, CLogicalEntity);
	DECLARE_DATADESC();

	enum EDoorLinkMethod
	{
		TargetName,
		HammerID,
		Origin
	};

	// Stores multiple references to link the door entity
	struct SDoorLinkReference
	{
		string_t m_sTargetName;
		int m_iHammerID;
		Vector m_Origin;
	};

	COutputEvent m_OnMapSpawn;
	EHANDLE m_hDoorEntity;
	EDoorLinkMethod m_DoorLinkMethod;
	SDoorLinkReference m_DoorLinkReference;
	string_t m_sPropertyTargetName;

public:
	void NotifyMapSpawn();

	CHandle<CHL2RPProperty> m_hProperty;
	string_t m_sDisplayName;
};

class CHL2RPProperty : public CLogicalEntity
{
	DECLARE_CLASS(CHL2RPProperty, CLogicalEntity);
	DECLARE_DATADESC();

public:
	string_t m_sDisplayName;
	int m_iPrice;
	CUtlVector<CHandle<CHL2RPPropertyDoor>> m_DoorList;
};

#endif // !HL2RP_PROPERTY_H
