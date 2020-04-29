#ifndef ROLEPLAY_COLORS_H
#define ROLEPLAY_COLORS_H
#ifdef _WIN32
#pragma once
#endif

// Chat prefixes:
#define DEFAULT_CHAT_COLOR					"\x01"
#define TEAM_CHAT_COLOR						"\x03"
#define GREEN_CHAT_COLOR					"\x04"
#define CUSTOM_CHAT_COLOR_PREFIX_NO_ALPHA	"\x07"
#define CUSTOM_CHAT_COLOR_PREFIX_ALPHA		"\x08"

// Custom chat colors:
#define DEEP_SKY_BLUE_CHAT_COLOR			CUSTOM_CHAT_COLOR_PREFIX_NO_ALPHA	"00BFFF"
#define INDIAN_RED_CHAT_COLOR				CUSTOM_CHAT_COLOR_PREFIX_NO_ALPHA	"CD5C5C"
#define YELLOW_CHAT_COLOR					CUSTOM_CHAT_COLOR_PREFIX_NO_ALPHA	"FFFF00"

// Must-use common chat color references:
#define STATIC_TALK_COLOR					DEEP_SKY_BLUE_CHAT_COLOR
#define DYNAMIC_TALK_COLOR					GREEN_CHAT_COLOR	// Before params

// HUD color parameters
static const Color GreenHUDColor() { return Color(0, 255, 0, 255); }
static const Color WhiteHUDColor() { return Color(255, 255, 255, 255); }
static const Color PeacefulMainHUDColor() { return Color(0, 0, 255, 255); } // Dark blue
static const Color CriminalMainHUDColor() { return Color(255, 0, 0, 255); } // Red
static const Color RegionHUDColor() { return Color(255, 102, 102, 255); } // Indian red
static const Color PhoneHUDColor() { return Color(255, 140, 0, 255); } // Dark orange
static const Color PVPProtectionHUDColor() { return GreenHUDColor(); }
static const Color PlayerInfoHUDColor() { return WhiteHUDColor(); }
static const Color DoorInfoHUDColor() { return GreenHUDColor(); }
static const Color WelcomeHUDColor() { return WhiteHUDColor(); }

#endif // ROLEPLAY_COLORS_H
