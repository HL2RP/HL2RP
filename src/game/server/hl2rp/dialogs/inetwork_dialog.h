#ifndef INETWORK_DIALOG_H
#define INETWORK_DIALOG_H
#pragma once

#include <hl2rp_util_shared.h>
#include <UtlSortVector.h>

#define NETWORK_DIALOG_TITLE_SIZE 64
#define NETWORK_DIALOG_MSG_SIZE   256

#define NETWORK_MENU_PAGE_MAX_ITEMS    8
#define NETWORK_MENU_ITEM_DISPLAY_SIZE 64

#define NETWORK_MENU_ACTION_NONE      -1
#define NETWORK_MENU_ACTION_FROM_ITEM NETWORK_MENU_ACTION_NONE

#define NETWORK_DIALOG_REWIND_SOUND "Buttons.snd9"
#define NETWORK_MENU_ITEM_SOUND     "Buttons.snd37"

abstract_class INetworkDialog
{
	void ForwardToMOTD(); // Sends dialog URL to the MOTD, pointing to our MOTDDialogBuilder web module

	bool mAllowParentThink; // Useful for shared logic within a hierarchy

public:
	INetworkDialog(CHL2Roleplayer*, const char* pTitle, const char* pMessage,
		int action = 0, bool isAdminOnly = false, bool allowParentThink = false);

	virtual ~INetworkDialog() = default;

	virtual void Think();
	virtual void Send(bool allowMOTDFwd = true);
	virtual void InitSendData(DIALOG_TYPE&, KeyValues*, bool forMOTD, bool allowESCHint) = 0;
	virtual void HandleCommandText(const char*, bool fromMOTD) {}

	int mStackIndex = -1;
	SUtlField mMessageArg;

protected:
	virtual void HandleChildNotice(int action, const SUtlField& info) {}

	void NoticeParent(int action, const SUtlField& info, bool rewind = true);

	CHL2Roleplayer* mpPlayer;
	char mTitle[NETWORK_DIALOG_TITLE_SIZE], mMessage[NETWORK_DIALOG_MSG_SIZE];
	bool mIsAdminOnly; // When enabled, dialog automatically kicks ex-admins
	int mAction; // Action to notify parent on command handle
};

// Simple wrapper for DIALOG_TEXT and DIALOG_MSG types (selected based on 'msg' length)
class CNetworkMsgDialog : public INetworkDialog
{
	void InitSendData(DIALOG_TYPE&, KeyValues*, bool, bool) OVERRIDE;

public:
	using INetworkDialog::INetworkDialog;
};

class CNetworkEntryBox : public INetworkDialog
{
	void InitSendData(DIALOG_TYPE&, KeyValues*, bool, bool) OVERRIDE;
	void HandleCommandText(const char*, bool) OVERRIDE;

public:
	CNetworkEntryBox(CHL2Roleplayer*, const char* pTitle,
		const char* pMessage = "", int action = 0, bool isAdminOnly = false, bool allowParentThink = true);
};

class CNetworkMenu : public INetworkDialog
{
public:
	CNetworkMenu(CHL2Roleplayer*, const char* pTitle = "", const char* pMessage = "", int action = NETWORK_MENU_ACTION_FROM_ITEM,
		bool isAdminOnly = false, bool allowParentThink = false, bool rewindIfEmpty = true);

	void AddItem(int action, const char* pDisplay, const SUtlField& info = {});

protected:
	class CItem
	{
	public:
		CItem(int action, const SUtlField& info, const char* pDisplay);

		class CLess
		{
		public:
			bool Less(CItem*, CItem*, void*);
		};

		int mAction;
		SUtlField mInfo;
		char mDisplay[NETWORK_MENU_ITEM_DISPLAY_SIZE];
	};

	void Send(bool allowMOTDFwd = true) OVERRIDE;

	void RemoveItem(int index);
	void RemoveItemByAction(int);
	void RemoveAllItems();
	void AddMapLinkItems(const char* pMapAlias, int mapLinkAction, int groupLinkAction);

	template<typename... T>
	void RemoveItemsByActions(T... actions)
	{
		for (auto action : { actions... })
		{
			RemoveItemByAction(action);
		}
	}

	bool mShowItemNumbers = false;
	CAutoDeleteAdapter<CUtlSortVector<CItem*, CItem::CLess>> mItems; // Sorted items by action

private:
	class CESCPageInfo
	{
	public:
		CESCPageInfo(CNetworkMenu*, int pageItemIndex);

		int mMaxPageItems = NETWORK_MENU_PAGE_MAX_ITEMS;
	};

	void InitSendData(DIALOG_TYPE&, KeyValues*, bool, bool) OVERRIDE;
	void HandleCommandText(const char*, bool) OVERRIDE;

	virtual void UpdateItems() {} // NOTE: Never delete the menu here, for safety (Send calls this)
	virtual void OnPreSendDialog(int pageStartIndex, int pageEndIndex, KeyValues* pSendData) {} // Called after current page index is shifted
	virtual void SelectItem(CItem*);

	void AddItemSendData(KeyValues*, int index, const char* pDisplay,
		const char* pNavigationSymbol, bool forMOTD, const char* pMOTDNavigationKey);
	void PlayItemSoundAndSend();
	void PlayItemSound();

	bool mRewindIfEmpty; // Needed e.g. to keep menu when only create/update items are supported, but are hidden while an entity is saving into DB
	bool mIsFirstDisplay = true;
	int mCurESCItemIndex = 0, mCurMOTDItemIndex = 0; // Indices to current page's first item, for both dedicated ESC panel and MOTD variants
};

class CPlayerListMenu : public CNetworkMenu
{
	void UpdateItems() OVERRIDE;
	void OnPreSendDialog(int, int, KeyValues*) OVERRIDE;

	bool mShowMissingPlayers;

public:
	CPlayerListMenu(CHL2Roleplayer*, const char* pTitle, const char* pMessage,
		int action, bool isAdminOnly = false, bool allowParentThink = false, bool showMissingPlayers = false);

	CAutoLessFuncAdapter<CUtlRBTree<uint64>> mSteamIdNumbers;
};

// Confirmation menu which passes the specified action to the parent when clicking 'Accept'; NETWORK_MENU_ACTION_NONE on cancel
class CConfirmMenu : public CNetworkMenu
{
public:
	CConfirmMenu(CHL2Roleplayer*, int acceptAction, const char* pWarning, const SUtlField& warningArg = {});
};

#endif // !INETWORK_DIALOG_H
