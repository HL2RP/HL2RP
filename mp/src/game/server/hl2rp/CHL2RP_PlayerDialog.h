#ifndef HL2RP_PLAYERDIALOG_H
#define HL2RP_PLAYERDIALOG_H

#include "CHL2RP_Zone.h"
#include "CHL2RP_Phrases.h"

const int MAX_MENU_ITEMS = 64;
const int MENU_ITEM_LENGTH = 64;
const int MENU_TITLE_LENGTH = 64;
const int MENU_MSG_LENGTH = 512;

class CPropRationDispenser;

abstract_class CHL2RP_PlayerDialog
{
public:
	virtual void Display() {}
	virtual ~CHL2RP_PlayerDialog() = 0;

protected:
	CHL2RP_PlayerDialog(CHL2RP_Player *pPlayer, const char *pszTitle, const char *pszMsg);
	void SetCommonKVKeys(KeyValues *dialogKv) const;

	char m_szTitle[MENU_TITLE_LENGTH];
	char m_szMsg[MENU_MSG_LENGTH];
	CHL2RP_Player &m_Player;
};

abstract_class CHL2RP_PlayerEntryBox : public CHL2RP_PlayerDialog
{
protected:
	CHL2RP_PlayerEntryBox(CHL2RP_Player *pPlayer, const char *pszTitle = "", const char *pszMsg = "");

public:
	virtual void CallHandler(const char *pszText) = 0;
	void Display() OVERRIDE;
};

abstract_class CHL2RP_PlayerMenu : public CHL2RP_PlayerDialog
{
public:
	virtual void CallHandler(int iIndex) = 0;
	void ChangePage(int pageStartIndex);
	void Display() OVERRIDE;
	virtual void DisplayParent() const {}

	int m_iSecretToken, m_iNumItems;

	// Global index that corresponds to the first item of current page
	// Avoids being hardly calculated checking for navigation items
	int m_iCurrentPageStartIndex;

	bool m_bHasParent;

protected:
	CHL2RP_PlayerMenu(CHL2RP_Player *player, bool hasParent = false, const char *title = "", const char *msg = "");

	// Returns insert index if successful (namely old numItems) or -1 on failure
	template<class... Param> int AddItem(int action, const char *display, Param&&... params)
	{
		CPhraseParam paramsBuff[] = { params... };
		return AddItemAtIndex(m_iNumItems, action, display, true, paramsBuff, ARRAYSIZE(paramsBuff));
	}

	int AddItem(int action, const char *display, bool translateDisplay = false);
	// TODO: add variadic templatized AddItemAtIndex, when needed
	int AddItemAtIndex(int index, int action, const char *display,
		bool translateDisplay = false, CPhraseParam *paramsBuff = NULL, int paramCount = 0, ...);
	void TrySendAdminMenu() const;

	char m_szItemDisplay[MAX_MENU_ITEMS][MENU_ITEM_LENGTH];
	int m_iItemAction[MAX_MENU_ITEMS];
};

class CHL2RP_PlayerMainMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerMainMenu(CHL2RP_Player *player);

private:
	enum ItemAction
	{
#ifdef HL2DM_RP
		SPECIAL_WEAPONS,
#endif
		ADMINISTRATION
	};

	void CallHandler(int iIndex) OVERRIDE;
};

#ifdef HL2DM_RP
class CHL2RP_PlayerSpecialWeaponsMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerSpecialWeaponsMenu(CHL2RP_Player *player);

private:
	enum ItemAction
	{
		WEAPON_CITIZENPACKAGE,
		WEAPON_CITIZENSUITCASE,
		WEAPON_MOLOTOV
	};

	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;
};
#endif

class CHL2RP_PlayerAdminMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerAdminMenu(CHL2RP_Player *player);

private:
	enum ItemAction
	{
		TRANSLATE_PHRASE,
		ADD_ZONE,
		MANAGE_DISPENSERS,
#ifdef HL2DM_RP
		MANAGE_COMPAT_ANIM_MODELS
#endif
	};

	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;
};

class CHL2RP_PlayerPhraseLanguageMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerPhraseLanguageMenu(CHL2RP_Player *player);

private:
	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;

	const char *m_pszItemLanguageShortName[MAX_MENU_ITEMS];
};

class CHL2RP_PlayerPhraseHeaderMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerPhraseHeaderMenu(CHL2RP_Player *player, const char *languageShortName);

private:
	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;

	const char *m_pszLanguageShortName, *m_pszItemHeader[MAX_MENU_ITEMS];
};

class CHL2RP_PlayerPhraseTranslationEntryBox : public CHL2RP_PlayerEntryBox
{
public:
	CHL2RP_PlayerPhraseTranslationEntryBox(CHL2RP_Player *player, const char *languageShortName, const char *header);

private:
	void CallHandler(const char *pszText);

	const char *m_pszLanguageShortName, *m_pszHeader;
};

class CHL2RP_PlayerPhraseTranslationSaveMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerPhraseTranslationSaveMenu(CHL2RP_Player *player, const char *languageShortName, const char *header, const char *translation);

private:
	enum ItemAction
	{
		SAVE,
		DISCARD
	};

	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;

	const char *m_pszLanguageShortName, *m_pszHeader;
	char m_szTranslation[MAX_PHRASE_TRANSLATION_LENGTH];
};

class CHL2RP_PlayerZoneMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerZoneMenu(CHL2RP_Player *player);

	enum HeightEditState
	{
		STOPPED,
		RAISING,
		LOWERING
	};

	CHL2RP_Zone m_CurrentZone;
	HeightEditState m_HeightEditState;

private:
	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;

	enum ItemAction
	{
		SAVE,
		// Quickly make point actions equal to point indexes,
		// respecting the size definition:
		SET_INITIAL_POINT,
		SET_LAST_POINT = SET_INITIAL_POINT + MAX_ZONE_POINTS - 1,
		ADD_INITIAL_POINT,
		ADD_LAST_POINT = ADD_INITIAL_POINT + MAX_ZONE_POINTS - 1
	};
};

class CHL2RP_PlayerDispenserMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerDispenserMenu(CHL2RP_Player *player);

	CPropRationDispenser *m_pCurrentDispenser;

private:
	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;

	enum ItemAction
	{
		CREATE_DISPENSER,
		MOVE_CURRENT_DISPENSER,
		REMOVE_CURRENT_DISPENSER,
		REMOVE_LOOKING_DISPENSER,
		SAVE_CURRENT_DISPENSER,
		SAVE_LOOKING_DISPENSER
	};
};

#ifdef HL2DM_RP
class CHL2RP_PlayerManageCompatModelsMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerManageCompatModelsMenu(CHL2RP_Player *player);

private:
	enum ItemAction
	{
		ADD_MODEL,
		REMOVE_MODEL
	};

	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;
};

class CHL2RP_PlayerAddCompatModelTypeMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerAddCompatModelTypeMenu(CHL2RP_Player *player);

private:
	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;
};

class CHL2RP_PlayerAddCompatModelDisplayEntryBox : public CHL2RP_PlayerEntryBox
{
public:
	CHL2RP_PlayerAddCompatModelDisplayEntryBox(CHL2RP_Player *player, int type);

private:
	void CallHandler(const char *pszText);

	int m_iType;
};

class CHL2RP_PlayerAddCompatModelPathEntryBox : public CHL2RP_PlayerEntryBox
{
public:
	CHL2RP_PlayerAddCompatModelPathEntryBox(CHL2RP_Player *player, const char *id, int type);

private:
	void CallHandler(const char *pszText);

	char m_szId[MENU_ITEM_LENGTH];
	int m_iType;
};

class CHL2RP_PlayerSaveCompatModelMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerSaveCompatModelMenu(CHL2RP_Player *player, const char *id, const char *path, int type);

private:
	enum ItemAction
	{
		SAVE,
		DISCARD
	};

	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;

	char m_szId[MENU_ITEM_LENGTH], m_szPath[MAX_PATH];
	int m_iType;
};

class CHL2RP_PlayerRemoveCompatModelMenu : public CHL2RP_PlayerMenu
{
public:
	CHL2RP_PlayerRemoveCompatModelMenu(CHL2RP_Player *player);

private:
	void CallHandler(int iIndex) OVERRIDE;
	void DisplayParent() const OVERRIDE;
};
#endif

#endif // HL2RP_PLAYERDIALOG_H
