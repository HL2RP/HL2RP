#ifndef PLAYER_DIALOG_H
#define PLAYER_DIALOG_H

#include "AutoLocalizer.h"
#include "UtlSortVector.h"

#define PLAYERDIALOG_MENU_ITEM_DISPLAY_SIZE	64
#define PLAYERDIALOG_ENTRYBOX_INPUT_SIZE	256

class CHL2RP_Player;
class CMenu;

class CPlayerDialog
{
public:
	virtual ~CPlayerDialog() { }

	virtual void Display(CHL2RP_Player* pPlayer) = 0;
	virtual void HandleTextInput(CHL2RP_Player* pPlayer, char* pText) { }
	virtual void HandleItemSelectionInput(CHL2RP_Player* pPlayer, const char* pSecretTokenText,
		const char* pIndexText) { }
	virtual void RewindDialog(CHL2RP_Player* pPlayer) { }

protected:
	CPlayerDialog(CHL2RP_Player* pPlayer, const char* pTitle, const char* pDescription);

	void SetTitle(const char* pTitle);
	void SetDescription(const char* pDescription);

	// A structure which contains the keys to be networked to player
	KeyValuesAD m_NetKeyValues;
};

class CEntryBox : public CPlayerDialog
{
	void HandleTextInput(CHL2RP_Player* pPlayer, char* pText) OVERRIDE;
	virtual void HandleFilledTextInput(CHL2RP_Player* pPlayer, const char* pText) = 0;

public:
	void Display(CHL2RP_Player* pPlayer) OVERRIDE;

protected:
	CEntryBox(CHL2RP_Player* pPlayer, const char* pTitle = NULL, const char* pDescription = NULL);
};

class CMenuItem
{
public:
	CMenuItem() { }
	CMenuItem(const char* pDisplay);
	CMenuItem(CHL2RP_Player* pPlayer, const char* pDisplayToken); // Localized

	bool operator<(CMenuItem& item);

	virtual int GetSortPriority();
	virtual bool IsNavigator();
	virtual void PlayClickSound(CHL2RP_Player* pPlayer);
	virtual void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) { }

	// Used to delay the display assignment for saving constructors
	CMenuItem* SetDisplay(const char* pDisplay);
	CMenuItem* SetLocalizedDisplay(CHL2RP_Player* pPlayer, const char* pDisplay);

	char m_Display[PLAYERDIALOG_MENU_ITEM_DISPLAY_SIZE];

protected:
	enum ESortPriority
	{
		Data,
		PageRewind,
		PageReturn,
		PageAdvance,
	};
};

// A menu item that knows the menu it belongs to
template<class T = CMenu>
class CMenuAwareItem : public CMenuItem
{
public:
	// Used to delay the containing menu linkage for saving constructors, but you must remember to call it
	CMenuItem* Link(T* pMenu)
	{
		m_pMenu = pMenu;
		return this;
	}

protected:
	CMenuAwareItem() { }

	CMenuAwareItem(T* pMenu) : m_pMenu(pMenu)
	{

	}

	T* m_pMenu;
};

// The 'Next' menu item
class CPageAdvanceMenuItem : public CMenuItem
{
	int GetSortPriority() OVERRIDE;
	bool IsNavigator() OVERRIDE;
	void PlayClickSound(CHL2RP_Player* pPlayer) OVERRIDE;
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

// The 'Previous' menu item
class CPageReturnMenuItem : public CMenuItem
{
	int GetSortPriority() OVERRIDE;
	bool IsNavigator() OVERRIDE;
	void PlayClickSound(CHL2RP_Player* pPlayer) OVERRIDE;
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

// The 'Up' menu item
class CPageRewindMenuItem : public CMenuItem
{
	int GetSortPriority() OVERRIDE;
	bool IsNavigator() OVERRIDE;
	void PlayClickSound(CHL2RP_Player* pPlayer) OVERRIDE;
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

// The menu item comparison sorting class, needed for organized displaying
class CMenuItemLess
{
public:
	bool Less(CMenuItem* const& pItem1, CMenuItem* const& pItem2, void* pCtx);
};

class CMenuPage : public CUtlSortVector<CMenuItem*, CMenuItemLess>
{
public:
	~CMenuPage();
};

class CMenu : public CPlayerDialog
{
	void HandleItemSelectionInput(CHL2RP_Player* pPlayer, const char* pSecretTokenText,
		const char* pIndexText) OVERRIDE;

	int m_iCurrentPageIndex, m_iSecretToken;

public:
	void Display(CHL2RP_Player* pPlayer) OVERRIDE;

	void AdvancePage(CHL2RP_Player* pPlayer);
	void ReturnPage(CHL2RP_Player* pPlayer);

protected:
	CMenu(CHL2RP_Player* pPlayer, const char* pTitle = NULL, const char* pDescription = NULL);

	virtual int CreatePage(CHL2RP_Player* pPlayer);

	// Adds an item while creating auxiliar needed new pages and navigation items aswell
	void AddItem(CHL2RP_Player* pPlayer, CMenuItem* pItem);

	CUtlVectorAutoPurge<CMenuPage*> m_PagesList;
};

// A menu which should have an 'Back' button on all pages to re-create a dialog when clicked
class CDialogRewindableMenu : public CMenu
{
	int CreatePage(CHL2RP_Player* pPlayer) OVERRIDE;

protected:
	CDialogRewindableMenu(CHL2RP_Player* pPlayer, const char* pTitle = NULL,
		const char* pDescription = NULL);
};

#endif // PLAYER_DIALOG_H
