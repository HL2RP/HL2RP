#ifndef PLAYER_DIALOGS_H
#define PLAYER_DIALOGS_H
#pragma once

#include "inetwork_dialog.h"
#include <hl2rp_util_shared.h>

class CHL2RP_Property;
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

class CPropertyDoorMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		CreateProperty,
		SetPropertyName,
		LinkZone,
		UnlinkZone,
		LinkNewDoor,
		UnlinkDoor,
		LinkToMap,
		LinkToMapGroup,
		BuyHouse,
		SellHouse,
		SetDoorName,
		GiveKey,
		ViewOrTakeKeys,
		LockDoor,
		UnlockDoor,
		SelectDoor,
		DeleteProperty
	);

	void UpdateItems() OVERRIDE;
	void Think() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;
	void HandleChildNotice(int, const SUtlField&) OVERRIDE;

	bool ValidateProperty();
	bool IsDoorSaved(CHL2RP_PropertyDoorData*&);
	void LinkToMapAlias(const char*);

	CHL2RP_Property* mpProperty = NULL;
	EHANDLE mhDoor;
	SDatabaseId mPropertyId, mDoorId;

public:
	CPropertyDoorMenu(CHL2Roleplayer*, CBaseEntity*);
};

class CMapGroupMenu : public CNetworkMenu
{
public:
	CMapGroupMenu(CHL2Roleplayer*, int action);
};

class CAdminMenu : public CNetworkMenu
{
	SCOPED_ENUM(EItemAction,
		ManageDispensers
	);

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
		LinkToMap,
		LinkToMapGroup,
		Delete,
		SelectAiming
	);

	void UpdateItems() OVERRIDE;
	void Think() OVERRIDE;
	void SelectItem(CItem*) OVERRIDE;
	void HandleChildNotice(int, const SUtlField&) OVERRIDE;

	void LinkToMapAlias(const char*);

	CHandle<CRationDispenserProp> mhDispenser;
	SDatabaseId mDispenserId; // For detecting DB insertion finish
	bool mAllowCreationAsAdmin = true;

public:
	CDispensersMenu(CHL2Roleplayer*, CRationDispenserProp*);
};

#endif // !PLAYER_DIALOGS_H
