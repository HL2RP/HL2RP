#include <cbase.h>
#include "HL2RP_Player.h"

// CON_COMMAND handles quotes automatically for command name, not for description
CON_COMMAND(teleport_test, "teleport_test")
{
	// This is how we obtain the player pointer from command, so we can interact with it
	// We can cast to hl2rp_player. The _Fast version calls static_cast.
	// It is safe because we're in the HL2RP context, and in this context
	// players are always CHL2RP_Players. And when it returns NULL static_cast is safe too.
	// The point is to avoid dynamic_cast.
	CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_GetCommandClient());

	// Adding some information so you can learn about arguments class details
	// 'args' is a ConCommand object, like that code you sent me in Steam.
	// So it's the active command invokation class.
	Msg("The arguments are: %s. The argument count is: %i\n", args.ArgS(), args.ArgC());

	// We're going to try make generic Teleport, where you pass the desired origin as parameters
	if (pPlayer != NULL && args.ArgC() > 3)
	{
		pPlayer->m_iCrime; // Doing sample stuff to show you can interact with its variables already
		float x = Q_atof(args.Arg(1));
		float y = Q_atof(args.Arg(2));
		float z = Q_atof(args.Arg(3));
		Vector origin(x, y, z); // Here I had typical vector confusion. Class Vector is what needs to be used for physical vectors.
		pPlayer->Teleport(&origin, NULL, NULL);
		Msg("Teleported player to %f %f %f\n", x, y, z);
	}

	// Well basic teleport is done
}
