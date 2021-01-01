// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "player_dialogs.h"
#include <hl2_roleplayer.h>
#include <hl2rp_gamerules.h>
#include <dal.h>
#include <prop_ration_dispenser.h>
#include <ration_dispenser_dao.h>

#define DISPENSER_MANAGE_MOVE_STEP_DIST 1.0f

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
		mAllowManageOthers = false;
	}
}

void CDispensersMenu::UpdateItems()
{
	*mMessage = '\0';
	RemoveAllItems();

	if (mpPlayer->IsAdmin() && mAllowManageOthers)
	{
		AddItem(EItemAction::Create, "#HL2RP_Menu_Dispenser_Create");
		AddItem(EItemAction::SelectAiming, "#HL2RP_Menu_Dispenser_Select");
	}

	if (mhDispenser != NULL)
	{
		CBaseLocalizeFmtStr<>(mpPlayer, mMessage).Localize("#HL2RP_Menu_Dispenser_Msg_Active",
			mhDispenser->mRationsAmmo, mhDispenser->mpMapAlias);

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
				uint minMapGroupsCount = 1;

				if (mhDispenser->mpMapAlias != STRING(gpGlobals->mapname))
				{
					AddItem(EItemAction::LinkToMap, "#HL2RP_Menu_Dispenser_LinkToMap");
					minMapGroupsCount = 2;
				}

				if (HL2RPRules()->mMapGroups.Count() >= minMapGroupsCount)
				{
					AddItem(EItemAction::LinkToMapGroup, "#HL2RP_Menu_Dispenser_LinkToMapGroup");
				}
			}
		}
	}
}

void CDispensersMenu::Think()
{
	if (mhDispenser != NULL)
	{
		if (mpPlayer->HasCombineGrants(mhDispenser->HasSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED))
			&& (mAllowManageOthers ? mpPlayer->IsAdmin()
				: mpPlayer->IsWithinDistance(mhDispenser, HL2_ROLEPLAYER_USE_KEEP_MAX_DIST, true)))
		{
			if (!mDispenserId.IsValid() && mhDispenser->mDatabaseId.IsValid())
			{
				mDispenserId = mhDispenser->mDatabaseId;
				Send();
			}

			return mpPlayer->SendBeam(mhDispenser->GetAbsOrigin(), Color(0, 255, 0),
				HL2_ROLEPLAYER_SMALL_BEAMS_WIDTH);
		}
	}
	else if (mAllowManageOthers && mpPlayer->IsAdmin())
	{
		return;
	}

	mpPlayer->RewindCurrentDialog();
}

void CDispensersMenu::SelectItem(CItem* pItem)
{
	if (pItem->mAction == EItemAction::Create)
	{
		Vector start = mpPlayer->EyePosition();
		trace_t trace;
		UTIL_TraceLine(start, start + mpPlayer->EyeDirection3D() * HL2_ROLEPLAYER_COMBINE_AIM_TRACE_DIST,
			mpPlayer->PhysicsSolidMaskForEntity(), mpPlayer, mpPlayer->GetCollisionGroup(), &trace);

		if (trace.DidHit())
		{
			if (fabs(trace.plane.normal.z) < 1.0f - DOT_1DEGREE) // Require a vertical surface
			{
				QAngle angles;
				VectorAngles(trace.plane.normal, angles);
				angles.x = angles.z = 0.0f;
				CRationDispenserProp* pDispenser = static_cast<CRationDispenserProp*>
					(CBaseEntity::Create("prop_ration_dispenser", trace.endpos, angles));

				if (pDispenser != NULL)
				{
					pDispenser->mpMapAlias = HL2RPRules()->GetIdealMapAlias();
					pDispenser->mIsLocked = true;
					mhDispenser = pDispenser;
					mDispenserId = pDispenser->mDatabaseId = IDTO_LOADING_DATABASE_ID;
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
		CRationDispenserProp* pDispenser = dynamic_cast<CRationDispenserProp*>(mpPlayer->mhAimingEntity.Get());

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
				"#HL2RP_Menu_Dispenser_SetRations_Msg", EChildAction::SetRations, true));
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
				return mpPlayer->PushAndSendDialog(new CDispenserMapGroupMenu(mpPlayer));
			}

			LinkToMapAlias(HL2RPRules()->mMapGroups[0]);
			break;
		}
		case EItemAction::Delete:
		{
			DAL().AddDAO(new CDispensersSaveDAO(mhDispenser, false));
			mhDispenser->Remove();
			mhDispenser = NULL;
			break;
		}
		}
	}

	Send();
}

void CDispensersMenu::HandleChildNotice(int action, const char* pInfo)
{
	if (mhDispenser != NULL)
	{
		if (action == EChildAction::SetRations)
		{
			mhDispenser->mRationsAmmo = Q_atoi(pInfo);
			return DAL().AddDAO(new CDispensersSaveDAO(mhDispenser));
		}

		LinkToMapAlias(HL2RPRules()->mMapGroups.GetElementOrDefault(pInfo, "")); // EChildAction::LinkToMapGroup
	}
}

void CDispensersMenu::LinkToMapAlias(const char* pAlias)
{
	const char* pOldAlias = mhDispenser->mpMapAlias;
	mhDispenser->mpMapAlias = pAlias;
	DAL().AddDAO(new CDispensersSaveDAO(mhDispenser, pOldAlias));
}

CDispenserMapGroupMenu::CDispenserMapGroupMenu(CHL2Roleplayer* pPlayer)
	: CNetworkMenu(pPlayer, "#HL2RP_Menu_Dispenser_LinkToMapGroup", "", true)
{
	FOR_EACH_DICT_FAST(HL2RPRules()->mMapGroups, i)
	{
		AddItem(0, HL2RPRules()->mMapGroups[i]);
	}
}

void CDispenserMapGroupMenu::SelectItem(CItem* pItem)
{
	RewindAndNoticeParent(CDispensersMenu::EChildAction::LinkToMapGroup, pItem->mDisplay);
}
