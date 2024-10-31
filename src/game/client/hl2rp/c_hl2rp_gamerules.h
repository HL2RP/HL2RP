#ifndef C_HL2RP_GAMERULES_H
#define C_HL2RP_GAMERULES_H
#pragma once

#include <hl2rp_gamerules_shared.h>

class CCityZone;
class CHL2RP_Property;

class C_HL2RPRules : public CBaseHL2RPRules, CGameEventListener
{
	void LevelShutdownPostEntity() OVERRIDE;
	void FireGameEvent(IGameEvent*) OVERRIDE;

public:
	C_HL2RPRules();

	CAutoLessFuncAdapter<CAutoDeleteAdapter<CUtlMap<int, CHL2RP_Property*>>> mPropertyById;
	CAutoLessFuncAdapter<CUtlMap<int, CHandle<CCityZone>>> mCityZoneByIndex; // Zone by server's EHANDLE entry index
};

#endif // !C_HL2RP_GAMERULES_H
