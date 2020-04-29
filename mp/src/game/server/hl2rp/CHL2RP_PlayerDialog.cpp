#include "cbase.h"
#include "CHL2RP_PlayerDialog.h"
#include "CHL2RP.h"
#include "language.h"
#include "CHL2RP_Player.h"
#include "CPropRationDispenser.h"
#include "inetchannel.h"

#ifdef HL2DM_RP
#include "filesystem.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Forward declare required phrase headers
extern const char DEFAULT_PHRASE_LANGUAGE[];

extern const char PARENT_MENU_ITEM_HEADER[];
extern const char NEXT_MENU_ITEM_HEADER[];
extern const char PREVIOUS_MENU_ITEM_HEADER[];
extern const char MAIN_MENU_TITLE_HEADER[];
extern const char MAIN_MENU_MSG_HEADER[];
extern const char PHRASE_LANGUAGE_MENU_TITLE_HEADER[];
extern const char PHRASE_LANGUAGE_MENU_MSG_HEADER[];
extern const char PHRASE_HEADER_MENU_TITLE_HEADER[];
extern const char PHRASE_HEADER_MENU_MSG_HEADER[];
extern const char PHRASE_TRANSLATION_ENTRY_BOX_TITLE_HEADER[];
extern const char PHRASE_TRANSLATION_ENTRY_BOX_MSG_HEADER[];
extern const char PHRASE_TRANSLATION_SAVE_MENU_TITLE_HEADER[];
extern const char PHRASE_TRANSLATION_SAVE_MENU_MSG_HEADER[];
extern const char ZONE_MENU_MSG_HEADER[];
extern const char SAVE_MENU_ITEM_HEADER[];
extern const char DISCARD_MENU_ITEM_HEADER[];
extern const char ADMIN_MENU_ITEM_HEADER[];
extern const char ADMIN_MENU_TITLE_HEADER[];
extern const char ADMIN_MENU_MSG_HEADER[];
extern const char TRANSLATE_PHRASE_MENU_ITEM_HEADER[];
extern const char ADD_ZONE_MENU_ITEM_HEADER[];
extern const char MANAGE_DISPENSERS_MENU_ITEM_HEADER[];
extern const char ADD_POINT_MENU_ITEM_HEADER[];
extern const char SET_POINT_MENU_ITEM_HEADER[];
extern const char CREATE_DISPENSER_MENU_ITEM_HEADER[];
extern const char MOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER[];
extern const char SAVE_CURRENT_DISPENSER_MENU_ITEM_HEADER[];
extern const char REMOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER[];
extern const char SAVE_AIMED_DISPENSER_MENU_ITEM_HEADER[];
extern const char REMOVE_AIMED_DISPENSER_MENU_ITEM_HEADER[];
extern const char DEFAULT_PHRASE_TRANSLATION_HINT_HEADER[];

#ifdef HL2DM_RP
extern const char SPECIAL_WEAPONS_MENU_ITEM_HEADER[];
extern const char SPECIAL_WEAPONS_MENU_MSG_HEADER[];
extern const char WEAPON_CITIZENPACKAGE_MENU_ITEM_HEADER[];
extern const char WEAPON_CITIZENSUITCASE_MENU_ITEM_HEADER[];
extern const char WEAPON_MOLOTOV_MENU_ITEM_HEADER[];
extern const char MANAGE_COMPAT_MODELS_MENU_ITEM_HEADER[];
extern const char MANAGE_COMPAT_MODELS_MENU_TITLE_HEADER[];
extern const char MANAGE_COMPAT_MODELS_MENU_MSG_HEADER[];
extern const char ADD_COMPAT_MODEL_MENU_ITEM_HEADER[];
extern const char REMOVE_COMPAT_MODEL_MENU_ITEM_HEADER[];
extern const char ADD_COMPAT_MODEL_TYPE_MENU_TITLE_HEADER[];
extern const char ADD_COMPAT_MODEL_TYPE_MENU_MSG_HEADER[];
extern const char ADD_COMPAT_MODEL_ID_ENTRY_BOX_TITLE_HEADER[];
extern const char ADD_COMPAT_MODEL_ID_ENTRY_BOX_MSG_HEADER[];
extern const char ADD_COMPAT_MODEL_PATH_ENTRY_BOX_TITLE_HEADER[];
extern const char ADD_COMPAT_MODEL_PATH_ENTRY_BOX_MSG_HEADER[];
extern const char SAVE_COMPAT_MODEL_MENU_TITLE_HEADER[];
extern const char SAVE_COMPAT_MODEL_MENU_MSG_HEADER[];
extern const char REMOVE_COMPAT_MODELS_MENU_TITLE_HEADER[];
extern const char REMOVE_COMPAT_MODELS_MENU_MSG_HEADER[];

extern char *sTeamNames[];
#endif

FORCEINLINE const Color DialogTitleColor()
{
	return Color(0, 255, 0, 255);
}

const int MAX_VALVE_MENU_ITEMS_PER_PAGE = 8;

// With current menu handling implementation, the
// items array doesn't include navigation buttons.
// Assign the navigation buttons sequential indexes greater than
// the max custom page item index, to detect them easily:
const int PARENT_MENU_ITEM_PAGE_INDEX = MAX_VALVE_MENU_ITEMS_PER_PAGE;
const int PREVIOUS_MENU_ITEM_PAGE_INDEX = PARENT_MENU_ITEM_PAGE_INDEX + 1;
const int NEXT_MENU_ITEM_PAGE_INDEX = PREVIOUS_MENU_ITEM_PAGE_INDEX + 1;

const char NORMAL_MENU_ITEM_SOUND[] = "ui/buttonclick.wav";
const char NEXT_MENU_ITEM_SOUND[] = "buttons/button15.wav";
const char PREVIOUS_MENU_ITEM_SOUND[] = "buttons/button16.wav";
const char PARENT_MENU_ITEM_SOUND[] = "buttons/button9.wav";
const char ENTRY_BOX_INPUT_SOUND[] = "friends/friend_join.wav";

const int MAX_VALVE_DIALOG_TIME = 200;

CHL2RP_PlayerDialog::CHL2RP_PlayerDialog(CHL2RP_Player *pPlayer, const char *pszTitle, const char *pszMsg) : m_Player(*pPlayer)
{
	Q_strncpy(m_szTitle, pszTitle, sizeof(m_szTitle));
	Q_strncpy(m_szMsg, pszMsg, sizeof(m_szMsg));
}

CHL2RP_PlayerDialog::~CHL2RP_PlayerDialog()
{

}

void CHL2RP_PlayerDialog::SetCommonKVKeys(KeyValues *dialogKv) const
{
	dialogKv->SetString("title", m_szTitle);
	dialogKv->SetInt("level", m_Player.m_iLastDialogLevel); // Let CPluginHelpersCheck decrement level later
	dialogKv->SetColor("color", DialogTitleColor());
	dialogKv->SetInt("time", MAX_VALVE_DIALOG_TIME);
	dialogKv->SetString("msg", m_szMsg);
}

CHL2RP_PlayerEntryBox::CHL2RP_PlayerEntryBox(CHL2RP_Player *pPlayer, const char *pszTitle, const char *pszMsg) :
CHL2RP_PlayerDialog(pPlayer, pszTitle, pszMsg)
{

}

void CHL2RP_PlayerEntryBox::Display()
{
	delete m_Player.m_pCurrentDialog;
	m_Player.m_pCurrentDialog = this;
	KeyValues *entryBoxKv = new KeyValues("entry");
	SetCommonKVKeys(entryBoxKv);
	entryBoxKv->SetString("command", "entryboxtext");
	UTIL_SendDialog(&m_Player, entryBoxKv, DIALOG_ENTRY, static_cast<INetChannel *>(engine->GetPlayerNetInfo(m_Player.entindex())));
	entryBoxKv->deleteThis();
}

CHL2RP_PlayerMenu::CHL2RP_PlayerMenu(CHL2RP_Player *player, bool hasParent, const char *title, const char *msg) :
m_iNumItems(0), m_iCurrentPageStartIndex(0), m_bHasParent(hasParent), CHL2RP_PlayerDialog(player, title, msg)
{

}

int CHL2RP_PlayerMenu::AddItem(int action, const char *display, bool translateDisplay)
{
	return AddItemAtIndex(m_iNumItems, action, display, translateDisplay);
}

int CHL2RP_PlayerMenu::AddItemAtIndex(int index, int action, const char *display, bool translateDisplay,
	CPhraseParam *paramsBuff, int paramCount, ...)
{
	if (index < m_iNumItems)
	{
		// Shift up if there is still a free slot:
		if (m_iNumItems < MAX_MENU_ITEMS)
		{
			for (int i = m_iNumItems; index < i; i--)
			{
				m_iItemAction[i] = m_iItemAction[i - 1];
				Q_strcpy(m_szItemDisplay[i], m_szItemDisplay[i - 1]);
			}
		}
		else
		{
			return -1;
		}
	}
	else if (m_iNumItems >= MAX_MENU_ITEMS)
	{
		return -1;
	}

	m_iItemAction[index] = action;

	if (translateDisplay)
	{
		CHL2RP_Phrases::LoadForPlayer(&m_Player, m_szItemDisplay[index], sizeof(m_szItemDisplay[0]), display, NULL, paramsBuff, paramCount);
	}
	else
	{
		// Format with the variable parameters
		va_list args;
		va_start(args, paramCount);
		Q_vsnprintf(m_szItemDisplay[index], sizeof(m_szItemDisplay[0]), display, args);
		va_end(args);
	}

	m_iNumItems++;
	return index;
}

void CHL2RP_PlayerMenu::Display()
{
	delete m_Player.m_pCurrentDialog;
	m_Player.m_pCurrentDialog = this;
	ChangePage(m_iCurrentPageStartIndex);
}

void CHL2RP_PlayerMenu::ChangePage(int pageStartIndex)
{
	m_iCurrentPageStartIndex = pageStartIndex;
	KeyValues *menuKv = new KeyValues("menu"), *itemKv;
	SetCommonKVKeys(menuKv);

	// Renew token to prevent user being able
	// to bind still on menu if he discovered it:
	m_iSecretToken = rand();
	char command[256], display[MENU_ITEM_LENGTH];

	// Start calculating next page start index depending on amount of navigation items... Limit it to menu's number of items
	int nextPageStartIndex = pageStartIndex + MAX_VALVE_MENU_ITEMS_PER_PAGE - m_bHasParent - (m_iCurrentPageStartIndex > 0),
		relPageIndex = 0;

	if (nextPageStartIndex < m_iNumItems)
	{
		// Custom page items do not reach last global custom. Free space for "Next" button
		nextPageStartIndex--;
	}
	else
	{
		nextPageStartIndex = m_iNumItems;
	}

	for (int i = pageStartIndex; i < nextPageStartIndex; i++)
	{
		Q_snprintf(command, sizeof(command), "menuitem %i %i", relPageIndex++, m_iSecretToken);
		Q_snprintf(display, sizeof(display), "%i. %s", relPageIndex, m_szItemDisplay[i]);
		itemKv = menuKv->CreateNewKey();
		itemKv->SetString("msg", display);
		itemKv->SetString("command", command);
	}

	if (m_bHasParent)
	{
		itemKv = menuKv->CreateNewKey();
		CHL2RP_Phrases::LoadForPlayer(&m_Player, display, sizeof(display), PARENT_MENU_ITEM_HEADER, NULL, ++relPageIndex);
		itemKv->SetString("msg", display);
		Q_snprintf(command, sizeof(command), "menuitem %i %i", PARENT_MENU_ITEM_PAGE_INDEX, m_iSecretToken);
		itemKv->SetString("command", command);
	}

	if (pageStartIndex > 0)
	{
		itemKv = menuKv->CreateNewKey();
		CHL2RP_Phrases::LoadForPlayer(&m_Player, display, sizeof(display), PREVIOUS_MENU_ITEM_HEADER, NULL, ++relPageIndex);
		itemKv->SetString("msg", display);
		Q_snprintf(command, sizeof(command), "menuitem %i %i", PREVIOUS_MENU_ITEM_PAGE_INDEX, m_iSecretToken);
		itemKv->SetString("command", command);
	}

	if (nextPageStartIndex < m_iNumItems)
	{
		itemKv = menuKv->CreateNewKey();
		CHL2RP_Phrases::LoadForPlayer(&m_Player, display, sizeof(display), NEXT_MENU_ITEM_HEADER, NULL, ++relPageIndex);
		itemKv->SetString("msg", display);
		Q_snprintf(command, sizeof(command), "menuitem %i %i", NEXT_MENU_ITEM_PAGE_INDEX, m_iSecretToken);
		itemKv->SetString("command", command);
	}

	UTIL_SendDialog(&m_Player, menuKv, DIALOG_MENU, static_cast<INetChannel *>(engine->GetPlayerNetInfo(m_Player.entindex())));
	menuKv->deleteThis();
}

void CHL2RP_PlayerMenu::TrySendAdminMenu() const
{
	if (m_Player.IsAdmin())
	{
		(new CHL2RP_PlayerAdminMenu(&m_Player))->Display();
	}
	else
	{
		(new CHL2RP_PlayerMainMenu(&m_Player))->Display();
	}
}

CHL2RP_PlayerMainMenu::CHL2RP_PlayerMainMenu(CHL2RP_Player *player) : CHL2RP_PlayerMenu(player)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), MAIN_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), MAIN_MENU_MSG_HEADER, NULL);

#ifdef HL2DM_RP
	AddItem(SPECIAL_WEAPONS, SPECIAL_WEAPONS_MENU_ITEM_HEADER, true);
#endif

	if (player->IsAdmin())
	{
		AddItem(ADMINISTRATION, ADMIN_MENU_ITEM_HEADER, true);
	}
}

void CHL2RP_PlayerMainMenu::CallHandler(int iIndex)
{
	switch (iIndex)
	{
#ifdef HL2DM_RP
	case SPECIAL_WEAPONS:
	{
		(new CHL2RP_PlayerSpecialWeaponsMenu(&m_Player))->Display();
		break;
	}
#endif
	case ADMINISTRATION:
	{
		if (m_Player.IsAdmin())
		{
			(new CHL2RP_PlayerAdminMenu(&m_Player))->Display();
		}
		else
		{
			ChangePage(0);
		}

		break;
	}
	}
}

#ifdef HL2DM_RP
CHL2RP_PlayerSpecialWeaponsMenu::CHL2RP_PlayerSpecialWeaponsMenu(CHL2RP_Player *player) : CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), SPECIAL_WEAPONS_MENU_ITEM_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), SPECIAL_WEAPONS_MENU_MSG_HEADER, NULL);

	if (player->Weapon_OwnsThisType("weapon_citizenpackage") != NULL)
	{
		AddItem(WEAPON_CITIZENPACKAGE, WEAPON_CITIZENPACKAGE_MENU_ITEM_HEADER, true);
	}

	if (player->Weapon_OwnsThisType("weapon_citizensuitcase") != NULL)
	{
		AddItem(WEAPON_CITIZENSUITCASE, WEAPON_CITIZENSUITCASE_MENU_ITEM_HEADER, true);
	}

	if (player->Weapon_OwnsThisType("weapon_molotov") != NULL)
	{
		AddItem(WEAPON_MOLOTOV, WEAPON_MOLOTOV_MENU_ITEM_HEADER, true);
	}
}

void CHL2RP_PlayerSpecialWeaponsMenu::CallHandler(int iIndex)
{
	CBaseCombatWeapon *weapon = NULL;

	switch (m_iItemAction[iIndex])
	{
	case WEAPON_CITIZENPACKAGE:
	{
		weapon = m_Player.Weapon_OwnsThisType("weapon_citizenpackage");
		break;
	}
	case WEAPON_CITIZENSUITCASE:
	{
		weapon = m_Player.Weapon_OwnsThisType("weapon_citizensuitcase");
		break;
	}
	case WEAPON_MOLOTOV:
	{
		weapon = m_Player.Weapon_OwnsThisType("weapon_molotov");
		break;
	}
	}

	if (weapon != NULL)
	{
		m_Player.Weapon_Switch(weapon);
	}

	// Make new menu to refresh owning weapons:
	(new CHL2RP_PlayerSpecialWeaponsMenu(&m_Player))->Display();
}

void CHL2RP_PlayerSpecialWeaponsMenu::DisplayParent() const
{
	(new CHL2RP_PlayerMainMenu(&m_Player))->Display();
}
#endif

CHL2RP_PlayerAdminMenu::CHL2RP_PlayerAdminMenu(CHL2RP_Player *player) : CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), ADMIN_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), ADMIN_MENU_MSG_HEADER, NULL);
	AddItem(TRANSLATE_PHRASE, TRANSLATE_PHRASE_MENU_ITEM_HEADER, true);
	AddItem(ADD_ZONE, ADD_ZONE_MENU_ITEM_HEADER, true);
	AddItem(MANAGE_DISPENSERS, MANAGE_DISPENSERS_MENU_ITEM_HEADER, true);

#ifdef HL2DM_RP
	AddItem(MANAGE_COMPAT_ANIM_MODELS, MANAGE_COMPAT_MODELS_MENU_ITEM_HEADER, true);
#endif
}

void CHL2RP_PlayerAdminMenu::CallHandler(int iIndex)
{
	if (m_Player.IsAdmin())
	{
		switch (m_iItemAction[iIndex])
		{
		case TRANSLATE_PHRASE:
		{
			(new CHL2RP_PlayerPhraseLanguageMenu(&m_Player))->Display();
			break;
		}
		case ADD_ZONE:
		{
			(new CHL2RP_PlayerZoneMenu(&m_Player))->Display();
			break;
		}
		case MANAGE_DISPENSERS:
		{
			(new CHL2RP_PlayerDispenserMenu(&m_Player))->Display();
			break;
		}
#ifdef HL2DM_RP
		case MANAGE_COMPAT_ANIM_MODELS:
		{
			(new CHL2RP_PlayerManageCompatModelsMenu(&m_Player))->Display();
			break;
		}
#endif
		}
	}
	else
	{
		(new CHL2RP_PlayerMainMenu(&m_Player))->Display();
	}
}

void CHL2RP_PlayerAdminMenu::DisplayParent() const
{
	(new CHL2RP_PlayerMainMenu(&m_Player))->Display();
}

CHL2RP_PlayerPhraseLanguageMenu::CHL2RP_PlayerPhraseLanguageMenu(CHL2RP_Player *player) :
CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), PHRASE_LANGUAGE_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), PHRASE_LANGUAGE_MENU_MSG_HEADER, NULL);

	FOR_EACH_LANGUAGE(eLang)
	{
		int index = AddItem(0, GetLanguageName((ELanguage)eLang), false);
		if (index != -1)
		{
			m_pszItemLanguageShortName[index] = (GetLanguageShortName((ELanguage)eLang));
		}
	}
}

void CHL2RP_PlayerPhraseLanguageMenu::CallHandler(int iIndex)
{
	(new CHL2RP_PlayerPhraseHeaderMenu(&m_Player, m_pszItemLanguageShortName[iIndex]))->Display();
}

void CHL2RP_PlayerPhraseLanguageMenu::DisplayParent() const
{
	TrySendAdminMenu();
}

CHL2RP_PlayerPhraseHeaderMenu::CHL2RP_PlayerPhraseHeaderMenu(CHL2RP_Player *player, const char *languageShortName) :
m_pszLanguageShortName(languageShortName), CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), PHRASE_HEADER_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), PHRASE_HEADER_MENU_MSG_HEADER, NULL);
	KeyValues *defaultLanguageKv = CHL2RP::s_pPhrases->FindKey(DEFAULT_PHRASE_LANGUAGE);

	for (KeyValues *headerKv = defaultLanguageKv->GetFirstSubKey(); headerKv != NULL; headerKv = headerKv->GetNextKey())
	{
		int index = AddItem(0, headerKv->GetName(), false);

		if (index != -1)
		{
			m_pszItemHeader[index] = headerKv->GetName();
		}
	}
}

void CHL2RP_PlayerPhraseHeaderMenu::CallHandler(int iIndex)
{
	(new CHL2RP_PlayerPhraseTranslationEntryBox(&m_Player, m_pszLanguageShortName, m_pszItemHeader[iIndex]))->Display();
}

void CHL2RP_PlayerPhraseHeaderMenu::DisplayParent() const
{
	(new CHL2RP_PlayerPhraseLanguageMenu(&m_Player))->Display();
}

CHL2RP_PlayerPhraseTranslationEntryBox::CHL2RP_PlayerPhraseTranslationEntryBox(CHL2RP_Player *player, const char *languageShortName, const char *header) :
m_pszLanguageShortName(languageShortName), m_pszHeader(header), CHL2RP_PlayerEntryBox(player)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), PHRASE_TRANSLATION_ENTRY_BOX_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), PHRASE_TRANSLATION_ENTRY_BOX_MSG_HEADER, NULL);

	char multiMsg[MAX_PHRASE_TRANSLATION_LENGTH];

	CHL2RP_Phrases::LoadForPlayer(player, multiMsg, sizeof(multiMsg), DEFAULT_PHRASE_TRANSLATION_HINT_HEADER, false, header);
	ClientPrint(player, HUD_PRINTCONSOLE, multiMsg);
	ClientPrint(player, HUD_PRINTCONSOLE, CHL2RP::s_pPhrases->FindKey(DEFAULT_PHRASE_LANGUAGE)->GetString(header));
}

void CHL2RP_PlayerPhraseTranslationEntryBox::CallHandler(const char *pszText)
{
	CUtlString replaceStr(pszText);
	replaceStr = replaceStr.Replace("\\n", "\n").Replace("\\t", "\t");
	pszText = replaceStr.Get();
	(new CHL2RP_PlayerPhraseTranslationSaveMenu(&m_Player, m_pszLanguageShortName, m_pszHeader, pszText))->Display();
}

CHL2RP_PlayerPhraseTranslationSaveMenu::CHL2RP_PlayerPhraseTranslationSaveMenu
(CHL2RP_Player *player, const char *languageShortName, const char *header, const char *translation)
: m_pszLanguageShortName(languageShortName), m_pszHeader(header), CHL2RP_PlayerMenu(player, true)
{
	Q_strncpy(m_szTranslation, translation, sizeof(m_szTranslation));
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), PHRASE_TRANSLATION_SAVE_MENU_MSG_HEADER, false, translation);
	AddItem(SAVE, SAVE_MENU_ITEM_HEADER, true);
	AddItem(DISCARD, DISCARD_MENU_ITEM_HEADER, true);
}

void CHL2RP_PlayerPhraseTranslationSaveMenu::CallHandler(int iIndex)
{
	switch (m_iItemAction[iIndex])
	{
	case SAVE:
	{
		KeyValues *translationKv = CHL2RP::s_pPhrases->FindKey(m_pszLanguageShortName, true);
		translationKv->SetString(m_pszHeader, m_szTranslation);
		CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPhraseTxn(m_pszLanguageShortName, m_pszHeader, m_szTranslation));
		break;
	}
	}

	(new CHL2RP_PlayerPhraseHeaderMenu(&m_Player, m_pszLanguageShortName))->Display();
}

void CHL2RP_PlayerPhraseTranslationSaveMenu::DisplayParent() const
{
	(new CHL2RP_PlayerPhraseTranslationEntryBox(&m_Player, m_pszLanguageShortName, m_pszHeader))->Display();
}

CHL2RP_PlayerZoneMenu::CHL2RP_PlayerZoneMenu(CHL2RP_Player *player) :
m_HeightEditState(STOPPED), m_CurrentZone(player->GetAbsOrigin()), CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), MAIN_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), ZONE_MENU_MSG_HEADER, NULL);
	AddItem(ADD_INITIAL_POINT, ADD_POINT_MENU_ITEM_HEADER, 1);
}

void CHL2RP_PlayerZoneMenu::CallHandler(int iIndex)
{
	switch (m_iItemAction[iIndex])
	{
	case SAVE:
	{
		CHL2RP::s_Zones.AddToTail(m_CurrentZone);

		// Display after, or the new menu fatally
		// overrides the current zone object:
		(new CHL2RP_PlayerZoneMenu(&m_Player))->Display();

		break;
	}
	default:
	{
		const Vector& origin = m_Player.GetAbsOrigin();

		if ((ADD_INITIAL_POINT <= m_iItemAction[iIndex]) && (m_iItemAction[iIndex] <= ADD_LAST_POINT))
		{
			// Acceptable time to initialize heights (first point):
			if (m_iItemAction[iIndex] == ADD_INITIAL_POINT)
			{
				m_CurrentZone.m_flMinHeight = m_CurrentZone.m_flMaxHeight = origin.z;
			}
			else if (origin.z < m_CurrentZone.m_flMinHeight)
			{
				m_CurrentZone.m_flMinHeight = origin.z;
			}
			else if (origin.z > m_CurrentZone.m_flMaxHeight)
			{
				m_CurrentZone.m_flMaxHeight = origin.z;
			}

			m_CurrentZone.m_iNumPoints++;
			m_CurrentZone.m_vecPointsXY[m_iItemAction[iIndex] - ADD_INITIAL_POINT].x = origin.x;
			m_CurrentZone.m_vecPointsXY[m_iItemAction[iIndex] - ADD_INITIAL_POINT].y = origin.y;
			CHL2RP_Phrases::LoadForPlayer(&m_Player, m_szItemDisplay[iIndex], sizeof(m_szItemDisplay[0]),
				SET_POINT_MENU_ITEM_HEADER, NULL, m_iItemAction[iIndex] - ADD_INITIAL_POINT + 1);

			if (m_iItemAction[iIndex] < ADD_LAST_POINT)
			{
				AddItem(m_iItemAction[iIndex] + 1, ADD_POINT_MENU_ITEM_HEADER, m_iItemAction[iIndex] - ADD_INITIAL_POINT + 2);

				if (m_iNumItems == 4)
				{
					AddItemAtIndex(0, SAVE, SAVE_MENU_ITEM_HEADER, true);
					iIndex++;
				}
			}

			m_iItemAction[iIndex] += (SET_INITIAL_POINT - ADD_INITIAL_POINT);
		}
		else
		{
			if (origin.z < m_CurrentZone.m_flMinHeight)
			{
				m_CurrentZone.m_flMinHeight = origin.z;
			}
			else if (origin.z > m_CurrentZone.m_flMaxHeight)
			{
				m_CurrentZone.m_flMaxHeight = origin.z;
			}

			m_CurrentZone.m_vecPointsXY[m_iItemAction[iIndex] - SET_INITIAL_POINT].x = origin.x;
			m_CurrentZone.m_vecPointsXY[m_iItemAction[iIndex] - SET_INITIAL_POINT].y = origin.y;
		}

		ChangePage(m_iCurrentPageStartIndex);
	}
	}
}

void CHL2RP_PlayerZoneMenu::DisplayParent() const
{
	TrySendAdminMenu();
}

CHL2RP_PlayerDispenserMenu::CHL2RP_PlayerDispenserMenu(CHL2RP_Player *player) : m_pCurrentDispenser(NULL), CHL2RP_PlayerMenu(player, true)
{
	AddItem(CREATE_DISPENSER, CREATE_DISPENSER_MENU_ITEM_HEADER, true);
	AddItem(MOVE_CURRENT_DISPENSER, MOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER, true);
	AddItem(SAVE_CURRENT_DISPENSER, SAVE_CURRENT_DISPENSER_MENU_ITEM_HEADER, true);
	AddItem(REMOVE_CURRENT_DISPENSER, REMOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER, true);
	AddItem(SAVE_LOOKING_DISPENSER, SAVE_AIMED_DISPENSER_MENU_ITEM_HEADER, true);
	AddItem(REMOVE_LOOKING_DISPENSER, REMOVE_AIMED_DISPENSER_MENU_ITEM_HEADER, true);
}

void CHL2RP_PlayerDispenserMenu::CallHandler(int iIndex)
{
	switch (m_iItemAction[iIndex])
	{
	case CREATE_DISPENSER:
	{
		trace_t tr;
		const Vector &origin = m_Player.EyePosition();
		const Vector &end = origin + m_Player.EyeDirection3D() * MAX_TRACE_LENGTH;
		UTIL_TraceLine(origin, end, MASK_ALL, &m_Player, COLLISION_GROUP_NONE, &tr);

		if (tr.DidHit())
		{
			QAngle angles;
			VectorAngles(tr.plane.normal, angles);
			angles.x = angles.z = 0.0f;
			m_pCurrentDispenser = static_cast<CPropRationDispenser *>(CBaseEntity::Create("prop_ration_dispenser", tr.endpos, angles));
		}

		break;
	}
	case MOVE_CURRENT_DISPENSER:
	{
		if (m_pCurrentDispenser != NULL)
		{
			trace_t tr;
			const Vector &origin = m_Player.EyePosition();
			const Vector &end = origin + m_Player.EyeDirection3D() * MAX_TRACE_LENGTH;

			UTIL_TraceLine(origin, end, MASK_ALL, &m_Player, COLLISION_GROUP_NONE, &tr);

			if (tr.DidHit())
			{
				QAngle angles;
				VectorAngles(tr.plane.normal, angles);
				angles.x = angles.z = 0.0f;

				m_pCurrentDispenser->Teleport(&tr.endpos, &angles, NULL);
			}
		}

		break;
	}
	case SAVE_CURRENT_DISPENSER:
	{
		if ((m_pCurrentDispenser != NULL) && (CHL2RP::s_pSQL != NULL))
		{
			if (m_pCurrentDispenser->m_iDatabaseID == INVALID_DATABASE_ID)
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CInsertDispenserTxn(m_pCurrentDispenser));
			}
			else
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertDispenserTxn(m_pCurrentDispenser));
			}
		}

		break;
	}
	case REMOVE_CURRENT_DISPENSER:
	{
		if (m_pCurrentDispenser != NULL)
		{
			if (CHL2RP::s_pSQL != NULL)
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CDeleteDispenserTxn(m_pCurrentDispenser));
			}

			m_pCurrentDispenser->Remove();
			m_pCurrentDispenser = NULL;
		}

		break;
	}
	case SAVE_LOOKING_DISPENSER:
	{
		trace_t tr;
		const Vector &origin = m_Player.EyePosition();
		const Vector &end = origin + m_Player.EyeDirection3D() * MAX_TRACE_LENGTH;
		UTIL_TraceLine(origin, end, MASK_ALL, &m_Player, COLLISION_GROUP_NONE, &tr);
		CPropRationDispenser *dispenser = dynamic_cast<CPropRationDispenser *>(tr.m_pEnt);

		if (dispenser != NULL && CHL2RP::s_pSQL != NULL)
		{
			if (dispenser->m_iDatabaseID == INVALID_DATABASE_ID)
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CInsertDispenserTxn(dispenser));
			}
			else
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertDispenserTxn(dispenser));
			}
		}

		break;
	}
	case REMOVE_LOOKING_DISPENSER:
	{
		trace_t tr;
		const Vector &origin = m_Player.EyePosition();
		const Vector &end = origin + m_Player.EyeDirection3D() * MAX_TRACE_LENGTH;
		UTIL_TraceLine(origin, end, MASK_ALL, &m_Player, COLLISION_GROUP_NONE, &tr);
		CPropRationDispenser *dispenser = dynamic_cast<CPropRationDispenser *>(tr.m_pEnt);

		if (dispenser != NULL)
		{
			if (CHL2RP::s_pSQL != NULL)
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CDeleteDispenserTxn(dispenser));
			}

			dispenser->Remove();
		}

		break;
	}
	}

	ChangePage(m_iCurrentPageStartIndex);
}

void CHL2RP_PlayerDispenserMenu::DisplayParent() const
{
	TrySendAdminMenu();
}

#ifdef HL2DM_RP
CHL2RP_PlayerManageCompatModelsMenu::CHL2RP_PlayerManageCompatModelsMenu(CHL2RP_Player *player) :
CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), MANAGE_COMPAT_MODELS_MENU_TITLE_HEADER);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), MANAGE_COMPAT_MODELS_MENU_MSG_HEADER);
	AddItem(ADD_MODEL, ADD_COMPAT_MODEL_MENU_ITEM_HEADER, true);
	AddItem(REMOVE_MODEL, REMOVE_COMPAT_MODEL_MENU_ITEM_HEADER, true);
}

void CHL2RP_PlayerManageCompatModelsMenu::CallHandler(int iIndex)
{
	switch (m_iItemAction[iIndex])
	{
	case ADD_MODEL:
	{
		(new CHL2RP_PlayerAddCompatModelTypeMenu(&m_Player))->Display();
		break;
	}
	case REMOVE_MODEL:
	{
		(new CHL2RP_PlayerRemoveCompatModelMenu(&m_Player))->Display();
		break;
	}
	}
}

void CHL2RP_PlayerManageCompatModelsMenu::DisplayParent() const
{
	TrySendAdminMenu();
}

CHL2RP_PlayerAddCompatModelTypeMenu::CHL2RP_PlayerAddCompatModelTypeMenu(CHL2RP_Player *player) :
CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), ADD_COMPAT_MODEL_TYPE_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), ADD_COMPAT_MODEL_TYPE_MENU_MSG_HEADER, NULL);
	AddItem(0, sTeamNames[TEAM_REBELS]);
	AddItem(1, sTeamNames[TEAM_COMBINE]);
}

void CHL2RP_PlayerAddCompatModelTypeMenu::CallHandler(int iIndex)
{
	(new CHL2RP_PlayerAddCompatModelDisplayEntryBox(&m_Player, m_iItemAction[iIndex]))->Display();
}

void CHL2RP_PlayerAddCompatModelTypeMenu::DisplayParent() const
{
	(new CHL2RP_PlayerManageCompatModelsMenu(&m_Player))->Display();
}

CHL2RP_PlayerAddCompatModelDisplayEntryBox::CHL2RP_PlayerAddCompatModelDisplayEntryBox(CHL2RP_Player *player, int type) :
m_iType(type), CHL2RP_PlayerEntryBox(player)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), ADD_COMPAT_MODEL_ID_ENTRY_BOX_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), ADD_COMPAT_MODEL_ID_ENTRY_BOX_MSG_HEADER, NULL);
}

void CHL2RP_PlayerAddCompatModelDisplayEntryBox::CallHandler(const char *pszText)
{
	if (pszText[0] != '\0')
	{
		(new CHL2RP_PlayerAddCompatModelPathEntryBox(&m_Player, pszText, m_iType))->Display();
	}
	else
	{
		(new CHL2RP_PlayerAddCompatModelDisplayEntryBox(&m_Player, m_iType))->Display();
	}
}

CHL2RP_PlayerAddCompatModelPathEntryBox::CHL2RP_PlayerAddCompatModelPathEntryBox(CHL2RP_Player *player, const char *id, int type) :
m_iType(type), CHL2RP_PlayerEntryBox(player)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), ADD_COMPAT_MODEL_PATH_ENTRY_BOX_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), ADD_COMPAT_MODEL_PATH_ENTRY_BOX_MSG_HEADER, NULL);
	Q_strncpy(m_szId, id, sizeof(m_szId));
}

void CHL2RP_PlayerAddCompatModelPathEntryBox::CallHandler(const char *pszText)
{
	char fixedPath[MAX_PATH];
	V_FixupPathName(fixedPath, sizeof(fixedPath), pszText);

	if (filesystem->FileExists(fixedPath))
	{
		(new CHL2RP_PlayerSaveCompatModelMenu(&m_Player, m_szId, fixedPath, m_iType))->Display();
	}
	else
	{
		// Path doesn't exist, show dialog to input different path:
		(new CHL2RP_PlayerAddCompatModelPathEntryBox(&m_Player, m_szId, m_iType))->Display();
	}
}

CHL2RP_PlayerSaveCompatModelMenu::CHL2RP_PlayerSaveCompatModelMenu(CHL2RP_Player *player, const char *id, const char *path, int type) :
m_iType(type), CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), SAVE_COMPAT_MODEL_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), SAVE_COMPAT_MODEL_MENU_MSG_HEADER, NULL);
	Q_strcpy(m_szId, id);
	Q_strcpy(m_szPath, path);
	AddItem(SAVE, SAVE_MENU_ITEM_HEADER, true);
	AddItem(DISCARD, DISCARD_MENU_ITEM_HEADER, true);
}

void CHL2RP_PlayerSaveCompatModelMenu::CallHandler(int iIndex)
{
	switch (iIndex)
	{
	case SAVE:
	{
		for (int i = 0; i < ARRAYSIZE(CHL2RP::s_CompatAnimationModelPaths); i++)
		{
			if (CHL2RP::s_CompatAnimationModelPaths[i].HasElement(m_szId))
			{
				// Alias already exists, show dialog to input different alias and exit:
				(new CHL2RP_PlayerAddCompatModelDisplayEntryBox(&m_Player, m_iType))->Display();
				return;
			}

			for (unsigned int j = 0; j < CHL2RP::s_CompatAnimationModelPaths[i].Count(); j++)
			{
				if (!Q_stricmp(CHL2RP::s_CompatAnimationModelPaths[i][j], m_szPath))
				{
					// Path already exists, show dialog to input different path and exit:
					(new CHL2RP_PlayerAddCompatModelPathEntryBox(&m_Player, m_szId, m_iType))->Display();
					return;
				}
			}
		}

		CBaseEntity::PrecacheModel(m_szPath);
		CHL2RP::s_CompatAnimationModelPaths[m_iType].Insert(m_szId, CUtlString(m_szPath));

		if (CHL2RP::s_pSQL != NULL)
		{
			CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertCompatAnimModelTxn(m_szId, m_szPath, m_iType));
		}

		break;
	}
	}

	(new CHL2RP_PlayerManageCompatModelsMenu(&m_Player))->Display();
}

void CHL2RP_PlayerSaveCompatModelMenu::DisplayParent() const
{
	(new CHL2RP_PlayerAddCompatModelPathEntryBox(&m_Player, m_szId, m_iType))->Display();
}

CHL2RP_PlayerRemoveCompatModelMenu::CHL2RP_PlayerRemoveCompatModelMenu(CHL2RP_Player *player) :
CHL2RP_PlayerMenu(player, true)
{
	CHL2RP_Phrases::LoadForPlayer(player, m_szTitle, sizeof(m_szTitle), REMOVE_COMPAT_MODELS_MENU_TITLE_HEADER, NULL);
	CHL2RP_Phrases::LoadForPlayer(player, m_szMsg, sizeof(m_szMsg), REMOVE_COMPAT_MODELS_MENU_MSG_HEADER, NULL);

	for (int i = 0; i < ARRAYSIZE(CHL2RP::s_CompatAnimationModelPaths); i++)
	{
		for (int j = CHL2RP::s_CompatAnimationModelPaths[i].First(); CHL2RP::s_CompatAnimationModelPaths[i].IsValidIndex(j);
			j = CHL2RP::s_CompatAnimationModelPaths[i].Next(j))
		{
			AddItem(0, CHL2RP::s_CompatAnimationModelPaths[i].GetElementName(j), false);
		}
	}
}

void CHL2RP_PlayerRemoveCompatModelMenu::CallHandler(int iIndex)
{
	for (int i = 0; i < ARRAYSIZE(CHL2RP::s_CompatAnimationModelPaths); i++)
	{
		int j = CHL2RP::s_CompatAnimationModelPaths[i].Find(m_szItemDisplay[iIndex]);

		if (CHL2RP::s_CompatAnimationModelPaths[i].IsValidIndex(j))
		{
			CHL2RP::s_CompatAnimationModelPaths[i].RemoveAt(j);

			if (CHL2RP::s_pSQL != NULL)
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CDeleteCompatAnimModelTxn(m_szItemDisplay[iIndex]));
			}

			break;
		}
	}

	(new CHL2RP_PlayerRemoveCompatModelMenu(&m_Player))->Display();
}

void CHL2RP_PlayerRemoveCompatModelMenu::DisplayParent() const
{
	(new CHL2RP_PlayerManageCompatModelsMenu(&m_Player))->Display();
}
#endif

CON_COMMAND(menuitem, "<index> - Selects a menu page item (0-index based)")
{
	CHL2RP_Player *pPlayer = CHL2RP_Player::ToThisClassFast(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		CHL2RP_PlayerMenu *currentMenu = dynamic_cast<CHL2RP_PlayerMenu *>(pPlayer->m_pCurrentDialog);

		if (currentMenu == NULL)
		{
			ClientPrint(pPlayer, HUD_PRINTTALK, "Menu item error: close your unrelated dialog");
		}
		else if (atoi(args.Arg(2)) != currentMenu->m_iSecretToken)
		{
			ClientPrint(pPlayer, HUD_PRINTTALK, "Menu item error: stop trying to send item selection commands");
		}
		else
		{
			int relPageIndex = atoi(args.Arg(1));

			if (relPageIndex >= 0)
			{
				// "Back" item:
				if (relPageIndex == PARENT_MENU_ITEM_PAGE_INDEX)
				{
					if (currentMenu->m_bHasParent)
					{
						currentMenu->DisplayParent();
						engine->ClientCommand(pPlayer->edict(), "play ", PARENT_MENU_ITEM_SOUND);
						return;
					}
				}
				// "Previous" item:
				else if (relPageIndex == PREVIOUS_MENU_ITEM_PAGE_INDEX)
				{
					// Prevent player from guessing the token and
					// changing page with a negative index:
					if (currentMenu->m_iCurrentPageStartIndex > 0)
					{
						// Start calculating previous page start index depending on amount of navigation items...
						int previousPageStartIndex = currentMenu->m_iCurrentPageStartIndex
							- MAX_VALVE_MENU_ITEMS_PER_PAGE + currentMenu->m_bHasParent + 1;

						if (previousPageStartIndex > 0)
						{
							// Previous page is not first one. Take account of existing space for "Previous" button
							previousPageStartIndex++;
						}

						currentMenu->ChangePage(previousPageStartIndex);
						engine->ClientCommand(pPlayer->edict(), "play ", PREVIOUS_MENU_ITEM_SOUND);
						return;
					}
				}
				else
				{
					// Start calculating next page start index depending on amount of navigation items... Limit it to menu's number of items
					int nextPageStartIndex = currentMenu->m_iCurrentPageStartIndex + MAX_VALVE_MENU_ITEMS_PER_PAGE
						- currentMenu->m_bHasParent - (currentMenu->m_iCurrentPageStartIndex > 0);

					if (nextPageStartIndex < currentMenu->m_iNumItems)
					{
						// Custom page items do not reach last global custom. Take account of existing space for "Next" button
						nextPageStartIndex--;
					}
					else
					{
						nextPageStartIndex = currentMenu->m_iNumItems;
					}

					// Custom menu item
					if (relPageIndex < MAX_VALVE_MENU_ITEMS_PER_PAGE)
					{
						// Prevent player from guessing the token and trying to trigger it without an existing custom item
						if (currentMenu->m_iCurrentPageStartIndex + relPageIndex < nextPageStartIndex)
						{
							currentMenu->CallHandler(currentMenu->m_iCurrentPageStartIndex + relPageIndex);
							engine->ClientCommand(pPlayer->edict(), "play %s", NORMAL_MENU_ITEM_SOUND);
							return;
						}
					}
					// "Next" item. Prevent player from guessing the token and trying to trigger it without a next page
					else if (relPageIndex == NEXT_MENU_ITEM_PAGE_INDEX && nextPageStartIndex < currentMenu->m_iNumItems)
					{
						currentMenu->ChangePage(nextPageStartIndex);
						engine->ClientCommand(pPlayer->edict(), "play %s", NEXT_MENU_ITEM_SOUND);
						return;
					}
				}
			}

			// Common index error throw place:
			ClientPrint(pPlayer, HUD_PRINTTALK, "Menu item error: invalid item index");
		}
	}
}

CON_COMMAND(entryboxtext, "<text> - Inputs entry box text")
{
	CHL2RP_Player *pPlayer = CHL2RP_Player::ToThisClassFast(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		CHL2RP_PlayerEntryBox *currentEntryBox = dynamic_cast<CHL2RP_PlayerEntryBox *>(pPlayer->m_pCurrentDialog);

		if (currentEntryBox != NULL)
		{
			char *text = (char *)args.ArgS();
			int textLen = Q_strlen(text);

			if (textLen && text[0] == '"' && text[textLen - 1] == '"')
			{
				text[textLen - 1] = '\0';
				text++;
			}

			currentEntryBox->CallHandler(text);
			engine->ClientCommand(pPlayer->edict(), "play %s", ENTRY_BOX_INPUT_SOUND);
		}
		else
			ClientPrint(pPlayer, HUD_PRINTTALK, "You can't do this because you don't have an active translation entry box");
	}
}
