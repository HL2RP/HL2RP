#include <cbase.h>
#include <hl2mp_player.h>
#include <networkstringtable_gamedll.h>

// Returns the descriptive name of this .dll. E.g., Half-Life, or Team Fortress 2.
const char* GetGameDescription()
{
	return "Half-Life 2: Roleplay";
}

#ifndef HL2DM_RP
void InstallGameRules()
{
	CreateGameRulesObject("CHL2RPRules");
}
#endif // !HL2DM_RP
