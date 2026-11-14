//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basedoor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CBaseDoor
#undef CBaseDoor
#endif

IMPLEMENT_CLIENTCLASS_DT(C_BaseDoor, DT_BaseDoor, CBaseDoor)
	RecvPropFloat(RECVINFO(m_flWaveHeight)),

#ifdef HL2RP
	RecvPropDataTable(RECVINFO_DT(mPropertyDoorData), 0, &REFERENCE_RECV_TABLE(DT_HL2RP_PropertyDoorData)),
	RecvPropBool(RECVINFO(m_bLocked))
#endif // HL2RP
END_RECV_TABLE()

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

IMPLEMENT_HL2RP_NETWORKCLASS(RotDoor)
RecvPropDataTable(RECVINFO_DT(mPropertyDoorData), 0, &REFERENCE_RECV_TABLE(DT_HL2RP_PropertyDoorData)),
RecvPropBool(RECVINFO(m_bLocked)),
RecvPropInt(RECVINFO(m_toggle_state))
END_RECV_TABLE()

IMPLEMENT_HL2RP_NETWORKCLASS(FuncMoveLinear)
RecvPropDataTable(RECVINFO_DT(mPropertyDoorData), 0, &REFERENCE_RECV_TABLE(DT_HL2RP_PropertyDoorData)),
RecvPropBool(RECVINFO(m_bLocked))
END_RECV_TABLE()

IMPLEMENT_HL2RP_NETWORKCLASS(BaseButton)
RecvPropDataTable(RECVINFO_DT(mPropertyDoorData), 0, &REFERENCE_RECV_TABLE(DT_HL2RP_PropertyDoorData)),
RecvPropBool(RECVINFO(m_bLocked))
END_RECV_TABLE()
#endif // HL2RP

C_BaseDoor::C_BaseDoor( void )
{
	m_flWaveHeight = 0.0f;
}

C_BaseDoor::~C_BaseDoor( void )
{
}
