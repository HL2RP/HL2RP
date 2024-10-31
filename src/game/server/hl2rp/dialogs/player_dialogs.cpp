// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "player_dialogs.h"
#include <hl2_roleplayer.h>
#include <hl2rp_gamerules.h>
#include <hl2rp_localizer.h>
#include <hl2rp_property.h>
#include <dal.h>
#include <hl2rp_property_dao.h>
#include <prop_ration_dispenser.h>
#include <ration_dispenser_dao.h>

#define DISPENSER_MANAGE_MOVE_STEP_DIST 1.0f

extern ConVar gMaxMapPlayerHomesCVar, gMaxHomeKeysCVar;

CMainMenu::CMainMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_Main_Title")
{
	AddItem(EItemAction::Inventory, "#HL2RP_Menu_Inventory");
	AddItem(EItemAction::ChangeJob, "#HL2RP_Menu_ChangeJob");
	AddItem(EItemAction::ChangeModel, "#HL2RP_Menu_ChangeModel");
	AddItem(EItemAction::Actions, "#HL2RP_Menu_Actions");
	AddItem(EItemAction::Settings, "#HL2RP_Menu_Settings");
}

void CMainMenu::UpdateItems()
{
	RemoveItemByAction(EItemAction::Admin);

	if (mpPlayer->IsAdmin())
	{
		AddItem(EItemAction::Admin, "#HL2RP_Menu_Admin");
	}
}

void CMainMenu::SelectItem(CItem* pItem)
{
	switch (pItem->mAction)
	{
	case EItemAction::Inventory:
	{
		return mpPlayer->PushAndSendDialog(new CInventoryMenu(mpPlayer));
	}
	case EItemAction::ChangeJob:
	{
		return mpPlayer->PushAndSendDialog(new CFactionChangeMenu(mpPlayer));
	}
	case EItemAction::ChangeModel:
	{
		return mpPlayer->PushAndSendDialog(new CModelGroupChangeMenu(mpPlayer));
	}
	case EItemAction::Actions:
	{
		return mpPlayer->PushAndSendDialog(new CActionsMenu(mpPlayer));
	}
	case EItemAction::Settings:
	{
		return mpPlayer->PushAndSendDialog(new CSettingsMenu(mpPlayer));
	}
	case EItemAction::Admin:
	{
		return mpPlayer->PushAndSendDialog(new CAdminMenu(mpPlayer));
	}
	}
}

CInventoryMenu::CInventoryMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_Inventory")
{

}

void CInventoryMenu::UpdateItems()
{
#ifdef HL2RP_LEGACY
	RemoveItemByAction(EItemAction::HiddenWeapons);

	for (int i = 0; i < mpPlayer->WeaponCount(); ++i)
	{
		if (mpPlayer->GetWeapon(i) != NULL && !mpPlayer->GetWeapon(i)->VisibleInWeaponSelection())
		{
			AddItem(EItemAction::HiddenWeapons, "#HL2RP_Menu_HiddenWeapons");
			break;
		}
	}
#endif // HL2RP_LEGACY
}

void CInventoryMenu::SelectItem(CItem* pItem)
{
	switch (pItem->mAction)
	{
#ifdef HL2RP_LEGACY
	case EItemAction::HiddenWeapons:
	{
		return mpPlayer->PushAndSendDialog(new CHiddenWeaponsMenu(mpPlayer));
	}
#endif // HL2RP_LEGACY
	case EItemAction::_Count:
	{
		break;
	}
	}
}

CFactionChangeMenu::CFactionChangeMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_Faction")
{
	AddItem(EFaction::Citizen, "#HL2RP_Faction_Citizen");
}

void CFactionChangeMenu::UpdateItems()
{
	RemoveItemByAction(EFaction::Combine);

	if (mpPlayer->HasFactionAccess(EFaction::Combine))
	{
		AddItem(EFaction::Combine, "#HL2RP_Faction_Combine");
	}
}

void CFactionChangeMenu::SelectItem(CItem* pItem)
{
	if (mpPlayer->HasFactionAccess(pItem->mAction))
	{
		if (HL2RPRules()->mJobByName[pItem->mAction].Count() > 0)
		{
			return mpPlayer->PushAndSendDialog(new CJobChangeMenu(mpPlayer, pItem->mAction));
		}

		mpPlayer->ChangeFaction(pItem->mAction);
	}

	Send();
}

CJobChangeMenu::CJobChangeMenu(CHL2Roleplayer* pPlayer, int faction)
	: CNetworkMenu(pPlayer, "#HL2RP_Menu_ChangeJob"), mFaction(faction)
{

}

void CJobChangeMenu::UpdateItems()
{
	RemoveAllItems();
	auto& jobs = HL2RPRules()->mJobByName[mFaction];

	FOR_EACH_MAP_FAST(jobs, i)
	{
		if (mpPlayer->HasJobAccess(jobs[i]))
		{
			AddItem(i, jobs.Key(i));
		}
	}
}

void CJobChangeMenu::Think()
{
	if (!mpPlayer->HasFactionAccess(mFaction))
	{
		mpPlayer->RewindCurrentDialog();
	}
}

void CJobChangeMenu::SelectItem(CItem* pItem)
{
	if (mpPlayer->HasJobAccess(HL2RPRules()->mJobByName[mFaction][pItem->mAction]))
	{
		string_t oldJobName = mpPlayer->mJobName;
		mpPlayer->mJobName = MAKE_STRING(HL2RPRules()->mJobByName[mFaction].Key(pItem->mAction));

		if (mFaction != mpPlayer->mFaction)
		{
			mpPlayer->ChangeFaction(mFaction, mpPlayer->mJobName);
		}
		// If changing Combine job, suicide to respawn with updated equipment (classic behavior)
		else if (mFaction == EFaction::Combine && mpPlayer->mJobName.Get() != oldJobName)
		{
			mpPlayer->CommitSuicide();
		}
	}

	Send();
}

CModelGroupChangeMenu::CModelGroupChangeMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_Model_Group")
{

}

void CModelGroupChangeMenu::UpdateItems()
{
	RemoveAllItems();
	auto& models = HL2RPRules()->mPlayerModelsByGroup;
	auto& jobs = HL2RPRules()->mJobByName[mpPlayer->mFaction];
	int index = jobs.Find(mpPlayer->mJobName->ToCStr());

	// Add job models
	if (jobs.IsValidIndex(index))
	{
		FOR_EACH_DICT_FAST(jobs[index]->mModelGroupIndices, i)
		{
			int groupIndex = jobs[index]->mModelGroupIndices[i];

			// Check to prevent duplication later
			if (!mpPlayer->HasModelGroupAccess(models.InvalidIndex(), models[groupIndex]->mType))
			{
				AddItem(groupIndex, models.Key(groupIndex));
			}
		}
	}

	// Add other granted models
	for (int i = EPlayerModelType::Citizen; i < ARRAYSIZE(models.mGroupIndices); ++i)
	{
		if (mpPlayer->HasModelGroupAccess(models.InvalidIndex(), i))
		{
			for (auto groupIndex : models.mGroupIndices[i])
			{
				AddItem(groupIndex, models.Key(groupIndex));
			}
		}
	}
}

void CModelGroupChangeMenu::SelectItem(CItem* pItem)
{
	if (mpPlayer->HasModelGroupAccess(pItem->mAction))
	{
		if (HL2RPRules()->mPlayerModelsByGroup[pItem->mAction]->Count() > 1)
		{
			return mpPlayer->PushAndSendDialog(new CModelChangeMenu(mpPlayer, pItem->mAction));
		}

		mpPlayer->SetModel(pItem->mAction, 0);
	}

	Send();
}

CModelChangeMenu::CModelChangeMenu(CHL2Roleplayer* pPlayer, int groupIndex)
	: CNetworkMenu(pPlayer, "#HL2RP_Menu_ChangeModel"), mModelGroupIndex(groupIndex)
{
	FOR_EACH_DICT_FAST(*HL2RPRules()->mPlayerModelsByGroup[groupIndex], i)
	{
		AddItem(i, HL2RPRules()->mPlayerModelsByGroup[groupIndex]->GetElementName(i));
	}
}

void CModelChangeMenu::Think()
{
	if (!mpPlayer->HasModelGroupAccess(mModelGroupIndex))
	{
		mpPlayer->RewindCurrentDialog();
	}
}

void CModelChangeMenu::SelectItem(CItem* pItem)
{
	mpPlayer->SetModel(mModelGroupIndex, pItem->mAction);
	Send();
}

CActionsMenu::CActionsMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_Actions")
{

}

void CActionsMenu::UpdateItems()
{
	RemoveAllItems();

	if (mpPlayer->GetActiveWeapon() != NULL)
	{
		AddItem(EItemAction::LowerWeapon, "#HL2RP_Menu_Action_LowerWeapon");
		AddItem(EItemAction::HideWeapon, "#HL2RP_Menu_Action_HideWeapon");

		if (mpPlayer->mFaction == EFaction::Citizen)
		{
			AddItem(EItemAction::DropWeapon, "#HL2RP_Menu_Action_DropWeapon");
		}
	}
}

void CActionsMenu::SelectItem(CItem* pItem)
{
	if (mpPlayer->GetActiveWeapon() != NULL)
	{
		switch (pItem->mAction)
		{
		case EItemAction::LowerWeapon:
		{
			mpPlayer->m_LowerWeaponTimer.Force();
			mpPlayer->mIsWeaponManuallyLowered = true;
			break;
		}
		case EItemAction::DropWeapon:
		{
			if (mpPlayer->mFaction == EFaction::Citizen)
			{
				mpPlayer->Weapon_Drop(mpPlayer->GetActiveWeapon());
				mpPlayer->StartAdmireGlovesAnimation();
				CBaseViewModel* pViewModel = mpPlayer->GetViewModel();

				if (pViewModel != NULL)
				{
					pViewModel->RemoveEffects(EF_NODRAW); // Restore VM visibility, cleared from Weapon_Drop
				}
			}

			break;
		}
		case EItemAction::HideWeapon:
		{
			mpPlayer->ClearActiveWeapon();
			mpPlayer->StartAdmireGlovesAnimation();
			break;
		}
		}
	}

	Send();
}

CSettingsMenu::CSettingsMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_Settings")
{
#ifdef HL2RP_LEGACY
	AddItem(EItemAction::ClearHUDHints, "#HL2RP_HUDHints_Clear");
#endif // HL2RP_LEGACY

	AddItem(EItemAction::Region, "#HL2RP_Menu_Settings_Region");
}

void CSettingsMenu::SelectItem(CItem* pItem)
{
	switch (pItem->mAction)
	{
#ifdef HL2RP_LEGACY
	case EItemAction::ClearHUDHints:
	{
		return mpPlayer->PushAndSendDialog(new CHUDHintsClearMenu(mpPlayer));
	}
#endif // HL2RP_LEGACY
	case EItemAction::Region:
	{
		return mpPlayer->PushAndSendDialog(new CRegionSettingsMenu(mpPlayer));
	}
	}
}

CRegionSettingsMenu::CRegionSettingsMenu(CHL2Roleplayer* pPlayer)
	: CNetworkMenu(pPlayer, "#HL2RP_Menu_Settings_Region", "#HL2RP_Menu_Region_Msg")
{

}

void CRegionSettingsMenu::UpdateItems()
{
	RemoveAllItems();
	mpPlayer->mMiscFlags.IsBitSet(EPlayerMiscFlag::IsRegionListEnabled) ?
		AddItem(EItemAction::DisableList, "#HL2RP_Menu_Region_DisableList")
		: AddItem(EItemAction::EnableList, "#HL2RP_Menu_Region_EnableList");
	mpPlayer->mMiscFlags.IsBitSet(EPlayerMiscFlag::AllowsRegionVoiceOnly) ?
		AddItem(EItemAction::SetGlobalVoice, "#HL2RP_Menu_Region_SetGlobalVoice")
		: AddItem(EItemAction::SetRegionVoice, "#HL2RP_Menu_Region_SetRegionVoice");
}

void CRegionSettingsMenu::SelectItem(CItem* pItem)
{
	switch (pItem->mAction)
	{
	case EItemAction::EnableList:
	{
		mpPlayer->mMiscFlags.SetBit(EPlayerMiscFlag::IsRegionListEnabled);
		break;
	}
	case EItemAction::DisableList:
	{
		mpPlayer->mMiscFlags.ClearBit(EPlayerMiscFlag::IsRegionListEnabled);
		break;
	}
	case EItemAction::SetRegionVoice:
	{
		mpPlayer->mMiscFlags.SetBit(EPlayerMiscFlag::AllowsRegionVoiceOnly);
		break;
	}
	case EItemAction::SetGlobalVoice:
	{
		mpPlayer->mMiscFlags.ClearBit(EPlayerMiscFlag::AllowsRegionVoiceOnly);
		break;
	}
	}

	Send();
}

#ifdef HL2RP_LEGACY
CHiddenWeaponsMenu::CHiddenWeaponsMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_HiddenWeapons")
{

}

void CHiddenWeaponsMenu::UpdateItems()
{
	RemoveAllItems();

	for (int i = 0; i < mpPlayer->WeaponCount(); ++i)
	{
		CBaseCombatWeapon* pWeapon = mpPlayer->GetWeapon(i);

		if (pWeapon != NULL && !pWeapon->VisibleInWeaponSelection())
		{
			AddItem(0, UTIL_VarArgs("%s_Menu", pWeapon->GetPrintName()), pWeapon->GetClassname());
		}
	}
}

void CHiddenWeaponsMenu::SelectItem(CItem* pItem)
{
	CBaseCombatWeapon* pWeapon = mpPlayer->Weapon_OwnsThisType(pItem->mInfo);
	Send();

	if (pWeapon != NULL)
	{
		mpPlayer->Weapon_Switch(pWeapon);
	}
}

CHUDHintsClearMenu::CHUDHintsClearMenu(CHL2Roleplayer* pPlayer)
	: CNetworkMenu(pPlayer, "#GameUI_Confirm", "#HL2RP_HUDHints_Clear_Warning")
{
	AddItem(EItemAction::Accept, "#GameUI_Accept");
	AddItem(EItemAction::Cancel, "#GameUI_Cancel");
}

void CHUDHintsClearMenu::SelectItem(CItem* pItem)
{
	switch (pItem->mAction)
	{
	case EItemAction::Accept:
	{
		mpPlayer->mLearnedHUDHints = 0;
	}
	}

	mpPlayer->RewindCurrentDialog();
}
#endif // HL2RP_LEGACY

CPropertyDoorMenu::CPropertyDoorMenu(CHL2Roleplayer* pPlayer, CBaseEntity* pDoor)
	: CNetworkMenu(pPlayer, "#HL2RP_ManagePropertyDoor"), mhDoor(pDoor)
{
	CHL2RP_PropertyDoorData* pPropertyData = pDoor->GetPropertyDoorData();

	if (pPropertyData->mProperty != NULL)
	{
		mpProperty = pPropertyData->mProperty;
		mPropertyId = mpProperty->mDatabaseId;
		mDoorId = pPropertyData->mDatabaseId;
	}
}

void CPropertyDoorMenu::UpdateItems()
{
	*mMessage = '\0';
	RemoveAllItems();
	AddItem(EItemAction::SelectDoor, "#HL2RP_Menu_Property_SelectDoor");

	if (ValidateProperty())
	{
		CBaseLocalizeFmtStr<> message(mpPlayer, mMessage);
		message.Format("%t\n\n- ID: %s\n- %t", "#HL2RP_Menu_Property_Msg",
			(int)mpProperty->mDatabaseId, "#HL2RP_Menu_Msg_LinkedMapAlias", mpProperty->mpMapAlias);

		if (mpPlayer->IsAdmin())
		{
			AddItem(EItemAction::SetPropertyName, "#HL2RP_Menu_Property_SetName");
			AddItem(EItemAction::LinkNewDoor, "#HL2RP_Menu_Property_LinkNewDoor");
			AddItem(EItemAction::DeleteProperty, "#HL2RP_Menu_Property_Delete");
			AddMapLinkItems(mpProperty->mpMapAlias, EItemAction::LinkToMap, EItemAction::LinkToMapGroup);

			if (mpProperty->mhZone.IsValid())
			{
				AddItem(EItemAction::UnlinkZone, "#HL2RP_Menu_Property_UnlinkZone");

				if (mpProperty->mhZone != NULL && mpProperty->mhZone->GetEntityName() != NULL_STRING)
				{
					message.Format("\n- %t: %s", "#HL2RP_Menu_Property_Msg_LinkedZone",
						STRING(mpProperty->mhZone->GetEntityName()));
				}
			}
			else
			{
				AddItem(EItemAction::LinkZone, "#HL2RP_Menu_Property_LinkZone");
			}
		}

		if (mpProperty->HasOwner())
		{
			if (mpProperty->HasAccess(mpPlayer, false))
			{
				AddItem(EItemAction::SellHouse, "#HL2RP_Menu_Property_SellHouse");

				if (mpProperty->IsOwner(mpPlayer)
					&& (int)mpProperty->mGrantedSteamIdNumbers.Count() < gMaxHomeKeysCVar.GetInt())
				{
					AddItem(EItemAction::GiveKey, "#HL2RP_Menu_Property_GiveKey");
				}
			}

			if (mpProperty->mGrantedSteamIdNumbers.Count() > 0)
			{
				AddItem(EItemAction::ViewOrTakeKeys, "#HL2RP_Menu_Property_ViewOrTakeKeys");
			}

			message.Format("\n- %t: %s", "#HL2RP_Menu_Property_Msg_OwnerID", mpProperty->mOwnerSteamIdNumber);
		}
		else if (mpProperty->mType == EHL2RP_PropertyType::Home
			&& HL2RPRules()->mDatabaseIOFlags.IsBitSet(EHL2RPDatabaseIOFlag::ArePropertiesLoaded)
			&& (int)mpPlayer->mHomes.Count() < gMaxMapPlayerHomesCVar.GetInt())
		{
			AddItem(EItemAction::BuyHouse, "#HL2RP_Menu_Property_BuyHouse");
		}
	}

	if (mhDoor != NULL)
	{
		CHL2RP_PropertyDoorData* pPropertyData = mhDoor->GetPropertyDoorData();

		if (pPropertyData->mProperty != NULL)
		{
			if (pPropertyData->mDatabaseId.IsValid())
			{
				CHL2RP_Property* pProperty = pPropertyData->mProperty;

				if (pProperty->HasAccess(mpPlayer, false))
				{
					if (pProperty == mpProperty && pProperty->mDoors.Count() > 1 && mpPlayer->IsAdmin())
					{
						AddItem(EItemAction::UnlinkDoor, "#HL2RP_Menu_Property_UnlinkDoor");
					}

					AddItem(EItemAction::SetDoorName, "#HL2RP_Menu_Property_SetDoorName");
				}

				if (pProperty->HasAccess(mpPlayer))
				{
					mhDoor->IsLocked() ? AddItem(EItemAction::UnlockDoor, "#HL2RP_Menu_Property_UnlockDoor")
						: AddItem(EItemAction::LockDoor, "#HL2RP_Menu_Property_LockDoor");
				}
			}
		}
		else if (mpPlayer->IsAdmin())
		{
			AddItem(EItemAction::CreateProperty, "#HL2RP_Menu_Property_Create");
		}
	}
}

void CPropertyDoorMenu::Think()
{
	if (!mPropertyId.IsValid() && ValidateProperty())
	{
		mPropertyId = mpProperty->mDatabaseId;
		Send();
	}

	if (mhDoor != NULL)
	{
		CHL2RP_PropertyDoorData* pPropertyData;

		if (!mDoorId.IsValid() && IsDoorSaved(pPropertyData))
		{
			mDoorId = pPropertyData->mDatabaseId;
			Send();
		}

		mpPlayer->SendBeam(mhDoor->WorldSpaceCenter(), HL2RP_HUD_COLOR_GREEN, HL2_ROLEPLAYER_SMALL_BEAMS_WIDTH);
	}
}

void CPropertyDoorMenu::SelectItem(CItem* pItem)
{
	switch (pItem->mAction)
	{
	case EItemAction::CreateProperty:
	{
		CNetworkMenu* pMenu = new CNetworkMenu(mpPlayer, "#HL2RP_Menu_Property_Type_Title", "", true, pItem->mAction);
		pMenu->AddItem(0, "#HL2RP_Menu_Property_Type_Public", EHL2RP_PropertyType::Public);
		pMenu->AddItem(0, "#HL2RP_Menu_Property_Type_Home", EHL2RP_PropertyType::Home);
		pMenu->AddItem(0, "#HL2RP_Faction_Combine", EHL2RP_PropertyType::Combine);
		pMenu->AddItem(0, "#HL2RP_Admin", EHL2RP_PropertyType::Admin);
		return mpPlayer->PushAndSendDialog(pMenu);
	}
	case EItemAction::SetPropertyName:
	{
		return mpPlayer->PushAndSendDialog(new CNetworkEntryBox(mpPlayer,
			"#HL2RP_Menu_Property_SetName", "", pItem->mAction, true));
	}
	case EItemAction::LinkZone:
	{
		if (mpPlayer->IsAdmin() && ValidateProperty() && !mpProperty->mhZone.IsValid())
		{
			for (int i = mpPlayer->mZonesWithin.Count(); --i >= 0;)
			{
				if (mpPlayer->mZonesWithin[i] != NULL)
				{
					if (mpPlayer->mZonesWithin[i]->mpProperty == NULL)
					{
						mpProperty->LinkZone(mpPlayer->mZonesWithin[i], mpPlayer->GetAbsOrigin());
						DAL().AddDAO(new CPropertiesSaveDAO(mpProperty));
					}

					// Stop at the highest existing zone regardless of its linked property,
					// to be consistent with the property load DAO's zone search criteria
					break;
				}
			}
		}

		break;
	}
	case EItemAction::UnlinkZone:
	{
		if (mpPlayer->IsAdmin() && ValidateProperty() && mpProperty->mhZone.IsValid())
		{
			mpProperty->UnlinkZone();
			DAL().AddDAO(new CPropertiesSaveDAO(mpProperty));
		}

		break;
	}
	case EItemAction::LinkNewDoor:
	{
		CBaseEntity* pEntity = mpPlayer->mAimInfo.mhMainEntity;
		CHL2RP_PropertyDoorData* pPropertyData = UTIL_GetPropertyDoorData(pEntity);

		if (pPropertyData != NULL && pPropertyData->mProperty == NULL && mpPlayer->IsAdmin()
			&& ValidateProperty() && mpPlayer->IsWithinDistance(pEntity, HL2_ROLEPLAYER_USE_KEEP_MAX_DIST))
		{
			mhDoor = pEntity;
			mDoorId = {};
			mpProperty->LinkDoor(pEntity);
			DAL().AddDAO(new CPropertyDoorInsertDAO(pEntity));
		}

		break;
	}
	case EItemAction::UnlinkDoor:
	{
		CHL2RP_PropertyDoorData* pPropertyData;

		if (IsDoorSaved(pPropertyData) && pPropertyData->mProperty == mpProperty && mpPlayer->IsAdmin())
		{
			if (mpProperty->mDoors.Count() < 2)
			{
				mpPlayer->Print(HUD_PRINTTALK, "#HL2RP_Property_UnlinkDoor_Deny");
				break;
			}

			DAL().AddDAO(new CPropertyDoorsSaveDAO(mhDoor, false));
			mpProperty->mDoors.Remove(mhDoor);
			pPropertyData->mProperty = NULL;
			mhDoor.Term();
		}

		break;
	}
	case EItemAction::LinkToMap:
	{
		LinkToMapAlias(STRING(gpGlobals->mapname));
		break;
	}
	case EItemAction::LinkToMapGroup:
	{
		if (HL2RPRules()->mMapGroups.Count() > 1)
		{
			return mpPlayer->PushAndSendDialog(new CMapGroupMenu(mpPlayer, pItem->mAction));
		}

		LinkToMapAlias(HL2RPRules()->mMapGroups[0]);
		break;
	}
	case EItemAction::BuyHouse:
	{
		if ((int)mpPlayer->mHomes.Count() < gMaxMapPlayerHomesCVar.GetInt() && ValidateProperty()
			&& mpProperty->mType == EHL2RP_PropertyType::Home && !mpProperty->HasOwner())
		{
			mpPlayer->mHomes.Insert(mpProperty);
			mpProperty->mOwnerSteamIdNumber = mpPlayer->GetSteamIDAsUInt64();
			VCRHook_Time(&mpProperty->mOwnerLastSeenTime);
			HL2RPRules()->AddPlayerName(mpProperty->mOwnerSteamIdNumber, mpPlayer->GetPlayerName());
			mpProperty->Synchronize();
		}

		break;
	}
	case EItemAction::SellHouse:
	{
		if (ValidateProperty() && mpProperty->HasOwner() && mpProperty->HasAccess(mpPlayer, false))
		{
			mpProperty->Disown(mpPlayer, 50);
		}

		break;
	}
	case EItemAction::SetDoorName:
	{
		return mpPlayer->PushAndSendDialog(new CNetworkEntryBox(mpPlayer,
			"#HL2RP_Menu_Property_SetDoorName", "", pItem->mAction));
	}
	case EItemAction::GiveKey:
	{
		if (ValidateProperty() && mpProperty->IsOwner(mpPlayer)
			&& (int)mpProperty->mGrantedSteamIdNumbers.Count() < gMaxHomeKeysCVar.GetInt())
		{
			CPlayerListMenu* pMenu = new CPlayerListMenu(mpPlayer, "#HL2RP_Menu_Property_GiveKey",
				"#HL2RP_Menu_Property_GiveKey_Msg", pItem->mAction);
			pMenu->mMessageArg = gMaxHomeKeysCVar.GetInt() - (int)mpProperty->mGrantedSteamIdNumbers.Count();

			ForEachRoleplayer([&](CHL2Roleplayer* pTarget)
			{
				if (!pTarget->IsBot() && *pTarget->GetNetworkIDString() != '\0' && !mpProperty->HasAccess(pTarget))
				{
					pMenu->mSteamIdNumbers.Insert(pTarget->GetSteamIDAsUInt64());
				}
			});

			return mpPlayer->PushAndSendDialog(pMenu);
		}

		break;
	}
	case EItemAction::ViewOrTakeKeys:
	{
		if (ValidateProperty())
		{
			CPlayerListMenu* pMenu = new CPlayerListMenu(mpPlayer,
				"#HL2RP_Menu_Property_ViewOrTakeKeys", "", pItem->mAction, true);
			pMenu->mSteamIdNumbers.CopyFrom(mpProperty->mGrantedSteamIdNumbers);
			return mpPlayer->PushAndSendDialog(pMenu);
		}

		break;
	}
	case EItemAction::LockDoor:
	case EItemAction::UnlockDoor:
	{
		CHL2RP_PropertyDoorData* pPropertyData;

		if (IsDoorSaved(pPropertyData) && pPropertyData->mProperty.Get()->HasAccess(mpPlayer))
		{
			if (mpPlayer->IsWithinDistance(mhDoor, HL2_ROLEPLAYER_USE_KEEP_MAX_DIST))
			{
				UTIL_SetDoorLockState(mhDoor, mpPlayer, pItem->mAction == EItemAction::LockDoor, true);
				break;
			}

			mpPlayer->Print(HUD_PRINTTALK, "#HL2RP_Far_From_Entity");
		}

		break;
	}
	case EItemAction::SelectDoor:
	{
		CBaseEntity* pEntity = mpPlayer->mAimInfo.mhMainEntity;
		CHL2RP_PropertyDoorData* pPropertyData = UTIL_GetPropertyDoorData(pEntity);

		if (pPropertyData != NULL && pPropertyData->mProperty != NULL
			&& mpPlayer->IsWithinDistance(pEntity, HL2_ROLEPLAYER_USE_KEEP_MAX_DIST))
		{
			if (pPropertyData->mProperty != mpProperty)
			{
				mpProperty = pPropertyData->mProperty;
				mPropertyId = mpProperty->mDatabaseId;
				mpPlayer->Print(HUD_PRINTTALK, "#HL2RP_Property_ChangeNotice");
			}

			mhDoor = pEntity;
			mDoorId = pPropertyData->mDatabaseId;
		}

		break;
	}
	case EItemAction::DeleteProperty:
	{
		if (mpPlayer->IsAdmin() && ValidateProperty() && (!mpProperty->HasOwner() || mpProperty->Disown(mpPlayer, 100)))
		{
			FOR_EACH_DICT_FAST(mpProperty->mDoors, i)
			{
				if (mpProperty->mDoors[i] != NULL)
				{
					CHL2RP_PropertyDoorData* pPropertyData = mpProperty->mDoors[i]->GetPropertyDoorData();

					if (pPropertyData->mDatabaseId.IsValid())
					{
						DAL().AddDAO(new CPropertyDoorsSaveDAO(mpProperty->mDoors[i], false));
					}

					pPropertyData->mProperty = NULL;
				}
			}

			mpProperty->Synchronize(false);
			HL2RPRules()->mProperties.Remove(mpProperty);
			delete mpProperty;
			mpProperty = NULL;
			mDoorId = {};
		}

		break;
	}
	}

	Send();
}

void CPropertyDoorMenu::HandleChildNotice(int action, const SUtlField& info)
{
	switch (action)
	{
	case EItemAction::CreateProperty:
	{
		if (mhDoor != NULL && mhDoor->GetPropertyDoorData()->mProperty == NULL && mpPlayer->IsAdmin())
		{
			mpProperty = new CHL2RP_Property(HL2RPRules()->GetIdealMapAlias(), info.mInt);
			mpProperty->LinkDoor(mhDoor);
			HL2RPRules()->mProperties.Insert(mpProperty);
			mPropertyId = mDoorId = {};
			DAL().AddDAO(new CPropertyInsertDAO(mpProperty));
		}

		return;
	}
	case EItemAction::SetPropertyName:
	{
		if (mpPlayer->IsAdmin() && ValidateProperty())
		{
			V_strcpy_safe(mpProperty->mName, UTIL_TrimQuotableString(info.mString.Get()));
			mpProperty->Synchronize();
		}

		return;
	}
	case EItemAction::SetDoorName:
	{
		CHL2RP_PropertyDoorData* pPropertyData;

		if (IsDoorSaved(pPropertyData) && pPropertyData->mProperty.Get()->HasAccess(mpPlayer, false))
		{
			Q_strncpy(pPropertyData->mName.GetForModify(),
				UTIL_TrimQuotableString(info.mString.Get()), sizeof(pPropertyData->mName.m_Value));
			DAL().AddDAO(new CPropertyDoorsSaveDAO(mhDoor));
		}

		return;
	}
	case EItemAction::GiveKey:
	{
		CHL2Roleplayer* pTarget = ToHL2Roleplayer(UTIL_PlayerBySteamID(info.mUInt64));

		if (pTarget != NULL && ValidateProperty() && mpProperty->IsOwner(mpPlayer)
			&& (int)mpProperty->mGrantedSteamIdNumbers.Count() < gMaxHomeKeysCVar.GetInt())
		{
			mpProperty->mGrantedSteamIdNumbers.Insert(info.mUInt64);
			mpProperty->SendSteamIdGrantToPlayers(info.mUInt64);
			HL2RPRules()->AddPlayerName(info.mUInt64, pTarget->GetPlayerName());
			DAL().AddDAO(new CPropertyGrantsSaveDAO(mpProperty, info.mUInt64, true));
			mpPlayer->Print(HUD_PRINTTALK, "#HL2RP_Property_Key_Given", pTarget->GetPlayerName());
			pTarget->Print(HUD_PRINTTALK, "#HL2RP_Property_Key_Received", mpProperty->mName, mpPlayer->GetPlayerName());
		}

		return;
	}
	case EItemAction::ViewOrTakeKeys:
	{
		if (ValidateProperty() && mpProperty->IsOwner(mpPlayer))
		{
			mpProperty->mGrantedSteamIdNumbers.Remove(info.mUInt64);
			mpProperty->SendSteamIdGrantToPlayers(info.mUInt64, false);
			DAL().AddDAO(new CPropertyGrantsSaveDAO(mpProperty, info.mUInt64));
			mpPlayer->Print(HUD_PRINTTALK, "#HL2RP_Property_Key_Taken_Issuer",
				HL2RPRules()->mPlayerNameBySteamIdNum.GetElementOrDefault(info.mUInt64, ""));
			CBasePlayer* pTarget = UTIL_PlayerBySteamID(info.mUInt64);

			if (pTarget != NULL)
			{
				ToHL2Roleplayer(pTarget)->Print(HUD_PRINTTALK, "#HL2RP_Property_Key_Taken_Target",
					mpPlayer->GetPlayerName(), mpProperty->mName);
			}
		}

		return;
	}
	}

	LinkToMapAlias(HL2RPRules()->mMapGroups.GetElementOrDefault(info, "")); // EItemAction::LinkToMapGroup
}

bool CPropertyDoorMenu::ValidateProperty()
{
	if (HL2RPRules()->mProperties.HasElement(mpProperty))
	{
		return mpProperty->mDatabaseId.IsValid();
	}

	mpProperty = NULL;
	return false;
}

bool CPropertyDoorMenu::IsDoorSaved(CHL2RP_PropertyDoorData*& pPropertyData)
{
	pPropertyData = UTIL_GetPropertyDoorData(mhDoor);
	return (pPropertyData != NULL && pPropertyData->mProperty != NULL && pPropertyData->mDatabaseId.IsValid());
}

void CPropertyDoorMenu::LinkToMapAlias(const char* pAlias)
{
	if (mpPlayer->IsAdmin() && ValidateProperty())
	{
		V_swap(pAlias, mpProperty->mpMapAlias);
		DAL().AddDAO(new CPropertiesSaveDAO(mpProperty, pAlias));
	}
}

CMapGroupMenu::CMapGroupMenu(CHL2Roleplayer* pPlayer, int action)
	: CNetworkMenu(pPlayer, "#HL2RP_Menu_LinkToMapGroup", "", true)
{
	FOR_EACH_DICT_FAST(HL2RPRules()->mMapGroups, i)
	{
		AddItem(action, HL2RPRules()->mMapGroups[i]);
	}
}

CAdminMenu::CAdminMenu(CHL2Roleplayer* pPlayer) : CNetworkMenu(pPlayer, "#HL2RP_Menu_Admin", "", true)
{
	AddItem(EItemAction::ManageDispensers, "#HL2RP_Menu_Dispensers");
}

void CAdminMenu::SelectItem(CItem* pItem)
{
	switch (pItem->mAction)
	{
	case EItemAction::ManageDispensers:
	{
		return mpPlayer->PushAndSendDialog(new CDispensersMenu(mpPlayer, NULL));
	}
	}
}

CDispensersMenu::CDispensersMenu(CHL2Roleplayer* pPlayer, CRationDispenserProp* pDispenser)
	: CNetworkMenu(pPlayer, "#HL2RP_Menu_Dispensers"), mhDispenser(pDispenser)
{
	if (pDispenser != NULL)
	{
		mDispenserId = pDispenser->mDatabaseId;
		mAllowCreationAsAdmin = false;
	}
}

void CDispensersMenu::UpdateItems()
{
	*mMessage = '\0';
	RemoveAllItems();

	if (mpPlayer->IsAdmin() && mAllowCreationAsAdmin)
	{
		AddItem(EItemAction::Create, "#HL2RP_Menu_Dispenser_Create");
		AddItem(EItemAction::SelectAiming, "#HL2RP_Menu_Dispenser_Select");
	}

	if (mhDispenser != NULL)
	{
		CBaseLocalizeFmtStr<>(mpPlayer, mMessage).Format("%t\n- %3t", "#HL2RP_Menu_Dispenser_Msg_Active",
			mhDispenser->mRationsAmmo, "#HL2RP_Menu_Msg_LinkedMapAlias", mhDispenser->mpMapAlias);

		if (mpPlayer->HasCombineGrants(mhDispenser->HasSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED)))
		{
			mhDispenser->mIsLocked ? AddItem(EItemAction::Unlock, "#HL2RP_Menu_Unlock") :
				AddItem(EItemAction::Lock, "#HL2RP_Menu_Lock");

			if (mpPlayer->IsAdmin() && mDispenserId.IsValid())
			{
				AddItem(EItemAction::SetRations, "#HL2RP_Menu_Dispenser_SetRations");
				mhDispenser->HasSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED) ?
					AddItem(EItemAction::UnsetCC, "#HL2RP_Menu_Dispenser_UnsetCC")
					: AddItem(EItemAction::SetCC, "#HL2RP_Menu_Dispenser_SetCC");
				AddItem(EItemAction::MoveFront, "#HL2RP_Menu_Dispenser_MoveFront");
				AddItem(EItemAction::MoveBack, "#HL2RP_Menu_Dispenser_MoveBack");
				AddItem(EItemAction::Delete, "#HL2RP_Menu_Dispenser_Delete");
				AddMapLinkItems(mhDispenser->mpMapAlias, EItemAction::LinkToMap, EItemAction::LinkToMapGroup);
			}
		}
	}
}

void CDispensersMenu::Think()
{
	if (mhDispenser != NULL)
	{
		if (mpPlayer->HasCombineGrants(mhDispenser->HasSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED))
			&& (mAllowCreationAsAdmin ? mpPlayer->IsAdmin()
				: mpPlayer->IsWithinDistance(mhDispenser, HL2_ROLEPLAYER_USE_KEEP_MAX_DIST)))
		{
			if (!mDispenserId.IsValid() && mhDispenser->mDatabaseId.IsValid())
			{
				mDispenserId = mhDispenser->mDatabaseId;
				Send();
			}

			return mpPlayer->SendBeam(mhDispenser->GetAbsOrigin(),
				HL2RP_HUD_COLOR_GREEN, HL2_ROLEPLAYER_SMALL_BEAMS_WIDTH);
		}
	}
	else if (mAllowCreationAsAdmin && mpPlayer->IsAdmin())
	{
		return;
	}

	mpPlayer->RewindCurrentDialog();
}

void CDispensersMenu::SelectItem(CItem* pItem)
{
	if (pItem->mAction == EItemAction::Create)
	{
		if (mpPlayer->mAimInfo.mhMainEntity != NULL)
		{
			if (fabs(mpPlayer->mAimInfo.mHitNormal.z) < 1.0f - DOT_1DEGREE) // Require a vertical surface
			{
				QAngle angles;
				VectorAngles(mpPlayer->mAimInfo.mHitNormal, angles);
				angles.x = angles.z = 0.0f;
				CRationDispenserProp* pDispenser = static_cast<CRationDispenserProp*>
					(CBaseEntity::Create("prop_ration_dispenser", mpPlayer->mAimInfo.mHitPosition, angles));

				if (pDispenser != NULL)
				{
					pDispenser->mpMapAlias = HL2RPRules()->GetIdealMapAlias();
					pDispenser->mIsLocked = true;
					mhDispenser = pDispenser;
					mDispenserId = {};
					DAL().AddDAO(new CDispenserInsertDAO(pDispenser));
				}
			}
			else
			{
				mpPlayer->Print(HUD_PRINTTALK, "#HL2RP_Dispenser_BadSurface");
			}
		}
	}
	else if (pItem->mAction == EItemAction::SelectAiming)
	{
		CRationDispenserProp* pDispenser = dynamic_cast<CRationDispenserProp*>(mpPlayer->mAimInfo.mhMainEntity.Get());

		if (pDispenser != NULL)
		{
			mhDispenser = pDispenser;
			mDispenserId = pDispenser->mDatabaseId;
		}
	}
	else if (mhDispenser != NULL)
	{
		switch (pItem->mAction)
		{
		case EItemAction::SetRations:
		{
			return mpPlayer->PushAndSendDialog(new CNetworkEntryBox(mpPlayer, "#HL2RP_Menu_Dispenser_SetRations",
				"#HL2RP_Menu_Dispenser_SetRations_Msg", pItem->mAction, true));
		}
		case EItemAction::SetCC:
		case EItemAction::UnsetCC:
		{
			(pItem->mAction == EItemAction::SetCC) ? mhDispenser->AddSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED)
				: mhDispenser->RemoveSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED);
			DAL().AddDAO(new CDispensersSaveDAO(mhDispenser));
			break;
		}
		case EItemAction::MoveFront:
		case EItemAction::MoveBack:
		{
			Vector origin;
			AngleVectors(mhDispenser->GetAbsAngles(), &origin);
			origin = mhDispenser->GetAbsOrigin() + origin * (pItem->mAction == EItemAction::MoveFront ?
				DISPENSER_MANAGE_MOVE_STEP_DIST : -DISPENSER_MANAGE_MOVE_STEP_DIST);
			mhDispenser->Teleport(&origin, NULL, NULL);
			DAL().AddDAO(new CDispensersSaveDAO(mhDispenser));
			break;
		}
		case EItemAction::Lock:
		case EItemAction::Unlock:
		{
			if (mhDispenser->mIsLocked != (pItem->mAction == EItemAction::Lock))
			{
				mhDispenser->Use(mpPlayer, mpPlayer, USE_SPECIAL1, 0.0f);
			}

			break;
		}
		case EItemAction::LinkToMap:
		{
			LinkToMapAlias(STRING(gpGlobals->mapname));
			break;
		}
		case EItemAction::LinkToMapGroup:
		{
			if (HL2RPRules()->mMapGroups.Count() > 1)
			{
				return mpPlayer->PushAndSendDialog(new CMapGroupMenu(mpPlayer, pItem->mAction));
			}

			LinkToMapAlias(HL2RPRules()->mMapGroups[0]);
			break;
		}
		case EItemAction::Delete:
		{
			DAL().AddDAO(new CDispensersSaveDAO(mhDispenser, false));
			mhDispenser->Remove();
			mhDispenser.Term();
			break;
		}
		}
	}

	Send();
}

void CDispensersMenu::HandleChildNotice(int action, const SUtlField& info)
{
	if (mhDispenser != NULL)
	{
		if (action == EItemAction::SetRations)
		{
			mhDispenser->mRationsAmmo = info.ToInt();
			return DAL().AddDAO(new CDispensersSaveDAO(mhDispenser));
		}

		LinkToMapAlias(HL2RPRules()->mMapGroups.GetElementOrDefault(info, "")); // EItemAction::LinkToMapGroup
	}
}

void CDispensersMenu::LinkToMapAlias(const char* pAlias)
{
	V_swap(pAlias, mhDispenser->mpMapAlias);
	DAL().AddDAO(new CDispensersSaveDAO(mhDispenser, pAlias));
}
