// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2_roleplayer.h"
#include "hl2rp_gameinterface.h"
#include <prop_ration_dispenser.h>
#include "trigger_city_zone.h"
#include <dal.h>
#include <player_dao.h>
#include <hl2mp_gameinterface.h>
#include <viewport_panel_names.h>

#define HL2_ROLEPLAYER_MOTDGUIDE_MAX_SEND_DELAY 2.0f

#define HL2_ROLEPLAYER_HUD_THINK_INTERVAL        1.0f
#define HL2_ROLEPLAYER_HUD_THINK_EXTRA_HOLD_TIME 0.1f // Simple way to fix flickering due to self-networking inaccuracy

#define HL2_ROLEPLAYER_AUTOMATIC_CRIME_GAIN 3 // Periodic crime gain when within an automatic crime zone and applicable

void DropVariableAmmo(CBasePlayer*, CBaseCombatWeapon*);

#ifdef HL2RP_FULL
IMPLEMENT_HL2RP_NETWORKCLASS(HL2Roleplayer)
SendPropInt(SENDINFO(m_iMaxHealth)),
SendPropInt(SENDINFO(mMovementFlags)),
SendPropInt(SENDINFO(mSeconds)),
SendPropInt(SENDINFO(mCrime))
END_NETWORK_TABLE()
#endif // HL2RP_FULL

LINK_ENTITY_TO_CLASS(info_player_citizen, CServerOnlyEntity)
LINK_ENTITY_TO_CLASS(info_player_police, CServerOnlyEntity)

void CBaseEntity::OnValueChanged_m_iHealth()
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(this);

	if (pPlayer != NULL && !pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
	{
		pPlayer->OnDatabasePropChanged(EPlayerDatabasePropType::Health);
	}
}

void CBasePlayer::OnValueChanged_m_ArmorValue()
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(this);

	if (!pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
	{
		pPlayer->OnDatabasePropChanged(EPlayerDatabasePropType::Armor);
	}
}

void CBaseCombatCharacter::OnElementChanged_m_iAmmo(int index, const int&)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(this);

	if (pPlayer != NULL)
	{
		if (!pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
		{
			pPlayer->mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::UpdateAmmunitionOnLoaded);
		}
		else if (!pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
		{
			DAL().AddDAO(new CPlayersAmmunitionSaveDAO(pPlayer, index, m_iAmmo[index]));
		}
	}
}

void CBaseCombatCharacter::OnWeaponChanged(CBaseCombatWeapon* pWeapon, bool isOwned)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(this);

	if (pPlayer != NULL)
	{
		if (!pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
		{
			pPlayer->mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::UpdateWeaponsOnLoaded);
		}
		else if (!pPlayer->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
		{
			DAL().AddDAO(new CPlayersWeaponsSaveDAO(pPlayer, pWeapon, isOwned));
		}
	}
}

void CBaseCombatCharacter::OnElementChanged_m_hMyWeapons(int index, const CBaseCombatWeaponHandle& oldWeaponHandle)
{
	if (m_hMyWeapons[index] != NULL)
	{
		return OnWeaponChanged(m_hMyWeapons[index], true);
	}

	OnWeaponChanged(oldWeaponHandle, false);
}

CRestorablePlayerEquipment::CRestorablePlayerEquipment(CHL2Roleplayer* pPlayer)
	: mHealth(pPlayer->m_iHealth), mArmor(pPlayer->ArmorValue())
{
	pPlayer->mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled);
}

void CRestorablePlayerEquipment::Restore(CHL2Roleplayer* pPlayer)
{
	pPlayer->SetHealth(mHealth > 0 ? mHealth : pPlayer->GetHealth());
	pPlayer->SetArmorValue(mArmor);

	FOR_EACH_MAP_FAST(mAmmoCountByIndex, i)
	{
		pPlayer->GiveAmmo(mAmmoCountByIndex[i], mAmmoCountByIndex.Key(i), false);
	}

	FOR_EACH_DICT_FAST(mClipsByWeaponClassname, i)
	{
		CBaseCombatWeapon* pWeapon = pPlayer->Weapon_OwnsThisType(mClipsByWeaponClassname.GetElementName(i));

		if (pWeapon == NULL)
		{
			CBaseEntity* pEntity = CBaseEntity::CreateNoSpawn(mClipsByWeaponClassname.GetElementName(i),
				pPlayer->GetLocalOrigin(), vec3_angle);

			if (pEntity != NULL)
			{
				// If created entity is not a valid weapon, remove it
				if (!pEntity->IsBaseCombatWeapon())
				{
					UTIL_RemoveImmediate(pEntity);
					continue;
				}

				// Proceed to give the weapon. Set loaded clips ignoring maximum allowed, for convenience.
				pWeapon = pEntity->MyCombatWeaponPointer();
				DispatchSpawn(pWeapon);

				if (pWeapon->UsesClipsForAmmo1())
				{
					pWeapon->m_iClip1 = Max(0, mClipsByWeaponClassname[i].first);
				}
				else
				{
					pWeapon->SetPrimaryAmmoCount(0);
				}

				if (pWeapon->UsesClipsForAmmo2())
				{
					pWeapon->m_iClip2 = Max(0, mClipsByWeaponClassname[i].second);
				}
				else
				{
					pWeapon->SetSecondaryAmmoCount(0);
				}

				// Equip directly instead of via Touch(), since it prevents picking weapon when player is out of world
				pPlayer->Weapon_Equip(pWeapon);
			}
		}
		else
		{
			// Add up loaded clips ignoring maximum allowed, for convenience
			if (pWeapon->UsesClipsForAmmo1())
			{
				pWeapon->m_iClip1 += Max(0, mClipsByWeaponClassname[i].first);
			}

			if (pWeapon->UsesClipsForAmmo2())
			{
				pWeapon->m_iClip2 += Max(0, mClipsByWeaponClassname[i].second);
			}
		}
	}

	pPlayer->mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled);
}

CHL2Roleplayer::CHL2Roleplayer()
{

}

void CHL2Roleplayer::InitialSpawn()
{
	BaseClass::InitialSpawn();

	if (GetNetworkIDString()[0] != '\0')
	{
		LoadFromDatabase();
	}
}

void CHL2Roleplayer::Spawn()
{
	BaseClass::Spawn();
	HUDThink();

	if (GetTeamNumber() != TEAM_COMBINE && mRestorableCitizenEquipment.IsValid())
	{
		mRestorableCitizenEquipment->Restore(this);
		mRestorableCitizenEquipment.Delete();
	}
}

CBaseEntity* CHL2Roleplayer::EntSelectSpawnPoint()
{
	const char* pClassname = (GetTeamNumber() == TEAM_COMBINE) ? "info_player_police" : "info_player_citizen";
	CUtlVector<CBaseEntity*> spawnPoints;

	for (CBaseEntity* pSpawnPoint = NULL;
		(pSpawnPoint = gEntList.FindEntityByClassname(pSpawnPoint, pClassname)) != NULL;)
	{
		spawnPoints.AddToTail(pSpawnPoint);
	}

	if (!spawnPoints.IsEmpty())
	{
		InitSLAMProtectTime();
		return spawnPoints.Random();
	}

	return BaseClass::EntSelectSpawnPoint();
}

void CHL2Roleplayer::PostThink()
{
	BaseClass::PostThink();

	if (mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
	{
		if (!mMiscFlags.IsBitSet(EPlayerDatabaseMiscFlag::IsMOTDGuideSent)
			&& gpGlobals->curtime >= mMOTDGuideSendReadyTime)
		{
			const char* pLanguage = engine->GetClientConVarValue(entindex(), "cl_language");
			const char* pLangMOTDGuideFileName = UTIL_VarArgs("%s_%s", HL2RP_MOTD_GUIDE_BASE_NAME, pLanguage);

			if (g_pStringTableInfoPanel->FindStringIndex(pLangMOTDGuideFileName) == INVALID_STRING_INDEX)
			{
				pLangMOTDGuideFileName = UTIL_VarArgs("%s_%s", HL2RP_MOTD_GUIDE_BASE_NAME, "english");
			}

			KeyValuesAD data("data");
			data->SetString("title", HL2RP_TITLE);
			data->SetString("type", "1");
			data->SetString("msg", pLangMOTDGuideFileName);
			ShowViewPortPanel(PANEL_INFO, true, data);
			mMiscFlags.SetBit(EPlayerDatabaseMiscFlag::IsMOTDGuideSent);
		}
	}

	if (IsAlive())
	{
		localizebuf_t message;

		if (ComputeAimingEntityAndHUD(message))
		{
#ifdef HL2RP_LEGACY
			SendHUDMessage(EPlayerHUDType::AimingEntity, message, HL2RP_CENTER_HUD_SPECIAL_POS,
				HL2RP_AIMING_HUD_DEFAULT_Y_POS, mhAimingEntity->IsPlayer() ?
				HL2RP_AIMING_HUD_DEFAULT_PLAYER_COLOR : HL2RP_AIMING_HUD_DEFAULT_GENERAL_COLOR);
#endif // HL2RP_LEGACY
		}

		// Send City Zone HUD, except for polices inside an automatic crime zone (unneeded)
		if (mhCityZone != NULL && (GetTeamNumber() != TEAM_COMBINE || mhCityZone->mType != ECityZoneType::AutoCrime)
			&& AcquireHUDTime(EPlayerHUDType::Alert))
		{
			CLocalizeFmtStr message;
			gHL2RPLocalizer.Localize(this, message, false, "#HL2RP_CityZone_Notice",
				STRING(mhCityZone->GetEntityName()));
			message.AppendFormat(" (%s)", gHL2RPLocalizer.Localize(this, CCityZone::sTypeTokens[mhCityZone->mType]));
			SendHUDMessage(EPlayerHUDType::Alert, message, HL2RP_CENTER_HUD_SPECIAL_POS,
				HL2RP_ALERT_HUD_DEFAULT_Y_POS, HL2RP_ALERT_HUD_DEFAULT_COLOR);
		}
	}
	else
	{
		mhCityZone = NULL;
	}
}

void CHL2Roleplayer::Print(int type, const char* pText, bool networked)
{
#ifdef HL2RP_FULL
	if (networked)
#endif // HL2RP_FULL
	{
		ClientPrint(this, type, pText);
	}
}

void CHL2Roleplayer::HUDThink()
{
	SendMainHUD();
	SetContextThink(&ThisClass::HUDThink, gpGlobals->curtime + HL2_ROLEPLAYER_HUD_THINK_INTERVAL, "HUDThinkContext");

	if (IsAlive())
	{
		++mSeconds;

		if (GetTeamNumber() != TEAM_COMBINE && mhCityZone != NULL && mhCityZone->mType == ECityZoneType::AutoCrime)
		{
			mCrime += HL2_ROLEPLAYER_AUTOMATIC_CRIME_GAIN;
		}
		else
		{
			--mCrime;
		}
	}
}

void CHL2Roleplayer::SendMainHUD()
{
#ifdef HL2RP_LEGACY
	localizebuf_t message;
	ComputeMainHUD(message);
	SendHUDMessage(EPlayerHUDType::Main, message, HL2RP_MAIN_HUD_DEFAULT_X_POS, HL2RP_MAIN_HUD_DEFAULT_Y_POS,
		mCrime > 0 ? HL2RP_MAIN_HUD_DEFAULT_CRIMINAL_TEXT_COLOR : HL2RP_MAIN_HUD_DEFAULT_NORMAL_TEXT_COLOR);
#endif // HL2RP_LEGACY
}

void CHL2Roleplayer::SendHUDHint(EPlayerHUDHintType::_Value type, const char* pToken, bool networked)
{
	if (!mSentHUDHints.IsBitSet(type))
	{
#ifdef HL2RP_FULL
		if (networked)
		{
			CSingleUserRecipientFilter filter(this);
			filter.MakeReliable();
			UserMessageBegin(filter, HL2RP_KEY_HINT_USER_MESSAGE);
			WRITE_LONG(type);
			WRITE_STRING(pToken);
			MessageEnd();
		}
#else
		ClientPrint(this, HUD_PRINTCENTER, gHL2RPLocalizer.Localize(this, pToken));
#endif // HL2RP_FULL

		mSentHUDHints.SetBit(type);
	}
}

bool CHL2Roleplayer::AcquireHUDTime(EPlayerHUDType::_Value type, bool force)
{
	if (force || mHUDExpireTimers[type].Expired())
	{
		mHUDExpireTimers[type].Set(HL2_ROLEPLAYER_HUD_THINK_INTERVAL);
		return true;
	}

	return false;
}

void CHL2Roleplayer::SendHUDMessage(EPlayerHUDType::_Value type, const char* pMessage,
	float xPos, float yPos, const Color& color)
{
	hudtextparms_t params{};
	params.channel = type;
	params.x = (xPos < 0.0f) ? HL2RP_CENTER_HUD_SPECIAL_POS : xPos / 640.0f;
	params.y = (yPos < 0.0f) ? HL2RP_CENTER_HUD_SPECIAL_POS : yPos / 480.0f;
	params.r1 = color.r();
	params.g1 = color.g();
	params.b1 = color.b();
	params.a1 = color.a();
	params.holdTime = HL2_ROLEPLAYER_HUD_THINK_INTERVAL + HL2_ROLEPLAYER_HUD_THINK_EXTRA_HOLD_TIME;
	UTIL_HudMessage(this, params, pMessage);
}

bool CHL2Roleplayer::PassesDamageFilter(const CTakeDamageInfo& info)
{
	return (mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded) && BaseClass::PassesDamageFilter(info));
}

void CHL2Roleplayer::Weapon_Drop(CBaseCombatWeapon* pWeapon, const Vector* pDirection, const Vector* pVelocity)
{
	BaseClass::Weapon_Drop(pWeapon, pDirection, pVelocity);

	if (pWeapon != NULL)
	{
		DropVariableAmmo(this, pWeapon);
	}
}

void CHL2Roleplayer::Weapon_Equip(CBaseCombatWeapon* pWeapon)
{
	// Equip, but revert any extra clips or ammo given by BaseClass (conservation principle)
	int clip1 = pWeapon->Clip1(), clip2 = pWeapon->Clip2();
	int primaryAmmoCount = GetAmmoCount(pWeapon->GetPrimaryAmmoType());
	int secondaryAmmoCount = GetAmmoCount(pWeapon->GetSecondaryAmmoType());
	BaseClass::Weapon_Equip(pWeapon);
	pWeapon->m_iClip1 = clip1;
	pWeapon->m_iClip2 = clip2;

	if (pWeapon->UsesPrimaryAmmo())
	{
		SetAmmoCount(primaryAmmoCount, pWeapon->GetPrimaryAmmoType());
		TransferPrimaryAmmoFromWeapon(pWeapon);
	}

	if (pWeapon->UsesSecondaryAmmo())
	{
		SetAmmoCount(secondaryAmmoCount, pWeapon->GetSecondaryAmmoType());
		TransferSecondaryAmmoFromWeapon(pWeapon);
	}
}

bool CHL2Roleplayer::Weapon_EquipAmmoOnly(CBaseCombatWeapon* pOtherWeapon)
{
	CBaseCombatWeapon* pMyWeapon = Weapon_OwnsThisType(pOtherWeapon->GetClassname());

	if (pMyWeapon != NULL)
	{
		// Transfer both reload ammo and clips to the reserve ammo
		int givenAmmoCount = 0;

		if (pMyWeapon->UsesPrimaryAmmo())
		{
			givenAmmoCount += TransferPrimaryAmmoFromWeapon(pOtherWeapon);
		}

		if (pMyWeapon->UsesSecondaryAmmo())
		{
			givenAmmoCount += TransferSecondaryAmmoFromWeapon(pOtherWeapon);
		}

		if (pOtherWeapon->Clip1() > 0)
		{
			int takenClip = GiveAmmo(pOtherWeapon->Clip1(), pOtherWeapon->GetPrimaryAmmoType(), false);
			pOtherWeapon->m_iClip1 -= takenClip;
			givenAmmoCount += takenClip;
		}

		if (pOtherWeapon->Clip2() > 0)
		{
			int takenClip = GiveAmmo(pOtherWeapon->Clip2(), pOtherWeapon->GetSecondaryAmmoType(), false);
			pOtherWeapon->m_iClip2 -= takenClip;
			givenAmmoCount += takenClip;
		}

		return (givenAmmoCount > 0);
	}

	return false;
}

void CHL2Roleplayer::Event_Killed(const CTakeDamageInfo& info)
{
	BaseClass::Event_Killed(info);
	RemoveAllAmmo(); // Default code doesn't call this when player doesn't have weapons
	SetArmorValue(0);
}

void CHL2Roleplayer::OnDatabasePropChanged(EPlayerDatabasePropType::_Value propType)
{
	if (propType == EPlayerDatabasePropType::Crime)
	{
		SendMainHUD();
	}

	if (mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
	{
		CBitFlags<> selectedProps;
		selectedProps.SetBit(propType);
		return DAL().AddDAO(new CPlayersMainDataSaveDAO(this, selectedProps));
	}

	// Set to update main data once player loads
	mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::UpdateMainDataOnLoaded);
}

void CHL2Roleplayer::LoadFromDatabase()
{
	if (!IsFakeClient())
	{
		DAL().AddDAO(new CPlayerLoadDAO(this));
	}
}

int CHL2Roleplayer::TransferPrimaryAmmoFromWeapon(CBaseCombatWeapon* pWeapon)
{
	int givenCount = GiveAmmo(pWeapon->GetPrimaryAmmoCount(), pWeapon->GetPrimaryAmmoType(), false);
	pWeapon->SetPrimaryAmmoCount(0);
	return givenCount;
}

int CHL2Roleplayer::TransferSecondaryAmmoFromWeapon(CBaseCombatWeapon* pWeapon)
{
	int givenCount = GiveAmmo(pWeapon->GetSecondaryAmmoCount(), pWeapon->GetSecondaryAmmoType(), false);
	pWeapon->SetSecondaryAmmoCount(0);
	return givenCount;
}

CON_COMMAND_F(closed_htmlpage, "Handles player closing main MOTD window", FCVAR_HIDDEN)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		pPlayer->mMOTDGuideSendReadyTime = gpGlobals->curtime + HL2_ROLEPLAYER_MOTDGUIDE_MAX_SEND_DELAY;
	}
}
