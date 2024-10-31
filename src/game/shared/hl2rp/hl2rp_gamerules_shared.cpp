// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gamerules_shared.h"
#include <ammodef.h>

#ifdef GAME_DLL
#include <hl2rp_gamerules.h>
#else
#include <c_hl2rp_gamerules.h>
#endif // GAME_DLL

#ifdef HL2RP_FULL
REGISTER_GAMERULES_CLASS(CHL2RPRules)
#endif // HL2RP_FULL

void HL2RP_InitAmmoDefs(CAmmoDef& def)
{
	def.AddAmmoType("Molotov", DMG_BURN, TRACER_NONE, 0, 0, INT_MAX, 0.0f, ITEM_FLAG_NOITEMPICKUP, 1, 1);
	def.AddAmmoType("Ration", DMG_GENERIC, TRACER_NONE, 0, 0, INT_MAX, 0.0f, ITEM_FLAG_NOITEMPICKUP, 1, 1);
	def.AddAmmoType("Suitcase", DMG_GENERIC, TRACER_NONE, 0, 0, 1, 0.0f, ITEM_FLAG_NOAMMOPICKUPS, 1, 1);
}

CHL2RPRules* HL2RPRules()
{
	return static_cast<CHL2RPRules*>(GameRules());
}

ConVar gRegionMaxRadiusCVar("sv_region_max_radius", "350.0", FCVAR_ARCHIVE | FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Max. regional communication distance between players");
