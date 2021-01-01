// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "trigger_city_zone.h"
#include "hl2_roleplayer.h"

const char* CCityZone::sTypeTokens[ECityZoneType::_Count] =
{
	"#HL2RP_Zone_Generic",
	"#HL2RP_Zone_AutoCrime",
	"#HL2RP_Zone_NoCrime",
	"#HL2RP_Zone_NoKill",
	"#HL2RP_Zone_Jail",
	"#HL2RP_Zone_VIPJail",
	"#HL2RP_Zone_Execution",
	"#HL2RP_Zone_Home_Unowned"
};

LINK_ENTITY_TO_CLASS(trigger_city_zone, CCityZone)

BEGIN_DATADESC(CCityZone)
DEFINE_KEYFIELD_NOT_SAVED(mType, FIELD_INTEGER, "type")
END_DATADESC()

void CCityZone::Spawn()
{
	SetModel(STRING(GetModelName()));
	Think();
}

void CCityZone::Think()
{
	SetNextThink(TICK_INTERVAL);

	ForEachRoleplayer([&](CHL2Roleplayer* pPlayer)
	{
		if (!pPlayer->mZonesWithin[mType])
		{
			if (IsEntityWithin(pPlayer))
			{
				pPlayer->mZonesWithin[mType] = this;
				pPlayer->SendZoneHUD(mType);
			}
		}
		else if (pPlayer->mZonesWithin[mType] == this && !IsEntityWithin(pPlayer))
		{
			pPlayer->mZonesWithin[mType] = NULL;
		}
	});
}

bool CCityZone::IsEntityWithin(CBaseEntity* pEntity)
{
	Vector mins = pEntity->WorldAlignMins();

	// Trace every point of hull mins to detect whether entity is fully inside us
	for (int i = 0;; VectorYawRotate(mins, 90.0f, mins))
	{
		if (!IsPointWithin(pEntity->GetAbsOrigin() + mins))
		{
			return false;
		}
		else if (++i > 3)
		{
			return true;
		}
	}
}

bool CCityZone::IsPointWithin(const Vector& origin)
{
	trace_t trace;

	if (VPhysicsGetObject() != NULL)
	{
		// Use VPhysics box tracing API, since no interface supports tracing
		// points against physics objects without a studio model
		physcollision->TraceBox(origin, origin, vec3_origin, vec3_origin,
			VPhysicsGetObject()->GetCollide(), GetAbsOrigin(), GetAbsAngles(), &trace);
	}
	else
	{
		UTIL_TraceModel(origin, origin, vec3_origin, vec3_origin, this, COLLISION_GROUP_NONE, &trace);
	}

	return trace.startsolid;
}
