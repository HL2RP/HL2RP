#ifndef TRIGGER_CITY_ZONE_H
#define TRIGGER_CITY_ZONE_H
#pragma once

SCOPED_ENUM(ECityZoneType, // Prioritized in ascending order
	None,
	AutoCrime, // Adds some crime up automatically each second to citizens
	NoCrime,   // Prevents players from acquiring crime at all
	NoKill,    // Prevents players from killing each other ("safe zone")
	Jail,      // Cell for average criminals
	VIPJail,   // Reserved cell to most wanted criminals, they must stay longer than average
	Execution, // Optional death chamber for prisoners being released by a police
	Home
);

class CCityZone : public CServerOnlyEntity
{
	DECLARE_CLASS(CCityZone, CServerOnlyEntity)
	DECLARE_DATADESC()

	void Think() OVERRIDE;

public:
	static const char* sTypeTokens[ECityZoneType::_Count]; // For localization

	void Spawn() OVERRIDE;

	bool IsEntityWithin(CBaseEntity*);
	bool IsPointWithin(const Vector&);

	ECityZoneType mType;
};

#endif // !TRIGGER_CITY_ZONE_H
