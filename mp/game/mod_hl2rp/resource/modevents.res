//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

// No spaces in event names, max length 32
// All strings are case sensitive
//
// valid data key types are:
//   string : a zero terminated string
//   bool   : unsigned int, 1 bit
//   byte   : unsigned int, 8 bit
//   short  : signed int, 16 bit
//   long   : signed int, 32 bit
//   float  : float, 32 bit
//   local  : any data, but not networked to clients
//
// following key names are reserved:
//   local      : if set to 1, event is not networked to clients
//   unreliable : networked, but unreliable
//   suppress   : never fire this event
//   time	: firing server time
//   eventid	: holds the event ID

"ModEvents"
{
	"player_death"
	{
		"userid"			"short" // User ID who died
		"attacker"			"short" // User ID who killed
		"killer_entindex"	"short" // Fallback entindex, in case killer is a NPC
		"weapon"			"string" // Weapon name killer used
	}

	"npc_death"
	{
		"victim_entindex"	"short"
		"killer_entindex"	"short"
		"weapon"			"string"
	}

	"teamplay_round_start" // Round restart
	{
		"full_reset" "bool" // Is this a full reset of the map?
	}

	"spec_target_updated"
	{

	}

	"achievement_earned"
	{
		"player"		"byte" // entindex of the player
		"achievement"	"short" // Achievement ID
	}
}
