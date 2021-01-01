// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2_roleplayer.h"
#include <prediction.h>
#include <usermessages.h>

BEGIN_PREDICTION_DATA(C_HL2Roleplayer)
DEFINE_PRED_FIELD(mIsInStickyWalkMode, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE) // Needed to fix prediction conflicts
END_PREDICTION_DATA()

IMPLEMENT_HL2RP_NETWORKCLASS(HL2Roleplayer)
RecvPropInt(RECVINFO(m_iMaxHealth)),
RecvPropInt(RECVINFO(mSeconds)),
RecvPropInt(RECVINFO(mCrime)),
RecvPropInt(RECVINFO(mFaction)),
RecvPropInt(RECVINFO(mAccessFlags)),
RecvPropInt(RECVINFO(mMiscFlags)),
RecvPropBool(RECVINFO(mIsInStickyWalkMode)),
RecvPropString(RECVINFO(mJobName))
END_NETWORK_TABLE()

// Simulates server-side UserMessages on client, for direct display (no networking)
class CLocalUserMessenger : public old_bf_write_static<PAD_NUMBER(MAX_USER_MSG_DATA, 4)>
{
public:
	// Returns whether the object is valid during prediction, to prevent duplicating the message
	operator bool()
	{
		return prediction->IsFirstTimePredicted();
	}

	void Dispatch(const char* pName)
	{
		bf_read reader(m_StaticData, GetNumBytesWritten());
		clientdll->DispatchUserMessage(usermessages->LookupUserMessage(pName), reader);
	}
};

C_HL2Roleplayer* GetLocalHL2Roleplayer()
{
	return ToHL2Roleplayer(C_HL2MP_Player::GetLocalPlayer());
}

int C_HL2Roleplayer::GetMaxHealth() const
{
	return m_iMaxHealth;
}

const char* C_HL2Roleplayer::GetDisplayName()
{
	return GetPlayerName();
}

void C_HL2Roleplayer::PostThink()
{
	BaseClass::PostThink();

	if (mAddIntersectingEnts)
	{
		mAddIntersectingEnts = false;
		SetNextThink(gpGlobals->curtime + BCC_INTERSECTING_ENTS_ADD_DELAY, BCC_INTERSECTING_ENTS_ADD_CONTEXT);
	}
}

void C_HL2Roleplayer::LocalPrint(int type, const char* pText, const char* pArg)
{
	CLocalUserMessenger messenger;

	if (messenger)
	{
		messenger.WriteByte(type);
		messenger.WriteString(pText);
		messenger.WriteString(pArg);
		messenger.WriteBytes("\0\0\0", 3); // Zero out remaining lines required by TextMsg handler (overflow error)
		messenger.Dispatch("TextMsg");
	}
}

void C_HL2Roleplayer::LocalDisplayHUDHint(EPlayerHUDHintType type, const char* pToken)
{
	CLocalUserMessenger messenger;

	if (messenger)
	{
		messenger.WriteLong(type);
		messenger.WriteString(pToken);
		messenger.Dispatch(HL2RP_KEY_HINT_USER_MESSAGE);
	}
}
