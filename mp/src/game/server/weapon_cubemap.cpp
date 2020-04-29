//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_cubemap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( weapon_cubemap, CWeaponCubemap );

IMPLEMENT_SERVERCLASS_ST( CWeaponCubemap, DT_WeaponCubemap )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCubemap::Precache( void )
{
	BaseClass::Precache();
}

void CWeaponCubemap::Spawn( void )
{
	BaseClass::Spawn();
}
