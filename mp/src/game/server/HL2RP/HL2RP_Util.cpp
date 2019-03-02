#include <cbase.h>
#include "HL2RP_Util.h"
#include "HL2RP_Player.h"
#include <DAL/IDAO.h>
#include <inetchannel.h>
#include <utlbuffer.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// Engine limits
#define NETMSG_TYPE_BITS 6
#define NET_MAX_MESSAGE 4096
#define NET_SETCONVAR 5
#define SVC_MENU 29

int UTIL_EnsureAddition(int original, int amount)
{
	if (amount > 0)
	{
		return (original < INT_MAX - amount) ? original + amount : INT_MAX;
	}
	else
	{
		return original;
	}
};

bool UTIL_IsSavedEntity(CBaseEntity* pEntity, int databaseId)
{
	return (pEntity->m_iHammerID > 0 || databaseId != IDAO_INVALID_DATABASE_ID);
}

void UTIL_HUDMessage(CHL2RP_Player* pPlayer, int channel, float x, float y, float duration,
	const Color& color, const char* pMessage)
{
	hudtextparms_t textParms;
	textParms.channel = channel;
	textParms.x = x;
	textParms.y = y;
	textParms.r1 = color.r();
	textParms.g1 = color.g();
	textParms.b1 = color.b();
	textParms.effect = 0;
	textParms.holdTime = duration;
	textParms.fadeinTime = 0.0f;
	textParms.fadeoutTime = 0.0f;

	// Small heuristic to reduce flickering for cyclic channels
	float elapsedTime = gpGlobals->curtime - pPlayer->m_flNextHUDChannelTime[channel];

	// Check within valid range. This function may have been called before allowed time.
	if (elapsedTime > 0 && elapsedTime < 0.1)
	{
		textParms.holdTime += elapsedTime;
	}

	UTIL_HudMessage(pPlayer, textParms, pMessage);
	pPlayer->m_flNextHUDChannelTime[channel] = gpGlobals->curtime + duration;
}

inline INetChannel* GetPlayerNetChannel(CBasePlayer* pPlayer)
{
	return static_cast<INetChannel*>(engine->GetPlayerNetInfo(pPlayer->entindex()));
}

bool UTIL_SendDialog(CHL2RP_Player* pPlayer, KeyValues* pDialog, DIALOG_TYPE dialogType)
{
	if (pPlayer != NULL)
	{
		INetChannel* pNetChannel = GetPlayerNetChannel(pPlayer);

		if (pNetChannel != NULL)
		{
			byte data[NET_MAX_MESSAGE];
			CUtlBuffer buffer;
			pDialog->WriteAsBinary(buffer);
			bf_write msg(data, sizeof(data));
			msg.WriteUBitLong(SVC_MENU, NETMSG_TYPE_BITS);
			msg.WriteShort(dialogType);
			msg.WriteWord(buffer.TellPut());
			msg.WriteBytes(buffer.Base(), buffer.TellPut());
			return pNetChannel->SendData(msg);
		}
	}

	return false;
}

bool UTIL_SendMsgDialog(CHL2RP_Player* pPlayer, const char* pTitle, const Color& color)
{
	KeyValues* pDialog = new KeyValues("DIALOG_MSG"); // Any name is valid
	pDialog->SetString("title", pTitle);
	pDialog->SetColor("color", color);
	pDialog->SetInt("level", --pPlayer->m_iLastDialogLevel);
	bool retVal = UTIL_SendDialog(pPlayer, pDialog, DIALOG_MSG);
	pDialog->deleteThis();
	return retVal;
}

bool UTIL_SendTextDialog(CHL2RP_Player* pPlayer, const char* pMsg, const char* pTitle,
	const Color& color, int holdTime)
{
	KeyValues* pDialog = new KeyValues("DIALOG_TEXT"); // Any name is valid
	pDialog->SetString("title", pTitle);
	pDialog->SetString("msg", pMsg);
	pDialog->SetColor("color", color);
	pDialog->SetInt("level", --pPlayer->m_iLastDialogLevel);
	pDialog->SetInt("time", holdTime);
	bool retVal = UTIL_SendDialog(pPlayer, pDialog, DIALOG_TEXT);
	pDialog->deleteThis();
	return retVal;
}

bool UTIL_SendAskConnectDialog(CHL2RP_Player* pPlayer, const char* pIPString, float holdTime)
{
	KeyValues* pDialog = new KeyValues("DIALOG_ASKCONNECT"); // Any name is valid
	pDialog->SetString("title", pIPString);
	pDialog->SetFloat("time", holdTime);
	bool retVal = UTIL_SendDialog(pPlayer, pDialog, DIALOG_ASKCONNECT);
	pDialog->deleteThis();
	return retVal;
}

bool UTIL_SendConVarValue(CBasePlayer* pPlayer, const char* pCVarName, const char* pValueString)
{
	if (pPlayer == NULL)
	{
		return false;
	}

	INetChannel* pNetChannel = GetPlayerNetChannel(pPlayer);

	if (pNetChannel == NULL)
	{
		engine->SetFakeClientConVarValue(pPlayer->edict(), pCVarName, pValueString);
		return true;
	}
	else if (pNetChannel->IsLoopback() || pNetChannel->IsNull())
	{
		// This scope should mean we are on a listenserver, or not even in one
		ConVarRef cvarRef(pCVarName);

		if (cvarRef.IsValid())
		{
			cvarRef.SetValue(pValueString);
			return true;
		}
	}
	else
	{
		byte data[256]; // Should not need more size
		bf_write msg(data, sizeof(data));
		msg.WriteUBitLong(NET_SETCONVAR, NETMSG_TYPE_BITS);
		msg.WriteByte(1);
		msg.WriteString(pCVarName);
		msg.WriteString(pValueString);
		return pNetChannel->SendData(msg);
	}

	return false;
}
