// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2_roleplayer.h"
#include "hl2rp_gamerules.h"
#include <dal.h>
#include <hl2rp_localizer.h>
#include <hl2rp_property.h>
#include <hl2rp_property_dao.h>
#include <player_dao.h>
#include <player_dialogs.h>
#include <in_buttons.h>

#define HL2RP_SPAWN_POINT_SF_AI_ONLY 1

#define HL2_ROLEPLAYER_INITIAL_SUICIDE_DELAY 5.0f // Delay since first spawn to allow unforced suicides

#define HL2_ROLEPLAYER_WEAPON_IDLE_DELAY      10.0f
#define HL2_ROLEPLAYER_WEAPON_READY_MAX_DELAY 0.5f

#define HL2_ROLEPLAYER_HUD_THINK_CONTEXT         "HUDThink"
#define HL2_ROLEPLAYER_HUD_FONT_ESTIMATED_HEIGHT 0.036

#define HL2_ROLEPLAYER_USE_SPECIAL2_HOLD_TIME 1.0f // Required USE key holdtime to activate USE_SPECIAL2

#define HL2_ROLEPLAYER_AUTOMATIC_CRIME_GAIN 3 // Periodic crime gain when within an automatic crime zone and applicable

#define HL2_ROLEPLAYER_BEAMS_EYE_OFFSET_DOWN 20.0f
#define HL2_ROLEPLAYER_BEAMS_ALPHA           150
#define HL2_ROLEPLAYER_BEAM_RING_RADIUS      80.0f
#define HL2_ROLEPLAYER_BEAM_RING_DURATION    0.2f
#define HL2_ROLEPLAYER_BEAM_RING_WIDTH       3.0f

extern ConVar gCopVIPSkinsAllowCVar, gMaxHomeInactivityDays;

#ifdef HL2RP_FULL
static void SendProxy_ZoneWithin(const SendProp*, const void*, const void* pData, DVariant* pOut, int, int)
{
	pOut->m_Int = ((EHANDLE*)pData)->GetEntryIndex();
}

IMPLEMENT_HL2RP_NETWORKCLASS(HL2Roleplayer)
SendPropInt(SENDINFO(m_iMaxHealth)),
SendPropInt(SENDINFO(mDatabaseIOFlags)),
SendPropInt(SENDINFO(mSeconds)),
SendPropInt(SENDINFO(mCrime)),
SendPropInt(SENDINFO(mFaction), 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(mAccessFlags)),
SendPropInt(SENDINFO(mMiscFlags)),
SendPropBool(SENDINFO(mIsInStickyWalkMode)),
SendPropStringT(SENDINFO(mJobName)),
SendPropArray(SendPropInt(SENDINFO_ARRAY(mZonesWithin), -1, 0, SendProxy_ZoneWithin), mZonesWithin),
END_SEND_TABLE()
#endif // HL2RP_FULL

LINK_ENTITY_TO_CLASS(info_citizen_start, CServerOnlyEntity)
LINK_ENTITY_TO_CLASS(info_police_start, CServerOnlyEntity)

const char* gPlayerDatabasePropNames[EPlayerDatabasePropType::_Count] =
{
	"name",
	"seconds",
	"crime",
	"faction",
	"job",
	"modelGroup",
	"modelAlias",
	"health",
	"armor",
	"accessFlags",
	"miscFlags",
	HL2RP_LEARNED_HUD_HINTS_FIELD_NAME
};

CRestorablePlayerEquipment::CRestorablePlayerEquipment(CHL2Roleplayer* pPlayer, bool copyWeapons)
	: CPlayerEquipment(pPlayer->GetHealth(), pPlayer->ArmorValue(), false)
{
	pPlayer->mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled);

	if (copyWeapons)
	{
		for (int i = 0; i < MAX_AMMO_SLOTS; ++i)
		{
			int count = pPlayer->GetAmmoCount(i);

			if (count > 0)
			{
				mAmmoCountByIndex.Insert(i, count);
			}
		}

		for (int i = 0; i < pPlayer->WeaponCount(); ++i)
		{
			CBaseCombatWeapon* pWeapon = pPlayer->GetWeapon(i);

			if (pWeapon != NULL)
			{
				AddWeapon(pWeapon->GetClassname(), pWeapon->m_iClip1, pWeapon->m_iClip2);
			}
		}
	}
}

void CRestorablePlayerEquipment::Restore(CHL2Roleplayer* pPlayer, int altHealth)
{
	mHealth = Max(mHealth, altHealth); // Use highest health source value
	Equip(pPlayer);
	pPlayer->mRestorableEquipment.Delete();
	pPlayer->mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled);
}

void CHL2Roleplayer::CHUDExpireTimer::Set(EPlayerHUDType originalType, float delay)
{
	CSimpleSimTimer::Set(delay);
	mOriginalType = originalType;
}

CHL2Roleplayer::CHL2Roleplayer()
{

}

void CHL2Roleplayer::InitialSpawn()
{
	BaseClass::InitialSpawn();
	ChangeTeam();
	mFirstSpawnTime = gpGlobals->curtime;
	m_fNextSuicideTime = gpGlobals->curtime + HL2_ROLEPLAYER_INITIAL_SUICIDE_DELAY;
	auto& properties = HL2RPRules()->mProperties;

	FOR_EACH_DICT_FAST(properties, i)
	{
		if (properties[i]->IsOwner(this))
		{
			mHomes.InsertIfNotFound(properties[i]);
			VCRHook_Time(&properties[i]->mOwnerLastSeenTime);
		}

#ifdef HL2RP_FULL
		if (properties[i]->mDatabaseId.IsValid())
		{
			properties[i]->Synchronize(true, false, CSingleUserRecipientFilter(this));

			FOR_EACH_DICT_FAST(properties[i]->mGrantedSteamIdNumbers, j)
			{
				properties[i]->SendSteamIdGrantToPlayers(properties[i]->mGrantedSteamIdNumbers[j],
					true, CSingleUserRecipientFilter(this));
			}
		}
#endif // HL2RP_FULL
	}

#ifdef HL2RP_FULL
	FOR_EACH_MAP_FAST(HL2RPRules()->mPlayerNameBySteamIdNum, i)
	{
		HL2RPRules()->SendPlayerName(i, CSingleUserRecipientFilter(this));
	}

	for (CCityZone* pZone = NULL; (pZone = gEntList.NextEntByClass(pZone)) != NULL;)
	{
		pZone->SendToPlayers(true, CSingleUserRecipientFilter(this));
	}
#endif // HL2RP_FULL

	if (*GetNetworkIDString() != '\0')
	{
		LoadFromDatabase();
	}
}

void CHL2Roleplayer::OnDisconnect()
{
	// NOTE: To prevent double load case upon listenserver retry, we don't clear the IsLoaded flag
	mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled);
}

void CHL2Roleplayer::Spawn()
{
	BaseClass::Spawn();
	m_LowerWeaponTimer.Set(HL2_ROLEPLAYER_WEAPON_IDLE_DELAY);
	mCrime = 0; // NOTE: Provisional only (until cleaning crime gets easier)
	SetContextThink(&ThisClass::HUDThink,
		gpGlobals->curtime + HL2_ROLEPLAYER_HUD_THINK_PERIOD, HL2_ROLEPLAYER_HUD_THINK_CONTEXT);
	auto& jobs = HL2RPRules()->mJobByName[mFaction];
	int jobIndex = jobs.Find(mJobName->ToCStr()), jobHealth = 0;

	if (jobs.IsValidIndex(jobIndex))
	{
		jobHealth = jobs[jobIndex]->mHealth;
		jobs[jobIndex]->Equip(this);
	}

	for (int i = 0; i < mZonesWithin.Count(); ++i)
	{
		mZonesWithin.Set(i, NULL);
	}

	if (mFaction == EFaction::Combine)
	{
		// Equip basic cop equipment
		GiveNamedItem("weapon_stunstick");
		GiveNamedItem("weapon_pistol");
	}
	else if (mRestorableEquipment.IsValid())
	{
		mRestorableEquipment->Restore(this, jobHealth);
		Print(HUD_PRINTTALK, "#HL2RP_Citizen_Equipment_Restored");
	}
}

CBaseEntity* CHL2Roleplayer::EntSelectSpawnPoint()
{
	CUtlVector<CBaseEntity*> spawnPoints;
	const char* pClassName = (mFaction == EFaction::Citizen) ? "info_citizen_start" : "info_police_start";

	for (CBaseEntity* pSpawnPoint = NULL;
		(pSpawnPoint = gEntList.FindEntityByClassname(pSpawnPoint, pClassName)) != NULL; )
	{
		if (!pSpawnPoint->HasSpawnFlags(HL2RP_SPAWN_POINT_SF_AI_ONLY))
		{
			spawnPoints.AddToTail(pSpawnPoint);
		}
	}

	if (!spawnPoints.IsEmpty())
	{
		InitSLAMProtectTime();
		return spawnPoints.Random();
	}

	return BaseClass::EntSelectSpawnPoint();
}

void CHL2Roleplayer::PrintWelcomeInfo()
{
	if (mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsNewCitizenPrintPending))
	{
		ForEachRoleplayer([&](CHL2Roleplayer* pPlayer)
		{
			pPlayer->Print(HUD_PRINTTALK, "#HL2RP_New_Player", GetPlayerName());
		});

		return mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::IsNewCitizenPrintPending);
	}

	Print(HUD_PRINTTALK, "#HL2RP_Player_Reloaded");

	if (mHomes.Count() > 0)
	{
		Print(HUD_PRINTTALK, "#HL2RP_Owned_Houses_Info", CNumStr(mHomes.Count()));
	}
}

void CHL2Roleplayer::NetworkVarChanged_m_iHealth()
{
	if (!mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
	{
		OnDatabasePropChanged(GetHealth(), EPlayerDatabasePropType::Health);
	}
}

void CHL2Roleplayer::NetworkVarChanged_m_ArmorValue()
{
	if (!mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
	{
		OnDatabasePropChanged(ArmorValue(), EPlayerDatabasePropType::Armor);
	}
}

void CHL2Roleplayer::NetworkArrayChanged_m_hMyWeapons(int index, const CBaseCombatWeaponHandle& oldWeapon)
{
	oldWeapon ? OnWeaponChanged(oldWeapon, false) : OnWeaponChanged(m_hMyWeapons[index], true);
}

void CHL2Roleplayer::OnWeaponChanged(CBaseCombatWeapon* pWeapon, bool isOwned)
{
	if (!mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
	{
		mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::UpdateWeaponsOnLoaded);
	}
	else if (!mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
	{
		DAL().AddDAO(new CPlayersWeaponsSaveDAO(this, pWeapon, isOwned));
	}
}

void CHL2Roleplayer::NetworkArrayChanged_m_iAmmo(int index, const int&)
{
	if (!mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
	{
		mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::UpdateAmmunitionOnLoaded);
	}
	else if (!mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsEquipmentSaveDisabled))
	{
		DAL().AddDAO(new CPlayersAmmunitionSaveDAO(this, index, m_iAmmo[index]));
	}
}

bool CHL2Roleplayer::IsBot() const
{
	return (BaseClass::IsBot() || !ThisClass::IsNetClient());
}

bool CHL2Roleplayer::IsFakeClient() const
{
	return ThisClass::IsBot();
}

bool CHL2Roleplayer::IsNetClient() const
{
	return (engine->GetPlayerNetInfo(entindex()) != NULL);
}

IResponseSystem* CHL2Roleplayer::GetResponseSystem()
{
	return HL2RPRules()->mpPlayerResponseSystem;
}

void CHL2Roleplayer::SetPlayerName(const char* pName)
{
	BaseClass::SetPlayerName(pName);
	OnDatabasePropChanged(pName, EPlayerDatabasePropType::Name);
	int index = HL2RPRules()->mPlayerNameBySteamIdNum.Find(GetSteamIDAsUInt64());

	if (HL2RPRules()->mPlayerNameBySteamIdNum.IsValidIndex(index))
	{
		HL2RPRules()->mPlayerNameBySteamIdNum[index] = pName;
		HL2RPRules()->SendPlayerName(index);
	}
}

void CHL2Roleplayer::SetModel(const char* pModel)
{
	// If a model group is set, block default model change calls (especially from HL2MP code)
	if (mModelGroup.Get() == NULL_STRING)
	{
		BaseClass::SetModel(pModel);
	}
}

void CHL2Roleplayer::SetModel(int groupIndex, int aliasIndex)
{
	BaseClass::SetModel(HL2RPRules()->mPlayerModelsByGroup[groupIndex]->Element(aliasIndex));
	m_nSkin = RandomInt(1, GetModelPtr()->numskinfamilies());
	mModelGroup = MAKE_STRING(HL2RPRules()->mPlayerModelsByGroup.Key(groupIndex));
	mModelAlias = MAKE_STRING(HL2RPRules()->mPlayerModelsByGroup[groupIndex]->GetElementName(aliasIndex));
}

void CHL2Roleplayer::ChangeTeam(int team)
{
	if (team != TEAM_SPECTATOR)
	{
		team = (mFaction == EFaction::Citizen) ? TEAM_REBELS : TEAM_COMBINE; // Ensure correct team

		// Teleport to correct team spawn when player may not get killed
		if (team != GetTeamNumber() && m_fNextSuicideTime > gpGlobals->curtime && IsAlive())
		{
			ForceRespawn();
		}
	}
	else
	{
		if (!IsBot())
		{
			if (!IsAdmin())
			{
				return SendHUDMessage(EPlayerHUDType::Alert, "#HL2RP_Spectator_Deny", HL2RP_CENTER_HUD_SPECIAL_POS,
					HL2RP_ALERT_HUD_DEFAULT_Y_POS, HL2RP_ALERT_HUD_DEFAULT_COLOR, HL2_ROLEPLAYER_ALERT_HUD_HOLD_TIME);
			}
			else if (mFaction == EFaction::Citizen && !mRestorableEquipment.IsValid() && IsAlive())
			{
				mRestorableEquipment.Attach(new CRestorablePlayerEquipment(this));
			}
		}

		ClearActiveWeapon();
		CommitSuicide(false, true); // Force suicide, as default HL2MP does when joining spectator
	}

	BaseClass::ChangeTeam(team);
	SetupPlayerSoundsByModel(STRING(GetModelName())); // Fix footstep sounds for custom models (fallback)
}

void CHL2Roleplayer::SetAnimation(PLAYER_ANIM animation)
{
	int sequence = 0;
	Activity idealActivity = GetActivity();
	bool keepJumping = (!FBitSet(GetFlags(), FL_ONGROUND) && GetActivity() == ACT_JUMP),
		shouldAim = (GetActiveWeapon() != NULL && !GetActiveWeapon()->IsLowered());

	if (FBitSet(GetFlags(), FL_FROZEN | FL_ATCONTROLS))
	{
		animation = PLAYER_IDLE;
	}

	switch (animation)
	{
	case PLAYER_IDLE:
	{
		if (!keepJumping)
		{
			if (FBitSet(GetFlags(), FL_ANIMDUCKING))
			{
				idealActivity = shouldAim ? ACT_CROUCH : ACT_CROUCHIDLE;
				break;
			}

			idealActivity = shouldAim ? ACT_IDLE_AIM : ACT_IDLE;
		}

		break;
	}
	case PLAYER_WALK:
	{
		if (!keepJumping)
		{
			if (FBitSet(GetFlags(), FL_ANIMDUCKING))
			{
				idealActivity = shouldAim ? ACT_WALK_CROUCH_AIM : ACT_WALK_CROUCH;
			}
			else if (IsWalking())
			{
				idealActivity = shouldAim ? ACT_WALK_AIM : ACT_WALK;
			}
			else
			{
				idealActivity = shouldAim ? ACT_RUN_AIM : ACT_RUN;
			}
		}

		break;
	}
	case PLAYER_JUMP:
	{
		idealActivity = ACT_JUMP;
		break;
	}
	case PLAYER_DIE:
	{
		return; // NOTE: Player will become ragdoll
	}
	case PLAYER_ATTACK1:
	{
		RestartGesture(HL2RPRules()->GetBestTranslatedActivity(this, ACT_GESTURE_RANGE_ATTACK1, false, sequence));
		return Weapon_SetActivity(Weapon_TranslateActivity(ACT_RANGE_ATTACK1), 0.0f);
	}
	case PLAYER_RELOAD:
	{
		return RestartGesture(HL2RPRules()->GetBestTranslatedActivity(this, ACT_GESTURE_RELOAD, false, sequence));
	}
	}

	SetActivity(idealActivity);
	Activity translatedActivity = HL2RPRules()->GetBestTranslatedActivity(this, idealActivity, shouldAim, sequence);

	if (GetSequence() != sequence && GetSequenceActivity(GetSequence()) != translatedActivity)
	{
		SetPlaybackRate(1.0f);
		ResetSequence(sequence);
		SetCycle(0);
	}
}

void CHL2Roleplayer::UpdateWeaponPosture()
{
	if (GetActiveWeapon() != NULL)
	{
		if (((m_flForwardMove != 0.0f || m_flSideMove != 0.0f) && !mIsWeaponManuallyLowered)
			|| FBitSet(m_nButtons, IN_ATTACK | IN_ATTACK2 | IN_RELOAD) || GetActiveWeapon()->m_bInReload)
		{
			if (GetActiveWeapon()->IsLowered())
			{
				GetActiveWeapon()->Ready();
				GetActiveWeapon()->SetWeaponIdleTime(0.0f); // Allow raising while in a transition, next
				GetActiveWeapon()->WeaponIdle();
				float delay = Min(GetActiveWeapon()->SequenceDuration(), HL2_ROLEPLAYER_WEAPON_READY_MAX_DELAY);
				SetNextAttack(gpGlobals->curtime + delay);
				m_LowerWeaponTimer.Set(HL2_ROLEPLAYER_WEAPON_IDLE_DELAY + delay);
				mIsWeaponManuallyLowered = false;
			}
			else if (m_LowerWeaponTimer.GetRemaining() < HL2_ROLEPLAYER_WEAPON_IDLE_DELAY)
			{
				m_LowerWeaponTimer.Set(HL2_ROLEPLAYER_WEAPON_IDLE_DELAY);
			}
		}
		else if (m_LowerWeaponTimer.Expired() && GetActiveWeapon()->Lower())
		{
			SetNextAttack(FLT_MAX);
			GetActiveWeapon()->WeaponIdle();
		}
	}
}

void CHL2Roleplayer::PlayerUse()
{
	bool isMainEntityNearby = false;
	CBaseEntity* pUsableEntity = mAimInfo.mhMainEntity;

	if (pUsableEntity != NULL)
	{
		if (mAimInfo.mhBackEntity != NULL)
		{
			pUsableEntity = mAimInfo.mhBackEntity;
		}
		else if (mAimInfo.mEndDistance < PLAYER_USE_RADIUS && !pUsableEntity->IsWorld())
		{
			isMainEntityNearby = true;
		}

		CBaseToggle* pBaseToggle;
		auto pPropertyData = pUsableEntity->GetPropertyDoorData();

		// Try toggling sliding property door
		if (FBitSet(m_afButtonPressed, IN_USE) && pPropertyData != NULL && pPropertyData->mProperty != NULL
			&& pPropertyData->mProperty.Get()->HasAccess(this) && (mAimInfo.mEndDistance < PLAYER_USE_RADIUS
				|| pPropertyData->mProperty.Get()->mType > EHL2RP_PropertyType::Home)
			&& (pBaseToggle = dynamic_cast<CBaseToggle*>(pUsableEntity)) != NULL)
		{
			// Auto-unlock Combine doors, for players fluency
			if (pPropertyData->mProperty.Get()->mType == EHL2RP_PropertyType::Combine)
			{
				UTIL_SetDoorLockState(pBaseToggle, NULL, false, pPropertyData->mDatabaseId.IsValid());
			}

			pBaseToggle->AcceptInput(pBaseToggle->m_toggle_state == TS_AT_TOP
				|| pBaseToggle->m_toggle_state == TS_GOING_UP ? "Close" : "Open", this, this);
			CLEARBITS(m_afButtonPressed, IN_USE); // Debounce USE key as we used the door (avoid deny sound)
		}
	}

	BaseClass::PlayerUse(); // Call late to be unaffected by possible IN_USE strip from pressed buttons

	// Handle special use
	if (FBitSet(m_afButtonPressed, IN_RELOAD))
	{
		if (isMainEntityNearby && mSpecialUseLastTime + HL2_ROLEPLAYER_DOUBLE_KEYPRESS_MAX_DELAY > gpGlobals->curtime)
		{
			pUsableEntity->Use(this, this, USE_SPECIAL1, 0.0f);
		}

		mSpecialUseLastTime = gpGlobals->curtime;
	}
	else if (FBitSet(m_nButtons, IN_RELOAD)
		&& gpGlobals->curtime >= mSpecialUseLastTime + HL2_ROLEPLAYER_USE_SPECIAL2_HOLD_TIME)
	{
		isMainEntityNearby ? pUsableEntity->Use(this, this, USE_SPECIAL2, 0.0f) : SendRootDialog(new CMainMenu(this));
		mSpecialUseLastTime = gpGlobals->curtime;
	}
}

CBaseEntity* CHL2Roleplayer::FindUseEntity()
{
	CBaseEntity* pEntity = (mAimInfo.mhBackEntity != NULL) ? mAimInfo.mhBackEntity : mAimInfo.mhMainEntity;
	return ((mAimInfo.mEndDistance < PLAYER_USE_RADIUS && IsUseableEntity(pEntity, 0)) ?
		pEntity : BaseClass::FindUseEntity());
}

void CHL2Roleplayer::PostThink()
{
	BaseClass::PostThink();

	if (IsAlive())
	{
		IsDamageProtected() ? SetRenderColor(0, 255, 0) : SetRenderColor(255, 255, 255);
		bool fixModel = false;
		auto& models = HL2RPRules()->mPlayerModelsByGroup;
		int modelIndex = models.Find(mModelGroup->ToCStr());
		mAimInfo.mhBackEntity.Term();
		CBaseEntity* pOldEntity = mAimInfo.mhMainEntity;
		GetAimInfo(mAimInfo);

		if (mAimInfo.mhMainEntity != pOldEntity)
		{
			SendAimingEntityHUD();
		}

		// Check access to current model and reset to a default HL2MP one on fail
		if (models.IsValidIndex(modelIndex) && !HasModelGroupAccess(modelIndex))
		{
			mModelGroup = NULL_STRING;
			fixModel = true;
		}

		// Try using current job model
		if (mModelGroup.Get() == NULL_STRING)
		{
			auto& jobs = HL2RPRules()->mJobByName[mFaction];
			int jobIndex = jobs.Find(mJobName->ToCStr());

			if (jobs.IsValidIndex(jobIndex) && jobs[jobIndex]->mModelGroupIndices.Count() > 0)
			{
				int groupIndex = RandomInt(0, jobs[jobIndex]->mModelGroupIndices.Count() - 1);
				groupIndex = jobs[jobIndex]->mModelGroupIndices[groupIndex];
				SetModel(groupIndex, RandomInt(0, models[groupIndex]->Count() - 1));
				fixModel = false;
			}
		}

		if (fixModel)
		{
			SetPlayerTeamModel();
		}
	}
	else if (GetTeamNumber() == TEAM_SPECTATOR && !IsBot() && (!mp_allowspectators.GetBool() || !IsAdmin()))
	{
		HandleCommand_JoinTeam(FIRST_GAME_TEAM); // Quit spectator correctly (team will be adjusted)
	}
}

#ifdef HL2RP_LEGACY
void CHL2Roleplayer::LocalPrint(int type, const char* pText, const char* pArg)
{
	Print(type, pText, pArg);
}

void CHL2Roleplayer::LocalDisplayHUDHint(EPlayerHUDHintType type,
	const char* pToken, const char* pArg1, const char* pArg2)
{
	SendHUDHint(type, pToken, pArg1, pArg2);
}

void CHL2Roleplayer::SendMainHUD()
{
	CLocalizeFmtStr<> text(this);
	GetMainHUD(text);
	SendHUDMessage(EPlayerHUDType::Main, text, HL2RP_MAIN_HUD_DEFAULT_X_POS, HL2RP_MAIN_HUD_DEFAULT_Y_POS,
		(mCrime > 0) ? HL2RP_MAIN_HUD_DEFAULT_CRIMINAL_TEXT_COLOR : HL2RP_MAIN_HUD_DEFAULT_NORMAL_TEXT_COLOR);
}

void CHL2Roleplayer::SendAimingEntityHUD()
{
	if (mAimInfo.mhMainEntity != NULL && mAimInfo.mEndDistance < PLAYER_USE_RADIUS)
	{
		CLocalizeFmtStr<> text(this);
		mAimInfo.mhMainEntity->GetHUDInfo(this, text);

		if (text.mLength > 0)
		{
			SendHUDMessage(EPlayerHUDType::AimingEntity, text, HL2RP_CENTER_HUD_SPECIAL_POS,
				HL2RP_AIMING_HUD_DEFAULT_Y_POS, mAimInfo.mhMainEntity->IsPlayer() ?
				HL2RP_AIMING_HUD_DEFAULT_PLAYER_COLOR : HL2RP_AIMING_HUD_DEFAULT_GENERAL_COLOR);
		}
	}
}
#endif // HL2RP_LEGACY

void CHL2Roleplayer::Print(int type, const char* pText, const char* pArg1, const char* pArg2)
{
#ifdef HL2RP_LEGACY
	CLocalizeFmtStr<> localizedText(this, type == HUD_PRINTTALK);

	// Auto localize
	if (*pText == '#')
	{
		pText = localizedText.Localize(pText, pArg1, pArg2);
	}

	if (type == HUD_PRINTTALK)
	{
		// Use SayText2 to prevent raw color hexcodes from being printed in console by default HL2DM client
		CSingleUserRecipientFilter filter(this);
		filter.MakeReliable();
		return UTIL_SayText2Filter(filter, this, true, pText);
	}
#endif // HL2RP_LEGACY

	ClientPrint(this, type, pText, pArg1, pArg2);
}

void CHL2Roleplayer::HUDThink()
{
	if (IsAlive())
	{
		++mSeconds;

#ifdef HL2RP_LEGACY
		SendAimingEntityHUD();
		CLocalizeFmtStr<> text(this);
		CUtlVector<CBasePlayer*> regionPlayers;
		GetPlayersInRegion(regionPlayers);

		if (!regionPlayers.IsEmpty())
		{
			int count = GetRegionHUD(regionPlayers, text);
			SendHUDMessage(EPlayerHUDType::Communication, text, HL2RP_COMM_HUD_DEFAULT_X_POS,
				HL2RP_COMM_HUD_DEFAULT_Y_POS + (HL2_ROLEPLAYER_REGION_MAX_PLAYERS - count)
				* HL2_ROLEPLAYER_HUD_FONT_ESTIMATED_HEIGHT, HL2RP_REGION_HUD_PLAYERS_DEFAULT_COLOR);
			text.mLength = 0;
		}

		if (GetZoneHUD(text))
		{
			SendHUDMessage(EPlayerHUDType::Zone, text, HL2RP_CENTER_HUD_SPECIAL_POS,
				HL2RP_ZONE_HUD_DEFAULT_Y_POS, HL2RP_ZONE_HUD_DEFAULT_COLOR);
		}
#endif // HL2RP_LEGACY

		if (mFaction == EFaction::Citizen && mZonesWithin[ECityZoneType::AutoCrime] != NULL
			&& mZonesWithin[ECityZoneType::Home] == NULL)
		{
			mCrime += HL2_ROLEPLAYER_AUTOMATIC_CRIME_GAIN;
		}
		else
		{
			--mCrime;
		}
	}

	if (!mDialogStack.IsEmpty())
	{
		mDialogStack.Tail()->Think();
	}

	FOR_EACH_DICT_FAST(mHomes, i)
	{
		VCRHook_Time(&mHomes[i]->mOwnerLastSeenTime);
		DAL().AddDAO(new CPropertiesSaveDAO(mHomes[i]));
	}

	SendMainHUD();
	SetNextThink(gpGlobals->curtime + HL2_ROLEPLAYER_HUD_THINK_PERIOD, HL2_ROLEPLAYER_HUD_THINK_CONTEXT);
}

void CHL2Roleplayer::SendHUDHint(EPlayerHUDHintType type, const char* pToken, const char* pArg1, const char* pArg2)
{
	if (!mLearnedHUDHints.IsBitSet(type))
	{
#ifdef HL2RP_FULL
		CSingleUserRecipientFilter filter(this);
		filter.MakeReliable();
		UserMessageBegin(filter, HL2RP_KEY_HINT_USER_MESSAGE);
		WRITE_LONG(type);
		WRITE_STRING(pToken);
		WRITE_STRING(pArg1);
		WRITE_STRING(pArg2);
		MessageEnd();
#else
		CLocalizeFmtStr<> text(this);
		SendHUDMessage(EPlayerHUDType::Alert, StringFuncs<char>::ToUpper((char*)text.Localize(pToken, pArg1, pArg2)),
			HL2RP_CENTER_HUD_SPECIAL_POS, HL2RP_ALERT_HUD_DEFAULT_Y_POS,
			HL2RP_ALERT_HUD_DEFAULT_COLOR, HL2_ROLEPLAYER_ALERT_HUD_HOLD_TIME);
#endif // HL2RP_FULL

		mLearnedHUDHints.SetBit(type);
	}
}

void CHL2Roleplayer::OnPreSendHUDMessage(bf_write* pWriter)
{
	bf_read reader(pWriter->GetData(), pWriter->GetNumBytesWritten());
	int channel = reader.ReadByte() % ARRAYSIZE(mHUDExpireTimers);

	// Fix conflicting channel already in use by our internal HUD
	if (mHUDExpireTimers[channel].mOriginalType > EPlayerHUDType::None && FixHUDChannel(channel))
	{
		int curBit = pWriter->m_iCurBit;
		pWriter->Reset();
		pWriter->WriteByte(channel);
		pWriter->SeekToBit(curBit); // Restore GetNumBytesWritten return value (needed)
	}

	reader.SeekRelative(200); // Seek until holdtime
	mHUDExpireTimers[channel].Set(EPlayerHUDType::None, reader.ReadFloat());
}

void CHL2Roleplayer::SendHUDMessage(EPlayerHUDType type, const char* pMessage,
	float xPos, float yPos, const Color& color, float holdTime)
{
	hudtextparms_t params{};
	params.channel = type;

	// Reuse any active channel in use for this type to prevent spamming messages of the same type
	for (int i = 0; i < ARRAYSIZE(mHUDExpireTimers); ++i)
	{
		if (mHUDExpireTimers[i].mOriginalType == type && !mHUDExpireTimers[i].Expired())
		{
			params.channel = i;
			mHUDExpireTimers[i].Force();
			break;
		}
	}

	params.x = xPos;
	params.y = yPos;
	params.r1 = color.r();
	params.g1 = color.g();
	params.b1 = color.b();
	params.a1 = color.a();
	params.holdTime = holdTime;
	FixHUDChannel(params.channel);
	HL2RPRules()->mHUDMsgInterceptor.Pause();
	UTIL_HudMessage(this, params, gHL2RPLocalizer.Localize(this, pMessage));
	HL2RPRules()->mHUDMsgInterceptor.Resume();
	mHUDExpireTimers[params.channel].Set(type, holdTime);
}

bool CHL2Roleplayer::FixHUDChannel(int& channel)
{
	if (!mHUDExpireTimers[channel].Expired())
	{
		for (int i = 0; i < ARRAYSIZE(mHUDExpireTimers); ++i)
		{
			if (mHUDExpireTimers[i].Expired())
			{
				channel = i;
				return true;
			}
		}
	}

	return false;
}

bool CHL2Roleplayer::HasFactionAccess(int faction)
{
	return (faction == EFaction::Citizen || IsAdmin()
		|| mAccessFlags.IsAnyBitSet(EPlayerAccessFlag::Combine, EPlayerAccessFlag::VIPCombine));
}

bool CHL2Roleplayer::HasJobAccess(CJobData* pJob)
{
	return (FBitSet(mAccessFlags, pJob->mRequiredAccessFlag) == pJob->mRequiredAccessFlag
		|| (pJob->mRequiredAccessFlag < EPlayerAccessFlag::Admin ?
			IsAdmin() : mAccessFlags >= pJob->mRequiredAccessFlag));
}

int CHL2Roleplayer::CheckCurrentJobAccess()
{
	auto& jobs = HL2RPRules()->mJobByName[mFaction];
	int jobIndex = jobs.Find(mJobName->ToCStr());

	if (!jobs.IsValidIndex(jobIndex) || !HasJobAccess(jobs[jobIndex]))
	{
		// If on Combine, (re)assign to first allowed job. Combine members should always have a job, for consistency.
		if (mFaction == EFaction::Combine)
		{
			FOR_EACH_MAP_FAST(jobs, i)
			{
				if (HasJobAccess(jobs[i]))
				{
					mJobName = MAKE_STRING(jobs.Key(i));
					return i;
				}
			}
		}

		mJobName = NULL_STRING;
		return jobs.InvalidIndex();
	}

	return jobIndex;
}

bool CHL2Roleplayer::HasModelGroupAccess(int groupIndex, int fallbackType)
{
	auto& jobs = HL2RPRules()->mJobByName[mFaction];
	int jobIndex = jobs.Find(mJobName->ToCStr());

	// Check job model first, no matter its type
	if (jobs.IsValidIndex(jobIndex) && jobs[jobIndex]->mModelGroupIndices.HasElement(groupIndex))
	{
		return true;
	}
	else if (HL2RPRules()->mPlayerModelsByGroup.IsValidIndex(groupIndex))
	{
		fallbackType = HL2RPRules()->mPlayerModelsByGroup[groupIndex]->mType;
	}

	switch (fallbackType)
	{
	case EPlayerModelType::Citizen:
	{
		return (mFaction == EFaction::Citizen
			&& (!jobs.IsValidIndex(jobIndex) || jobs[jobIndex]->mModelGroupIndices.Count() < 1));
	}
	case EPlayerModelType::VIPCitizen:
	{
		return ((mFaction == EFaction::Citizen || gCopVIPSkinsAllowCVar.GetBool())
			&& (mAccessFlags.IsBitSet(EPlayerAccessFlag::VIPCitizen) || IsAdmin()));
	}
	case EPlayerModelType::Admin:
	{
		return IsAdmin();
	}
	case EPlayerModelType::Root:
	{
		return mAccessFlags.IsBitSet(EPlayerAccessFlag::Root);
	}
	}

	return false;
}

void CHL2Roleplayer::ChangeFaction(int faction, string_t newJobName)
{
	if (faction != mFaction)
	{
		mFaction = faction;
		mJobName = newJobName;

		if (GetTeamNumber() != TEAM_SPECTATOR)
		{
			if (IsAlive())
			{
				if (faction == EFaction::Combine && !mRestorableEquipment.IsValid())
				{
					mRestorableEquipment.Attach(new CRestorablePlayerEquipment(this));
				}

				ClearActiveWeapon();
			}

			ChangeTeam();
		}
	}
}

void CHL2Roleplayer::ModifyOrAppendCriteria(AI_CriteriaSet& set)
{
	BaseClass::ModifyOrAppendPlayerCriteria(set);
	set.AppendCriteria("soundsType", CNumStr(GetPlayerModelType()));
}

int CHL2Roleplayer::OnTakeDamage(const CTakeDamageInfo& info)
{
	if (IsAlive())
	{
		// Check job's god mode
		auto& jobs = HL2RPRules()->mJobByName[mFaction];
		int index = jobs.Find(mJobName->ToCStr());

		if (jobs.IsValidIndex(index) && jobs[index]->mIsGod)
		{
			return 0;
		}
		else if (IsDamageProtected())
		{
			// Block the damage if it isn't clearly self guilty (for realism)
			if (info.GetAttacker() != this || FBitSet(info.GetDamageType(), DMG_CRUSH))
			{
				return 0;
			}
		}
		else if (mFaction == EFaction::Combine)
		{
			CHL2Roleplayer* pPlayer = ToHL2Roleplayer(info.GetAttacker());

			if (pPlayer != NULL && pPlayer != this && pPlayer->mFaction == mFaction)
			{
				return 0; // Block inter-Combine damage
			}
		}
	}

	int result = BaseClass::OnTakeDamage(info);

	if (result > 0 && IsAlive())
	{
		DispatchResponse("TLK_PAIN");
	}

	return result;
}

void CHL2Roleplayer::Event_Killed(const CTakeDamageInfo& info)
{
	if (mFaction == EFaction::Combine)
	{
		ClearActiveWeapon();
	}

	BaseClass::Event_Killed(info);
	SetArmorValue(0);
	mAimInfo.mhMainEntity.Term();
}

void CHL2Roleplayer::OnDatabasePropChanged(const SUtlField& field, EPlayerDatabasePropType type)
{
	if (mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
	{
		DAL().AddDAO(new CPlayersMainDataSaveDAO(this, field, type));
	}
	// Set to update main data once player loads (except from default health change)
	else if (type != EPlayerDatabasePropType::Health || field.mInt != 100)
	{
		mDatabaseIOFlags.SetBit(EPlayerDatabaseIOFlag::UpdateMainDataOnLoaded);
	}
}

void CHL2Roleplayer::LoadFromDatabase()
{
	if (!IsBot())
	{
		DAL().AddDAO(new CPlayerLoadDAO(this));
	}
}

void CHL2Roleplayer::OnChatMessagePassed(CBasePlayer* pTarget, bool teamOnly)
{
	// Notify sender of successful region messages, to confirm when in doubt.
	// NOTE: Limit visibility to prevent finding hidden players.
	if (teamOnly && FVisible(pTarget))
	{
		CSingleUserRecipientFilter filter(this);
		Vector origin = pTarget->GetAbsOrigin();
		origin.z += HL2_ROLEPLAYER_BEAM_RING_WIDTH / 2.0f; // Clear ground
		Color color = HL2RP_REGION_HUD_PLAYERS_DEFAULT_COLOR;
		te->BeamRingPoint(filter, 0.0f, origin, 0.0f, HL2_ROLEPLAYER_BEAM_RING_RADIUS,
			PrecacheModel(HL2_ROLEPLAYER_BEAMS_PATH), 0, 0, 0, HL2_ROLEPLAYER_BEAM_RING_DURATION,
			HL2_ROLEPLAYER_BEAM_RING_WIDTH, 0, 0.0f, color.r(), color.g(), color.b(), color.a(), 0);
	}
}

void CHL2Roleplayer::SendBeam(const Vector& end, const Color& color, float width)
{
	CSingleUserRecipientFilter filter(this);
	filter.SetIgnorePredictionCull(true);
	Vector start = EyePosition();
	start.z -= HL2_ROLEPLAYER_BEAMS_EYE_OFFSET_DOWN;
	te->BeamPoints(filter, 0.0f, &start, &end, PrecacheModel(HL2_ROLEPLAYER_BEAMS_PATH),
		0, 0, 0, HL2_ROLEPLAYER_HUD_THINK_PERIOD, width, width, 0, 0.0f,
		color.r(), color.g(), color.b(), HL2_ROLEPLAYER_BEAMS_ALPHA, 0);
}

void CHL2Roleplayer::SendRootDialog(INetworkDialog* pDialog)
{
	mDialogStack.PurgeAndDeleteElements();
	PushAndSendDialog(pDialog);
}

void CHL2Roleplayer::PushAndSendDialog(INetworkDialog* pDialog)
{
	mDialogStack.AddToTail(pDialog);
	pDialog->Send();
}

void CHL2Roleplayer::RewindCurrentDialog()
{
	// Kill current dialog
	delete mDialogStack.Tail();
	mDialogStack.RemoveMultipleFromTail(1);

	if (!mDialogStack.IsEmpty())
	{
		mDialogStack.Tail()->Send();
		CSingleUserRecipientFilter filter(this);
		EmitSound(filter, GetSoundSourceIndex(), NETWORK_DIALOG_REWIND_SOUND);
	}
}

static void HandleAccessChangeCommand(const CCommand& args, bool grant)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(UTIL_GetCommandClient());

	if (UTIL_IsCommandIssuedByServerAdmin()
		|| (pPlayer != NULL && pPlayer->mAccessFlags.IsBitSet(EPlayerAccessFlag::Root)))
	{
		CHL2Roleplayer* pTarget = NULL;
		int userId = Q_atoi(args.Arg(1));

		if (userId > 0)
		{
			pTarget = ToHL2Roleplayer(UTIL_PlayerByUserId(userId));
		}
		else if (pPlayer != NULL)
		{
			pTarget = ToHL2Roleplayer(pPlayer->mAimInfo.mhMainEntity);
		}

		if (pTarget != NULL)
		{
			CLocalizeFmtStr<> issuerFlagsText(pPlayer, true), targetFlagsText(pTarget, true);
			bool flagsChanged = false, useAllFlags = (args.FindArg("all") != NULL);

			const char* flagTokens[EPlayerAccessFlag::_Count][2] =
			{
				{"cop", "#HL2RP_Faction_Combine"}, {"vipcitizen", "#HL2RP_VIPCitizen"},
				{"vipcop", "#HL2RP_VIPCombine"}, {"admin", "#HL2RP_Admin"}, {"root", "#HL2RP_Root"}
			}, * pSeparator = "";

			for (int i = 0; i < ARRAYSIZE(flagTokens); ++i)
			{
				if (useAllFlags || args.FindArg(flagTokens[i][0]) != NULL)
				{
					if (grant)
					{
						if (pTarget->mAccessFlags.IsBitSet(i))
						{
							continue;
						}

						pTarget->mAccessFlags.SetBit(i);
					}
					else if (pTarget->mAccessFlags.IsBitSet(i))
					{
						pTarget->mAccessFlags.ClearBit(i);
					}
					else
					{
						continue;
					}

					flagsChanged = true;
					issuerFlagsText += pSeparator;
					targetFlagsText += pSeparator;
					issuerFlagsText.Format("%s", gHL2RPLocalizer.Localize(pPlayer, flagTokens[i][1]));
					targetFlagsText.Format("%s", gHL2RPLocalizer.Localize(pTarget, flagTokens[i][1]));
					pSeparator = ", ";
				}
			}

			if (flagsChanged)
			{
				// Print before checking derived access, so that messages like team change get printed later
				UTIL_ReplyToCommand(HUD_PRINTTALK, grant ? "#HL2RP_Access_Granted_Issuer"
					: "#HL2RP_Access_Removed_Issuer", pTarget->GetPlayerName(), issuerFlagsText);
				pTarget->Print(HUD_PRINTTALK, grant ? "#HL2RP_Access_Granted_Target"
					: "#HL2RP_Access_Removed_Target", UTIL_GetCommandIssuerName(), targetFlagsText);
				string_t oldJobName = pTarget->mJobName;

				if (!pTarget->HasFactionAccess(pTarget->mFaction))
				{
					pTarget->ChangeFaction(~pTarget->mFaction);
				}
				else if (oldJobName != NULL_STRING)
				{
					pTarget->CheckCurrentJobAccess();

					if (pTarget->mJobName.Get() != oldJobName && pTarget->mFaction == EFaction::Combine)
					{
						pTarget->CommitSuicide();
					}
				}
			}
		}
	}
}

CON_COMMAND(give_access, "[userid] [all|cop|vipcitizen|vipcop|admin|root]..."
	" - Gives a player certain grant/s. If userid isn't set, command will target aiming player.")
{
	HandleAccessChangeCommand(args, true);
}

CON_COMMAND(remove_access, "[userid] [all|cop|vipcitizen|vipcop|admin|root]..."
	" - Removes grant/s from a player. If userid isn't set, command will target aiming player.")
{
	HandleAccessChangeCommand(args, false);
}
