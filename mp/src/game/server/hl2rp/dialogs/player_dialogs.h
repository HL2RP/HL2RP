#ifndef PLAYER_DIALOGS_H
#define PLAYER_DIALOGS_H
#pragma once

#include "inetwork_dialog.h"
#include <idto.h>

class CRationDispenserProp;

class CMainMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		Inventory,
		ChangeJob,
		ChangeModel,
		Actions,
		Settings,
		Admin
	);

	void UpdateItems() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CMainMenu(CHL2Roleplayer*);
};

class CInventoryMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		HiddenWeapons
	);

	void UpdateItems() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CInventoryMenu(CHL2Roleplayer*);
};

class CFactionChangeMenu : public CNetworkMenu
{
	void UpdateItems() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CFactionChangeMenu(CHL2Roleplayer*);
};

class CJobChangeMenu : public CNetworkMenu
{
	void UpdateItems() OVERRIDE;
	void Think() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

	int mFaction;

public:
	CJobChangeMenu(CHL2Roleplayer*, int faction);
};

class CModelGroupChangeMenu : public CNetworkMenu
{
	void UpdateItems() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CModelGroupChangeMenu(CHL2Roleplayer*);
};

class CModelChangeMenu : public CNetworkMenu
{
	void Think() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

	int mModelGroupIndex;

public:
	CModelChangeMenu(CHL2Roleplayer*, int groupIndex);
};

class CActionsMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		LowerWeapon,
		HideWeapon,
		DropWeapon
	);

	void UpdateItems() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CActionsMenu(CHL2Roleplayer*);
};

class CSettingsMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		ClearHUDHints,
		Region
	);

	void SelectItem(CItem*) OVERRIDE;

public:
	CSettingsMenu(CHL2Roleplayer*);
};

class CRegionSettingsMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		EnableList,
		DisableList,
		SetRegionVoice,
		SetGlobalVoice
	);

	void UpdateItems() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CRegionSettingsMenu(CHL2Roleplayer*);
};

#ifdef HL2RP_LEGACY
class CHiddenWeaponsMenu : public CNetworkMenu
{
	void UpdateItems() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CHiddenWeaponsMenu(CHL2Roleplayer*);
};

class CHUDHintsClearMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		Accept,
		Cancel
	);

	void SelectItem(CItem*) OVERRIDE;

public:
	CHUDHintsClearMenu(CHL2Roleplayer*);
};
#endif // HL2RP_LEGACY

// The top admin menu. All other admin-only menus should inherit from it to get player auto kick functionality.
class CAdminMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		ManageDispensers
	);

	void UpdateItems() OVERRIDE;
	void Think() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;

public:
	CAdminMenu(CHL2Roleplayer*);
};

class CDispensersMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		Create,
		SetRations,
		SetCC,
		UnsetCC,
		MoveFront,
		MoveBack,
		Lock,
		Unlock,
		Delete,
		SelectAiming
	);

	void UpdateItems() OVERRIDE;
	void Think() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;
	void HandleChildCommand(const CCommand&, int) OVERRIDE;

	CHandle<CRationDispenserProp> mhDispenser;
	SDatabaseIdDTO mDispenserId; // For detecting DB insertion finish
	bool mAllowManageOthers = true;

public:
	CDispensersMenu(CHL2Roleplayer*, CRationDispenserProp*);
};

#endif // !PLAYER_DIALOGS_H