#ifndef DISPENSER_MENU_H
#define DISPENSER_MENU_H
#pragma once

#include "PlayerDialog.h"

class CPropRationDispenser;

class CDispenserMenu : public CDialogRewindableMenu
{
	void RewindDialog(CHL2RP_Player* pPlayer) OVERRIDE;

public:
	CDispenserMenu(CHL2RP_Player* pPlayer);

	CPropRationDispenser* GetLookingDispenser(CHL2RP_Player* pPlayer);
	void TrySaveDispenser(CHL2RP_Player* pPlayer, CPropRationDispenser* pDispenser);
	void TryRemoveDispenser(CHL2RP_Player* pPlayer, CPropRationDispenser* pDispenser);

	CHandle<CPropRationDispenser> m_hDispenser;
};

class CDispenserCreateItem : public CMenuAwareItem<CDispenserMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CDispenserMoveCurrentItem : public CMenuAwareItem<CDispenserMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CDispenserDeleteCurrentItem : public CMenuAwareItem<CDispenserMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CDispenserDeleteAimedItem : public CMenuAwareItem<CDispenserMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CDispenserSaveCurrentItem : public CMenuAwareItem<CDispenserMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

class CDispenserSaveAimedItem : public CMenuAwareItem<CDispenserMenu>
{
	void HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer) OVERRIDE;
};

#endif // !DISPENSER_MENU_H
