#include <cbase.h>
#include <filesystem.h>
#include <gameinterface.h>
#include "HL2RPProperty.h"
#include <mapentities.h>
#include <utlbuffer.h>

void CServerGameClients::GetPlayerLimits(int& minPlayers, int& maxPlayers, int& defaultMaxPlayers) const
{
	minPlayers = 2;
	maxPlayers = defaultMaxPlayers = MAX_PLAYERS - 1;
}

void CServerGameDLL::LevelInit_ParseAllEntities(const char* pMapEntities)
{
	// Load a .MAP file related to current map for adding extra entities on top of it
	char entsPath[MAX_PATH];
	Q_snprintf(entsPath, sizeof(entsPath), "maps/%s.map", STRING(gpGlobals->mapname));
	CUtlBuffer buffer;

	if (filesystem->ReadFile(entsPath, "GAME", buffer))
	{
		char* pMapEntitiesText = static_cast<char*>(buffer.Base());
		MapEntity_ParseAllEntities(pMapEntitiesText);
	}

	// Link property doors and trigger custom map start actions on them
	for (CBaseEntity* pEntity = NULL;
		(pEntity = gEntList.FindEntityByClassname(pEntity, "hl2rp_property_door")) != NULL;)
	{
		CHL2RPPropertyDoor* pPropertyDoor = static_cast<CHL2RPPropertyDoor*>(pEntity);
		pPropertyDoor->NotifyMapSpawn();
	}
}