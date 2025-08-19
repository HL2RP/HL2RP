#ifndef TRIGGER_CITY_ZONE_H
#define TRIGGER_CITY_ZONE_H
#pragma once

#include "hl2rp_shareddefs.h"

SCOPED_ENUM(ECityZoneType, // Prioritized in ascending order
	None = -1,
	Generic,
	AutoCrime, // Adds some crime up automatically each second to citizens
	NoCrime,   // Prevents players from acquiring crime at all
	NoKill,    // Prevents players from killing each other ("safe zone")
	Jail,      // Cell for average criminals
	VIPJail,   // Reserved cell to most wanted criminals, they must stay longer than average
	Execution, // Optional death chamber for prisoners being released by a police
	Home
);

class CHL2RP_Property;

class CCityZone : public CServerOnlyEntity
{
	DECLARE_CLASS(CCityZone, CServerOnlyEntity)

#ifdef GAME_DLL
	DECLARE_DATADESC()

	void Spawn() OVERRIDE;
	void UpdateOnRemove() OVERRIDE;
	void Think() OVERRIDE;

public:
	bool IsEntityWithin(CBaseEntity*);
	bool IsPointWithin(const Vector&);
	void SendToPlayers(bool create = true, CRecipientFilter && = CBroadcastRecipientFilter()) HL2RP_FULL_FUNCTION;

	ECityZoneType mType;
#endif // GAME_DLL

public:
	static const char* sTypeTokens[ECityZoneType::_Count]; // For localization

	CHL2RP_Property* mpProperty;
};

#endif // !TRIGGER_CITY_ZONE_H
