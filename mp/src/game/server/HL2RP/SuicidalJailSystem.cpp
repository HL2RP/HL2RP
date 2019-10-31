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


CON_COMMAND(cuffing, "cuffing")
{
	CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_GetCommandClient());


	if (pPlayer != NULL)
	{
		color32 clr = pPlayer->GetRenderColor();
		Msg(" ----------------- \n");
		Msg(" player color: r:%i g:%i b:%i a:%i ", clr.r, clr.g, clr.b, clr.a);
		pPlayer->RemoveAllWeapons();
		pPlayer->RemoveSuit();
		pPlayer->SetMaxSpeed(80.0f);
		pPlayer->SetRenderColor(0, 75, 255, 255);
	}
}

CON_COMMAND(uncuffing, "uncuffing")
{
	CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_GetCommandClient());


	if (pPlayer != NULL)
	{
		pPlayer->EquipSuit();
		pPlayer->SetMaxSpeed(120);
		pPlayer->StopWalking();
		pPlayer->GiveNamedItem("weapon_physcannon");
		pPlayer->SetRenderColor(255, 255, 255, 255);
	}
}



CON_COMMAND(setcolor, "setcolor")
{
	CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_GetCommandClient());


	if (pPlayer != NULL)
	{
		char colorBuf[32];
		Q_snprintf(colorBuf, sizeof(colorBuf), "%s %s %s %s", args.Arg(1), args.Arg(2), args.Arg(3), args.Arg(4));
		//byte myColorR = Q_atoi(args.Arg(1));
		//byte myColorG = Q_atoi(args.Arg(2));
		//byte myColorB = Q_atoi(args.Arg(3));
		//byte myColorA = Q_atoi(args.Arg(4));
		//byte myColorR = *(byte *)args.Arg(1);
		//byte myColorG = (byte)args.Arg(2);
		//byte myColorB = (byte)args.Arg(3);
		//byte myColorA = (byte)args.Arg(4);

		pPlayer->KeyValue("rendercolor32", colorBuf);
		//pPlayer->SetRenderColor(myColorR, myColorG, myColorB, myColorA);
	}
}

