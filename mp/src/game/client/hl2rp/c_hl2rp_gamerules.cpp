// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2rp_gamerules.h"
#include "c_hl2_roleplayer.h"
#include <hl2rp_configuration.h>
#include <clientmode_shared.h>
#include <gameeventdefs.h>

C_HL2RPRules::C_HL2RPRules()
{
	ListenForGameEvent("player_activate");
}

void C_HL2RPRules::FireGameEvent(IGameEvent* pEvent)
{
	if (Q_strcmp(pEvent->GetName(), "player_activate") == 0)
	{
		// Notify the server of current learned HUD hints, to save server from sending these again
		engine->ServerCmdKeyValues(new KeyValues(HL2RP_SENT_HUD_HINTS_UPDATE_EVENT_NAME,
			HL2RP_SENT_HUD_HINTS_UPDATE_EVENT_DATA_KEY, gHL2RPConfiguration.mUserData->GetInt("sentHUDHints")));
	}
}
