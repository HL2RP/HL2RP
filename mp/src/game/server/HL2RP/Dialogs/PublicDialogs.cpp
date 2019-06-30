#include <cbase.h>
#include "PublicDialogs.h"
#include "AdminMenu.h"
#include <HL2RPRules.h>
#include <HL2RP_Player.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifdef HL2DM_RP
CSpecialWeaponsMenu::CSpecialWeaponsMenu(CHL2RP_Player* pPlayer) : CDialogRewindableMenu(pPlayer,
	GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Special_Weapons"))
{
	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Special_Weapons"));

	for (int i = MAX_WEAPONS - 1; i >= 0 && pPlayer->GetWeapon(i) != NULL; i--)
	{
		if (!pPlayer->GetWeapon(i)->VisibleInWeaponSelection())
		{
			AddItem(pPlayer, (new CSpecialWeaponPickItem(this, pPlayer->GetWeapon(i)))->
				SetLocalizedDisplay(pPlayer, pPlayer->GetWeapon(i)->GetPrintName()));
		}
	}
}

void CSpecialWeaponsMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CMainMenu>();
}

void CSpecialWeaponsEnterItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CSpecialWeaponsMenu>();
}

CSpecialWeaponPickItem::CSpecialWeaponPickItem(CSpecialWeaponsMenu* pMenu,
	CHandle<CBaseCombatWeapon> weaponHandle) : CMenuAwareItem(pMenu), m_hWeapon(weaponHandle)
{

}

void CSpecialWeaponPickItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (m_hWeapon != NULL)
	{
		pPlayer->Weapon_Switch(m_hWeapon);
	}

	// Make new menu to refresh owned weapons
	pPlayer->DisplayDialog<CSpecialWeaponsMenu>();
}
#endif // HL2DM_RP

void CJobSwitchEnterItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobSwitchTeamMenu>();
}

void CAdminMenuItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		pPlayer->DisplayDialog<CAdminMenu>();
	}
}

CMainMenu::CMainMenu(CHL2RP_Player* pPlayer)
	: CMenu(pPlayer, GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Title_Main"))
{
#ifdef HL2DM_RP
	AddItem(pPlayer, (new CSpecialWeaponsEnterItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Special_Weapons"));
#endif // HL2DM_RP

	SetDescription(GetHL2RPAutoLocalizer().Localize(pPlayer, "#HL2RP_Menu_Msg_Main"));
	AddItem(pPlayer, (new CJobSwitchEnterItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Job_Switch"));

	if (pPlayer->IsAdmin())
	{
		AddItem(pPlayer, (new CAdminMenuItem)->SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Admin"));
	}
}

void CJobSwitchCitizenItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobSwitchAliasMenu>(EJobTeamIndex::Citizen);
}

void CJobSwitchPoliceItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobSwitchAliasMenu>(EJobTeamIndex::Police);
}

CJobSwitchTeamMenu::CJobSwitchTeamMenu(CHL2RP_Player* pPlayer)
	: CDialogRewindableMenu(pPlayer, GetHL2RPAutoLocalizer().
		Localize(pPlayer, "#HL2RP_Menu_Title_Job_Switch"))
{
	AddItem(pPlayer, (new CJobSwitchCitizenItem)->SetDisplay("Citizen"));
	AddItem(pPlayer, (new CJobSwitchPoliceItem)->SetDisplay("Police"));
}

void CJobSwitchTeamMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CMainMenu>();
}

CJobSwitchAliasMenu::CJobSwitchAliasMenu(CHL2RP_Player* pPlayer, int jobTeamIndex)
	: CDialogRewindableMenu(pPlayer, GetHL2RPAutoLocalizer().
		Localize(pPlayer, "#HL2RP_Menu_Title_Job_Switch")), m_iJobTeamIndex(jobTeamIndex)
{
	FOR_EACH_DICT_FAST(HL2RPRules()->m_JobTeamsModels[jobTeamIndex], i)
	{
		AddItem(pPlayer, (new CJobSwitchAliasItem)->Link(this)->
			SetDisplay(HL2RPRules()->m_JobTeamsModels[jobTeamIndex].GetElementName(i)));
	}
}

void CJobSwitchAliasMenu::SwitchToJob(CHL2RP_Player* pPlayer, const char* pAlias)
{
	int jobAliasIndex = HL2RPRules()->m_JobTeamsModels[m_iJobTeamIndex].Find(pAlias);

	if (HL2RPRules()->m_JobTeamsModels[m_iJobTeamIndex].IsValidIndex(jobAliasIndex))
	{
		pPlayer->SetModel(HL2RPRules()->m_JobTeamsModels[m_iJobTeamIndex][jobAliasIndex].m_ModelPath);
	}

	Display(pPlayer);
}

void CJobSwitchAliasMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CJobSwitchTeamMenu>();
}

void CJobSwitchAliasItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	m_pMenu->SwitchToJob(pPlayer, m_Display);
}
