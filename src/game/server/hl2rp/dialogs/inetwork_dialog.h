#ifndef INETWORK_DIALOG_H
#define INETWORK_DIALOG_H
#pragma once

#include <hl2rp_util.h>
#include <UtlSortVector.h>

#define NETWORK_DIALOG_MSG_SIZE 256

#define NETWORK_MENU_PAGE_MAX_ITEMS    8
#define NETWORK_MENU_ITEM_DISPLAY_SIZE 64

#define NETWORK_MENU_ACTION_FROM_ITEM -1

abstract_class INetworkDialog
{
	bool mIsFirstDisplay = true;

public:
	virtual ~INetworkDialog() = default;

	virtual void Send() = 0;
	virtual void Think();
	virtual void HandleCommand(const CCommand&) = 0;

	SUtlField mMessageArg;

protected:
	INetworkDialog(CHL2Roleplayer*, const char* pTitleToken, const char* pMessage, bool isAdminOnly, int action);

	virtual void HandleChildNotice(int action, const SUtlField& info) {}

	void InitSendData(KeyValues*);
	void RewindAndNoticeParent(int action, const SUtlField& info);

	CHL2Roleplayer* mpPlayer;
	const char* mpTitleToken;
	char mMessage[NETWORK_DIALOG_MSG_SIZE];
	bool mIsAdminOnly; // When enabled, dialog automatically kicks ex-admins
	int mAction; // Action to notify parent on command handle
};

class CNetworkEntryBox : public INetworkDialog
{
	void Send() OVERRIDE;
	void HandleCommand(const CCommand&) OVERRIDE;

public:
	CNetworkEntryBox(CHL2Roleplayer*, const char* pTitleToken, const char* pMessage = "",
		int action = 0, bool isAdminOnly = false);
};

class CNetworkMenu : public INetworkDialog
{
public:
	CNetworkMenu(CHL2Roleplayer*, const char* pTitleToken, const char* pMessage = "",
		bool isAdminOnly = false, int action = NETWORK_MENU_ACTION_FROM_ITEM, bool rewindIfEmpty = true);

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
	bool mRewindIfEmpty;
};

class CPlayerListMenu : public CNetworkMenu
{
	void UpdateItems() OVERRIDE;
	void OnPreSendDialog(int, KeyValues*) OVERRIDE;

	bool mShowMissingPlayers;

public:
	CPlayerListMenu(CHL2Roleplayer*, const char* pTitleToken, const char* pMessage,
		int action, bool showMissingPlayers = false, bool isAdminOnly = false);

	CAutoLessFuncAdapter<CUtlRBTree<uint64>> mSteamIdNumbers;
};

#endif // !INETWORK_DIALOG_H
