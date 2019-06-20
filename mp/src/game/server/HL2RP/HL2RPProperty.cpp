#include <cbase.h>
#include "HL2RPProperty.h"
#include "HL2RPRules.h"
#include <toolframework/itoolentity.h>

// Radius from the door origin stored in a property door to search a map door within
#define HL2RP_PROPERTY_DOOR_ORIGIN_LINK_RADIUS	0.5f

class IServerTools;

LINK_ENTITY_TO_CLASS(hl2rp_property_door, CHL2RPPropertyDoor);

BEGIN_DATADESC(CHL2RPPropertyDoor)
DEFINE_OUTPUT(m_OnMapSpawn, "OnMapSpawn"),
DEFINE_KEYFIELD_NOT_SAVED(m_DoorLinkMethod, FIELD_INTEGER, "doorlinkmethod"),
DEFINE_KEYFIELD_NOT_SAVED(m_DoorLinkReference.m_sTargetName, FIELD_STRING, "doortargetname"),
DEFINE_KEYFIELD_NOT_SAVED(m_DoorLinkReference.m_iHammerID, FIELD_INTEGER, "doorhammerid"),
DEFINE_KEYFIELD_NOT_SAVED(m_DoorLinkReference.m_Origin, FIELD_VECTOR, "doororigin"),
DEFINE_KEYFIELD_NOT_SAVED(m_sPropertyTargetName, FIELD_STRING, "propertytargetname"),
DEFINE_KEYFIELD_NOT_SAVED(m_sDisplayName, FIELD_STRING, "displayname")
END_DATADESC();

void CHL2RPPropertyDoor::NotifyMapSpawn()
{
	m_OnMapSpawn.FireOutput(this, this);
	CBaseEntity* pEntity = gEntList.FindEntityByName(NULL, STRING(m_sPropertyTargetName));

	if (pEntity != NULL && FClassnameIs(pEntity, "hl2rp_property"))
	{
		CHL2RPProperty* pProperty = static_cast<CHL2RPProperty*>(pEntity);
		CBaseEntity* pDoorEntity;

		switch (m_DoorLinkMethod)
		{
		case EDoorLinkMethod::HammerID:
		{
			IServerTools* pServerTools = static_cast<IServerTools*>(Sys_GetFactoryThis()
				(VSERVERTOOLS_INTERFACE_VERSION, NULL));

			if (pServerTools != NULL)
			{
				pDoorEntity = pServerTools->FindEntityByHammerID(m_DoorLinkReference.m_iHammerID);
				break;
			}

			return;
		}
		case EDoorLinkMethod::Origin:
		{
			pDoorEntity = gEntList.FindEntityInSphere(NULL, m_DoorLinkReference.m_Origin,
				HL2RP_PROPERTY_DOOR_ORIGIN_LINK_RADIUS);
			break;
		}
		default: // Default to targetname, for convenience
		{
			pDoorEntity = gEntList.FindEntityByName(NULL, STRING(m_DoorLinkReference.m_sTargetName));
		}
		}

		if (pDoorEntity != NULL)
		{
			m_hProperty = pProperty;
			pProperty->m_DoorList.AddToTail(this);
			m_hDoorEntity = pDoorEntity;
			HL2RPRules()->m_PropertyDoorTable.Insert(pDoorEntity->GetRefEHandle().ToInt(), this);
		}
	}
}

LINK_ENTITY_TO_CLASS(hl2rp_property, CHL2RPProperty);

BEGIN_DATADESC(CHL2RPProperty)
DEFINE_KEYFIELD_NOT_SAVED(m_sDisplayName, FIELD_STRING, "displayname"),
DEFINE_KEYFIELD_NOT_SAVED(m_iPrice, FIELD_INTEGER, "price")
END_DATADESC();
