// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "trigger_city_zone.h"
#include <collisionutils.h>

#ifdef GAME_DLL
#include <hl2_roleplayer.h>

#ifdef HL2RP_FULL
#include "hl2rp_property.h"
#endif // HL2RP_FULL
#endif // GAME_DLL

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

#ifdef GAME_DLL
BEGIN_DATADESC(CCityZone)
DEFINE_KEYFIELD_NOT_SAVED(mType, FIELD_INTEGER, "type")
END_DATADESC()

void CCityZone::Spawn()
{
	if (VPhysicsGetObject() != NULL)
	{
		Vector mins, maxs;
		physcollision->CollideGetAABB(&mins, &maxs, VPhysicsGetObject()->GetCollide(), vec3_origin, GetAbsAngles());
		SetSize(mins, maxs);
	}
	else
	{
		SetModel(STRING(GetModelName()));
	}

	Think();
}

void CCityZone::UpdateOnRemove()
{
	SendToPlayers(false);
	BaseClass::UpdateOnRemove();
}

void CCityZone::Think()
{
	SetNextThink(TICK_INTERVAL);

	ForEachRoleplayer([&](CHL2Roleplayer* pPlayer)
	{
		if (pPlayer->mZonesWithin[mType] == NULL)
		{
			if (IsEntityWithin(pPlayer))
			{
				pPlayer->mZonesWithin.Set(mType, this);
			}
		}
		else if (pPlayer->mZonesWithin[mType] == this && !IsEntityWithin(pPlayer))
		{
			pPlayer->mZonesWithin.Set(mType, NULL);
		}
	});
}

bool CCityZone::IsEntityWithin(CBaseEntity* pEntity)
{
	Vector mins = pEntity->WorldAlignMins(), maxs = pEntity->WorldAlignMaxs();
	maxs.z = mins.z;

	if (IsBoxIntersectingBox(GetAbsOrigin() + WorldAlignMins(), GetAbsOrigin() + WorldAlignMaxs(),
		pEntity->GetAbsOrigin() + mins, pEntity->GetAbsOrigin() + maxs))
	{
		// Trace every point of hull bottom to detect whether entity is fully inside us
		for (int i = 0;; VectorYawRotate(mins, 90.0f, mins))
		{
			if (!IsPointWithin(pEntity->GetAbsOrigin() + mins))
			{
				break;
			}
			else if (++i > 3)
			{
				return true;
			}
		}
	}

	return false;
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

#ifdef HL2RP_FULL
void CCityZone::SendToPlayers(bool create, CRecipientFilter&& filter)
{
	filter.MakeReliable();
	UserMessageBegin(filter, HL2RP_CITY_ZONE_UPDATE_MESSAGE);
	WRITE_LONG(GetRefEHandle().GetEntryIndex());

	if (create)
	{
		WRITE_LONG(mpProperty != NULL ? mpProperty->mDatabaseId : SDatabaseId());
	}
	
	MessageEnd();
}
#endif // HL2RP_FULL
#endif // GAME_DLL
