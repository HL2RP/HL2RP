#ifndef HL2RP_GAMERULES_SHARED_H
#define HL2RP_GAMERULES_SHARED_H
#pragma once

#include "hl2rp_util_shared.h"
#include <hl2mp_gamerules.h>

class CHL2RPRules;

class CBaseHL2RPRules : public CHL2MPRules
{
	DECLARE_CLASS(CBaseHL2RPRules, CHL2MPRules)

public:
	CUtlPooledStringMap<uint64> mPlayerNameBySteamIdNum;
};

CHL2RPRules* HL2RPRules();

#endif // !HL2RP_GAMERULES_SHARED_H
