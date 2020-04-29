#include "cbase.h"
#include "CHL2RP_Player.h"
#include "CHL2RP_PlayerDialog.h"
#include "ammodef.h"
#include "weapon_citizenpackage.h"
#include "CHL2RP.h"
#include "CHL2RP_Colors.h"
#include "in_buttons.h"
#include "CPropRationDispenser.h"
#include "CHL2RP_Util.h"
#include "CNetworkVarEx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Forward declare required phrase headers
extern const char WELCOME_CENTER_HUD_HEADER[];
extern const char MAIN_HUD_HEADER[];
extern const char REGION_HUD_HEADER[];
extern const char EXTENDED_REGION_HUD_HEADER[];
extern const char WEAPONS_AND_CLIPS_LOADED_HEADER[];
extern const char RESERVE_AMMO_LOADED_HEADER[];

const float HUD_INTERVAL = 1.0f;
const float AIM_IDLE_WAIT_SECONDS = 5.0;
const float IDEAL_WELCOME_MESSAGES_DISPLAY_DELAY = 2.0f; // Apply an heuristic delay since these connect messages may get skipped on client otherwise
const float WELCOME_HUD_DURATION = 10.0f;

const int DEFAULT_RATIONS_AMMO = 5;

const float INVENTORY_OPEN_USE_KEY_SECONDS = 1.0f;
const char INVENTORY_OPENING_SOUND[] = "buttons/button1.wav";

const float MAX_EDITING_ZONE_HEIGHT_SPEED_PER_SECOND = 100.0f;

const float IDEAL_MAIN_HUD_X_POS = 0.015f;
const float IDEAL_MAIN_HUD_Y_POS = 0.03f;
const float IDEAL_REGION_HUD_X_POS = 0.715f;
const float IDEAL_REGION_HUD_Y_POS = 0.65f;
const float IDEAL_WELCOME_HUD_Y_POS = 0.03f;

const int MAX_PLAYER_DATABASE_WEAPON_NAME_LENGTH = 32;

// Make each channel have a specific purpose for consistency and helping against overlapping:
// Reminder: #define MAX_NETMESSAGE	6, sadly this is the HUD channel limit client/engine sided (until date)
enum HudChannelIndex
{
	MAIN_HUD_CHANNEL,
	CRIME_HUD_CHANNEL,
	REGION_HUD_CHANNEL,
	PHONE_HUD_CHANNEL,
	PVP_PROTECTION_HUD_CHANNEL,
	PLAYER_INFO_HUD_CHANNEL,
	DOOR_INFO_HUD_CHANNEL = PLAYER_INFO_HUD_CHANNEL,
	WELCOME_HUD_CHANNEL = PLAYER_INFO_HUD_CHANNEL
};

enum TempEntityIndex
{
	EDITING_ZONE_TEMP_ENT_INDEX,
	INSIDE_SAVED_ZONE_TEMP_ENT_INDEX
};

int g_iBeamModelIndex;

LINK_ENTITY_TO_CLASS(player, CHL2RP_Player);

CHL2RP_Player::CHL2RP_Player()
{
	m_iSeconds = 0;
	m_iCrime = 0;
	Q_memset(m_flNextTempEntityDisplayTime, 0, sizeof m_flNextTempEntityDisplayTime);
	Q_memset(m_flNextHUDChannelTime, 0, sizeof m_flNextHUDChannelTime);
	m_pCurrentDialog = NULL;
	m_iLastDialogLevel = INT_MAX;
	m_AccessFlag = ACCESS_FLAG_NONE;
	m_flNextOpenInventoryTime = FLT_MAX;
	m_iSyncCapabilities = SYNC_CAPABILITY_NONE;

	// Force smooth camera view outside vehicle from the beginning (why not?):
	if (engine->IsDedicatedServer())
	{
		UTIL_SendConVarValue(this, "sv_client_predict", "1");
	}
	else
	{
		engine->ServerCommand("sv_client_predict 1\n");
	}
}

CHL2RP_Player::~CHL2RP_Player()
{
	delete m_pCurrentDialog;
}

void CHL2RP_Player::TrySyncWeaponFromBaseCombatCharacter(CBaseCombatWeapon &weapon, const CBaseCombatCharacter *pCharacter)
{
	CHL2RP_Player *pPlayer = ToThisClass(pCharacter);

	if ((pPlayer != NULL) && (CHL2RP::s_pSQL != NULL))
	{
		if (pPlayer->m_iSyncCapabilities & CHL2RP_Player::SYNC_CAPABILITY_ALTER_WEAPONS)
		{
			CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPlayerWeaponTxn(*pPlayer, weapon));
		}
	}
}

void CHL2RP_Player::TrySyncMainData()
{
	if (m_iSyncCapabilities & SYNC_CAPABILITY_UPSERT_MAIN_DATA)
	{
		CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPlayerTxn(*this));
	}
}

void CHL2RP_Player::Spawn()
{
	BaseClass::Spawn();
	GiveNamedItem("weapon_citizensuitcase");
	SetAmmoCount(1, GetAmmoDef()->Index("citizensuitcase"));

	if (GetAmmoCount("citizenpackage") > 0)
		GiveNamedItem("weapon_citizenpackage");

	if (m_iSyncCapabilities & SYNC_CAPABILITY_LOAD_WEAPONS)
	{
		m_iSyncCapabilities &= ~SYNC_CAPABILITY_LOAD_WEAPONS;
		CHL2RP::s_pSQL->AddAsyncTxn(new CLoadPlayerWeaponsTxn(*this));
	}
}

void CHL2RP_Player::InitialSpawn()
{
	SetContextThink(&CHL2RP_Player::DisplayWelcomeHUD, gpGlobals->curtime + IDEAL_WELCOME_MESSAGES_DISPLAY_DELAY, "DisplayWelcomeHUDContext");
	BaseClass::InitialSpawn();
}

void CHL2RP_Player::DisplayWelcomeHUD()
{
	char welcomeDisplay[MAX_VALVE_USER_MSG_TEXT_DATA];
	CHL2RP_Phrases::LoadForPlayer(this, welcomeDisplay, sizeof welcomeDisplay, WELCOME_CENTER_HUD_HEADER, NULL, (int)INVENTORY_OPEN_USE_KEY_SECONDS);
	CHL2RP_Util::HudMessage(this, WELCOME_HUD_CHANNEL, -1.0f, IDEAL_WELCOME_HUD_Y_POS, WELCOME_HUD_DURATION, WelcomeHUDColor(), welcomeDisplay);
}

void CHL2RP_Player::Precache()
{
	BaseClass::Precache();
	enginesound->PrecacheSound(INVENTORY_OPENING_SOUND);
	g_iBeamModelIndex = PrecacheModel("sprites/laserbeam.vmt");

#ifdef HL2DM_RP
	for (int i = 0; i < ARRAYSIZE(CHL2RP::s_CompatAnimationModelPaths); i++)
	{
		for (int j = CHL2RP::s_CompatAnimationModelPaths[i].First(); CHL2RP::s_CompatAnimationModelPaths[i].IsValidIndex(j);
			j = CHL2RP::s_CompatAnimationModelPaths[i].Next(j))
		{
			PrecacheModel(CHL2RP::s_CompatAnimationModelPaths[i][j]);
		}
	}
#endif
}

void CHL2RP_Player::PreThink()
{
	if (m_flNextHUDChannelTime[MAIN_HUD_CHANNEL] <= gpGlobals->curtime)
	{
		char mainHud[MAX_VALVE_USER_MSG_TEXT_DATA];
		int hours = m_iSeconds / 3600, minutes = (m_iSeconds / 60) % 60, remainingSeconds = m_iSeconds % 60;
		CHL2RP_Phrases::LoadForPlayer(this, mainHud, sizeof mainHud, MAIN_HUD_HEADER, NULL, hours,
			minutes, remainingSeconds, GetAmmoCount("citizenpackage"), m_iCrime);

		if (m_iCrime > 0)
		{
			CHL2RP_Util::HudMessage(this, MAIN_HUD_CHANNEL, IDEAL_MAIN_HUD_X_POS, IDEAL_MAIN_HUD_Y_POS, HUD_INTERVAL, CriminalMainHUDColor(), mainHud);
		}
		else
		{
			CHL2RP_Util::HudMessage(this, MAIN_HUD_CHANNEL, IDEAL_MAIN_HUD_X_POS, IDEAL_MAIN_HUD_Y_POS, HUD_INTERVAL, PeacefulMainHUDColor(), mainHud);
		}
	}

	if (m_flNextHUDChannelTime[REGION_HUD_CHANNEL] <= gpGlobals->curtime)
	{
		char regionHud[MAX_VALVE_USER_MSG_TEXT_DATA], regionPlayersHud[MAX_VALVE_USER_MSG_TEXT_DATA];
		int numRegionPlayers = 0;

		// Max 5 players to make 170 characters seems a reasonable size for the rest
		for (int i = 1, numChars = 0; i <= gpGlobals->maxClients; i++)
		{
			CHL2RP_Player *pNearPlayer = ToThisClassFast(CBaseEntity::Instance(i));

			if ((pNearPlayer != NULL) && (pNearPlayer != this) && (GetAbsOrigin() - pNearPlayer->GetAbsOrigin()).IsLengthLessThan(300))
			{
				if (numRegionPlayers++ < 5)
				{
					numChars += Q_snprintf(regionPlayersHud + numChars, sizeof regionPlayersHud - numChars, "\n%s", pNearPlayer->GetPlayerName());
				}
			}
		}

		if (numRegionPlayers > 0)
		{
			if (numRegionPlayers < 6)
			{
				CHL2RP_Phrases::LoadForPlayer(this, regionHud, sizeof regionHud, REGION_HUD_HEADER, NULL, regionPlayersHud);
			}
			else
			{
				CHL2RP_Phrases::LoadForPlayer(this, regionHud, sizeof regionHud,
					EXTENDED_REGION_HUD_HEADER, NULL, numRegionPlayers, regionPlayersHud);
			}

			CHL2RP_Util::HudMessage(this, REGION_HUD_CHANNEL, IDEAL_REGION_HUD_X_POS, IDEAL_REGION_HUD_Y_POS, HUD_INTERVAL, RegionHUDColor(), regionHud);
		}
	}

	if ((int)gpGlobals->curtime > (int)(gpGlobals->curtime - TICK_INTERVAL))
	{
		m_iSeconds = CHL2RP_Util::EnsureAddition(m_iSeconds, 1);

		if (m_iCrime > 0)
		{
			m_iCrime--;
		}

		TrySyncMainData();
	}

	CHL2RP_PlayerZoneMenu *zoneMenu = dynamic_cast<CHL2RP_PlayerZoneMenu *>(m_pCurrentDialog);

	if (zoneMenu != NULL)
	{
		switch (zoneMenu->m_HeightEditState)
		{
		case CHL2RP_PlayerZoneMenu::HeightEditState::RAISING:
		{
			zoneMenu->m_CurrentZone.m_flMaxHeight += MAX_EDITING_ZONE_HEIGHT_SPEED_PER_SECOND * TICK_INTERVAL;
			break;
		}
		case CHL2RP_PlayerZoneMenu::HeightEditState::LOWERING:
		{
			zoneMenu->m_CurrentZone.m_flMaxHeight -= MAX_EDITING_ZONE_HEIGHT_SPEED_PER_SECOND * TICK_INTERVAL;
			break;
		}
		}

		if (m_flNextTempEntityDisplayTime[EDITING_ZONE_TEMP_ENT_INDEX] <= gpGlobals->curtime)
		{
			CSingleUserRecipientFilter user(this);
			CHL2RP_Util::DisplayZone(user, zoneMenu->m_CurrentZone.m_vecPointsXY, zoneMenu->m_CurrentZone.m_iNumPoints,
				zoneMenu->m_CurrentZone.m_flMinHeight, zoneMenu->m_CurrentZone.m_flMaxHeight, g_iBeamModelIndex, HUD_INTERVAL, 1.0f, 1.0f, COLOR_RED);
			m_flNextTempEntityDisplayTime[EDITING_ZONE_TEMP_ENT_INDEX] = gpGlobals->curtime + HUD_INTERVAL;
		}
	}

	if (m_flNextTempEntityDisplayTime[INSIDE_SAVED_ZONE_TEMP_ENT_INDEX] <= gpGlobals->curtime)
	{
		for (int i = 0; i < CHL2RP::s_Zones.Count(); i++)
		{
			if (CHL2RP_Util::PointInside2DPolygon(GetAbsOrigin().AsVector2D(), CHL2RP::s_Zones[i].m_vecPointsXY, CHL2RP::s_Zones[i].m_iNumPoints)
				&& CHL2RP::s_Zones[i].m_flMinHeight <= GetAbsOrigin().z && GetAbsOrigin().z <= CHL2RP::s_Zones[i].m_flMaxHeight)
			{
				CSingleUserRecipientFilter user(this);
				CHL2RP_Util::DisplayZone(user, CHL2RP::s_Zones[i].m_vecPointsXY, CHL2RP::s_Zones[i].m_iNumPoints, CHL2RP::s_Zones[i].m_flMinHeight, CHL2RP::s_Zones[i].m_flMaxHeight, g_iBeamModelIndex, HUD_INTERVAL, 1.0f, 1.0f, COLOR_BLUE);
				m_flNextTempEntityDisplayTime[INSIDE_SAVED_ZONE_TEMP_ENT_INDEX] = gpGlobals->curtime + HUD_INTERVAL;
				break;
			}
		}
	}

	// Check if some extended network props changed externally to ensure calling their NetworkStateChangedEx
	CheckNetworkVarExChanged(m_iHealth);
	CheckNetworkVarExChanged(m_ArmorValue);
	CheckNetworkArrayExChanged(m_iAmmo);
	CheckNetworkArrayExChanged(m_hMyWeapons);

	BaseClass::PreThink();
}

void CHL2RP_Player::Event_Killed(const CTakeDamageInfo &info)
{
	// Will I drop my rations?
	int myRations = 0, lostRations = 0;
	CHL2RP_Player *pAttacker = ToThisClass(info.GetAttacker());
	char msg[MAX_VALVE_USER_MSG_TEXT_DATA];

	// If another player with less rations killed me, drop his amount:
	if (pAttacker != NULL && pAttacker->GetAmmoCount("citizenpackage") < myRations)
	{
		lostRations = pAttacker->GetAmmoCount("citizenpackage"); // Aux
		myRations = lostRations / 2; // The "placebo"
		lostRations -= myRations;

		Q_snprintf(msg, sizeof msg, DYNAMIC_TALK_COLOR "%s " STATIC_TALK_COLOR "killed you. You have lost " DYNAMIC_TALK_COLOR "%i" STATIC_TALK_COLOR " ration/s",
			pAttacker->GetPlayerName(), lostRations);
	}
	else
	{
		// I will drop my rations:
		lostRations = GetAmmoCount("citizenpackage"); // Aux
		myRations = lostRations / 2; // The "placebo"
		lostRations -= myRations;

		Q_snprintf(msg, sizeof msg, STATIC_TALK_COLOR "You died. You have lost " DYNAMIC_TALK_COLOR "%i " STATIC_TALK_COLOR "ration/s", lostRations);
	}

	if (lostRations)
	{
		SetAmmoCount(myRations, GetAmmoDef()->Index("citizenpackage"));
	}

	CWeaponCitizenPackage *package = dynamic_cast<CWeaponCitizenPackage *>(Weapon_OwnsThisType("weapon_citizenpackage"));

	if (package != NULL)
	{
		package->SetSecondaryAmmoCount(lostRations);
		Weapon_Drop(package);
	}

	ClientPrint(this, HUD_PRINTTALK, msg);
	BaseClass::Event_Killed(info);
}

void CHL2RP_Player::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	if (ucmd->buttons & IN_USE)
	{
		if (!(m_nButtons & IN_USE))
		{
			// Start opening inventory...
			m_flNextOpenInventoryTime = gpGlobals->curtime + INVENTORY_OPEN_USE_KEY_SECONDS;
			CPASAttenuationFilter filter;
			filter.AddAllPlayers();
			enginesound->EmitSound(filter, entindex(), CHAN_AUTO, INVENTORY_OPENING_SOUND, VOL_NORM, SNDLVL_35dB);

			// Begin attempting to use a facing dispenser...
			trace_t tr;
			const Vector &origin = EyePosition();
			const Vector &end = origin + EyeDirection3D() * PLAYER_USE_RADIUS;
			UTIL_TraceLine(origin, end, MASK_ALL, this, COLLISION_GROUP_NONE, &tr);
			CPropRationDispenser *pDispenser = dynamic_cast<CPropRationDispenser *>(tr.m_pEnt);

			if (pDispenser != NULL)
			{
				pDispenser->CHL2RP_PlayerUse(*this);
			}
		}
		else if (m_flNextOpenInventoryTime <= gpGlobals->curtime)
		{
			enginesound->StopSound(entindex(), CHAN_AUTO, INVENTORY_OPENING_SOUND);
			(new CHL2RP_PlayerMainMenu(this))->Display();
			m_flNextOpenInventoryTime = FLT_MAX;
		}
	}
	else if (m_flNextOpenInventoryTime != FLT_MAX)
	{
		enginesound->StopSound(entindex(), CHAN_AUTO, INVENTORY_OPENING_SOUND);
		m_flNextOpenInventoryTime = FLT_MAX;
	}

	BaseClass::PlayerRunCommand(ucmd, moveHelper);
}

void CHL2RP_Player::NetworkStateChangedEx_m_iAmmo(int index, const int &oldValue)
{
	// Prevent negative ammo from external sources:
	if (m_iAmmo[index] < 0)
	{
		m_iAmmo.m_Value[index] = 0;
	}

	if (!IsDisconnecting() && (m_iSyncCapabilities & SYNC_CAPABILITY_ALTER_AMMO))
	{
		if (m_iAmmo[index] > 0)
		{
			CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPlayerAmmoTxn(*this, index));
		}
		else
		{
			CHL2RP::s_pSQL->AddAsyncTxn(new CDeletePlayerAmmoTxn(*this, index));
		}
	}
}

void CHL2RP_Player::NetworkStateChangedEx_m_iHealth(const int &oldValue)
{
	// Prevent negative health from external sources
	if (m_iHealth < 0)
	{
		m_iHealth.m_Value = 0;
	}

	TrySyncMainData();
}

void CHL2RP_Player::NetworkStateChangedEx_m_ArmorValue(const int &oldValue)
{
	// Prevent negative armor from external sources:
	if (m_ArmorValue < 0)
	{
		m_ArmorValue.m_Value = 0;
	}

	TrySyncMainData();
}

void CHL2RP_Player::NetworkStateChangedEx_m_hMyWeapons(int index, const CBaseCombatWeaponHandle &oldValue)
{
	if (IsPlayer() && CHL2RP::s_pSQL != NULL)
	{
		if (m_iSyncCapabilities & SYNC_CAPABILITY_ALTER_WEAPONS)
		{
			if (m_hMyWeapons[index] != NULL)
			{
				CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPlayerWeaponTxn(*this, *m_hMyWeapons[index]));
			}
			// On normal conditions this check is unneeded, because function only enters with a different value.
			// But both new and old values are same when being called by a passive listener of external changes
			else if (oldValue != NULL)
			{
				if (IsDisconnecting() || gEntList.IsClearingEntities())
				{
					oldValue->Remove();
				}
				else
				{
					CHL2RP::s_pSQL->AddAsyncTxn(new CDeletePlayerWeaponTxn(*this, *oldValue));
				}
			}
		}
	}
}

// Purpose: Wrap CheckEntityNetworkVarExChanged for a passed CNetworkVarEx of CHL2RP_Player (or a base class)
template<class Handler>
void CHL2RP_Player::CheckNetworkVarExChanged(Handler &networkVarEx) const
{
	CHL2RP::CheckEntityNetworkVarExChanged(*this, networkVarEx);
}

// Purpose: Wrap CheckEntityNetworkArrayExChanged for a passed CNetworkArrayEx of CHL2RP_Player (or a base class)
template<class Handler>
void CHL2RP_Player::CheckNetworkArrayExChanged(Handler &networkArrayBaseEx) const
{
	CHL2RP::CheckEntityNetworkArrayExChanged(*this, networkArrayBaseEx);
}

CPlayerTxn::CPlayerTxn(CHL2RP_Player &player)
{
	ConstructPlayerTxn(player);
}

// Purpose: Common initializator to allow constructors access to multi level inherited transactions
void CPlayerTxn::ConstructPlayerTxn(CHL2RP_Player &player)
{
	Q_strcpy(m_szPlayerSteamID, player.GetNetworkIDString());
}

void CPlayerTxn::BindParametersGeneric(int iQueryId, const void *pStmt) const
{
	CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerSteamID, 1);
}

bool CPlayerTxn::IsValidPlayerTxnAndSteamIDEquals(const ThisClass *pTxn) const
{
	return ((pTxn != NULL) && !Q_strcmp(pTxn->m_szPlayerSteamID, m_szPlayerSteamID));
}

CSetUpPlayersTxn::CSetUpPlayersTxn()
{
	AddFormattedQuery("CREATE TABLE IF NOT EXISTS Player (steamid VARCHAR (%i) NOT NULL, name VARCHAR (%i), "
		"seconds INTEGER, crime INTEGER, accessflag INTEGER, health INTEGER, suit INTEGER, "
		"PRIMARY KEY (steamid), CHECK (seconds >= 0 AND crime >= 0 AND accessflag >= %i AND health > 0 AND suit >= 0));",
		MAX_NETWORKID_LENGTH, MAX_PLAYER_NAME_LENGTH, CHL2RP_Player::ACCESS_FLAG_NONE);
}

bool CSetUpPlayersTxn::ShouldUsePreparedStatements() const
{
	return false;
}

CLoadPlayerTxn::CLoadPlayerTxn(CHL2RP_Player &player) : CPlayerTxn(player)
{
	AddQuery("SELECT * FROM Player WHERE steamid = ?;");
}

void CLoadPlayerTxn::HandleQueryResults() const
{
	CHL2RP_Player *pPlayer = CHL2RP_Util::PlayerByNetworkIDString(m_szPlayerSteamID);

	if (pPlayer != NULL)
	{
		// Add up the resources instead of setting them either if player exists or is new,
		// since he may already got some of them in a legit way since his connection:
		if (CHL2RP::s_pSQL->FetchNextRow())
		{
			int m_iSeconds = CHL2RP::s_pSQL->FetchInt("seconds"), m_iCrime = CHL2RP::s_pSQL->FetchInt("crime"),
				accessFlag = CHL2RP::s_pSQL->FetchInt("accessflag");
			pPlayer->m_iSeconds = CHL2RP_Util::EnsureAddition(pPlayer->m_iSeconds, m_iSeconds);
			pPlayer->m_iCrime = CHL2RP_Util::EnsureAddition(pPlayer->m_iCrime, m_iCrime);

			if (accessFlag > pPlayer->ACCESS_FLAG_NONE)
			{
				pPlayer->m_AccessFlag = (CHL2RP_Player::AccessFlag)accessFlag;
			}

			pPlayer->SetHealth(CHL2RP::s_pSQL->FetchInt("health"));
			pPlayer->SetArmorValue(CHL2RP_Util::EnsureAddition(pPlayer->ArmorValue(), CHL2RP::s_pSQL->FetchInt("suit")));
		}
		else
		{
			pPlayer->GiveNamedItem("weapon_citizenpackage");
			pPlayer->GiveAmmo(DEFAULT_RATIONS_AMMO, GetAmmoDef()->Index("citizenpackage"), false);
		}

		// Allow outer upserts only happen after this consistent loaded state, for security
		pPlayer->m_iSyncCapabilities |= pPlayer->SYNC_CAPABILITY_UPSERT_MAIN_DATA;

		// Ensure player is inserted before attempting further syncings.
		// Sync loaded data values that may have been adjusted or merged
		CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPlayerTxn(*pPlayer));

		// Ammo giving does not require to have player spawned
		CHL2RP::s_pSQL->AddAsyncTxn(new CLoadPlayerAmmoTxn(*pPlayer));

		if (pPlayer->IsAlive())
		{
			CHL2RP::s_pSQL->AddAsyncTxn(new CLoadPlayerWeaponsTxn(*pPlayer));
		}
		else
		{
			// Allow loading pending weapons on next spawn
			pPlayer->m_iSyncCapabilities |= pPlayer->SYNC_CAPABILITY_LOAD_WEAPONS;
		}
	}
}

bool CLoadPlayerTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	return IsValidPlayerTxnAndSteamIDEquals(txn.ToTxnClass(this));
}

CUpsertPlayerTxn::CUpsertPlayerTxn(CHL2RP_Player &player) :
m_iPlayerSeconds(player.m_iSeconds), m_iPlayerCrime(player.m_iCrime), m_iPlayerAccessFlag(player.m_AccessFlag),
m_iPlayerHealth((player.GetHealth() > 0) ? player.GetHealth() : 100), m_iPlayerArmor((player.ArmorValue() > 0) ? player.ArmorValue() : 0), CPlayerTxn(player)
{
	Q_strcpy(m_szPlayerName, player.GetPlayerName());
}

void CUpsertPlayerTxn::BuildSQLiteQueries()
{
	AddQuery("UPDATE Player SET name = ?, seconds = ?, crime = ?, accessflag = ?, health = ?, suit = ? WHERE steamid = ?;");
	AddQuery("INSERT OR IGNORE INTO Player (steamid, name, seconds, crime, accessflag, health, suit) VALUES (?, ?, ?, ?, ?, ?, ?);");
}

void CUpsertPlayerTxn::BuildMySQLQueries()
{
	AddQuery("INSERT INTO Player (steamid, name, seconds, crime, accessflag, health, suit) VALUES (?, ?, ?, ?, ?, ?, ?) "
		"ON DUPLICATE KEY UPDATE name = ?, seconds = ?, crime = ?, accessflag = ?, health = ?, suit = ?;");
}

void CUpsertPlayerTxn::BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const
{
	int dataStartParamIndex;

	if (iQueryId)
	{
		dataStartParamIndex = 2;
		CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerSteamID, 1);
	}
	else
	{
		dataStartParamIndex = 1;
		CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerSteamID, 7);
	}

	CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerName, dataStartParamIndex);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerSeconds, ++dataStartParamIndex);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerCrime, ++dataStartParamIndex);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerAccessFlag, ++dataStartParamIndex);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerHealth, ++dataStartParamIndex);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerArmor, ++dataStartParamIndex);
}

void CUpsertPlayerTxn::BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const
{
	CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerSteamID, 1);
	CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerName, 2, 8);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerSeconds, 3, 9);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerCrime, 4, 10);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerAccessFlag, 5, 11);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerHealth, 6, 12);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerArmor, 7, 13);
}

bool CUpsertPlayerTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	return IsValidPlayerTxnAndSteamIDEquals(txn.ToTxnClass(this));
}

CPlayerWeaponTxn::CPlayerWeaponTxn(CBaseCombatWeapon &weapon) : m_sPlayerWeaponClassname(weapon.GetClassname())
{

}

void CPlayerWeaponTxn::BindIdentificators(const void *pStmt, int iStartParamIndex) const
{
	CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerSteamID, iStartParamIndex);
	CHL2RP::s_pSQL->BindString(pStmt, m_sPlayerWeaponClassname, ++iStartParamIndex);
}

void CPlayerWeaponTxn::BindParametersGeneric(int iQueryId, const void *pStmt) const
{
	BindIdentificators(pStmt, 1);
}

// Purpose: Replace derived transactions between themselves. It assumes they are safe candidates!
bool CPlayerWeaponTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	ThisClass *pThisClassOtherTxn = txn.ToTxnClass(this);
	return (IsValidPlayerTxnAndSteamIDEquals(pThisClassOtherTxn) && (pThisClassOtherTxn->m_sPlayerWeaponClassname == m_sPlayerWeaponClassname));
}

CSetUpPlayersWeaponsTxn::CSetUpPlayersWeaponsTxn()
{
	AddFormattedQuery("CREATE TABLE IF NOT EXISTS PlayerWeapon (steamid VARCHAR (%i) NOT NULL, "
		"weapon VARCHAR (%i) NOT NULL, clip1 INTEGER, clip2 INTEGER, PRIMARY KEY (steamid, weapon), "
		"FOREIGN KEY (steamid) REFERENCES Player (steamid) ON UPDATE CASCADE ON DELETE CASCADE, CHECK (clip1 >= 0 AND clip2 >= 0));",
		MAX_NETWORKID_LENGTH, MAX_PLAYER_DATABASE_WEAPON_NAME_LENGTH);
}

bool CSetUpPlayersWeaponsTxn::ShouldUsePreparedStatements() const
{
	return false;
}

CLoadPlayerWeaponsTxn::CLoadPlayerWeaponsTxn(CHL2RP_Player &player) : CPlayerTxn(player)
{
	AddQuery("SELECT * FROM PlayerWeapon WHERE steamid = ?;");
}

void CLoadPlayerWeaponsTxn::HandleQueryResults() const
{
	CHL2RP_Player *pPlayer = CHL2RP_Util::PlayerByNetworkIDString(m_szPlayerSteamID);

	if (pPlayer != NULL)
	{
		if (pPlayer->IsAlive())
		{
			while (CHL2RP::s_pSQL->FetchNextRow())
			{
				char weaponClassname[MAX_PLAYER_DATABASE_WEAPON_NAME_LENGTH];
				CHL2RP::s_pSQL->FetchString("weapon", weaponClassname, sizeof weaponClassname);
				int clip1 = CHL2RP::s_pSQL->FetchInt("clip1"), clip2 = CHL2RP::s_pSQL->FetchInt("clip2");
				CBaseCombatWeapon *weaponEnt = pPlayer->Weapon_OwnsThisType(weaponClassname);

				if (weaponEnt == NULL)
				{
					CBaseEntity *entity = CBaseEntity::Create(weaponClassname, pPlayer->GetLocalOrigin(), pPlayer->GetLocalAngles(), pPlayer);

					if (entity != NULL && entity->IsBaseCombatWeapon()) // Quickly check that it's a weapon
					{
						// Start equipping the weapon avoiding "free" default ammunition (force saved)...
						weaponEnt = static_cast<CBaseCombatWeapon *>(entity);
						int curPrimaryAmmoCount = pPlayer->GetAmmoCount(weaponEnt->GetPrimaryAmmoType()),
							curSecondaryAmmoCount = pPlayer->GetAmmoCount(weaponEnt->GetSecondaryAmmoType());
						pPlayer->Weapon_Equip(weaponEnt);
						weaponEnt->m_iClip1 = clip1;
						weaponEnt->m_iClip2 = clip2;
						pPlayer->SetAmmoCount(curPrimaryAmmoCount, weaponEnt->GetPrimaryAmmoType());
						pPlayer->SetAmmoCount(curSecondaryAmmoCount, weaponEnt->GetSecondaryAmmoType());
					}
				}
				else
				{
					weaponEnt->m_iClip1 = CHL2RP_Util::EnsureAddition(weaponEnt->Clip1(), clip1);
					weaponEnt->m_iClip2 = CHL2RP_Util::EnsureAddition(weaponEnt->Clip2(), clip2);
				}
			}

			pPlayer->m_iSyncCapabilities |= pPlayer->SYNC_CAPABILITY_ALTER_WEAPONS;

			// Start upserting all attached weapons to sync possible missmatches with current state...
			for (int i = 0; i < MAX_WEAPONS; i++)
			{
				if (pPlayer->GetWeapon(i))
				{
					CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPlayerWeaponTxn(*pPlayer, *pPlayer->GetWeapon(i)));
				}
			}

			char weaponsLoadedMsg[MAX_VALVE_USER_MSG_TEXT_DATA];
			CHL2RP_Phrases::LoadForPlayer(pPlayer, weaponsLoadedMsg, sizeof weaponsLoadedMsg, WEAPONS_AND_CLIPS_LOADED_HEADER, STATIC_TALK_COLOR);
			ClientPrint(pPlayer, HUD_PRINTTALK, weaponsLoadedMsg);
		}
		else
		{
			// Player death could have been prevented if he had received the weapons. Re-allow loading them on next spawn
			pPlayer->m_iSyncCapabilities |= pPlayer->SYNC_CAPABILITY_LOAD_WEAPONS;
		}
	}
}

bool CLoadPlayerWeaponsTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	return IsValidPlayerTxnAndSteamIDEquals(txn.ToTxnClass(this));
}

CUpsertPlayerWeaponTxn::CUpsertPlayerWeaponTxn(CHL2RP_Player &player, CBaseCombatWeapon &weapon) :
m_iPlayerWeaponClip1((weapon.Clip1() > 0) ? weapon.Clip1() : 0), m_iPlayerWeaponClip2((weapon.Clip2() > 0) ? weapon.Clip2() : 0),
CPlayerWeaponTxn(weapon)
{
	ConstructPlayerTxn(player);
}

void CUpsertPlayerWeaponTxn::BuildSQLiteQueries()
{
	AddQuery("UPDATE PlayerWeapon SET clip1 = ?, clip2 = ? WHERE steamid = ? AND weapon = ?;");
	AddQuery("INSERT OR IGNORE INTO PlayerWeapon (steamid, weapon, clip1, clip2) VALUES (?, ?, ?, ?);");
}

void CUpsertPlayerWeaponTxn::BuildMySQLQueries()
{
	AddQuery("INSERT INTO PlayerWeapon (steamid, weapon, clip1, clip2) VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE clip1 = ?, clip2 = ?;");
}

void CUpsertPlayerWeaponTxn::BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const
{
	int dataStartParamIndex;

	if (iQueryId)
	{
		BindIdentificators(pStmt, 1);
		dataStartParamIndex = 3;
	}
	else
	{
		dataStartParamIndex = 1;
		BindIdentificators(pStmt, 3);
	}

	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerWeaponClip1, dataStartParamIndex);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerWeaponClip2, ++dataStartParamIndex);
}

void CUpsertPlayerWeaponTxn::BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const
{
	BindIdentificators(pStmt, 1);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerWeaponClip1, 3, 5);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerWeaponClip2, 4, 6);
}

CDeletePlayerWeaponTxn::CDeletePlayerWeaponTxn(CHL2RP_Player &player, CBaseCombatWeapon &weapon) : CPlayerWeaponTxn(weapon)
{
	AddQuery("DELETE FROM PlayerWeapon WHERE steamid = ? AND weapon = ?;");
	ConstructPlayerTxn(player);
}

CPlayerAmmoTxn::CPlayerAmmoTxn(int iPlayerAmmoIndex) : m_iPlayerAmmoIndex(iPlayerAmmoIndex)
{

}

void CPlayerAmmoTxn::BindIdentificators(const void *pStmt, int iStartParamIndex) const
{
	CHL2RP::s_pSQL->BindString(pStmt, m_szPlayerSteamID, iStartParamIndex);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerAmmoIndex, ++iStartParamIndex);
}

void CPlayerAmmoTxn::BindParametersGeneric(int iQueryId, const void *pStmt) const
{
	BindIdentificators(pStmt, 1);
}

// Purpose: Replace derived transactions between themselves. It assumes they are safe candidates!
bool CPlayerAmmoTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	ThisClass *pThisClassOtherTxn = txn.ToTxnClass(this);
	return (IsValidPlayerTxnAndSteamIDEquals(pThisClassOtherTxn) && (pThisClassOtherTxn->m_iPlayerAmmoIndex == m_iPlayerAmmoIndex));
}

CSetUpPlayersAmmoTxn::CSetUpPlayersAmmoTxn() :
CAsyncSQLTxn("CREATE TABLE IF NOT EXISTS PlayerAmmo (steamid VARCHAR (" V_STRINGIFY(MAX_NETWORKID_LENGTH) ") NOT NULL, "
"`index` INTEGER NOT NULL, count INTEGER NOT NULL, PRIMARY KEY (steamid, `index`), "
"FOREIGN KEY (steamid) REFERENCES Player (steamid) ON UPDATE CASCADE ON DELETE CASCADE, CHECK (`index` > 0 AND count >= 0));")
{

}

bool CSetUpPlayersAmmoTxn::ShouldUsePreparedStatements() const
{
	return false;
}

CLoadPlayerAmmoTxn::CLoadPlayerAmmoTxn(CHL2RP_Player &player) : CPlayerTxn(player)
{
	AddQuery("SELECT * FROM PlayerAmmo WHERE steamid = ?;");
}

void CLoadPlayerAmmoTxn::HandleQueryResults() const
{
	CHL2RP_Player *pPlayer = CHL2RP_Util::PlayerByNetworkIDString(m_szPlayerSteamID);

	if (pPlayer != NULL)
	{
		while (CHL2RP::s_pSQL->FetchNextRow())
		{
			// Regarding ammo giving, it should work being alive or not:
			int index = CHL2RP::s_pSQL->FetchInt("index"), count = CHL2RP::s_pSQL->FetchInt("count");

			if (index > 0)
			{
				// Bypass max ammo check, allow any count:
				pPlayer->SetAmmoCount(CHL2RP_Util::EnsureAddition(count, pPlayer->GetAmmoCount(index)), index);
			}
		}

		pPlayer->m_iSyncCapabilities |= pPlayer->SYNC_CAPABILITY_ALTER_AMMO;

		// Start upserting all positive ammunition to sync possible inconsistent records with current state...
		if (!pPlayer->IsDisconnecting())
		{
			for (int i = 0; i < MAX_AMMO_SLOTS; i++)
			{
				if (pPlayer->GetAmmoCount(i) > 0)
				{
					CHL2RP::s_pSQL->AddAsyncTxn(new CUpsertPlayerAmmoTxn(*pPlayer, i));
				}
			}
		}

		char ammoLoadedMsg[MAX_VALVE_USER_MSG_TEXT_DATA];
		CHL2RP_Phrases::LoadForPlayer(pPlayer, ammoLoadedMsg, sizeof ammoLoadedMsg, RESERVE_AMMO_LOADED_HEADER, STATIC_TALK_COLOR);
		ClientPrint(pPlayer, HUD_PRINTTALK, ammoLoadedMsg);
	}
}

bool CLoadPlayerAmmoTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	return IsValidPlayerTxnAndSteamIDEquals(txn.ToTxnClass(this));
}

CUpsertPlayerAmmoTxn::CUpsertPlayerAmmoTxn(CHL2RP_Player &player, int iPlayerAmmoIndex) :
m_iPlayerAmmoCount((player.GetAmmoCount(iPlayerAmmoIndex) > 0) ? player.GetAmmoCount(iPlayerAmmoIndex) : 0),
CPlayerAmmoTxn(iPlayerAmmoIndex)
{
	ConstructPlayerTxn(player);
}

void CUpsertPlayerAmmoTxn::BuildSQLiteQueries()
{
	AddQuery("UPDATE PlayerAmmo SET count = ? WHERE steamid = ? AND `index` = ?;");
	AddQuery("INSERT OR IGNORE INTO PlayerAmmo (steamid, `index`, count) VALUES (?, ?, ?);");
}

void CUpsertPlayerAmmoTxn::BuildMySQLQueries()
{
	AddQuery("INSERT INTO PlayerAmmo (steamid, `index`, count) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE count = ?;");
}

void CUpsertPlayerAmmoTxn::BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const
{
	if (iQueryId)
	{
		BindIdentificators(pStmt, 1);
		CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerAmmoCount, 3);
	}
	else
	{
		CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerAmmoCount, 1);
		BindIdentificators(pStmt, 2);
	}
}

void CUpsertPlayerAmmoTxn::BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const
{
	BindIdentificators(pStmt, 1);
	CHL2RP::s_pSQL->BindInt(pStmt, m_iPlayerAmmoCount, 3, 4);
}

CDeletePlayerAmmoTxn::CDeletePlayerAmmoTxn(CHL2RP_Player &player, int iPlayerAmmoIndex) : CPlayerAmmoTxn(iPlayerAmmoIndex)
{
	AddQuery("DELETE FROM PlayerAmmo WHERE steamid = ? AND `index` = ?;");
	ConstructPlayerTxn(player);
}
