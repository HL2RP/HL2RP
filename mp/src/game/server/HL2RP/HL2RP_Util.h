#ifndef HL2RP_UTIL_H
#define HL2RP_UTIL_H
#ifdef _WIN32
#pragma once
#endif
// HL2RP_Util - Provides global functions to be reused on varied HL2RP sub-contexts

#include "HL2RP_Defs.h"

#define HL2RP_UTIL_DIALOG_DEFAULT_TITLE_COLOR	HL2RP_COLOR_HUD_GREEN
#define HL2RP_UTIL_DIALOG_TEXT_DEFAULT_TITLE_TEXT	"You have a Text, press ESC"
#define HL2RP_UTIL_DIALOG_PANEL_DEFAULT_DISPLAY_INTERVAL	200 // Max. Valve's client-side default
#define HL2RP_UTIL_DIALOG_ASKCONNECT_DEFAULT_INTERVAL 4.0f

class CHL2RP_Player;

// Adds up trying to avoid result overflowing
int UTIL_EnsureAddition(int original, int amount);

bool UTIL_IsSavedEntity(CBaseEntity* pEntity, int databaseId);

void UTIL_HUDMessage(CHL2RP_Player* pPlayer, int channel, float x, float y, float duration,
	const Color& color, const char* pMessage);

bool UTIL_SendDialog(CHL2RP_Player* pPlayer, KeyValues* pDialog, DIALOG_TYPE dialogType);

// Sends a dialog to a remote player showing a menu panel with different options.
// Note: This dialog will only last for 10 fixed seconds, as it simply consists of the HUD title bar.
// Output: True only if the player is remote or the host/loopback (not a fake client), and the dialog can be networked.
bool UTIL_SendMsgDialog(CHL2RP_Player* pPlayer, const char* pTitle,
	const Color& color = HL2RP_UTIL_DIALOG_DEFAULT_TITLE_COLOR);

// Sends a dialog to a remote player which may contain a large message displayed in a panel.
// Output: True only if the player is remote or the host/loopback (not a fake client), and the dialog can be networked.
bool UTIL_SendTextDialog(CHL2RP_Player* pPlayer, const char* pMsg,
	const char* pTitle = HL2RP_UTIL_DIALOG_TEXT_DEFAULT_TITLE_TEXT,
	const Color& color = HL2RP_UTIL_DIALOG_DEFAULT_TITLE_COLOR,
	int holdTime = HL2RP_UTIL_DIALOG_PANEL_DEFAULT_DISPLAY_INTERVAL);

// Sends a dialog to a remote player asking to connect to a specified server IP.
// Output: True only if the player is remote or the host/loopback (not a fake client), and the dialog can be networked.
// Note: This dialog always shows once received, ignoring the "level" value and overlapping with other dialogs.
bool UTIL_SendAskConnectDialog(CHL2RP_Player* pPlayer, const char* pIPString,
	float holdTime = HL2RP_UTIL_DIALOG_ASKCONNECT_DEFAULT_INTERVAL);

// Sends a ConVar value to a player. Might be a fake client, a remote player, or the loopback player.
// Output: For a fake client, always true. For a remote player, true if the ConVar can be networked.
// If the player is the host/loopback, true if the ConVar exists. False otherwise.
// Note that the final ConVar assignment is NOT involved in this return value.
bool UTIL_SendConVarValue(CBasePlayer* pPlayer, const char* pCVarName, const char* pValueString);

#endif // HL2RP_UTIL_H
