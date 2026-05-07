// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "inetwork_dialog.h"
#include <hl2_roleplayer.h>
#include <viewport_panel_names.h>

#define MOTDPANEL_TYPE_URL 2 // Treat msg as an URL link

#define MOTD_DIALOG_BASE64_DEC_SIZE 512 // Max. decoding in/out size for incoming MOTD commands
#define MOTD_DIALOG_BASE64_ENC_SIZE 2048 // Max. encoding out size for JSON responses (from motd_dialogquery requests)

SCOPED_ENUM(EMOTDDialogAction, // Command types
	Query,
	ForwardCommand,
	Rewind
)

// NOTE: Using re-declarations to prevent errors from GCSDK headers due to unlinked include dirs in our VPC
namespace GCSDK
{
	bool Base64Encode(const uint8* pSrc, uint32 len, char* pDest, uint32* pMaxLen, const char* pLineBreak = NULL);
	bool Base64Decode(const char* pSrc, uint32 maxLen, uint8* pDest, uint32* pResultLen, bool ignoreInvalidChars = false);
}

using namespace GCSDK;

extern ConVar sv_motd_unload_on_dismissal;

static ConVar sMOTDForwardURLCVar("sv_dialog_motdforward_url", "",
	FCVAR_ARCHIVE, "URL of MOTDDialogBuilder module to forward network dialogs");

void INetworkDialog::ForwardToMOTD()
{
	if (*sMOTDForwardURLCVar.GetString() != '\0')
	{
		KeyValuesAD data;
		data->SetInt("type", MOTDPANEL_TYPE_URL);
		data->SetString("msg", UTIL_VarArgs("%s?ip=%s&port=%i&userid=%i&secret=%i", sMOTDForwardURLCVar.GetString(),
			ConVarRef("ip").GetString(), ConVarRef("hostport").GetInt(), mpPlayer->GetUserID(), mpPlayer->mDialogSecret));
		data->SetBool("unload", sv_motd_unload_on_dismissal.GetBool());
		mpPlayer->ShowViewPortPanel(PANEL_INFO, true, data);
	}
}

static void AppendDialogKeyToJSON(KeyValues* pKey, CFmtStr1024& dest, const char*& pSeparator)
{
	char tmp[MOTD_DIALOG_BASE64_ENC_SIZE];
	uint32 resultLen = sizeof(tmp);

	if (Base64Encode((uint8*)pKey->GetString(), Q_strlen(pKey->GetString()), tmp, &resultLen))
	{
		dest.AppendFormat("%s \"%s\": \"%s\"", pSeparator, pKey->GetName(), tmp);
		pSeparator = ",";
	}
}

static void HandleMOTDDialogCmd(EMOTDDialogAction action, const CCommand& args)
{
	CHL2Roleplayer* pPlayer;

	if (UTIL_CheckCmdArgCount(args, 2) && UTIL_FindCmdTarget(args, pPlayer)
		&& !pPlayer->mDialogStack.IsEmpty() && (IsDebug() || pPlayer->mDialogSecret == Q_atoi(args.Arg(2))))
	{
		CFmtStr1024 result("{");
		pPlayer->mIsInMOTDDialogCmd = true;
		INetworkDialog* pDialog = pPlayer->mDialogStack.Tail();

		switch (action)
		{
		case EMOTDDialogAction::Rewind:
		{
			pPlayer->RewindDialogStack(pDialog->mStackIndex);
			break;
		}
		case EMOTDDialogAction::ForwardCommand:
		{
			uint32 len;
			char data[MOTD_DIALOG_BASE64_DEC_SIZE];

			if (!Base64Decode(args.Arg(3), sizeof(data), (uint8*)data, &len))
			{
				pPlayer->mIsInMOTDDialogCmd = false;
				return; // Let MOTDDialogBuilder raise an error (no replied JSON)
			}

			data[len] = '\0';
			pDialog->HandleCommandText(data, true);
		}
		}

		// Re-validate active player dialog, as it may have been deleted
		if (!pPlayer->mDialogStack.IsEmpty())
		{
			pDialog = pPlayer->mDialogStack.Tail();

			DIALOG_TYPE type;
			KeyValuesAD data;
			pDialog->InitSendData(type, data, true, false);
			data->SetInt("type", type);
			KeyValues* pItemsKV = data->GetFirstTrueSubKey();
			const char* pSeparator = "";

			FOR_EACH_VALUE(data, pKey)
			{
				AppendDialogKeyToJSON(pKey, result, pSeparator);
			}

			// Add menu items (if any)
			if (pItemsKV != NULL)
			{
				result.AppendFormat("%s \"items\": {", pSeparator);
				pSeparator = "";

				FOR_EACH_VALUE(pItemsKV, pItemKV)
				{
					AppendDialogKeyToJSON(pItemKV, result, pSeparator);
				}

				result += " }";
			}
		}

		result += " }";
		UTIL_ReplyToCommand(HUD_PRINTCONSOLE, result);
		pPlayer->mIsInMOTDDialogCmd = false;
	}
}

CON_COMMAND_F(motd_dialogquery, "<userid> <secret>", FCVAR_HIDDEN)
{
	HandleMOTDDialogCmd(EMOTDDialogAction::Query, args);
}

CON_COMMAND_F(motd_dialogcmd, "<userid> <secret> [base64 data]", FCVAR_HIDDEN)
{
	HandleMOTDDialogCmd(EMOTDDialogAction::ForwardCommand, args);
}

CON_COMMAND_F(motd_rewinddialog, "<userid> <secret>", FCVAR_HIDDEN)
{
	HandleMOTDDialogCmd(EMOTDDialogAction::Rewind, args);
}
