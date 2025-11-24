#ifndef INETWORK_DIALOG_H
#define INETWORK_DIALOG_H
#pragma once

#include <hl2rp_util_shared.h>
#include <UtlSortVector.h>

#define NETWORK_DIALOG_TITLE_SIZE 64
#define NETWORK_DIALOG_MSG_SIZE   256

#define NETWORK_MENU_PAGE_MAX_ITEMS    8
#define NETWORK_MENU_ITEM_DISPLAY_SIZE 64

#define NETWORK_MENU_ACTION_FROM_ITEM -1

#define NETWORK_DIALOG_REWIND_SOUND "Buttons.snd9"
#define NETWORK_MENU_ITEM_SOUND     "Buttons.snd37"

abstract_class INetworkDialog
{
	bool mAllowParentThink; // Useful for shared logic within a hierarchy

public:
	INetworkDialog(CHL2Roleplayer*, const char* pTitle, const char* pMessage,
		int action = 0, bool isAdminOnly = false, bool allowParentThink = false);

	virtual ~INetworkDialog() = default;

	virtual void Send() = 0;
	virtual void Think();
	virtual void HandleCommand(const CCommand&) {}

	int mStackIndex = -1;
	SUtlField mMessageArg;

protected:
	virtual void HandleChildNotice(int action, const SUtlField& info) {}

	KeyValues* InitSendData(KeyValues*, bool allowESCHint);
	void NoticeParent(int action, const SUtlField& info, bool rewind = true);

	CHL2Roleplayer* mpPlayer;
	char mTitle[NETWORK_DIALOG_TITLE_SIZE], mMessage[NETWORK_DIALOG_MSG_SIZE];
	bool mIsAdminOnly; // When enabled, dialog automatically kicks ex-admins
	int mAction; // Action to notify parent on command handle
};

// Simple wrapper for DIALOG_TEXT and DIALOG_MSG types (selected based on 'msg' length)
class CNetworkMsgDialog : public INetworkDialog
{
	void Send() OVERRIDE;

public:
	using INetworkDialog::INetworkDialog;
};

class CNetworkEntryBox : public INetworkDialog
{
	void Send() OVERRIDE;
	void HandleCommand(const CCommand&) OVERRIDE;

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

	void Send() OVERRIDE;

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

	int mCurPageItemIndex = 0;
	bool mShowItemNumbers = false;
	CAutoDeleteAdapter<CUtlSortVector<CItem*, CItem::CLess>> mItems; // Sorted items by action

private:
	class CPageInfo
	{
	public:
		CPageInfo(CNetworkMenu*, int pageItemIndex);

		int mMaxPageItems = NETWORK_MENU_PAGE_MAX_ITEMS;
	};

	void HandleCommand(const CCommand&) OVERRIDE;

	virtual void UpdateItems() {} // NOTE: Never delete the menu here, for safety (Send calls this)
	virtual void OnPreSendDialog(int pageEndIndex, KeyValues* pSendData) {} // Called after current page index is shifted
	virtual void SelectItem(CItem*);

	void AddItemSendData(KeyValues*, int index, const char* pDisplay);
	void PlayItemSound();

	int mSecretToken;
	bool mRewindIfEmpty; // Needed e.g. to keep menu when only create/update items are supported, but are hidden while an entity is saving into DB
	bool mIsFirstDisplay = true;
};

class CPlayerListMenu : public CNetworkMenu
{
	void UpdateItems() OVERRIDE;
	void OnPreSendDialog(int, KeyValues*) OVERRIDE;

	bool mShowMissingPlayers;

public:
	CPlayerListMenu(CHL2Roleplayer*, const char* pTitle, const char* pMessage,
		int action, bool isAdminOnly = false, bool allowParentThink = false, bool showMissingPlayers = false);

	CAutoLessFuncAdapter<CUtlRBTree<uint64>> mSteamIdNumbers;
};

#endif // !INETWORK_DIALOG_H
