// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2_roleplayer.h"
#include <prediction.h>
#include <usermessages.h>

IMPLEMENT_HL2RP_NETWORKCLASS(HL2Roleplayer)
RecvPropInt(RECVINFO(m_iMaxHealth)),
RecvPropInt(RECVINFO(mMovementFlags)),
RecvPropInt(RECVINFO(mSeconds)),
RecvPropInt(RECVINFO(mCrime))
END_NETWORK_TABLE()

// Simulates server-side UserMessages on client, for direct display (no networking)
class CLocalUserMessenger
{
	byte mBuffer[PAD_NUMBER(MAX_USER_MSG_DATA, 4)];
	bf_write mWriter;

public:
	CLocalUserMessenger() : mWriter(mBuffer, sizeof(mBuffer)) {}

	// Returns whether the object is valid during prediction, to prevent duplicating the message
	operator bool()
	{
		return prediction->IsFirstTimePredicted();
	}

	bf_write* operator->()
	{
		return &mWriter;
	}

	void Dispatch(const char* pName);
};

void CLocalUserMessenger::Dispatch(const char* pName)
{
	bf_read reader(mBuffer, mWriter.GetNumBytesWritten());
	clientdll->DispatchUserMessage(usermessages->LookupUserMessage(pName), reader);
}

C_HL2Roleplayer* GetLocalHL2Roleplayer()
{
	return ToHL2Roleplayer(C_HL2MP_Player::GetLocalPlayer());
}

int C_HL2Roleplayer::GetMaxHealth() const
{
	return m_iMaxHealth;
}

void C_HL2Roleplayer::Print(int type, const char* pText, ...)
{
	CLocalUserMessenger messenger;

	if (messenger)
	{
		messenger->WriteByte(type);
		messenger->WriteString(pText);
		messenger.Dispatch("TextMsg");
	}
}

void C_HL2Roleplayer::SendHUDHint(EPlayerHUDHintType::_Value type, const char* pToken, ...)
{
	CLocalUserMessenger messenger;

	if (messenger)
	{
		messenger->WriteLong(type);
		messenger->WriteString(pToken);
		messenger.Dispatch(HL2RP_KEY_HINT_USER_MESSAGE);
	}
}
