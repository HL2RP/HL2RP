#include "cbase.h"
#include "CHL2RP_Talk.h"
#include "CHL2RP.h"
#include "CHL2RP_Colors.h"
#include "CHL2RP_Player.h"
#include "CHL2RP_Phrases.h"
#include "basemultiplayerplayer.h"
#include "voice_gamemgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Forward declare required phrase headers
extern const char REGION_CHAT_HEADER[];

void CHL2RP_Talk::ClientHandleSay(edict_t *pEdict, CBasePlayer *pPlayer, const char *pszPlayerName, const char *msg_name, bool teamonly)
{
	char chat[MAX_VALVE_USER_MSG_TEXT_DATA], console[256];

	if (teamonly)
	{
		CHL2RP_Phrases::LoadForPlayer(CHL2RP_Player::ToThisClassFast(pPlayer), chat, sizeof(chat), REGION_CHAT_HEADER,
			INDIAN_RED_CHAT_COLOR, CPhraseParam(pszPlayerName, TEAM_CHAT_COLOR "%s" DEFAULT_CHAT_COLOR), msg_name);
		Q_snprintf(console, sizeof(console), "(Region) %s: %s\n", pszPlayerName, msg_name);
	}
	else
	{
		Q_snprintf(chat, sizeof(chat), DEFAULT_CHAT_COLOR "(OOC) " TEAM_CHAT_COLOR "%s " DEFAULT_CHAT_COLOR ": %s", pszPlayerName, msg_name);
		Q_snprintf(console, sizeof(console), "(OOC) %s: %s\n", pszPlayerName, msg_name);
	}

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	CBasePlayer *client = NULL;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		client = ToBaseMultiplayerPlayer(UTIL_PlayerByIndex(i));
		if (!client || !client->edict())
			continue;

		if (client->edict() == pEdict)
			continue;

		if (!(client->IsNetClient()))	// Not a client ? (should never be true)
			continue;

		if (teamonly && g_pGameRules->PlayerCanHearChat(client, pPlayer) != GR_TEAMMATE)
			continue;

		if (pPlayer && !client->CanHearAndReadChatFrom(pPlayer))
			continue;

		if (pPlayer && GetVoiceGameMgr() && GetVoiceGameMgr()->IsPlayerIgnoringPlayer(pPlayer->entindex(), i))
			continue;

		CSingleUserRecipientFilter user(client);
		user.MakeReliable();

		UTIL_SayText2Filter(user, pPlayer, true, chat);
	}

	if (pPlayer)
	{
		// print to the sending client
		CSingleUserRecipientFilter user(pPlayer);
		user.MakeReliable();

		UTIL_SayText2Filter(user, pPlayer, true, chat);
	}

	// echo to server console
	// Adrian: Only do this if we're running a dedicated server since we already print to console on the client.
	if (engine->IsDedicatedServer())
		Msg("%s", console);
}
