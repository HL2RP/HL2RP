// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include <ration.h>
#include <npcevent.h>

#define RATION_THROW_AIR_SPEED       200.0f
#define RATION_THROW_ANGULAR_VEL_YAW -100.0f

#define RATION_THROW_EYE_OFFSET_RIGHT   16.0f
#define RATION_THROW_EYE_OFFSET_FORWARD 32.0f
#define RATION_THROW_EYE_OFFSET_UP      -8.0f

#define RATION_THROW_MAX_PICKUP_BLOCK_TIME 3.0f

#define RATION_PICKUP_ALLOW_CONTEXT "RationAllowPickupContext"

BEGIN_DATADESC(CRation)
DEFINE_KEYFIELD_NOT_SAVED(m_iPrimaryAmmoCount, FIELD_INTEGER, "ammocount")
END_DATADESC()

void CRation::Spawn()
{
	int mapAmmoCount = GetPrimaryAmmoCount();
	BaseClass::Spawn();
	SetPrimaryAmmoCount(Max(0, mapAmmoCount)); // Preserve the ammo that may have been defined from map
}

void CRation::FallInit()
{
	CBaseCombatWeapon::FallInit();
}

void CRation::Materialize()
{
	CBaseCombatWeapon::Materialize();

	// Ration hit the floor, so, allow picking it up immediately for fluency
	SetContextThink(&ThisClass::AllowPickup, TICK_INTERVAL, RATION_PICKUP_ALLOW_CONTEXT);
}

void CRation::HandleAnimEvent(animevent_t* pEvent)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer != NULL && pEvent->event == EVENT_WEAPON_THROW && m_iClip1 > 0)
	{
		// Prepare new ration to throw
		QAngle angles(pPlayer->EyeAngles().x, pPlayer->EyeAngles().y, 0.0f);
		CRation* pRation = static_cast<CRation*>(Create("ration", pPlayer->WorldSpaceCenter(), angles));
		pRation->AddSpawnFlags(SF_NORESPAWN | SF_WEAPON_NO_PLAYER_PICKUP);
		pRation->SetPrimaryAmmoCount(m_iClip1);
		pRation->m_iClip1 = m_iClip1 = 0;
		Vector origin, forward, right, up, mins, maxs;
		pPlayer->EyePositionAndVectors(&origin, &forward, &right, &up);
		origin += right * RATION_THROW_EYE_OFFSET_RIGHT + forward * RATION_THROW_EYE_OFFSET_FORWARD
			+ up * RATION_THROW_EYE_OFFSET_UP;

		// Fix position if ration would end in solid
		matrix3x4_t rotation;
		AngleMatrix(angles, rotation);
		RotateAABB(rotation, pRation->WorldAlignMins(), pRation->WorldAlignMaxs(), mins, maxs);
		trace_t trace;
		UTIL_TraceHull(pPlayer->WorldSpaceCenter(), origin, mins, maxs, pPlayer->PhysicsSolidMaskForEntity(),
			pPlayer, pPlayer->GetCollisionGroup(), &trace);

		if (trace.DidHit())
		{
			origin = trace.endpos;
		}

		// Throw it
		Vector velocity = pPlayer->GetAbsVelocity() + forward * RATION_THROW_AIR_SPEED;
		pRation->Teleport(&origin, NULL, &velocity);
		AngularImpulse angularVelocity(0.0f, 0.0f, RATION_THROW_ANGULAR_VEL_YAW);
		pRation->ApplyLocalAngularVelocityImpulse(angularVelocity);
		AddEffects(EF_NODRAW);
		pRation->SetContextThink(&ThisClass::AllowPickup, gpGlobals->curtime + RATION_THROW_MAX_PICKUP_BLOCK_TIME,
			RATION_PICKUP_ALLOW_CONTEXT);
	}
}
