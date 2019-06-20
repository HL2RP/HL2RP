#include <cbase.h>
#include "AdminMenu.h"
#include "DispenserMenu.h"
#include <HL2RPRules.h>
#include <HL2RP_Player.h>
#include <DAL/JobDAO.h>
#include <language.h>
#include <DAL/PhraseDAO.h>
#include "PublicDialogs.h"
#include <utlbuffer.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define HL2RP_LOC_TOKEN_MENU_ITEM_DISCARD	"#HL2RP_Menu_Item_Discard"
#define HL2RP_LOC_TOKEN_MENU_ITEM_SAVE	"#HL2RP_Menu_Item_Save"

void CLocalizationManageItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CLocalizationLanguageMenu>();
	}
}

void CDispensersManageItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CDispenserMenu>();
	}
}

void CJobsManageItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CJobManageMenu>();
	}
}

// Checks if player is still admin. If not, displays main menu.
bool CAdminMenu::CheckAccess(CHL2RP_Player* pPlayer)
{
	if (pPlayer->IsAdmin())
	{
		return true;
	}

	ClientPrint(pPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
		Localize(pPlayer, "#HL2RP_Chat_Admin_Menu_Deny"));
	pPlayer->DisplayDialog<CMainMenu>();
	return false;
}

CAdminMenu::CAdminMenu(CHL2RP_Player* pPlayer) : CDialogRewindableMenu(pPlayer,
	GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Admin"))
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Admin"));
	AddItem(pPlayer, (new CLocalizationManageItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Phrase_Translate"));
	AddItem(pPlayer, (new CDispensersManageItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Dispensers_Manage"));
	AddItem(pPlayer, (new CJobsManageItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Jobs_Manage"));
}

void CAdminMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CMainMenu>();
}

void CLanguageItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CLocalizationTokenMenu>(m_pLangShortName);
	}
}

CLocalizationLanguageMenu::CLocalizationLanguageMenu(CHL2RP_Player* pPlayer) : CDialogRewindableMenu(pPlayer,
	GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Phrase_Language"))
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Phrase_Language"));

	FOR_EACH_LANGUAGE(eLang)
	{
		CLanguageItem* pItem = new CLanguageItem();
		pItem->SetLocalizedDisplay(pPlayer, GetLanguageVGUILocalization(static_cast<ELanguage>(eLang)));
		pItem->m_pLangShortName = GetLanguageShortName(static_cast<ELanguage>(eLang));
		AddItem(pPlayer, pItem);
	}
}

void CLocalizationLanguageMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CAdminMenu>();
}

void CLocalizationTokenItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CLocalizationValueEntryBox>(m_pLangShortName, m_Display);
	}
}

CLocalizationTokenMenu::CLocalizationTokenMenu(CHL2RP_Player* pPlayer, const char* pLangShortName)
	: CDialogRewindableMenu(pPlayer, GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Phrase_Header"))
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Phrase_Header"));
	KeyValues* pServerLangPhrases = GetHL2RPAutoLocalizer().m_pServerPhrases->
		FindKey(AUTOLOCALIZER_DEFAULT_LANGUAGE);

	if (pServerLangPhrases != NULL)
	{
		FOR_EACH_SUBKEY(pServerLangPhrases, pPhrase)
		{
			CLocalizationTokenItem* pItem = new CLocalizationTokenItem;
			pItem->SetDisplay(pPhrase->GetName());
			pItem->m_pLangShortName = pLangShortName;
			AddItem(pPlayer, pItem);
		}
	}
}

void CLocalizationTokenMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CLocalizationLanguageMenu>();
}

CLocalizationValueEntryBox::CLocalizationValueEntryBox(CHL2RP_Player* pPlayer,
	const char* pLangShortName, const char* pToken)
	: CEntryBox(pPlayer, GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_EntryBox_Title_Phrase_Translation")),
	m_pLangShortName(pLangShortName)
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_EntryBox_Msg_Phrase_Translation"));
	Q_strcpy(m_Token, pToken);
	autoloc_buf_ref msg = GetHL2RPAutoLocalizer().Localize(pPlayer,
		"#HL2RP_Console_Default_Translation_Hint", pToken);
	KeyValues* pServerLangPhrases = GetHL2RPAutoLocalizer().m_pServerPhrases->
		FindKey(AUTOLOCALIZER_DEFAULT_LANGUAGE);

	if (pServerLangPhrases != NULL)
	{
		Q_strncat(msg, pServerLangPhrases->GetString(pToken), sizeof(msg));
	}

	ClientPrint(pPlayer, HUD_PRINTTALK, "%s1", msg);
}

void CLocalizationValueEntryBox::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CLocalizationTokenMenu>(m_pLangShortName);
}

void CLocalizationValueEntryBox::HandleFilledTextInput(CHL2RP_Player* pPlayer, const char* pText)
{
	CUtlBuffer buf(NULL, 0, CUtlBuffer::TEXT_BUFFER);
	buf.PutDelimitedString(GetNoEscCharConversion(), pText);
	autoloc_buf localized;
	buf.GetDelimitedString(GetCStringCharConversion(), localized, sizeof(localized));
	pPlayer->DisplayDialog<CLocalizationSaveMenu>(m_pLangShortName, m_Token, localized);
}

CLocalizationSaveMenu::CLocalizationSaveMenu
(CHL2RP_Player* pPlayer, const char* pLangShortName, const char* pToken, const char* pRawTranslation)
	: CDialogRewindableMenu(pPlayer, GetHL2RPAutoLocalizer().
		Localize(pPlayer, "#HL2RP_Menu_Title_Phrase_Save")), m_pLangShortName(pLangShortName)
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Phrase_Save", pRawTranslation));
	Q_strcpy(m_Token, pToken);
	AddItem(pPlayer, (new CLocalizationSaveItem(this, pRawTranslation))->
		SetLocalizedDisplay(pPlayer, HL2RP_LOC_TOKEN_MENU_ITEM_SAVE));
	AddItem(pPlayer, (new CLocalizationDiscardItem)->Link(this)->
		SetLocalizedDisplay(pPlayer, HL2RP_LOC_TOKEN_MENU_ITEM_DISCARD));
}

void CLocalizationSaveMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CLocalizationValueEntryBox>(m_pLangShortName, m_Token);
}

void CLocalizationSaveMenu::SaveTranslation(CHL2RP_Player* pPlayer, const char* pRawTranslation)
{
	GetHL2RPAutoLocalizer().SetRawTranslation(m_pLangShortName, m_Token, pRawTranslation);
	TryCreateAsyncDAO<CPhraseSaveDAO>(m_pLangShortName, m_Token, pRawTranslation);
	Display(pPlayer);
}

void CLocalizationSaveMenu::DiscardTranslation(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CLocalizationTokenMenu>(m_pLangShortName);
}

CLocalizationSaveItem::CLocalizationSaveItem(CLocalizationSaveMenu* pMenu,
	const char* pRawTranslation) : CMenuAwareItem(pMenu)
{
	Q_strncpy(m_RawTranslation, pRawTranslation, sizeof(m_RawTranslation));
}

void CLocalizationSaveItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		m_pMenu->SaveTranslation(pPlayer, m_RawTranslation);
	}
}

void CLocalizationDiscardItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	m_pMenu->DiscardTranslation(pPlayer);
}

void CJobAddItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CJobRegisterMenu>();
	}
}

void CJobRemoveItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CJobRemoveTeamMenu>();
	}
}

CJobManageMenu::CJobManageMenu(CHL2RP_Player* pPlayer) : CDialogRewindableMenu(pPlayer,
	GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Jobs_Manage"))
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Jobs_Manage"));
	AddItem(pPlayer, (new CJobAddItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Job_Add"));
	AddItem(pPlayer, (new CJobRemoveItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Job_Remove"));
}

void CJobManageMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CAdminMenu>();
}

void CJobRegisterCitizenItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CJobAliasEntryBox>(EJobTeamIndex::Citizen);
	}
}

void CJobRegisterPoliceItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CJobAliasEntryBox>(EJobTeamIndex::Police);
	}
}

CJobRegisterMenu::CJobRegisterMenu(CHL2RP_Player* pPlayer)
	: CDialogRewindableMenu(pPlayer, GetHL2RPAutoLocalizer().
		Localize(pPlayer, "#HL2RP_Menu_Title_Job_Add"))
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "HL2RP_Menu_Msg_Job_Add"));
	AddItem(pPlayer, (new CJobRegisterCitizenItem)->SetDisplay("Citizen"));
	AddItem(pPlayer, (new CJobRegisterPoliceItem)->SetDisplay("Police"));
}

void CJobRegisterMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobManageMenu>();
}

CJobAliasEntryBox::CJobAliasEntryBox(CHL2RP_Player* pPlayer, int jobTeamIndex) :
	CEntryBox(pPlayer, GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_EntryBox_Title_Job_Alias")),
	m_JobTeamIndex(jobTeamIndex)
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_EntryBox_Msg_Job_Alias"));
}

void CJobAliasEntryBox::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobRegisterMenu>();
}

void CJobAliasEntryBox::HandleFilledTextInput(CHL2RP_Player* pPlayer, const char* pText)
{
	pPlayer->DisplayDialog<CJobModelEntryBox>(pText, m_JobTeamIndex);
}

CJobModelEntryBox::CJobModelEntryBox(CHL2RP_Player* pPlayer, const char* pAlias, int jobTeamIndex)
	: CEntryBox(pPlayer, GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_EntryBox_Title_Job_Model")),
	m_JobTeamIndex(jobTeamIndex)
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_EntryBox_Msg_Job_Model"));
	Q_strncpy(m_Alias, pAlias, sizeof(m_Alias));
}

void CJobModelEntryBox::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobAliasEntryBox>(m_JobTeamIndex);
}

void CJobModelEntryBox::HandleFilledTextInput(CHL2RP_Player* pPlayer, const char* pText)
{
	if (filesystem->FileExists(pText))
	{
		return pPlayer->DisplayDialog<CJobSaveMenu>(m_Alias, pText, m_JobTeamIndex);
	}

	// Path doesn't exist, show dialog to input different path
	pPlayer->DisplayDialog<CJobModelEntryBox>(m_Alias, m_JobTeamIndex);
}

CJobSaveMenu::CJobSaveMenu(CHL2RP_Player* pPlayer, const char* pAlias, const char* pPath, int jobTeamIndex)
	: CDialogRewindableMenu(pPlayer, GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Job_Save")),
	m_JobTeamIndex(jobTeamIndex)
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Job_Save"));
	Q_strcpy(m_Alias, pAlias);
	Q_strcpy(m_Path, pPath);
	AddItem(pPlayer, (new CJobSaveItem)->Link(this)->SetLocalizedDisplay(pPlayer, HL2RP_LOC_TOKEN_MENU_ITEM_SAVE));
	AddItem(pPlayer, (new CJobDiscardItem)->SetLocalizedDisplay(pPlayer, HL2RP_LOC_TOKEN_MENU_ITEM_DISCARD));
}

void CJobSaveMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobModelEntryBox>(m_Alias, m_JobTeamIndex);
}

void CJobSaveMenu::SaveJob(CHL2RP_Player* pPlayer)
{
	if (HL2RPRules()->m_JobTeamsModels[m_JobTeamIndex].HasElement(m_Alias))
	{
		ClientPrint(pPlayer, HUD_PRINTTALK, GetHL2RPAutoLocalizer().
			Localize(pPlayer, "#HL2RP_Menu_Msg_Job_Save_Failed"));
		return Display(pPlayer);
	}

	CBaseEntity::PrecacheModel(m_Path);
	SJob job(m_Path);
	HL2RPRules()->m_JobTeamsModels[m_JobTeamIndex].Insert(m_Alias, job);
	TryCreateAsyncDAO<CJobSaveDAO>(m_Alias, m_Path, m_JobTeamIndex);
}

void CJobSaveItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		m_pMenu->SaveJob(pPlayer);
	}
}

void CJobDiscardItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobManageMenu>();
}

void CJobRemoveCitizenItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CJobRemoveAliasMenu>(EJobTeamIndex::Citizen);
	}
}

void CJobRemovePoliceItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CJobRemoveAliasMenu>(EJobTeamIndex::Police);
	}
}

CJobRemoveTeamMenu::CJobRemoveTeamMenu(CHL2RP_Player* pPlayer) : CDialogRewindableMenu(pPlayer,
	GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Job_Main_Remove"))
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Job_Main_Remove"));
	AddItem(pPlayer, (new CJobRemoveCitizenItem)->SetDisplay("Citizen"));
	AddItem(pPlayer, (new CJobRemoveCitizenItem)->SetDisplay("Police"));
}

void CJobRemoveTeamMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobManageMenu>();
}

CJobRemoveAliasMenu::CJobRemoveAliasMenu(CHL2RP_Player* pPlayer, int jobTeamIndex)
	: CDialogRewindableMenu(pPlayer), m_JobTeamIndex(jobTeamIndex)
{
	FOR_EACH_DICT_FAST(HL2RPRules()->m_JobTeamsModels[jobTeamIndex], i)
	{
		if (!HL2RPRules()->m_JobTeamsModels[jobTeamIndex][i].m_IsReadOnly)
		{
			AddItem(pPlayer, (new CJobRemoveAliasItem)->Link(this)->
				SetDisplay(HL2RPRules()->m_JobTeamsModels[jobTeamIndex].GetElementName(i)));
		}
	}
}

void CJobRemoveAliasMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobRemoveTeamMenu>();
}

void CJobRemoveAliasMenu::RemoveJob(CHL2RP_Player* pPlayer, const char* pAlias)
{
	int jobIndex = HL2RPRules()->m_JobTeamsModels[m_JobTeamIndex].Find(pAlias);

	if (HL2RPRules()->m_JobTeamsModels[m_JobTeamIndex].IsValidIndex(jobIndex))
	{
		HL2RPRules()->m_JobTeamsModels[m_JobTeamIndex].RemoveAt(jobIndex);
		TryCreateAsyncDAO<CJobDeleteDAO>(pAlias);
	}

	pPlayer->DisplayDialog<CJobRemoveAliasMenu>(m_JobTeamIndex);
}

void CJobRemoveAliasItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		m_pMenu->RemoveJob(pPlayer, m_Display);
	}
}

void DenyAdminMenuAccess(CHL2RP_Player* pPlayer)
{
	ClientPrint(pPlayer, HUD_PRINTTALK, "Sorry, you don't have admin rights anymore");
	pPlayer->DisplayDialog<CMainMenu>();
}
