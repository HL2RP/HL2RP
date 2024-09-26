// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2_roleplayer.h"
#include "c_hl2rp_gamerules.h"
#include <prediction.h>
#include <usermessages.h>

static void RecvProxy_ZoneWithin(const CRecvProxyData* pData, void*, void* pOut)
{
	((EHANDLE*)pOut)->Set(HL2RPRules()->mCityZoneByIndex
		.GetElementOrDefault<int, C_BaseEntity*>(pData->m_Value.m_Int));
}

BEGIN_PREDICTION_DATA(C_HL2Roleplayer)
DEFINE_PRED_FIELD(mIsInStickyWalkMode, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE) // Needed to fix prediction conflicts
END_PREDICTION_DATA()

IMPLEMENT_HL2RP_NETWORKCLASS(HL2Roleplayer)
RecvPropInt(RECVINFO(m_iMaxHealth)),
RecvPropInt(RECVINFO(mDatabaseIOFlags)),
RecvPropInt(RECVINFO(mSeconds)),
RecvPropInt(RECVINFO(mCrime)),
RecvPropInt(RECVINFO(mFaction)),
RecvPropInt(RECVINFO(mAccessFlags)),
RecvPropInt(RECVINFO(mMiscFlags)),
RecvPropBool(RECVINFO(mIsInStickyWalkMode)),
RecvPropString(RECVINFO(mJobName)),
RecvPropArray(RecvPropEHandle(RECVINFO(mZonesWithin[0]), RecvProxy_ZoneWithin), mZonesWithin),
END_RECV_TABLE()

// Simulates server-side UserMessages on client, for direct display (no networking)
class CLocalUserMessenger : public CUtlBuffer
{
public:
	// Returns whether the object is valid during prediction, to prevent duplicating the message
	operator bool()
	{
		return prediction->IsFirstTimePredicted();
	}

	void Dispatch(const char* pName)
	{
		bf_read reader(Base(), TellPut());
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
		messenger.PutChar(type);
		messenger.PutString(pText);
		messenger.PutString(pArg);
		messenger.Put("\0\0\0", 3); // Zero out remaining lines required by TextMsg handler (overflow error)
		messenger.Dispatch("TextMsg");
	}
}

void C_HL2Roleplayer::LocalDisplayHUDHint(EPlayerHUDHintType type,
	const char* pToken, const char* pArg1, const char* pArg2)
{
	CLocalUserMessenger messenger;

	if (messenger)
	{
		messenger.PutInt(type);
		messenger.PutString(pToken);
		messenger.PutString(pArg1);
		messenger.PutString(pArg2);
		messenger.Dispatch(HL2RP_KEY_HINT_USER_MESSAGE);
	}
}
