//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( C_BASEDOOR_H )
#define C_BASEDOOR_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"

#ifdef HL2RP
#include <hl2rp_property.h>
#endif // HL2RP

#if defined( CLIENT_DLL )
#define CBaseDoor C_BaseDoor
#endif

class C_BaseDoor : public DOOR_BASECLASS(C_BaseEntity)
{
public:
	DECLARE_CLASS( C_BaseDoor, DOOR_BASECLASS(C_BaseEntity) );
	DECLARE_CLIENTCLASS();

	C_BaseDoor( void );
	~C_BaseDoor( void );

public:
	float		m_flWaveHeight;
};

#ifdef HL2RP
class C_FuncMoveLinear : public DOOR_BASECLASS(C_BaseEntity)
{
	DECLARE_CLASS(C_FuncMoveLinear, DOOR_BASECLASS(C_BaseEntity))
	DECLARE_CLIENTCLASS()
};

class C_BaseButton : public DOOR_BASECLASS(C_BaseEntity)
{
	DECLARE_CLASS(C_BaseButton, DOOR_BASECLASS(C_BaseEntity))
	DECLARE_CLIENTCLASS()
};
#endif // HL2RP

#endif // C_BASEDOOR_H
