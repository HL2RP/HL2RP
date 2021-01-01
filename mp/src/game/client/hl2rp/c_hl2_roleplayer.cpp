// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2_roleplayer.h"

#undef CHL2Roleplayer

IMPLEMENT_HL2RP_NETWORKCLASS(HL2Roleplayer)
RecvPropInt(RECVINFO(mMovementFlags)),
RecvPropInt(RECVINFO(mSeconds)),
RecvPropInt(RECVINFO(mCrime))
END_NETWORK_TABLE()

C_HL2Roleplayer* GetLocalHL2Roleplayer()
{
	return ToHL2Roleplayer(C_HL2MP_Player::GetLocalPlayer());
}

void C_HL2Roleplayer::SendHUDHint(EPlayerHUDHintType::Value type, const char* pToken, bool)
{
	IGameEvent* pEvent = gameeventmanager->CreateEvent(HL2RP_KEY_HINT_MESSAGE_LOCAL_EVENT_NAME);

	if (pEvent != NULL)
	{
		pEvent->SetInt("type", type);
		pEvent->SetString("token", pToken);
		gameeventmanager->FireEventClientSide(pEvent);
	}
}
