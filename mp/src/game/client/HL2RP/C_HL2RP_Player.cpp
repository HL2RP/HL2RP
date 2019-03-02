#include <cbase.h>
#include "C_HL2RP_Player.h"

// Avoid conflicting when refering to the real server class
#undef CHL2RP_Player

IMPLEMENT_CLIENTCLASS_DT(C_HL2RP_Player, DT_HL2RP_Player, CHL2RP_Player)
END_RECV_TABLE();
