#ifndef HL2RP_DEFS_H
#define HL2RP_DEFS_H
#pragma once

#include <Color.h>

// Talk prefixes
#define HL2RP_COLOR_TALK_DEFAULT	"\x01"
#define HL2RP_COLOR_TALK_TEAM	"\x03"
#define HL2RP_COLOR_TALK_GREEN	"\x04"
#define HL2RP_COLOR_TALK_CUSTOM_NO_ALPHA	"\x07"
#define HL2RP_COLOR_TALK_CUSTOM_ALPHA	"\x08"

// Custom talk colors
#define HL2RP_COLOR_TALK_DEEP_SKY_BLUE	HL2RP_COLOR_TALK_CUSTOM_NO_ALPHA	"00BFFF"
#define HL2RP_COLOR_TALK_INDIAN_RED	HL2RP_COLOR_TALK_CUSTOM_NO_ALPHA	"CD5C5C"
#define HL2RP_COLOR_TALK_YELLOW	HL2RP_COLOR_TALK_CUSTOM_NO_ALPHA	"FFFF00"

// Must-use common talk color references
#define HL2RP_COLOR_TALK_STATIC	HL2RP_COLOR_TALK_DEEP_SKY_BLUE
#define HL2RP_COLOR_TALK_DYNAMIC	HL2RP_COLOR_TALK_GREEN
#define HL2RP_COLOR_TALK_REGION	HL2RP_COLOR_TALK_INDIAN_RED
#define HL2RP_COLOR_TALK_PHONE	HL2RP_COLOR_TALK_YELLOW

// HUD color parameters
#define HL2RP_COLOR_HUD_DARK_BLUE	Color(0, 0, 255, 255)
#define HL2RP_COLOR_HUD_RED	Color(255, 0, 0, 255)
#define HL2RP_COLOR_HUD_GREEN	Color(0, 255, 0, 255)
#define HL2RP_COLOR_HUD_INDIAN_RED	Color(255, 102, 102, 255)
#define HL2RP_COLOR_HUD_DARK_ORANGE	Color(255, 140, 0, 255)

// Must-use common HUD color references
#define HL2RP_COLOR_HUD_PEACEFUL	HL2RP_COLOR_HUD_DARK_BLUE
#define HL2RP_COLOR_HUD_CRIMINAL	HL2RP_COLOR_HUD_RED // For self player's Main HUD and cop's Wanted-Level list
#define HL2RP_COLOR_HUD_REGION	HL2RP_COLOR_HUD_INDIAN_RED
#define HL2RP_COLOR_HUD_PHONE	HL2RP_COLOR_HUD_DARK_ORANGE
#define HL2RP_COLOR_HUD_ALERTS	HL2RP_COLOR_HUD_GREEN
#define HL2RP_COLOR_HUD_XHAIR_ENTS	HL2RP_COLOR_HUD_GREEN
#define HL2RP_COLOR_HUD_XHAIR_PLAYER	COLOR_WHITE

#endif // HL2RP_DEFS_H