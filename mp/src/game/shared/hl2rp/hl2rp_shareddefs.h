#ifndef HL2RP_SHAREDDEFS_H
#define HL2RP_SHAREDDEFS_H
#pragma once

#include <generic.h>

#ifdef HL2RP_FULL
#define DECLARE_HL2RP_SERVERCLASS()  DECLARE_SERVERCLASS()
#define DECLARE_HL2RP_NETWORKCLASS() DECLARE_NETWORKCLASS()

#define IMPLEMENT_HL2RP_NETWORKCLASS(classStem) \
	IMPLEMENT_NETWORKCLASS_ALIASED(classStem, DT_##classStem) \
	BEGIN_HL2RP_NETWORK_TABLE(classStem)

#ifdef GAME_DLL
#define BEGIN_HL2RP_NETWORK_TABLE(classStem) BEGIN_NETWORK_TABLE(C##classStem, DT_##classStem)

#define HL2RP_FULL_FUNCTION
#define HL2RP_LEGACY_FUNCTION           {}
#define HL2RP_CLIENT_OR_LEGACY_FUNCTION {}
#else
#define BEGIN_HL2RP_NETWORK_TABLE(classStem) BEGIN_NETWORK_TABLE(C_##classStem, DT_##classStem)

#define HL2RP_CLIENT_OR_LEGACY_FUNCTION

#define CHL2Roleplayer C_HL2Roleplayer
#endif // GAME_DLL
#else
#define DECLARE_HL2RP_SERVERCLASS()
#define DECLARE_HL2RP_NETWORKCLASS()
#define HL2RP_FULL_FUNCTION {}
#define HL2RP_LEGACY_FUNCTION
#define HL2RP_CLIENT_OR_LEGACY_FUNCTION
#endif // HL2RP_FULL

// HUD positions, based off screen percentages
#define HL2RP_CENTER_HUD_SPECIAL_POS  -1.0f
#define HL2RP_MAIN_HUD_DEFAULT_X_POS   0.015f
#define HL2RP_MAIN_HUD_DEFAULT_Y_POS   0.03f
#define HL2RP_ZONE_HUD_DEFAULT_Y_POS   0.03f
#define HL2RP_ALERT_HUD_DEFAULT_Y_POS  0.2f
#define HL2RP_AIMING_HUD_DEFAULT_Y_POS 0.78f
#define HL2RP_COMM_HUD_DEFAULT_X_POS   0.82f
#define HL2RP_COMM_HUD_DEFAULT_Y_POS   0.62f

// HUD colors
#define HL2RP_HUD_COLOR_GREEN                      Color(0, 255, 0, 255)
#define HL2RP_MAIN_HUD_DEFAULT_NORMAL_TEXT_COLOR   Color(0, 0, 255, 255)
#define HL2RP_MAIN_HUD_DEFAULT_CRIMINAL_TEXT_COLOR Color(255, 0, 0, 255)
#define HL2RP_ALERT_HUD_DEFAULT_COLOR              COLOR_WHITE
#define HL2RP_ZONE_HUD_DEFAULT_COLOR               HL2RP_HUD_COLOR_GREEN
#define HL2RP_AIMING_HUD_DEFAULT_GENERAL_COLOR     HL2RP_HUD_COLOR_GREEN
#define HL2RP_AIMING_HUD_DEFAULT_PLAYER_COLOR      COLOR_WHITE
#define HL2RP_REGION_HUD_HEADER_DEFAULT_COLOR      Color(255, 150, 150, 255)
#define HL2RP_REGION_HUD_PLAYERS_DEFAULT_COLOR     Color(255, 100, 100, 255)

#define HL2RP_KEY_HINT_USER_MESSAGE "HL2RPKeyHintText"

#define HL2RP_LEARNED_HUD_HINTS_FIELD_NAME        "learnedHUDHints"
#define HL2RP_LEARNED_HUD_HINTS_UPDATE_EVENT_NAME "LearnedHUDHintsUpdated"

#define NETWORK_DIALOG_REWIND_SOUND "Buttons.snd9"
#define NETWORK_MENU_ITEM_SOUND     "Buttons.snd37"

SCOPED_ENUM(EFaction, // NOTE: Don't change order
	Citizen,
	Combine
)

SCOPED_ENUM(EPlayerModelType, // NOTE: Don't change order
	Job,
	Citizen,
	VIPCitizen,
	Admin, // Generic admin
	Root
)

#endif // !HL2RP_SHAREDDEFS_H