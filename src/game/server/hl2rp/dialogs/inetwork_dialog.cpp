#include <cbase.h>
#include "inetwork_dialog.h"
#include <hl2_roleplayer.h>
#include <hl2rp_gamerules.h>
#include <hl2rp_localizer.h>

#define NETWORK_DIALOG_MAX_TIME 200
#define NETWORK_DIALOG_CMD_NAME "dialogcmd"

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

KeyValues* INetworkDialog::InitSendData(KeyValues* pData, bool allowESCHint)
{
	CLocalizeFmtCStr localizedMessage(mpPlayer);
	localizedMessage.Localize(mMessage, mMessageArg.ToString().Get());

	// HACK: Fix overflow from default panel layout by replacing rightmost line spaces with new line characters
	for (char* pEnd = localizedMessage.mDest + localizedMessage.mLength,
		*pCurLine = localizedMessage.mDest, *pNewLineChar; pCurLine < pEnd; pCurLine = pNewLineChar + 1)
	{
		if ((pNewLineChar = strchr(pCurLine, '\n')) == NULL)
		{
			pNewLineChar = pEnd;
		}

		if (pCurLine + NETWORK_MENU_MSG_LINE_MAX_SIZE < pNewLineChar)
		{
			char lineEndChar = pCurLine[NETWORK_MENU_MSG_LINE_MAX_SIZE];
			pCurLine[NETWORK_MENU_MSG_LINE_MAX_SIZE] = '\0';
			char* pLineEndSpace = Q_strrchr(pCurLine, ' ');
			pCurLine[NETWORK_MENU_MSG_LINE_MAX_SIZE] = lineEndChar;

			if (pLineEndSpace != NULL)
			{
				pNewLineChar = pLineEndSpace;
				*pNewLineChar = '\n';
			}
		}
	}

	pData->SetString("title", (allowESCHint && mpPlayer->mDialogStack.Size() < 2) ?
		CLocalizeFmtCStr(mpPlayer).Format("%t (%t)", mTitle, "#HL2RP_Dialog_Open_Hint")
		: gHL2RPLocalizer.Localize(mpPlayer, mTitle));
	pData->SetString("msg", localizedMessage);
	pData->SetColor("color", COLOR_GREEN);
	pData->SetInt("time", NETWORK_DIALOG_MAX_TIME);
	pData->SetInt("level", --mpPlayer->mLastDialogLevel);
	return pData;
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

void CNetworkMsgDialog::Send()
{
	KeyValuesAD data;
	bool showPanel = (*mMessage != '\0');
	UTIL_SendDialog(mpPlayer, InitSendData(data, showPanel), showPanel ? DIALOG_TEXT : DIALOG_MSG);
}

CNetworkEntryBox::CNetworkEntryBox(CHL2Roleplayer* pPlayer, const char* pTitle, const char* pMessage, int action,
	bool isAdminOnly, bool allowParentThink) : INetworkDialog(pPlayer, pTitle, pMessage, action, isAdminOnly, allowParentThink)
{

}

void CNetworkEntryBox::Send()
{
	KeyValuesAD data;
	InitSendData(data, true)->SetString("command", NETWORK_DIALOG_CMD_NAME);
	UTIL_SendDialog(mpPlayer, data, DIALOG_ENTRY);
}

void CNetworkEntryBox::HandleCommand(const CCommand& args)
{
	NoticeParent(mAction, args.ArgS());
}

CNetworkMenu::CPageInfo::CPageInfo(CNetworkMenu* pMenu, int pageItemIndex)
{
	// Check special slots for navigating back, previous and next
	if (pMenu->mpPlayer->mDialogStack.Size() > 1)
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

void CNetworkMenu::Send()
{
	UpdateItems();
	CPageInfo info(this, mCurPageItemIndex);

	if ((mRewindIfEmpty && mItems.IsEmpty()) || (mIsAdminOnly && !mpPlayer->IsAdmin()))
	{
		return mpPlayer->RewindDialogStack(mStackIndex); // Since there aren't items, try moving to parent
	}
	// Shift current page to the last page if overflowed due to item count changes
	else if (mCurPageItemIndex > 0 && mCurPageItemIndex + 2 > mItems.Size())
	{
		mCurPageItemIndex = 0;
		info = { this, 0 };

		for (int nextMaxPageItems = info.mMaxPageItems - 1;
			mCurPageItemIndex + info.mMaxPageItems < mItems.Size(); info.mMaxPageItems = nextMaxPageItems)
		{
			mCurPageItemIndex += info.mMaxPageItems;
		}
	}

	mSecretToken = rand();
	KeyValuesAD data;
	InitSendData(data, mIsFirstDisplay);
	mIsFirstDisplay = false;
	int pageEndIndex = Min(mCurPageItemIndex + info.mMaxPageItems, mItems.Size());

	for (int i = mCurPageItemIndex; i < pageEndIndex; ++i)
	{
		AddItemSendData(data, i, mItems[i]->mDisplay);
	}

	if (mCurPageItemIndex > 0)
	{
		AddItemSendData(data, NETWORK_MENU_PAGE_PREV_INDEX, "#HL2RP_Menu_Item_Prev");
	}

	if (pageEndIndex < mItems.Size())
	{
		AddItemSendData(data, NETWORK_MENU_PAGE_NEXT_INDEX, "#HL2RP_Menu_Item_Next");
	}

	if (mpPlayer->mDialogStack.Size() > 1)
	{
		AddItemSendData(data, NETWORK_MENU_PAGE_BACK_INDEX, "#HL2RP_Menu_Item_Back");
	}

	OnPreSendDialog(pageEndIndex, data);
	return UTIL_SendDialog(mpPlayer, data, DIALOG_MENU);
}

void CNetworkMenu::AddItemSendData(KeyValues* pData, int index, const char* pDisplay)
{
	KeyValues* pItemData = pData->CreateNewKey();
	pItemData->SetString("msg", (mShowItemNumbers && index >= 0) ? CLocalizeFmtCStr(mpPlayer)
		.Format("%s. %t", index - mCurPageItemIndex + 1, pDisplay) : gHL2RPLocalizer.Localize(mpPlayer, pDisplay));
	pItemData->SetString("command", UTIL_VarArgs(NETWORK_DIALOG_CMD_NAME " %i %i", index, mSecretToken));
}

void CNetworkMenu::HandleCommand(const CCommand& args)
{
	if (Q_atoi(args.Arg(2)) == mSecretToken)
	{
		int index = Q_atoi(args.Arg(1));

		switch (index)
		{
		case NETWORK_MENU_PAGE_PREV_INDEX:
		{
			if (mCurPageItemIndex > 0)
			{
				mCurPageItemIndex -= CPageInfo(this, mCurPageItemIndex - NETWORK_MENU_PAGE_MAX_ITEMS).mMaxPageItems;
				PlayItemSound();
				Send();
			}

			return;
		}
		case NETWORK_MENU_PAGE_NEXT_INDEX:
		{
			CPageInfo info(this, mCurPageItemIndex);

			if (mCurPageItemIndex + info.mMaxPageItems < mItems.Size())
			{
				mCurPageItemIndex += info.mMaxPageItems;
				PlayItemSound();
				Send();
			}

			return;
		}
		case NETWORK_MENU_PAGE_BACK_INDEX:
		{
			return mpPlayer->RewindDialogStack(mStackIndex);
		}
		}

		if (index >= 0 && index < mItems.Size())
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

void CPlayerListMenu::OnPreSendDialog(int pageEndIndex, KeyValues* pSendData)
{
	CLocalizeFmtCStr localizedMessage(mpPlayer);
	localizedMessage += pSendData->GetString("msg");

	if (localizedMessage.mLength > 0)
	{
		localizedMessage += "\n\n";
	}

	localizedMessage.Localize("#HL2RP_Menu_PlayerList_Msg_Header");

	for (int i = mCurPageItemIndex; i < pageEndIndex; ++i)
	{
		localizedMessage.Format("\n%s. %s", i - mCurPageItemIndex + 1, mItems[i]->mInfo.ToString().Get());
	}

	pSendData->SetString("msg", localizedMessage);
}

CON_COMMAND_F(dialogcmd, "", FCVAR_HIDDEN | FCVAR_SERVER_CAN_EXECUTE)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_GetCommandClient());

	if (pPlayer != NULL && !pPlayer->mDialogStack.IsEmpty())
	{
		pPlayer->mDialogStack.Tail()->HandleCommand(args);
	}
}

CON_COMMAND_EXTERN(entryboxtext, dialogcmd, "<text> - Inputs entry box text");
