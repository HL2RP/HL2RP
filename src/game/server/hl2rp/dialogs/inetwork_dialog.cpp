// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "inetwork_dialog.h"
#include <hl2_roleplayer.h>
#include <hl2rp_gamerules.h>
#include <hl2rp_localizer.h>

#define NETWORK_DIALOG_MAX_TIME 200
#define NETWORK_DIALOG_CMD_NAME "dialogcmd"

#define NETWORK_ENTRYBOX_CMD_NAME "entryboxtext"

#define NETWORK_MENU_PAGE_PREV_INDEX -1
#define NETWORK_MENU_PAGE_NEXT_INDEX -2
#define NETWORK_MENU_PAGE_BACK_INDEX -3

#define NETWORK_MENU_MSG_LINE_MAX_SIZE 34 // Max. displayable line size before overflow from default panel layout

INetworkDialog::INetworkDialog(CHL2Roleplayer* pPlayer, const char* pTitle, const char* pMessage, int action, bool isAdminOnly,
	bool allowParentThink) : mpPlayer(pPlayer), mAction(action), mIsAdminOnly(isAdminOnly), mAllowParentThink(allowParentThink)
{
	V_strcpy_safe(mTitle, pTitle);
	V_strcpy_safe(mMessage, pMessage);
}

void INetworkDialog::Think()
{
	int index = mStackIndex; // Get stack index for safety in case parent deletes current dialog (below)

	if (mAllowParentThink && index > 0)
	{
		auto& stack = mpPlayer->mDialogStack; // For safety (same as index)
		stack[index - 1]->Think();

		if (!stack.IsValidIndex(index) || stack[index] != this) // Check possible deletion from parent
		{
			return;
		}
	}

	if (mIsAdminOnly && !mpPlayer->IsAdmin())
	{
		mpPlayer->RewindDialogStack(index, "#HL2RP_Dialog_Access_Lost");
	}
}

void INetworkDialog::Send(bool allowMOTDFwd)
{
	DIALOG_TYPE type;
	KeyValuesAD data;
	InitSendData(type, data, false, true);
	UTIL_SendDialog(mpPlayer, type, data);

	if (allowMOTDFwd && !mpPlayer->mIsInMOTDDialogCmd && !mpPlayer->mMiscFlags.IsBitSet(EPlayerMiscFlag::AreMOTDDialogsDisabled))
	{
		ForwardToMOTD();
	}
}

void INetworkDialog::InitSendData(DIALOG_TYPE& type, KeyValues* pData, bool forMOTD, bool allowESCHint)
{
	CLocalizeFmtCStr localizedMessage(mpPlayer);
	localizedMessage.Localize(mMessage, mMessageArg.ToString().Get());

	if (!forMOTD)
	{
		pData->SetColor("color", COLOR_GREEN);
		pData->SetInt("time", NETWORK_DIALOG_MAX_TIME);
		pData->SetInt("level", --mpPlayer->mLastDialogLevel);
	}

	pData->SetString("title", (allowESCHint && mStackIndex < 1) ?
		CLocalizeFmtCStr(mpPlayer).Format("%t (%t)", mTitle, "#HL2RP_Dialog_Open_Hint")
		: gHL2RPLocalizer.Localize(mpPlayer, mTitle));
	pData->SetString("msg", localizedMessage);
}

void INetworkDialog::NoticeParent(int action, const SUtlField& info, bool rewind)
{
	if (mStackIndex > 0)
	{
		mpPlayer->mDialogStack[mStackIndex - 1]->HandleChildNotice(action, info);

		if (rewind)
		{
			return mpPlayer->RewindDialogStack(mStackIndex); // NOTE: Assumes dialog stack hasn't changed, which would make this wrong
		}
	}

	Send();
}

void CNetworkMsgDialog::InitSendData(DIALOG_TYPE& type, KeyValues* pData, bool forMOTD, bool allowESCHint)
{
	bool showPanel = (*mMessage != '\0');
	type = (showPanel ? DIALOG_TEXT : DIALOG_MSG);
	INetworkDialog::InitSendData(type, pData, forMOTD, allowESCHint && showPanel);
}

CNetworkEntryBox::CNetworkEntryBox(CHL2Roleplayer* pPlayer, const char* pTitle, const char* pMessage, int action,
	bool isAdminOnly, bool allowParentThink) : INetworkDialog(pPlayer, pTitle, pMessage, action, isAdminOnly, allowParentThink)
{

}

void CNetworkEntryBox::InitSendData(DIALOG_TYPE& type, KeyValues* pData, bool forMOTD, bool allowESCHint)
{
	type = DIALOG_ENTRY;
	INetworkDialog::InitSendData(type, pData, forMOTD, allowESCHint);
	pData->SetString("command", NETWORK_ENTRYBOX_CMD_NAME);

	// If for MOTD, add 'Back' item too when needed and localized 'Submit' label
	if (forMOTD)
	{
		pData->SetString("submitStr", gHL2RPLocalizer.Localize(mpPlayer, "#GameUI_Submit"));

		if (mStackIndex > 0)
		{
			pData->SetString("back", gHL2RPLocalizer.Localize(mpPlayer, "#HL2RP_Menu_Item_Back"));
		}
	}
}

void CNetworkEntryBox::HandleCommandText(const char* pText, bool)
{
	NoticeParent(mAction, pText);
}

CNetworkMenu::CESCPageInfo::CESCPageInfo(CNetworkMenu* pMenu, int pageItemIndex)
{
	// Check special slots for navigating back, previous and next
	if (pMenu->mStackIndex > 0)
	{
		--mMaxPageItems;
	}

	if (pageItemIndex > 0)
	{
		--mMaxPageItems;
	}

	if (pageItemIndex + mMaxPageItems < pMenu->mItems.Size())
	{
		--mMaxPageItems;
	}
}

CNetworkMenu::CItem::CItem(int action, const SUtlField& info, const char* pDisplay) : mAction(action), mInfo(info)
{
	V_strcpy_safe(mDisplay, pDisplay);
	*mDisplay = toupper(*mDisplay); // Fix casing from e.g. cached KeyValues
}

bool CNetworkMenu::CItem::CLess::Less(CItem* pLeft, CItem* pRight, void*)
{
	return (pLeft->mAction < pRight->mAction);
}

CNetworkMenu::CNetworkMenu(CHL2Roleplayer* pPlayer, const char* pTitle,
	const char* pMessage, int action, bool isAdminOnly, bool allowParentThink, bool rewindIfEmpty)
	: INetworkDialog(pPlayer, pTitle, pMessage, action, isAdminOnly, allowParentThink), mRewindIfEmpty(rewindIfEmpty)
{

}

void CNetworkMenu::AddItem(int action, const char* pDisplay, const SUtlField& info)
{
	mItems.Insert(new CItem(action, info, pDisplay));
}

void CNetworkMenu::RemoveItem(int index)
{
	delete mItems[index];
	mItems.Remove(index);
}

void CNetworkMenu::RemoveItemByAction(int action)
{
	FOR_EACH_VEC(mItems, i)
	{
		if (mItems[i]->mAction == action)
		{
			return RemoveItem(i);
		}
	}
}

void CNetworkMenu::RemoveAllItems()
{
	mItems.PurgeAndDeleteElements();
}

void CNetworkMenu::Send(bool allowMOTDFwd)
{
	UpdateItems();

	if ((mRewindIfEmpty && mItems.IsEmpty()) || (mIsAdminOnly && !mpPlayer->IsAdmin()))
	{
		return mpPlayer->RewindDialogStack(mStackIndex); // Since there aren't items, try moving to parent
	}
	// Shift current page to the last one, if overflowed due to item count changes, or current index points to the last item,
	// as it'd be the only custom item in its page and it fits in the previous one instead ('Next' button is unneeded there)
	else if (mCurESCItemIndex > 0 && mCurESCItemIndex + 2 > mItems.Size())
	{
		CESCPageInfo info(this, 0);

		for (mCurESCItemIndex = 0; mCurESCItemIndex + info.mMaxPageItems < mItems.Size(); info = { this, mCurESCItemIndex })
		{
			mCurESCItemIndex += info.mMaxPageItems;
		}
	}

	// Same as above, for MOTD
	if (mCurMOTDItemIndex >= mItems.Size())
	{
		int end = mItems.Size() - 1;
		mCurMOTDItemIndex = end - end % NETWORK_MENU_PAGE_MAX_ITEMS;
	}

	INetworkDialog::Send(allowMOTDFwd);
	mIsFirstDisplay = false;
}

void CNetworkMenu::InitSendData(DIALOG_TYPE& type, KeyValues* pData, bool forMOTD, bool allowESCHint)
{
	type = DIALOG_MENU;
	int pageStartIndex = mCurMOTDItemIndex, pageEndIndex = NETWORK_MENU_PAGE_MAX_ITEMS; // Default to MOTD (faster)

	if (!forMOTD)
	{
		pageStartIndex = mCurESCItemIndex;
		pageEndIndex = CESCPageInfo(this, pageStartIndex).mMaxPageItems;
	}

	pageEndIndex = Min(pageStartIndex + pageEndIndex, mItems.Size());

	for (int i = pageStartIndex; i < pageEndIndex; ++i)
	{
		AddItemSendData(pData, i, mItems[i]->mDisplay, NULL, forMOTD, NULL);
	}

	if (pageStartIndex > 0)
	{
		AddItemSendData(pData, NETWORK_MENU_PAGE_PREV_INDEX,
			"#HL2RP_Menu_Item_Prev", "#HL2RP_Menu_Symbol_Prev", forMOTD, "previous");
	}

	if (pageEndIndex < mItems.Size())
	{
		AddItemSendData(pData, NETWORK_MENU_PAGE_NEXT_INDEX,
			"#HL2RP_Menu_Item_Next", "#HL2RP_Menu_Symbol_Next", forMOTD, "next");
	}

	if (mStackIndex > 0)
	{
		AddItemSendData(pData, NETWORK_MENU_PAGE_BACK_INDEX,
			"#HL2RP_Menu_Item_Back", "#HL2RP_Menu_Symbol_Back", forMOTD, "back");
	}

	OnPreSendDialog(pageStartIndex, pageEndIndex, pData);
	INetworkDialog::InitSendData(type, pData, forMOTD, allowESCHint && mIsFirstDisplay);
}

void CNetworkMenu::AddItemSendData(KeyValues* pData, int index,
	const char* pDisplay, const char* pNavigationSymbol, bool forMOTD, const char* pMOTDNavigationKey)
{
	const char* pLocalizedDisplay = gHL2RPLocalizer.Localize(mpPlayer, pDisplay);

	if (!forMOTD)
	{
		// Set nested data (game format)
		CLocalizeFmtCStr extendedDisplay(mpPlayer);

		if (pNavigationSymbol != NULL)
		{
			pLocalizedDisplay = extendedDisplay.Format("%t %t", pNavigationSymbol, pLocalizedDisplay);
		}
		else if (mShowItemNumbers)
		{
			pLocalizedDisplay = extendedDisplay.Format("%s. %t", index - mCurESCItemIndex + 1, pLocalizedDisplay);
		}

		KeyValues* pItemData = pData->CreateNewKey();
		pItemData->SetString("msg", pLocalizedDisplay);
		return pItemData->SetString("command", UTIL_VarArgs(NETWORK_DIALOG_CMD_NAME " %i %i", index, mpPlayer->mDialogSecret));
	}
	else if (pMOTDNavigationKey != NULL)
	{
		return pData->SetString(pMOTDNavigationKey, pLocalizedDisplay); // Redirect navigation item to parent level (base key)
	}

	// For MOTD, set data at items group
	KeyValues* pItemsKV = pData->FindKey("items", true);
	pItemsKV->SetString(CNumStr(index), pLocalizedDisplay);
}

void CNetworkMenu::HandleCommandText(const char* pText, bool fromMOTD)
{
	int index;

	if (sscanf(pText, "%i", &index) > 0 && index < mItems.Size())
	{
		switch (index)
		{
		case NETWORK_MENU_PAGE_PREV_INDEX:
		{
			if (fromMOTD)
			{
				if (mCurMOTDItemIndex > 0)
				{
					mCurMOTDItemIndex -= NETWORK_MENU_PAGE_MAX_ITEMS;
					PlayItemSoundAndSend();
				}
			}
			else if (mCurESCItemIndex > 0)
			{
				mCurESCItemIndex -= CESCPageInfo(this, mCurESCItemIndex - NETWORK_MENU_PAGE_MAX_ITEMS).mMaxPageItems;
				PlayItemSoundAndSend();
			}

			return;
		}
		case NETWORK_MENU_PAGE_NEXT_INDEX:
		{
			if (!fromMOTD)
			{
				CESCPageInfo info(this, mCurESCItemIndex);

				if (mCurESCItemIndex + info.mMaxPageItems < mItems.Size())
				{
					mCurESCItemIndex += info.mMaxPageItems;
					PlayItemSoundAndSend();
				}
			}
			else if (mCurMOTDItemIndex + NETWORK_MENU_PAGE_MAX_ITEMS < mItems.Size())
			{
				mCurMOTDItemIndex += NETWORK_MENU_PAGE_MAX_ITEMS;
				PlayItemSoundAndSend();
			}

			return;
		}
		case NETWORK_MENU_PAGE_BACK_INDEX:
		{
			return mpPlayer->RewindDialogStack(mStackIndex);
		}
		}

		if (index >= 0)
		{
			PlayItemSound();
			SelectItem(mItems[index]);
		}
	}
}

void CNetworkMenu::SelectItem(CItem* pItem)
{
	NoticeParent(mAction > NETWORK_MENU_ACTION_FROM_ITEM ? mAction : pItem->mAction,
		pItem->mInfo.mType == SUtlField::EType::Null ? SUtlField(pItem->mDisplay) : pItem->mInfo);
}

void CNetworkMenu::PlayItemSoundAndSend()
{
	PlayItemSound();
	Send();
}

void CNetworkMenu::PlayItemSound() // NOTE: Call before a Send to safely access player (handler may delete menu)
{
	mpPlayer->EmitLocalSound(NETWORK_MENU_ITEM_SOUND);
}

void CNetworkMenu::AddMapLinkItems(const char* pMapAlias, int mapLinkAction, int groupLinkAction)
{
	uint minMapGroupsCount = 1;

	if (pMapAlias != STRING(gpGlobals->mapname))
	{
		AddItem(mapLinkAction, "#HL2RP_Menu_LinkToMap");
		minMapGroupsCount = 2;
	}

	if (HL2RPRules()->mMapGroups.Count() >= minMapGroupsCount)
	{
		AddItem(groupLinkAction, "#HL2RP_Menu_LinkToMapGroup");
	}
}

CPlayerListMenu::CPlayerListMenu(CHL2Roleplayer* pPlayer, const char* pTitle,
	const char* pMessage, int action, bool isAdminOnly, bool allowParentThink, bool showMissingPlayers)
	: CNetworkMenu(pPlayer, pTitle, pMessage, action, isAdminOnly, allowParentThink), mShowMissingPlayers(showMissingPlayers)
{
	mShowItemNumbers = true;
}

void CPlayerListMenu::UpdateItems()
{
	RemoveAllItems();

	if (mShowMissingPlayers)
	{
		FOR_EACH_DICT_FAST(mSteamIdNumbers, i)
		{
			CBasePlayer* pPlayer;
			const char* pPlayerName = "";
			int index = HL2RPRules()->mPlayerNameBySteamIdNum.Find(mSteamIdNumbers[i]);

			if (HL2RPRules()->mPlayerNameBySteamIdNum.IsValidIndex(index))
			{
				pPlayerName = HL2RPRules()->mPlayerNameBySteamIdNum[index];
			}
			else if ((pPlayer = UTIL_PlayerBySteamID(mSteamIdNumbers[i])) != NULL)
			{
				pPlayerName = pPlayer->GetPlayerName();
			}

			AddItem(0, pPlayerName, mSteamIdNumbers[i]);
		}

		return;
	}

	ForEachRoleplayer([&](CBasePlayer* pPlayer)
	{
		uint64 steamIdNumber = pPlayer->GetSteamIDAsUInt64();

		if (mSteamIdNumbers.HasElement(steamIdNumber))
		{
			AddItem(0, pPlayer->GetPlayerName(), steamIdNumber);
		}
	});
}

void CPlayerListMenu::OnPreSendDialog(int pageStartIndex, int pageEndIndex, KeyValues* pSendData)
{
	CLocalizeFmtCStr localizedMessage(mpPlayer);
	localizedMessage += pSendData->GetString("msg");

	if (localizedMessage.mLength > 0)
	{
		localizedMessage += "\n\n";
	}

	localizedMessage.Localize("#HL2RP_Menu_PlayerList_Msg_Header");

	for (int i = pageStartIndex; i < pageEndIndex; ++i)
	{
		localizedMessage.Format("\n%s. %s", i - pageStartIndex + 1, mItems[i]->mInfo.ToString().Get());
	}

	pSendData->SetString("msg", localizedMessage);
}

CConfirmMenu::CConfirmMenu(CHL2Roleplayer* pPlayer, int acceptAction, const char* pWarning, const SUtlField& warningArg)
	: CNetworkMenu(pPlayer, "#GameUI_Confirm", pWarning)
{
	mMessageArg = warningArg;
	AddItem(acceptAction, "#GameUI_Accept");
	AddItem(NETWORK_MENU_ACTION_NONE, "#GameUI_Cancel");
}

static void HandleDialogCommand(const CCommand& args, bool validateSecret)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_GetCommandClient());

	if (pPlayer != NULL && !pPlayer->mDialogStack.IsEmpty())
	{
		INetworkDialog* pDialog = pPlayer->mDialogStack.Tail();

		if (!validateSecret || pPlayer->mDialogSecret == Q_atoi(args.Arg(2)))
		{
			pDialog->HandleCommandText(args.ArgS(), false);
		}
	}
}

CON_COMMAND_F(dialogcmd, "[data] [secret]", FCVAR_HIDDEN | FCVAR_SERVER_CAN_EXECUTE)
{
	HandleDialogCommand(args, true); // NOTE: Require secret to prevent self player binding
}

CON_COMMAND_F(entryboxtext, "[text] - Inputs entry box text", FCVAR_HIDDEN | FCVAR_SERVER_CAN_EXECUTE)
{
	HandleDialogCommand(args, false); // NOTE: Skipping secret validation is acceptable here (no benefit from binding)
}
