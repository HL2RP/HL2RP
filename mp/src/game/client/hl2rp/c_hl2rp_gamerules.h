#ifndef C_HL2RP_GAMERULES_H
#define C_HL2RP_GAMERULES_H
#pragma once

#include <hl2rp_gamerules_shared.h>

class C_HL2RPRules : CBaseHL2RPRules, CGameEventListener
{
	void FireGameEvent(IGameEvent*) OVERRIDE;

public:
	C_HL2RPRules();
};

#endif // !C_HL2RP_GAMERULES_H
