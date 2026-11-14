//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_PROPS_H
#define C_PROPS_H
#ifdef _WIN32
#pragma once
#endif

#include "c_breakableprop.h"
#include "props_shared.h"

#ifdef HL2RP
#include <hl2rp_property.h>
#endif // HL2RP

#define CDynamicProp C_DynamicProp

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_DynamicProp : public C_BreakableProp
{
	DECLARE_CLASS( C_DynamicProp, C_BreakableProp );
public:
	DECLARE_NETWORKCLASS();

	// constructor, destructor
	C_DynamicProp( void );
	~C_DynamicProp( void );

	void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	unsigned int ComputeClientSideAnimationFlags();
	bool TestBoneFollowers( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	bool TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

private:
	C_DynamicProp( const C_DynamicProp & );

	bool	m_bUseHitboxesForRenderBox;
	int		m_iCachedFrameCount;
	Vector	m_vecCachedRenderMins;
	Vector	m_vecCachedRenderMaxs;
};

// ------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------ //
class C_BasePropDoor : public DOOR_BASECLASS(C_DynamicProp)
{
	DECLARE_CLASS(C_BasePropDoor, DOOR_BASECLASS(C_DynamicProp));

public:
	DECLARE_CLIENTCLASS();

	// constructor, destructor
	C_BasePropDoor(void);
	virtual ~C_BasePropDoor(void);

	virtual void OnDataChanged(DataUpdateType_t type);

	virtual bool TestCollision(const Ray_t & ray, unsigned int mask, trace_t & trace);

private:
	C_BasePropDoor(const C_BasePropDoor&);

#ifdef HL2RP
	void GetHUDInfo(CHL2Roleplayer*, CLocalizeFmtStr<>&) OVERRIDE;

	C_BasePropDoor* GetMaster();
	C_BasePropDoor* GetMainDoor(); // Returns master (if any) or self
	bool IsDoorClosed();

	DoorState_t m_eDoorState;
	CHandle<C_BasePropDoor> m_hMaster;
#endif // HL2RP
};

#endif // C_PROPS_H
