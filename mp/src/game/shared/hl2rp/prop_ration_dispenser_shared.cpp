// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "prop_ration_dispenser.h"
#include <ration.h>
#include <engine/IEngineSound.h>

#define RATION_DISPENSER_MODEL_PATH "models/props_combine/combine_dispenser.mdl"

LINK_ENTITY_TO_CLASS(prop_ration_dispenser, CRationDispenserProp)

#ifdef HL2RP_FULL
IMPLEMENT_HL2RP_NETWORKCLASS(RationDispenserProp)
#ifdef GAME_DLL
SendPropTime(SENDINFO(mNextTimeAvailable))
#else
RecvPropTime(RECVINFO(mNextTimeAvailable))
#endif // GAME_DLL
END_NETWORK_TABLE()
#endif // HL2RP_FULL

void CRationDispenserProp::Spawn()
{
	SetModelName(MAKE_STRING(RATION_DISPENSER_MODEL_PATH));
	SetSolid(SOLID_VPHYSICS);
	BaseClass::Spawn();
}

void CRationDispenserProp::Precache()
{
	PrecacheModel(RATION_DISPENSER_MODEL_PATH);
	PrecacheModel(RATION_DISPENSER_SUPPLY_GLOW_SPRITE_PATH);
	PrecacheModel(RATION_DISPENSER_DENY_GLOW_SPRITE_PATH);
	enginesound->PrecacheSound(RATION_DISPENSER_SUPPLY_SOUND);
	enginesound->PrecacheSound(RATION_DISPENSER_DENY_SOUND);
}

int CRationDispenserProp::ObjectCaps()
{
	return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE);
}

void CRationDispenserProp::OnContainedRationPickup()
{
	mNextTimeAvailable = gpGlobals->curtime + RATION_DISPENSER_AVAILABILITY_COOLDOWN;
	mhContainedRation = NULL;

	if (!IsSequenceFinished())
	{
		mNextTimeAvailable += SequenceDuration() * (1.0f - GetCycle());
	}
}
