//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"
#include "hl2mp_gameinterface.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameClients implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers = 2;
	defaultMaxPlayers = 16;
	maxplayers = MAX_PLAYERS;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

#ifndef HL2RP
void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}
#endif
