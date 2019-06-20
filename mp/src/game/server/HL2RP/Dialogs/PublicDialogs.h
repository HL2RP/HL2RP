#ifndef PUBLIC_DIALOGS_H
#define PUBLIC_DIALOGS_H
#pragma once

#include "PlayerDialog.h"

#ifdef HL2DM_RP
class CSpecialWeaponsMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CSpecialWeaponsMenu(CHL2RP_Player* pPlayer);
};

class CSpecialWeaponsEnterItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CSpecialWeaponPickItem : public CMenuAwareItem<CSpecialWeaponsMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;

	CHandle<CBaseCombatWeapon> m_hWeapon;

public:
	CSpecialWeaponPickItem(CSpecialWeaponsMenu* pMenu, CHandle<CBaseCombatWeapon> weaponHandle);
};
#endif // HL2DM_RP

class CJobSwitchEnterItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CAdminMenuItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CMainMenu : public CMenu
{
public:
	CMainMenu(CHL2RP_Player* player);
};

class CJobSwitchCitizenItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobSwitchPoliceItem : public CMenuItem
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CJobSwitchTeamMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CJobSwitchTeamMenu(CHL2RP_Player* pPlayer);
};

class CJobSwitchAliasMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

	int m_iJobTeamIndex;

public:
	CJobSwitchAliasMenu(CHL2RP_Player* pPlayer, int jobTeamIndex);

	void SwitchToJob(CHL2RP_Player* pPlayer, const char* pAlias);
};

class CJobSwitchAliasItem : public CMenuAwareItem<CJobSwitchAliasMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

#endif // !PUBLIC_DIALOGS_H
