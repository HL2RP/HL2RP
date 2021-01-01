// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include <prop_ration_dispenser.h>

#undef CRationDispenserProp

LINK_ENTITY_TO_CLASS(prop_ration_dispenser, C_RationDispenserProp)

IMPLEMENT_HL2RP_NETWORKCLASS(RationDispenserProp)
RecvPropTime(RECVINFO(mNextTimeAvailable))
END_NETWORK_TABLE()
