#include <cbase.h>
#include "inetwork_dialog.h"
#include <hl2_roleplayer.h>

#define NETWORK_DIALOG_MAX_TIME 200

#define NETWORK_MENU_PAGE_PREV_INDEX -1
#define NETWORK_MENU_PAGE_NEXT_INDEX -2
#define NETWORK_MENU_PAGE_BACK_INDEX -3

#define NETWORK_MENU_MSG_LINE_MAX_SIZE 34 // Max. displayable line size before overflow from default panel layout

INetworkDialog::INetworkDialog(CHL2Roleplayer* pPlayer, const char* pTitleToken, const char* pMessage, bool isAdminOnly)
	: mpPlayer(pPlayer), mpTitleToken(pTitleToken), mIsAdminOnly(isAdminOnly)
{
	V_strcpy_safe(mMessage, pMessage);
}

void INetworkDialog::Think()
{
	if (mIsAdminOnly && !mpPlayer->IsAdmin())
	{
		mpPlayer->RewindCurrentDialog();
	}
}

void INetworkDialog::InitSendData(KeyValues* pData)
{
	CLocalizeFmtStr<> localizedMessage(mpPlayer);
	localizedMessage.Localize(mMessage);

	// HACK: Fix overflow from default panel layout by replacing rightmost line spaces with new line characters
	for (char* pEnd = localizedMessage + localizedMessage.mLength,
		*pCurLine = localizedMessage, *pNewLineChar; pCurLine < pEnd; pCurLine = pNewLineChar + 1)
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

	pData->SetString("title", (mIsFirstDisplay && mpPlayer->mDialogStack.Size() < 2) ?
		CLocalizeFmtStr<>(mpPlayer).Format("%t (%t)", mpTitleToken, "#HL2RP_Dialog_Open_Hint")
		: gHL2RPLocalizer.Localize(mpPlayer, mpTitleToken));
	pData->SetString("msg", localizedMessage);
	pData->SetColor("color", COLOR_GREEN);
	pData->SetInt("time", NETWORK_DIALOG_MAX_TIME);
	pData->SetInt("level", --mpPlayer->mLastDialogLevel);
	mIsFirstDisplay = false;
}

void INetworkDialog::RewindAndNoticeParent(int action, const char* pInfo)
{
	int count = mpPlayer->mDialogStack.Size();

	if (count > 1)
	{
		mpPlayer->mDialogStack[count - 2]->HandleChildNotice(action, pInfo);
		mpPlayer->RewindCurrentDialog(); // NOTE: Assumes dialog stack hasn't changed, which would make this wrong
	}
}

CNetworkEntryBox::CNetworkEntryBox(CHL2Roleplayer* pPlayer, const char* pTitleToken, const char* pMessage, int action,
	bool isAdminOnly) : INetworkDialog(pPlayer, pTitleToken, pMessage, isAdminOnly), mAction(action)
{

}

void CNetworkEntryBox::Send()
{
	KeyValuesAD data("");
	InitSendData(data);
	data->SetString("command", "dialogcmd");
	UTIL_SendDialog(mpPlayer, data, DIALOG_ENTRY);
}

void CNetworkEntryBox::HandleCommand(const CCommand& args)
{
	RewindAndNoticeParent(mAction, args.ArgS());
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

bool CNetworkMenu::CItem::CLess::Less(CItem* const& pLeft, CItem* const& pRight, void*)
{
	return (pLeft->mAction <= pRight->mAction);
}

CNetworkMenu::CNetworkMenu(CHL2Roleplayer* pPlayer, const char* pTitleToken, const char* pMessage, bool isAdminOnly)
	: INetworkDialog(pPlayer, pTitleToken, pMessage, isAdminOnly)
{

}

void CNetworkMenu::AddItem(int action, const char* pDisplay, const char* pInfo)
{
	CItem* pItem = new CItem;
	pItem->mAction = action;
	pItem->mInfo = pInfo;
	V_strcpy_safe(pItem->mDisplay, pDisplay);
	*pItem->mDisplay = toupper(*pItem->mDisplay); // Fix casing from e.g. cached KeyValues
	mItems.Insert(pItem);
}

void CNetworkMenu::RemoveItem(int index)
{
	delete mItems[index];
	mItems.Remove(index);

	// Try returning to previous page
	if (mCurPageItemIndex > 0 && mCurPageItemIndex + 2 > mItems.Size())
	{
		mCurPageItemIndex -= CPageInfo(this, mCurPageItemIndex - NETWORK_MENU_PAGE_MAX_ITEMS).mMaxPageItems;
	}
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
	mCurPageItemIndex = 0;
}

void CNetworkMenu::Send()
{
	int oldPageItemIndex = mCurPageItemIndex;
	UpdateItems();

	if (mItems.IsEmpty() || (mIsAdminOnly && !mpPlayer->IsAdmin()))
	{
		return mpPlayer->RewindCurrentDialog(); // Since there aren't items, try moving to parent
	}
	else if (oldPageItemIndex < mItems.Size())
	{
		mCurPageItemIndex = oldPageItemIndex; // Return to previous page (items may have been removed)
	}

	mSecretToken = rand();
	KeyValuesAD data("");
	InitSendData(data);
	CPageInfo info(this, mCurPageItemIndex);
	int end = Min(mCurPageItemIndex + info.mMaxPageItems, mItems.Size());

	for (int i = mCurPageItemIndex; i < end; ++i)
	{
		AddItemSendData(data, i, mItems[i]->mDisplay);
	}

	if (mCurPageItemIndex > 0)
	{
		AddItemSendData(data, NETWORK_MENU_PAGE_PREV_INDEX, "#HL2RP_Menu_Item_Prev");
	}

	if (end < mItems.Size())
	{
		AddItemSendData(data, NETWORK_MENU_PAGE_NEXT_INDEX, "#HL2RP_Menu_Item_Next");
	}

	if (mpPlayer->mDialogStack.Size() > 1)
	{
		AddItemSendData(data, NETWORK_MENU_PAGE_BACK_INDEX, "#HL2RP_Menu_Item_Back");
	}

	return UTIL_SendDialog(mpPlayer, data, DIALOG_MENU);
}

void CNetworkMenu::AddItemSendData(KeyValues* pData, int index, const char* pDisplay)
{
	KeyValues* pItemData = pData->CreateNewKey();
	pItemData->SetString("msg", gHL2RPLocalizer.Localize(mpPlayer, pDisplay));
	pItemData->SetString("command", UTIL_VarArgs("dialogcmd %i %i", index, mSecretToken));
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
			return mpPlayer->RewindCurrentDialog();
		}
		}

		if (index >= 0 && index < mItems.Size())
		{
			PlayItemSound();
			SelectItem(mItems[index]);
		}
	}
}

void CNetworkMenu::PlayItemSound() // NOTE: Call before a Send to safely access player (handler may delete menu)
{
	CSingleUserRecipientFilter filter(mpPlayer);
	mpPlayer->EmitSound(filter, mpPlayer->GetSoundSourceIndex(), NETWORK_MENU_ITEM_SOUND);
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
