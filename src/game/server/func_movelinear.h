//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_MOVELINEAR_H
#define FUNC_MOVELINEAR_H

#pragma once

#include "basetoggle.h"
#include "entityoutput.h"

#ifdef HL2RP
#include <hl2rp_property.h>
#endif // HL2RP

class IPhysicsFluidController;


class CFuncMoveLinear : public DOOR_BASECLASS(CBaseToggle)
{
	DECLARE_CLASS( CFuncMoveLinear, DOOR_BASECLASS(CBaseToggle) );

#ifdef HL2RP
	DECLARE_HL2RP_SERVERCLASS()

	void InputLock(inputdata_t&);
	void InputUnlock(inputdata_t&);
#endif // HL2RP

public:
	void		Spawn( void );
	void		Precache( void );
	bool		CreateVPhysics( void );
	bool		ShouldSavePhysics( void );

	void		MoveTo(Vector vPosition, float flSpeed);
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void		MoveDone( void );
	void		StopMoveSound( void );
	void		Blocked( CBaseEntity *pOther );
	void		SetPosition( float flPosition );

	int			DrawDebugTextOverlays(void);

	// Input handlers
	void InputOpen( inputdata_t &inputdata );
	void InputClose( inputdata_t &inputdata );
	void InputSetPosition( inputdata_t &inputdata );
	void InputSetSpeed( inputdata_t &inputdata );
	
	DECLARE_DATADESC();

	Vector		m_vecMoveDir;			// Move direction.

	string_t	m_soundStart;			// start and looping sound
	string_t	m_soundStop;			// stop sound
	string_t	m_currentSound;			// sound I'm playing

	float		m_flBlockDamage;		// Damage inflicted when blocked.
	float		m_flStartPosition;		// Position of brush when spawned
	float		m_flMoveDistance;		// Total distance the brush can move

	IPhysicsFluidController *m_pFluidController;

	// Outputs
	COutputEvent m_OnFullyOpen;
	COutputEvent m_OnFullyClosed;
};
#endif // FUNC_MOVELINEAR_H
