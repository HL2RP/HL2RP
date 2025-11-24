#ifndef HL2RP_GAMERULES_SHARED_H
#define HL2RP_GAMERULES_SHARED_H
#pragma once

#include "hl2rp_util_shared.h"
#include <hl2mp_gamerules.h>

#ifdef CLIENT_DLL
#define CBaseHL2RPRules  C_BaseHL2RPRules
#define CHL2RPRulesProxy C_HL2RPRulesProxy
#endif // CLIENT_DLL

class CHL2RPRules;

class CBaseHL2RPRules : public CHL2MPRules
{
	DECLARE_CLASS(CBaseHL2RPRules, CHL2MPRules)

public:
	CUtlPooledStringMap<uint64> mPlayerNameBySteamIdNum;
};

class CHL2RPRulesProxy : public CHL2MPGameRulesProxy
{
	DECLARE_CLASS(CHL2RPRulesProxy, CHL2MPGameRulesProxy)

	friend class CHL2RPRules;

	void DayNightMapChangeThink();
};

CHL2RPRules* HL2RPRules();

#endif // !HL2RP_GAMERULES_SHARED_H
