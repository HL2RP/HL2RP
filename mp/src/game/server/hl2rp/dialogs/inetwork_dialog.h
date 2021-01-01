#ifndef INETWORK_DIALOG_H
#define INETWORK_DIALOG_H
#pragma once

#include <initializer_list>
#include <UtlSortVector.h>

#define NETWORK_DIALOG_MSG_SIZE 256

#define NETWORK_MENU_PAGE_MAX_ITEMS    8
#define NETWORK_MENU_ITEM_DISPLAY_SIZE 64

class CHL2Roleplayer;

abstract_class INetworkDialog
{
	bool mIsFirstDisplay = true;

public:
	virtual ~INetworkDialog() = default;

	virtual void Send() = 0;
	virtual void Think() {}
	virtual void HandleCommand(const CCommand&) = 0;
	virtual void HandleChildCommand(const CCommand&, int action) {}

protected:
	INetworkDialog(CHL2Roleplayer*, const char* pTitleToken, const char* pMessage);

	void InitSendData(KeyValues*);

	CHL2Roleplayer* mpPlayer;
	const char* mpTitleToken;
	char mMessage[NETWORK_DIALOG_MSG_SIZE];
};

class CNetworkEntryBox : public INetworkDialog
{
	void Send() OVERRIDE;
	void HandleCommand(const CCommand&) OVERRIDE;

	int mAction; // Action to notify parent on command handle

public:
	CNetworkEntryBox(CHL2Roleplayer*, const char* pTitleToken, const char* pMessage = "", int action = 0);
};

class CNetworkMenu : public INetworkDialog
{
protected:
	class CItem
	{
	public:
		int mAction;
		CUtlConstString mInfo;
		char mDisplay[NETWORK_MENU_ITEM_DISPLAY_SIZE];

		class CLess
		{
		public:
			bool Less(CItem* const&, CItem* const&, void*);
		};
	};

	CNetworkMenu(CHL2Roleplayer*, const char* pTitleToken, const char* pMessage = "");

	void Send() OVERRIDE;

	void AddItem(int action, const char* pDisplay, const char* pInfo = "");
	void RemoveItem(int index);
	void RemoveItemByAction(int);
	void RemoveAllItems();

	template<typename... T>
	void RemoveItemsByActions(T... actions)
	{
		for (auto action : { actions... })
		{
			RemoveItemByAction(action);
		}
	}

	CAutoPurgeAdapter<CUtlSortVector<CItem*, CItem::CLess>> mItems; // Sorted items by action

private:
	class CPageInfo
	{
	public:
		CPageInfo(CNetworkMenu*, int pageItemIndex);

		int mMaxPageItems = NETWORK_MENU_PAGE_MAX_ITEMS;
	};

	void HandleCommand(const CCommand&) OVERRIDE;
	virtual void UpdateItems() {} // NOTE: Never delete the menu here, for safety (Send calls this)
	virtual void SelectItem(CItem*) = 0;

	void AddItemSendData(KeyValues*, int index, const char* pDisplay);
	void PlayItemSound();

	int mSecretToken, mCurPageItemIndex = 0;
};

#endif // !INETWORK_DIALOG_H
