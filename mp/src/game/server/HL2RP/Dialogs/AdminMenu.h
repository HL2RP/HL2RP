#ifndef ADMIN_MENU_H
#define ADMIN_MENU_H
#pragma once

#include "PlayerDialog.h"

class CLocalizationManageItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CDispensersManageItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobsManageItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CAdminMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	// Checks if player is still admin. If not, displays main menu.
	static bool CheckAccess(CHL2RP_Player* pPlayer);

	CAdminMenu(CHL2RP_Player* pPlayer);
};

class CLanguageItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;

public:
	const char* m_pLangShortName;
};

class CLocalizationLanguageMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CLocalizationLanguageMenu(CHL2RP_Player* player);
};

class CLocalizationTokenItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;

public:
	const char* m_pLangShortName;
};

class CLocalizationTokenMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CLocalizationTokenMenu(CHL2RP_Player* player, const char* pLangShortName);
};

class CLocalizationValueEntryBox : public CEntryBox
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;
	void HandleFilledTextInput(CHL2RP_Player* pPlayer, const char* pText) OVERRIDE;

	const char* m_pLangShortName;
	char m_Token[PLAYERDIALOG_MENU_ITEM_DISPLAY_SIZE];

public:
	CLocalizationValueEntryBox(CHL2RP_Player* pPlayer, const char* pLangShortName, const char* pToken);
};

class CLocalizationSaveMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

	const char* m_pLangShortName;
	char m_Token[PLAYERDIALOG_MENU_ITEM_DISPLAY_SIZE];

public:
	CLocalizationSaveMenu(CHL2RP_Player* pPlayer, const char* pLangShortName,
		const char* pToken, const char* pRawTranslation);

	void SaveTranslation(CHL2RP_Player* pPlayer, const char* pRawTranslation);
	void RewindHeaderMenu(CHL2RP_Player* pPlayer);
};

class CLocalizationSaveItem : public CMenuAwareItem<CLocalizationSaveMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;

	autoloc_buf m_RawTranslation;

public:
	CLocalizationSaveItem(CLocalizationSaveMenu* pMenu, const char* pRawTranslation);
};

class CLocalizationDiscardItem : public CMenuAwareItem<CLocalizationSaveMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobAddItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobRemoveItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobManageMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CJobManageMenu(CHL2RP_Player* pPlayer);
};

class CJobRegisterCitizenItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobRegisterPoliceItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobRegisterMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CJobRegisterMenu(CHL2RP_Player* pPlayer);
};

class CJobAliasEntryBox : public CEntryBox
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;
	void HandleFilledTextInput(CHL2RP_Player* pPlayer, const char* pText) OVERRIDE;

public:
	CJobAliasEntryBox(CHL2RP_Player* pPlayer, int jobTeamIndex);

protected:
	int m_JobTeamIndex;
};

class CJobModelEntryBox : public CEntryBox
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;
	void HandleFilledTextInput(CHL2RP_Player* pPlayer, const char* pText) OVERRIDE;

	int m_JobTeamIndex;
	char m_Alias[PLAYERDIALOG_ENTRYBOX_INPUT_SIZE];

public:
	CJobModelEntryBox(CHL2RP_Player* pPlayer, const char* pAlias, int jobTeamIndex);
};

class CJobDiscardItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobSaveMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

	char m_Alias[PLAYERDIALOG_ENTRYBOX_INPUT_SIZE], m_Path[MAX_PATH];
	int m_JobTeamIndex;

public:
	CJobSaveMenu(CHL2RP_Player* pPlayer, const char* pAlias, const char* pPath, int jobTeamIndex);

	void SaveJob(CHL2RP_Player* pPlayer);
};

class CJobSaveItem : public CMenuAwareItem<CJobSaveMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobRemoveCitizenItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobRemovePoliceItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

// Shows job team selection for job removal (afterwards)
class CJobRemoveTeamMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CJobRemoveTeamMenu(CHL2RP_Player* pPlayer);
};

// Shows the jobs for a specific team and awaiting removal
class CJobRemoveAliasMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

	int m_JobTeamIndex;

public:
	CJobRemoveAliasMenu(CHL2RP_Player* pPlayer, int jobTeamIndex);

	void RemoveJob(CHL2RP_Player* pPlayer, const char* pAlias);
};

class CJobRemoveAliasItem : public CMenuAwareItem<CJobRemoveAliasMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

#endif // !ADMIN_MENU_H
