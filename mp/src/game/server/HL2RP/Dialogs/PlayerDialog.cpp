#include <cbase.h>
#include "PlayerDialog.h"
#include "HL2RPRules.h"
#include "HL2RP_Util.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define PLAYERDIALOG_ENTRYBOX_DEFAULT_TITLE_TEXT	"You have an Entry, press ESC"
#define PLAYERDIALOG_MENU_DEFAULT_TITLE_TEXT	"You have a Menu, press ESC"
#define PLAYERDIALOG_MENU_MAX_PAGE_ITEMS	8

// Player interaction sounds with dialogs (choosen by listening them)
#define PLAYERDIALOG_MENU_DATA_ITEM_SOUNDNAME	"ui/buttonclick.wav"
#define PLAYERDIALOG_MENU_NEXT_ITEM_SOUNDNAME	"buttons/button15.wav"
#define PLAYERDIALOG_MENU_PREVIOUS_ITEM_SOUNDNAME	"buttons/button16.wav"
#define PLAYERDIALOG_REWIND_ITEM_SOUNDNAME	"buttons/button9.wav"
#define PLAYERDIALOG_ENTRYBOX_INPUT_SOUNDNAME	"friends/friend_join.wav"

CPlayerDialog::CPlayerDialog(CHL2RP_Player * pPlayer, const char* pTitle,
	const char* pDescription) : m_NetKeyValues("Dialog")
{
	if (pTitle != NULL)
	{
		SetTitle(pTitle);
	}

	if (pDescription != NULL)
	{
		SetDescription(pDescription);
	}

	m_NetKeyValues->SetInt("level", --pPlayer->m_iLastDialogLevel);
	m_NetKeyValues->SetColor("color", HL2RP_UTIL_DIALOG_DEFAULT_TITLE_COLOR);
	m_NetKeyValues->SetInt("time", HL2RP_UTIL_DIALOG_PANEL_DEFAULT_DISPLAY_INTERVAL);
}

void CPlayerDialog::SetTitle(const char* pTitle)
{
	m_NetKeyValues->SetString("title", pTitle);
}

void CPlayerDialog::SetDescription(const char* pDescription)
{
	m_NetKeyValues->SetString("msg", pDescription);
}

CEntryBox::CEntryBox(CHL2RP_Player* pPlayer, const char* pTitle, const char* pDescription)
	: CPlayerDialog(pPlayer, pTitle, pDescription)
{
	m_NetKeyValues->SetString("command", "entryboxtext");
}

void CEntryBox::Display(CHL2RP_Player* pPlayer)
{
	UTIL_SendDialog(pPlayer, m_NetKeyValues, DIALOG_ENTRY);
}

void CEntryBox::HandleTextInput(CHL2RP_Player* pPlayer, char* pText)
{
	int lastLenIndex = Q_strlen(pText) - 1;

	if (lastLenIndex >= 0)
	{
		if (pText[0] == '"' && pText[lastLenIndex] == '"')
		{
			pText[lastLenIndex] = '\0';
			pText++;
		}

		engine->ClientCommand(pPlayer->edict(), "play %s", PLAYERDIALOG_ENTRYBOX_INPUT_SOUNDNAME);
		return HandleFilledTextInput(pPlayer, pText);
	}

	// So when text input is blank, player can rewind to the previous dialog
	engine->ClientCommand(pPlayer->edict(), "play %s", PLAYERDIALOG_REWIND_ITEM_SOUNDNAME);
	RewindDialog(pPlayer);
}

void CEntryBox::HandleItemSelectionInput(CHL2RP_Player* pPlayer, const char* pSecretTokenText,
	const char* pIndexText)
{
	ClientPrint(pPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
		Localize(pPlayer, "#HL2RP_EntryBox_Item_Selection_Deny"));
}

CMenuItem::CMenuItem(const char* pDisplay)
{
	SetDisplay(pDisplay);
}

CMenuItem::CMenuItem(CHL2RP_Player* pPlayer, const char* pDisplay)
{
	SetLocalizedDisplay(pPlayer, pDisplay);
}

bool CMenuItem::operator<(CMenuItem& item)
{
	return item.IsNavigator();
}

int CMenuItem::GetSortPriority()
{
	return ESortPriority::Data;
}

bool CMenuItem::IsNavigator()
{
	return false;
}

void CMenuItem::PlayClickSound(CHL2RP_Player* pPlayer)
{
	// Play custom/data item sound by default
	engine->ClientCommand(pPlayer->edict(), "play %s", PLAYERDIALOG_MENU_DATA_ITEM_SOUNDNAME);
}

CMenuItem* CMenuItem::SetDisplay(const char* pDisplay)
{
	Q_strncpy(m_Display, pDisplay, sizeof(m_Display));
	return this;
}

CMenuItem* CMenuItem::SetLocalizedDisplay(CHL2RP_Player* pPlayer, const char* pDisplay)
{
	GetHL2RPAutoLocalizer().LocalizeAt(pPlayer, m_Display, sizeof(m_Display), pDisplay);
	return this;
}

int CPageAdvanceMenuItem::GetSortPriority()
{
	return ESortPriority::PageAdvance;
}

bool CPageAdvanceMenuItem::IsNavigator()
{
	return true;
}

void CPageAdvanceMenuItem::PlayClickSound(CHL2RP_Player* pPlayer)
{
	engine->ClientCommand(pPlayer->edict(), "play %s", PLAYERDIALOG_MENU_NEXT_ITEM_SOUNDNAME);
}

void CPageAdvanceMenuItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pMenu->AdvancePage(pPlayer);
}

int CPageReturnMenuItem::GetSortPriority()
{
	return ESortPriority::PageReturn;
}

bool CPageReturnMenuItem::IsNavigator()
{
	return true;
}

void CPageReturnMenuItem::PlayClickSound(CHL2RP_Player* pPlayer)
{
	engine->ClientCommand(pPlayer->edict(), "play %s", PLAYERDIALOG_MENU_PREVIOUS_ITEM_SOUNDNAME);
}

void CPageReturnMenuItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pMenu->ReturnPage(pPlayer);
}

int CPageRewindMenuItem::GetSortPriority()
{
	return ESortPriority::PageRewind;
}

bool CPageRewindMenuItem::IsNavigator()
{
	return true;
}

void CPageRewindMenuItem::PlayClickSound(CHL2RP_Player* pPlayer)
{
	engine->ClientCommand(pPlayer->edict(), "play %s", PLAYERDIALOG_REWIND_ITEM_SOUNDNAME);
}

void CPageRewindMenuItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pMenu->RewindDialog(pPlayer);
}

bool CMenuItemLess::Less(CMenuItem* const& pItem1, CMenuItem* const& pItem2, void* pCtx)
{
	return (pItem1->GetSortPriority() < pItem2->GetSortPriority());
}

CMenuPage::~CMenuPage()
{
	PurgeAndDeleteElements();
}

CMenu::CMenu(CHL2RP_Player* pPlayer, const char* pTitle, const char* pDescription)
	: CPlayerDialog(pPlayer, pTitle, pDescription)
{
	m_iCurrentPageIndex = CreatePage(pPlayer); // Not at initializator list, for vtable safety
}

void CMenu::Display(CHL2RP_Player* pPlayer)
{
	Assert(m_PagesList.IsValidIndex(m_iCurrentPageIndex));

	// Refresh the secret token
	m_iSecretToken = rand();

	// Delete old network item keys with invalid index, to be updated with those of our active page
	for (KeyValues* pCurNetItem = m_NetKeyValues->GetFirstTrueSubKey(), *pNextNetItem;
		pCurNetItem != NULL; pCurNetItem = pNextNetItem)
	{
		int itemIndex = pCurNetItem->GetInt();
		pNextNetItem = pCurNetItem->GetNextTrueSubKey();

		if (m_PagesList[m_iCurrentPageIndex]->IsValidIndex(itemIndex))
		{
			m_NetKeyValues->RemoveSubKey(pCurNetItem);
		}
	}

	// Now, add our updated active page items
	FOR_EACH_VEC(*m_PagesList[m_iCurrentPageIndex], i)
	{
		const char* pName = UTIL_VarArgs("%i", i);
		KeyValues* pNetItem = m_NetKeyValues->FindKey(pName, true);
		const char* pCommand = UTIL_VarArgs("menuitem %i %i", m_iSecretToken, i);
		pNetItem->SetString("command", pCommand);
		const char* pDisplay = UTIL_VarArgs("%i. %s", i + 1,
			m_PagesList[m_iCurrentPageIndex]->Element(i)->m_Display);
		pNetItem->SetString("msg", pDisplay);
	}

	UTIL_SendDialog(pPlayer, m_NetKeyValues, DIALOG_MENU);
}

void CMenu::HandleTextInput(CHL2RP_Player* pPlayer, char* pText)
{
	// Compensate a client bug which commands twice when pressing enter on entryboxes,
	// and thus calling this function right after an active entrybox rewinded to a new menu
	if (pText[0] != '\0')
	{
		ClientPrint(pPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
			Localize(pPlayer, "#HL2RP_Menu_Text_Input_Deny"));
	}
}

void CMenu::HandleItemSelectionInput(CHL2RP_Player* pPlayer, const char* pSecretTokenText,
	const char* pIndexText)
{
	Assert(m_PagesList.IsValidIndex(m_iCurrentPageIndex));
	int secretToken = Q_atoi(pSecretTokenText);

	if (secretToken == m_iSecretToken)
	{
		int index = Q_atoi(pIndexText);

		if (m_PagesList.Count() > 0 && m_PagesList[m_iCurrentPageIndex]->IsValidIndex(index))
		{
			CMenuItem* pItem = m_PagesList[m_iCurrentPageIndex]->Element(index);
			pItem->PlayClickSound(pPlayer);
			return pItem->HandleSelection(this, pPlayer);
		}
	}

	ClientPrint(pPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
		Localize(pPlayer, "#HL2RP_Menu_Item_Error"));
}

int CMenu::CreatePage(CHL2RP_Player* pPlayer)
{
	return m_PagesList.AddToTail(new CMenuPage());
}

void CMenu::AdvancePage(CHL2RP_Player* pPlayer)
{
	m_iCurrentPageIndex++;
	Display(pPlayer);
}

void CMenu::ReturnPage(CHL2RP_Player* pPlayer)
{
	m_iCurrentPageIndex--;
	Display(pPlayer);
}

// Adds an item while creating auxiliar needed new pages and navigation items aswell
void CMenu::AddItem(CHL2RP_Player* pPlayer, CMenuItem* pItem)
{
	Assert(m_PagesList.Count() > 0); // We must have one since menu instantiated
	CMenuPage* pLastPage = m_PagesList.Tail();

	if (pLastPage->Count() >= PLAYERDIALOG_MENU_MAX_PAGE_ITEMS)
	{
		int newPageIndex = CreatePage(pPlayer);
		m_PagesList[newPageIndex]->Insert(pItem);

		// Add desired item and navigators required in this case
		FOR_EACH_VEC_BACK(*pLastPage, i)
		{
			if (!pLastPage->Element(i)->IsNavigator())
			{
				// Copy the last non-navigation item into the new page
				m_PagesList[newPageIndex]->Insert(pLastPage->Element(i));

				// Create a 'Next' navigation menu item, replacing last non-navigation item
				CMenuItem* pNavigator = (new CPageAdvanceMenuItem)->
					SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Next");
				pLastPage->Remove(i);
				pLastPage->Insert(pNavigator);

				// Create a 'Previous' navigation menu item, simply at end of the new page
				pNavigator = (new CPageReturnMenuItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Previous");
				m_PagesList[newPageIndex]->Insert(pNavigator);
				break;
			}
		}

		return;
	}

	pLastPage->Insert(pItem);
}

CDialogRewindableMenu::CDialogRewindableMenu(CHL2RP_Player* pPlayer, const char* pTitle,
	const char* pDescription) : CMenu(pPlayer, pTitle, pDescription)
{
	Assert(m_PagesList.Count() > 0); // We must have one since menu instantiated
	m_PagesList.Tail()->Insert((new CPageRewindMenuItem)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Parent"));
}

int CDialogRewindableMenu::CreatePage(CHL2RP_Player* pPlayer)
{
	int index = CMenu::CreatePage(pPlayer);
	m_PagesList[index]->Insert((new CPageRewindMenuItem)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Parent"));
	return index;
}

CON_COMMAND_F(menuitem, "<secret token> <index> - Selects a menu page item (0-index based)",
	FCVAR_SERVER_CAN_EXECUTE)
{
	CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		if (pPlayer->m_pCurrentDialog != NULL)
		{
			return pPlayer->m_pCurrentDialog->HandleItemSelectionInput(pPlayer, args.Arg(1),
				args.Arg(2));
		}

		ClientPrint(pPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
			Localize(pPlayer, "#HL2RP_Menu_Untracked"));
	}
}

CON_COMMAND_F(entryboxtext, "<text> - Inputs entry box text", FCVAR_SERVER_CAN_EXECUTE)
{
	CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		if (pPlayer->m_pCurrentDialog != NULL)
		{
			return pPlayer->m_pCurrentDialog->HandleTextInput(pPlayer, (char*)args.ArgS());
		}

		ClientPrint(pPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
			Localize(pPlayer, "#HL2RP_EntryBox_Untracked"));
	}
}
