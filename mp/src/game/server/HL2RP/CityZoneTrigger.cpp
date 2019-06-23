#include <cbase.h>
#include "HL2RP_Player.h"
#include <triggers.h>

// For now, it's a template to keep as a base in future
class CTriggerCityZoneBase : public CTriggerMultiple
{
	DECLARE_CLASS(CTriggerCityZoneBase, CTriggerMultiple);

protected:
	void StartTouch(CBaseEntity* pOther) OVERRIDE;
	void EndTouch(CBaseEntity* pOther) OVERRIDE;
};

// Variation that uses BSP collisions directly from the ongoing map
class CTriggerCityZone : public CTriggerCityZoneBase
{
	DECLARE_CLASS(CTriggerCityZone, CTriggerCityZoneBase);
};

// Variation that uses collisions from a model
class CTriggerCityZoneModel : public CTriggerCityZoneBase
{
	DECLARE_CLASS(CTriggerCityZoneModel, CTriggerCityZoneBase);
	DECLARE_DATADESC();

	void Spawn() OVERRIDE;

	string_t m_sCollisionModelName;
};

LINK_ENTITY_TO_CLASS(trigger_city_zone, CTriggerCityZone);

BEGIN_DATADESC(CTriggerCityZoneModel)
DEFINE_KEYFIELD_NOT_SAVED(m_sCollisionModelName, FIELD_STRING, "collisionmodel")
END_DATADESC();

LINK_ENTITY_TO_CLASS(trigger_city_zone_model, CTriggerCityZoneModel);

void CTriggerCityZoneBase::StartTouch(CBaseEntity* pOther)
{
	BaseClass::StartTouch(pOther);

	if (IsDebug() && pOther->IsPlayer())
	{
		CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(pOther);
		ClientPrint(pPlayer, HUD_PRINTTALK, UTIL_VarArgs("You enter a city zone. Targetname = %s.",
			STRING(GetEntityName())));
	}
}

void CTriggerCityZoneBase::EndTouch(CBaseEntity* pOther)
{
	BaseClass::EndTouch(pOther);

	if (IsDebug() && pOther->IsPlayer())
	{
		CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(pOther);
		ClientPrint(pPlayer, HUD_PRINTTALK, UTIL_VarArgs("You leave a city zone. Targetname = %s.",
			STRING(GetEntityName())));
	}
}

void CTriggerCityZoneModel::Spawn()
{
	PrecacheModel(STRING(m_sCollisionModelName));
	SetModelName(m_sCollisionModelName);
	SolidType_t solidType = GetSolid();
	BaseClass::Spawn();

	// If solid type was VPhysics, we respect it, removing myself if this init fails
	if (solidType == SOLID_VPHYSICS && VPhysicsInitStatic() == NULL)
	{
		return Remove();
	}

	SetSolid(solidType);
}