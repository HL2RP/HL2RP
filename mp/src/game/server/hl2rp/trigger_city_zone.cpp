// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "trigger_city_zone.h"
#include "hl2_roleplayer.h"

const char* CCityZone::sTypeTokens[ECityZoneType::_Count] =
{
	"#HL2RP_GenericZone",
	"#HL2RP_AutoCrimeZone",
	"#HL2RP_NoCrimeZone",
	"#HL2RP_NoKillZone",
	"#HL2RP_Jail",
	"#HL2RP_VIPJail",
	"#HL2RP_ExecutionRoom",
	"#HL2RP_HomeZone_Unowned"
};

LINK_ENTITY_TO_CLASS(trigger_city_zone, CCityZone)

BEGIN_DATADESC(CCityZone)
DEFINE_KEYFIELD_NOT_SAVED(mType, FIELD_INTEGER, "type")
END_DATADESC()

void CCityZone::Spawn()
{
	SetSolid(SOLID_CUSTOM);
	Think();
}

void CCityZone::Think()
{
	SetNextThink(TICK_INTERVAL);
	CUtlVector<CHL2Roleplayer*> players;
	CollectHumanPlayers(&players, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS);

	for (auto pPlayer : players)
	{
		if (pPlayer->mhCityZone == this)
		{
			if (!IsEntityWithin(pPlayer))
			{
				pPlayer->mhCityZone = NULL;
			}
		}
		else if ((pPlayer->mhCityZone == NULL || mType > pPlayer->mhCityZone->mType) && IsEntityWithin(pPlayer))
		{
			pPlayer->mhCityZone = this;
		}
	}
}

bool CCityZone::IsEntityWithin(CBaseEntity* pEntity)
{
	Vector localRotatingMins = pEntity->WorldAlignMins();

	// Trace every point of hull mins to detect whether entity is fully inside ours (we don't want intersection)
	for (int i = 0;; VectorYawRotate(localRotatingMins, 90.0f, localRotatingMins))
	{
		// We'll use VPhysics box tracing API since no interface supports
		// tracing points against physics objects without a studio model
		Vector absMins = pEntity->GetAbsOrigin() + localRotatingMins;
		trace_t tr;
		physcollision->TraceBox(absMins, absMins, vec3_origin, vec3_origin, VPhysicsGetObject()->GetCollide(),
			GetAbsOrigin(), GetAbsAngles(), &tr);

		if (!tr.startsolid)
		{
			return false;
		}
		else if (++i > 3)
		{
			break;
		}
	}

	return true;
}
