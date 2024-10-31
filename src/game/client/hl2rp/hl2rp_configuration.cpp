// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_configuration.h"
#include <hl2rp_shareddefs.h>
#include <filesystem.h>
#include <ienginevgui.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/QueryBox.h>

#define HL2RP_CONFIGURATION_USER_DATA_FILE (HL2RP_CONFIG_PATH "user_data.txt")

using namespace vgui;

CHL2RPConfiguration gHL2RPConfiguration;

static VPANEL FindChildPanel(VPANEL parent, CUtlStringList& pathNames, int index = 0)
{
	if (index >= pathNames.Size())
	{
		return parent;
	}

	for (auto child : g_pVGuiPanel->GetChildren(parent))
	{
		if (Q_strcmp(g_pVGuiPanel->GetName(child), pathNames[index]) == 0)
		{
			return FindChildPanel(child, pathNames, index + 1);
		}
	}

	return 0;
}

static Panel* FindGameUIChildPanel(const char* pPath)
{
	CUtlStringList pathNames;
	Q_SplitString(pPath, "/", pathNames);
	return g_pVGuiPanel->GetPanel(FindChildPanel(enginevgui->GetPanel(PANEL_GAMEUIDLL), pathNames), "GameUI");
}

class CHUDHintsClearDialog : QueryBox
{
	void OnCommand(const char* pCommand) OVERRIDE
	{
		QueryBox::OnCommand(pCommand);

		if (Q_strcmp(pCommand, "OK") == 0)
		{
			gHL2RPConfiguration.mUserData->SetInt(HL2RP_LEARNED_HUD_HINTS_FIELD_NAME, 0);
			FindSiblingByName("HUDHints_Clear_Button")->SetEnabled(false);
			engine->ServerCmdKeyValues(new KeyValues(HL2RP_LEARNED_HUD_HINTS_UPDATE_EVENT)); // Notify implicit reset
		}
	}

public:
	CHUDHintsClearDialog(Panel* pParent) : QueryBox("#GameUI_Confirm", "#HL2RP_HUDHints_Clear_Warning", pParent)
	{
		SetName("HUDHints_Clear_Dialog");
		DoModal();
		m_pMessageLabel->SetName("HUDHints_Clear_Warning");
		LoadControlSettings("resource/hudhints_clear_dialog.res");
		m_pMessageLabel->SetSize(GetWide(), GetTall());
	}
};

class CGameExperiencePage : public PropertyPage
{
	void OnCommand(const char* pCommand) OVERRIDE
	{
		if (Q_strcmp(pCommand, "ClearHUDHints") == 0)
		{
			new CHUDHintsClearDialog(this);
			return;
		}

		PropertyPage::OnCommand(pCommand);
	}

public:
	CGameExperiencePage(Panel* pParent) : PropertyPage(pParent, "GameExperience")
	{
		SetWide(0); // Prevent game from complaining about unsized parent in debug mode
		new Button(this, "HUDHints_Clear_Button", "#HL2RP_HUDHints_Clear", this, "ClearHUDHints");
	}
};

class CRoleplayMenu : public PropertyDialog
{
	~CRoleplayMenu() OVERRIDE
	{
		gHL2RPConfiguration.mpRoleplayMenu = NULL;
	}

public:
	CRoleplayMenu() : PropertyDialog(FindGameUIChildPanel("BaseGameUIPanel"), "RoleplayMenu")
	{
		SetDeleteSelfOnClose(true);
		SetVisible(true);
		CGameExperiencePage* pPage = new CGameExperiencePage(this);
		LoadControlSettings("resource/roleplay_menu.res");
		AddPage(pPage, "#HL2RP_GameExperience");
		MoveToCenterOfScreen();
	}
};

CHL2RPConfiguration::CHL2RPConfiguration() : mUserData("UserData")
{

}

void CHL2RPConfiguration::PostInit()
{
	mUserData->LoadFromFile(filesystem, HL2RP_CONFIGURATION_USER_DATA_FILE);

#if 0 // NOTE: Disabled until game's SDK and GameUI controls ABI (vtable) get sync'ed again. Currently fails.
	// Force localize now the game menu buttons that couldn't be localized at startup (tied to mod's localization)
	Menu* pMainMenu = static_cast<Menu*>(FindGameUIChildPanel("BaseGameUIPanel/GameMenu"));

	for (int i = 0; i < pMainMenu->GetItemCount(); ++i)
	{
		char text[64];
		MenuItem* pItem = pMainMenu->GetMenuItem(i);
		pItem->GetText(text, sizeof(text));
		Q_StripPrecedingAndTrailingWhitespace(text);

		if (*text == '#')
		{
			pItem->SetText(text);
		}
	}
#endif // 0
}

void CHL2RPConfiguration::Shutdown()
{
	mUserData->SaveToFile(filesystem, HL2RP_CONFIGURATION_USER_DATA_FILE);
}

void CHL2RPConfiguration::CmdDisplayRoleplayMenu(const CCommand&)
{
	if (mpRoleplayMenu == NULL)
	{
		mpRoleplayMenu = new CRoleplayMenu;
	}

	mpRoleplayMenu->Activate();
}
