#ifndef ROLEPLAY_TALK_H
#define ROLEPLAY_TALK_H
#ifdef _WIN32
#pragma once
#endif

class CHL2RP_Talk
{
public:
	static void ClientHandleSay(edict_t *pEdict, CBasePlayer *pPlayer, const char *pszPlayerName, const char *msg_name, bool teamonly);
};

#endif // ROLEPLAY_TALK_H
