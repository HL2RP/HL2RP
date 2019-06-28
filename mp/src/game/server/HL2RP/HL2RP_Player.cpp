#include <cbase.h>
#include "HL2RP_Player.h"
#include <ammodef.h>
#include <beam_flags.h>
#include "HL2RPProperty.h"
#include "HL2RPRules.h"
#include "HL2RP_Defs.h"
#include "HL2RP_Util.h"
#include <in_buttons.h>
#include "DAL/PlayerDAO.h"
#include "PropRationDispenser.h"
#include <Dialogs/PublicDialogs.h>
#include "Weapons/Ration.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define HL2RP_PLAYER_HUD_INTERVAL	1.0f
#define HL2RP_PLAYER_PLAYTIME_THINKCONTEXT_STRING	"PlayTimeContext"
#define HL2RP_PLAYER_HUD_ALERTS_CLEAR_CONTEXT	"CenterHUDLinesClearContext"
#define HL2RP_PLAYER_LOWER_WEAPON_WAIT_SECONDS	10.0f
#define HL2RP_PLAYER_HUD_WELCOME_DURATION	10.0f
#define HL2RP_PLAYER_MAX_STICKY_WALK_HINTS	5
#define HL2RP_PLAYER_STICKY_WALK_HINT_TIME	5.0f
#define HL2RP_PLAYER_INVENTORY_OPEN_USE_KEY_SECONDS	1
#define HL2RP_PLAYER_INVENTORY_OPENING_SOUND	"buttons/button1.wav"
#define HL2RP_PLAYER_HUD_MAIN_X_POS	0.007f
#define HL2RP_PLAYER_HUD_MAIN_Y_POS	0.03f
#define HL2RP_PLAYER_HUD_XHAIR_ENTITY_DISTANCE	150.0f
#define HL2RP_PLAYER_HUD_XHAIR_Y_POS	0.75f
#define HL2RP_PLAYER_HUD_REGION_X_POS	1.0f
#define HL2RP_PLAYER_HUD_REGION_Y_POS	0.65f
#define HL2RP_PLAYER_HUD_ALERTS_Y_POS	0.03f
#define HL2RP_PLAYER_HUD_REGION_MAX_PLAYERS	5
#define HL2RP_PLAYER_HUD_ALERTS_MAX_LINES	5

// Beam ring effect to play when region talk conditions pass
#define HL2RP_PLAYER_REGION_TALK_BEAMRING_END_RADIUS	50.0f
#define HL2RP_PLAYER_REGION_TALK_BEAMRING_END_DURATION	0.25f
#define HL2RP_PLAYER_REGION_TALK_BEAMRING_WIDTH 3.0f

// Apply an heuristic networking delay, or the HUD may get skipped on client otherwise
#define HL2RP_PLAYER_WELCOME_MESSAGES_DISPLAY_DELAY	2.0f

#define HL2RP_LOC_TOKEN_HUD_HINT_STICKY_WALK	"#HL2RP_HUD_Hint_Sticky_Walk"
#define HL2RP_LOC_TOKEN_HUD_REGION	"#HL2RP_HUD_Region"

// Make each channel have a specific purpose for consistency and helping against overlapping:
// Reminder: #define MAX_NETMESSAGE	6, sadly this is the HUD channel limit client/engine sided (until date)
enum EHUDChannelIndex
{
	Main,
	Crime,
	Region,
	Phone,
	Alerts, // For PVP, welcome HUD, etc
	CrosshairInfo, // For doors, faced player info, etc
};

// This is part of the same idea than with HUD channels: allow displaying ASAP (using a time variable),
// away from depending on any periodic function
enum ETempEntityIndex
{
	// TODO: Add required indexes for designated temp entity types
};

extern CBaseEntity* g_pLastSpawn;
extern CBaseEntity* g_pLastCombineSpawn;
extern CBaseEntity* g_pLastRebelSpawn;

#ifndef HL2DM_RP
IMPLEMENT_SERVERCLASS_ST(CHL2RP_Player, DT_HL2RP_Player)
END_SEND_TABLE();
#endif // !HL2DM_RP

CHUDChannelLine::CHUDChannelLine(EType type, const char* pText) : m_Type(type)
{
	Assert(pText != NULL);
	Q_strncpy(m_Text, pText, sizeof(m_Text));
}

bool CHUDChannelLine::operator==(const CHUDChannelLine& lineInfo) const
{
	return (lineInfo.m_Type != None && lineInfo.m_Type == m_Type);
}

bool CHUDChannelLineLess::Less(CHUDChannelLine const& lineInfo1, CHUDChannelLine const& lineInfo2, void* pCtx)
{
	return (lineInfo1.m_Type > lineInfo2.m_Type);
}

// Checks if the first spawnpoint parameter is nearer to the player than second. To be used during respawn.
class CSpawnPointLess
{
public:
	// Input: pCtx - Generic pointer to the player to measure the distances against
	bool Less(CBaseEntity* const& pSpot1, CBaseEntity* const& pSpot2, void* pCtx)
	{
		Assert(pCtx != NULL);
		CBasePlayer* pInSpawnSelectPlayer = static_cast<CBasePlayer*>(pCtx);

		// Use Manhattan distances to simplify comparison, which have enough accuracy for the purpose
		float dist1 = UTIL_DistApprox(pSpot1->GetAbsOrigin(), pInSpawnSelectPlayer->GetAbsOrigin());
		float dist2 = UTIL_DistApprox(pSpot2->GetAbsOrigin(), pInSpawnSelectPlayer->GetAbsOrigin());

		return (dist1 < dist2);
	}
};

CHL2RP_Player* ToHL2RP_Player(CBaseEntity* pEntity)
{
	if (pEntity != NULL && pEntity->IsPlayer())
	{
		return ToHL2RPPlayer_Fast(pEntity);
	}

	return NULL;
}

CHL2RP_Player::CHL2RP_Player() : m_JobTeamIndex(EJobTeamIndex::Citizen)
{
	m_iLastDialogLevel = INT_MAX; // INT_MAX level itself blocks the dialog from displaying
	m_flNextOpenInventoryTime = FLT_MAX;
	m_HiddenGesture.m_Activity = ACT_INVALID;
	m_iRemainingStickyWalkHints = HL2RP_PLAYER_MAX_STICKY_WALK_HINTS;
	m_DALSyncCaps.SetFlag(EDALSyncCap::LoadData);
}

CHL2RP_Player::~CHL2RP_Player()
{
	delete m_pCurrentDialog;

	if (m_pDeadLoadedEquipmentInfo != NULL)
	{
		m_pDeadLoadedEquipmentInfo->deleteThis();
	}
}

void CHL2RP_Player::Precache()
{
	BaseClass::Precache();
	enginesound->PrecacheSound(HL2RP_PLAYER_INVENTORY_OPENING_SOUND);
	ActivityList_RegisterPrivateActivity("ACT_CROUCH_PANICKED");
}

void CHL2RP_Player::InitialSpawn()
{
	BaseClass::InitialSpawn();
	SetModel(HL2RPRules()->m_JobTeamsModels[m_JobTeamIndex][0].m_ModelPath);

	// Early network ID string load case?
	if (!IsFakeClient() && Q_strcmp(GetNetworkIDString(), "") != 0
		&& m_DALSyncCaps.IsFlagSet(EDALSyncCap::LoadData))
	{
		TryCreateAsyncDAO<CPlayerLoadDAO>(this);
		m_DALSyncCaps.ClearFlag(EDALSyncCap::LoadData);
	}

	SetContextThink(&CHL2RP_Player::PlayTimeContextThink, gpGlobals->curtime + HL2RP_PLAYER_HUD_INTERVAL,
		HL2RP_PLAYER_PLAYTIME_THINKCONTEXT_STRING);
	SetContextThink(&CHL2RP_Player::DisplayWelcomeHUD, gpGlobals->curtime
		+ HL2RP_PLAYER_WELCOME_MESSAGES_DISPLAY_DELAY, "DisplayWelcomeHUDContext");
}

void CHL2RP_Player::Spawn()
{
	BaseClass::Spawn();

	// Remember death rations to restore after removing all items
	int rationIndex = GetAmmoDef()->Index("ration"),
		rationsCount = GetAmmoCount(rationIndex);

	RemoveAllItems(false); // Remove default spawn items from BaseClass

	if (m_pDeadLoadedEquipmentInfo != NULL)
	{
		TryGiveLoadedHpAndArmor(m_pDeadLoadedEquipmentInfo->GetInt("health"),
			m_pDeadLoadedEquipmentInfo->GetInt("suit"));

		FOR_EACH_SUBKEY(m_pDeadLoadedEquipmentInfo->FindKey(HL2RP_PLAYER_DEADLOAD_AMMODATA_KEYNAME), pRow)
		{
			SetAmmoCount(pRow->GetInt(), Q_atoi(pRow->GetName()));
		}

		FOR_EACH_SUBKEY(m_pDeadLoadedEquipmentInfo->FindKey(HL2RP_PLAYER_DEADLOAD_WEAPONDATA_KEYNAME), pRow)
		{
			TryGiveLoadedWeapon(pRow->GetName(), pRow->GetInt("clip1"), pRow->GetInt("clip2"));
		}

		m_pDeadLoadedEquipmentInfo->deleteThis();
		m_pDeadLoadedEquipmentInfo = NULL;
	}

	GiveNamedItem("suitcase");

	if (rationsCount > 0)
	{
		// Restore rations from death
		GiveNamedItem("ration");
		SetAmmoCount(0, rationIndex); // Substract default ammo
		GiveAmmo(rationsCount, rationIndex, false);
	}
}

void CHL2RP_Player::ChangeTeam(int team)
{
	// Check this to prevent BaseClass from causing an infinite cl_playermodel loop
	if (team != GetTeamNumber())
	{
		// For now, force the team that is related to my current job 
		switch (m_JobTeamIndex)
		{
		case EJobTeamIndex::Citizen:
		{
			team = TEAM_REBELS;
			break;
		}
		case EJobTeamIndex::Police:
		{
			team = TEAM_COMBINE;
			break;
		}
		}

		BaseClass::ChangeTeam(team);
	}
}

CBaseEntity* CHL2RP_Player::FindRandomNearbySpawnPoint(const char* pSpawnPointClassName)
{
	CUtlSortVector<CBaseEntity*, CSpawnPointLess> spawnPointSortedList;
	spawnPointSortedList.SetLessContext(this);

	// Begin: Gather all spawnpoints of parameter classname, sorting them ASC by distance to player
	for (CBaseEntity* pAuxSpawn = NULL;;)
	{
		pAuxSpawn = gEntList.FindEntityByClassname(pAuxSpawn, pSpawnPointClassName);

		if (pAuxSpawn == NULL)
		{
			// This means all entities were looked already, without success
			break;
		}
		else if (pAuxSpawn->edict() != NULL && GameRules()->IsSpawnPointValid(pAuxSpawn, this))
		{
			spawnPointSortedList.Insert(pAuxSpawn);
		}
	}

	if (spawnPointSortedList.Count() > 0)
	{
		// Now, randomize the final spawnpoint among few of the nearest ones obtained
		int index = RandomInt(0, spawnPointSortedList.Count() - 1);
		return spawnPointSortedList.Element(index);
	}

	return NULL;
}

void CHL2RP_Player::CenterHUDLinesClearContextThink()
{
	if (gpGlobals->curtime > m_flNextHUDChannelTime[EHUDChannelIndex::Alerts])
	{
		m_SharedHUDAlertList.RemoveAll();
	}
	else
	{
		SetContextThink(&CHL2RP_Player::CenterHUDLinesClearContextThink,
			m_flNextHUDChannelTime[EHUDChannelIndex::Alerts], HL2RP_PLAYER_HUD_ALERTS_CLEAR_CONTEXT);
	}
}

CBaseEntity* CHL2RP_Player::EntSelectSpawnPoint()
{
	CUtlFixedLinkedList<const char*> spawnClassNamesList(0, 3);

	switch (GetTeamNumber())
	{
	case TEAM_REBELS:
	{
		spawnClassNamesList.AddToHead("info_player_rebel");
		break;
	}
	case TEAM_COMBINE:
	{
		spawnClassNamesList.AddToHead("info_player_combine");
		break;
	}
	}

	spawnClassNamesList.AddToTail("info_player_deathmatch");
	spawnClassNamesList.AddToTail("info_player_start");

	FOR_EACH_LL(spawnClassNamesList, i)
	{
		// Randomize spawn point, among nearby ones
		CBaseEntity* pAuxSpawn = FindRandomNearbySpawnPoint(spawnClassNamesList[i]);

		if (pAuxSpawn != NULL)
		{
			m_flSlamProtectTime = gpGlobals->curtime + 0.5f;
			g_pLastSpawn = pAuxSpawn;
			return pAuxSpawn;
		}
	}

	return g_pLastSpawn;
}

void CHL2RP_Player::PlayerRunCommand(CUserCmd* pUcmd, IMoveHelper* pMoveHelper)
{
	BaseClass::PlayerRunCommand(pUcmd, pMoveHelper);

	if (pUcmd->buttons & IN_USE)
	{
		if (m_afButtonPressed & IN_USE)
		{
			// Start opening inventory...
			m_flNextOpenInventoryTime = gpGlobals->curtime + HL2RP_PLAYER_INVENTORY_OPEN_USE_KEY_SECONDS;
			CPASAttenuationFilter filter;
			filter.AddAllPlayers();
			enginesound->EmitSound(filter, entindex(), CHAN_AUTO, HL2RP_PLAYER_INVENTORY_OPENING_SOUND,
				VOL_NORM, SNDLVL_35dB);

			// Begin attempting to use a facing dispenser...
			trace_t tr;
			const Vector& origin = EyePosition();
			const Vector& end = origin + EyeDirection3D() * PLAYER_USE_RADIUS;
			UTIL_TraceLine(origin, end, MASK_ALL, this, COLLISION_GROUP_NONE, &tr);
			CPropRationDispenser* pDispenser = dynamic_cast<CPropRationDispenser*>(tr.m_pEnt);

			if (pDispenser != NULL)
			{
				pDispenser->CHL2RP_PlayerUse(*this);
			}
		}
		else if (m_flNextOpenInventoryTime <= gpGlobals->curtime)
		{
			enginesound->StopSound(entindex(), CHAN_AUTO, HL2RP_PLAYER_INVENTORY_OPENING_SOUND);
			DisplayDialog<CMainMenu>();
			m_flNextOpenInventoryTime = FLT_MAX;
		}
	}
	else if (m_flNextOpenInventoryTime != FLT_MAX)
	{
		enginesound->StopSound(entindex(), CHAN_AUTO, HL2RP_PLAYER_INVENTORY_OPENING_SOUND);
		m_flNextOpenInventoryTime = FLT_MAX;
	}
}

void CHL2RP_Player::PostThink()
{
	BaseClass::PostThink();

	if (m_flLockedPlaybackRate > 0.0f)
	{
		SetPlaybackRate(m_flLockedPlaybackRate);
	}

	if (m_flNextHUDChannelTime[EHUDChannelIndex::Main] <= gpGlobals->curtime)
	{
		const char* pMainHUD;
		int hours = m_iSeconds / 3600, minutes = (m_iSeconds / 60) % 60, remainingSeconds = m_iSeconds % 60;
		pMainHUD = GetHL2RPAutoLocalizer().Localize(this, "#HL2RP_HUD_Main", hours, minutes, remainingSeconds,
			GetAmmoCount("ration"), m_iCrime);

		if (m_iCrime > 0)
		{
			UTIL_HUDMessage(this, EHUDChannelIndex::Main, HL2RP_PLAYER_HUD_MAIN_X_POS,
				HL2RP_PLAYER_HUD_MAIN_Y_POS, HL2RP_PLAYER_HUD_INTERVAL, HL2RP_COLOR_HUD_CRIMINAL, pMainHUD);
		}
		else
		{
			UTIL_HUDMessage(this, EHUDChannelIndex::Main, HL2RP_PLAYER_HUD_MAIN_X_POS,
				HL2RP_PLAYER_HUD_MAIN_Y_POS, HL2RP_PLAYER_HUD_INTERVAL, HL2RP_COLOR_HUD_PEACEFUL, pMainHUD);
		}
	}

	if (m_flNextHUDChannelTime[EHUDChannelIndex::Region] <= gpGlobals->curtime)
	{
		char regionPlayersHUD[MAX_DISPLAYABLE_USER_MSG_DATA];
		int numRegionPlayers = 0;

		for (int i = 1, len = 0; i <= gpGlobals->maxClients; i++)
		{
			CHL2RP_Player* pNearPlayer = ToHL2RPPlayer_Fast(CBaseEntity::Instance(i));

			if (pNearPlayer != NULL && pNearPlayer != this && pNearPlayer->IsAlive()
				&& GetAbsOrigin().DistToSqr(pNearPlayer->GetAbsOrigin())
				<= HL2RP_RULES_REGION_PLAYER_INTER_RADIUS_SQR
				&& ++numRegionPlayers <= HL2RP_PLAYER_HUD_REGION_MAX_PLAYERS)
			{
				len += Q_snprintf(regionPlayersHUD + len, sizeof(regionPlayersHUD) - len,
					"\n%s ", pNearPlayer->GetPlayerName());
			}
		}

		if (numRegionPlayers > 0)
		{
			autoloc_buf_ref regionHUD = GetHL2RPAutoLocalizer().m_Localized;
			int regionHUDLen;

			if (numRegionPlayers <= HL2RP_PLAYER_HUD_REGION_MAX_PLAYERS)
			{
				int numPreLineFeeds = HL2RP_PLAYER_HUD_REGION_MAX_PLAYERS - numRegionPlayers;

				for (int i = 0; i < numPreLineFeeds; i++)
				{
					Q_snprintf(regionHUD + i, sizeof(regionHUD) - i, "\n");
				}

				GetHL2RPAutoLocalizer().LocalizeAt(this, regionHUD + numPreLineFeeds,
					sizeof(regionHUD) - numPreLineFeeds, HL2RP_LOC_TOKEN_HUD_REGION);
				regionHUDLen = Q_strlen(regionHUD);
			}
			else
			{
				GetHL2RPAutoLocalizer().Localize(this, HL2RP_LOC_TOKEN_HUD_REGION);
				regionHUDLen = Q_strlen(regionHUD);
				regionHUDLen += Q_snprintf(regionHUD + regionHUDLen, sizeof(regionHUD) - regionHUDLen,
					" (%i)", numRegionPlayers);
			}

			Q_snprintf(regionHUD + regionHUDLen, sizeof(regionHUD) - regionHUDLen, ": %s", regionPlayersHUD);
			UTIL_HUDMessage(this, EHUDChannelIndex::Region, HL2RP_PLAYER_HUD_REGION_X_POS,
				HL2RP_PLAYER_HUD_REGION_Y_POS, HL2RP_PLAYER_HUD_INTERVAL, HL2RP_COLOR_HUD_REGION, regionHUD);
		}
	}

	Vector dest;
	VectorMA(EyePosition(), HL2RP_PLAYER_HUD_XHAIR_ENTITY_DISTANCE, EyeDirection3D(), dest);
	trace_t trace;
	UTIL_TraceLine(EyePosition(), dest, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace);

	if (trace.DidHit() && m_flNextHUDChannelTime[EHUDChannelIndex::CrosshairInfo] <= gpGlobals->curtime)
	{
		CHL2RPPropertyDoor* pPropertyDoor = GetHL2RPPropertyDoor(trace.m_pEnt);

		if (pPropertyDoor != NULL && pPropertyDoor->m_hProperty != NULL)
		{
			const char* pDoorHUD = GetHL2RPAutoLocalizer().Localize(this, "HL2RP_HUD_Door",
				pPropertyDoor->m_hProperty->m_sDisplayName, pPropertyDoor->m_sDisplayName,
				pPropertyDoor->m_hProperty->m_iPrice);
			UTIL_HUDMessage(this, EHUDChannelIndex::CrosshairInfo, -1, HL2RP_PLAYER_HUD_XHAIR_Y_POS,
				HL2RP_PLAYER_HUD_INTERVAL, HL2RP_COLOR_HUD_XHAIR_ENTS, pDoorHUD);
		}
	}
}

// Set the activity based on an event or current state
void CHL2RP_Player::SetAnimation(PLAYER_ANIM playerAnim)
{
	if (GetFlags() & (FL_FROZEN | FL_ATCONTROLS))
	{
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_RUN;
	bool isJumping = (!(GetFlags() & FL_ONGROUND) && GetActivity() == ACT_JUMP);

	switch (playerAnim)
	{
	case PLAYER_JUMP:
	{
		idealActivity = ACT_JUMP;
		break;
	}
	case PLAYER_DIE:
	{
		if (m_lifeState == LIFE_ALIVE)
		{
			return;
		}

		break;
	}
	case PLAYER_ATTACK1:
	{
		Assert(m_hActiveWeapon != NULL);

		// Check if weapon is melee, since it uses a different base activity name
		if (m_hActiveWeapon->IsMeleeWeapon())
		{
			return SetAttack1Animation(ACT_MELEE_ATTACK1);
		}

		return SetAttack1Animation();
	}
	case PLAYER_RELOAD:
	{
		return TrySetIdealActivityOrGesture(ACT_RELOAD, ACT_RELOAD_LOW, ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD);
	}
	case PLAYER_IDLE:
	{
		// Still jumping, or attacking/reloading while having IDLE animation already
		if (isJumping || !IsActivityFinished() && (GetActivity() == ACT_RANGE_ATTACK1
			|| GetActivity() == ACT_RELOAD || GetActivity() == ACT_MELEE_ATTACK1))
		{
			return;
		}
		else if (GetFlags() & FL_ANIMDUCKING) // Crouching state
		{
			if (!IsActivityFinished() && (GetActivity() == ACT_RANGE_ATTACK1_LOW || GetActivity() == ACT_RELOAD_LOW))
			{
				return;
			}

			idealActivity = ACT_CROUCH;
		}
		else // Stand still state
		{
			idealActivity = ACT_IDLE;
		}

		break;
	}
	case PLAYER_WALK:
	{
		if (isJumping) // Still jumping
		{
			return;
		}
		else if (GetFlags() & FL_ANIMDUCKING) // Crouching state
		{
			idealActivity = ACT_WALK_CROUCH;
		}
		else if (IsWalking()) // Moving state
		{
			idealActivity = ACT_WALK;
		}
		else
		{
			idealActivity = ACT_RUN;
		}

		break;
	}
	}

	// Has the desired activity changed? CHANGE: Got rid of comparing current and desired sequence,
	// because player might be running a chain of sequences belonging to the current activity!
	int sequence = HL2RPRules()->GetPlayerActRemapSequence(this, idealActivity);

	if (GetSequenceActivity(GetSequence()) != GetSequenceActivity(sequence))
	{
		if (sequence == ACTIVITY_NOT_AVAILABLE)
		{
			sequence = 0; // Simply imitating default CHL2MP_Player code
		}

		SetIdealActivity(idealActivity, sequence);
		m_flLockedPlaybackRate = 0.0f;

		// Make hidden gesture visible
		if (m_HiddenGesture.m_Activity != ACT_INVALID)
		{
			int sequence = HL2RPRules()->GetPlayerActRemapSequence(this, m_HiddenGesture.m_Activity);
			int layer = AddGestureSequence(sequence);
			SetLayerCycle(layer, gpGlobals->curtime - m_HiddenGesture.m_flStartTime);
			m_HiddenGesture.m_Activity = ACT_INVALID;
		}
	}
}

void CHL2RP_Player::PackDeadPlayerItems()
{
	// Recover the rations that default code would remove on death
	int index = GetAmmoDef()->Index("ration"), rations = GetAmmoCount(index);
	BaseClass::PackDeadPlayerItems();
	SetAmmoCount(rations, index);
}

void CHL2RP_Player::Event_Killed(const CTakeDamageInfo& info)
{
	// Check if I'm connected before dropping rations
	if (!IsDisconnecting())
	{
		int rationIndex = GetAmmoDef()->Index("ration"), lostRations;
		CHL2RP_Player* pAttacker = ToHL2RP_Player(info.GetAttacker());
		autoloc_buf_ref msg = GetHL2RPAutoLocalizer().m_Localized;

		// Has another player with less rations killed me?
		if (pAttacker != NULL && pAttacker->GetAmmoCount(rationIndex) < GetAmmoCount(rationIndex))
		{
			// Yes. Set to drop his amount.
			lostRations = pAttacker->GetAmmoCount(rationIndex); // Aux

			GetHL2RPAutoLocalizer().Localize(this, "HL2RP_Chat_Killed_By_Player", pAttacker->GetPlayerName());
		}
		else
		{
			// No. Set to drop my amount.
			lostRations = GetAmmoCount(rationIndex); // Aux

			GetHL2RPAutoLocalizer().Localize(this, "HL2RP_Chat_Self_Killed");
		}

		lostRations -= lostRations / 2; // The placebo
		SubstractAmmoCount(lostRations, rationIndex);
		CRation* pRation = static_cast<CRation*>(Weapon_OwnsThisType("ration"));

		if (pRation != NULL)
		{
			Weapon_Drop(pRation); // Ensure it's dropped even when it isn't the active weapon
			pRation->SetSecondaryAmmoCount(lostRations);
		}

		int msgLen = Q_strlen(msg);
		msgLen += Q_snprintf(msg + msgLen, sizeof(msg) - msgLen, " ");
		GetHL2RPAutoLocalizer().LocalizeAt(this, msg + msgLen, sizeof(msg) - msgLen,
			"#HL2RP_Chat_Rations_Lost", lostRations);
		ClientPrint(this, HUD_PRINTTALK, msg);
	}

	BaseClass::Event_Killed(info); // Must call this only after completing the rations adjustments
}

void CHL2RP_Player::SetWeapon(int i, CBaseCombatWeapon* pWeapon)
{
	// HACK: On HL2DM, the special weapons can't be assigned a proper slot. Check if it's one of them.
	if (pWeapon != NULL && !pWeapon->CanBeSelected())
	{
		// Re-point the index to the highest as possible, so that client can select original weapons
		for (i = m_hMyWeapons.Count() - 1; i >= 0 && GetWeapon(i) != NULL; i--);
	}

	CBaseCombatWeapon* pOldWeapon = GetWeapon(i);
	BaseClass::SetWeapon(i, pWeapon);

	if (m_DALSyncCaps.IsFlagSet(EDALSyncCap::SaveData))
	{
		if (pWeapon != NULL)
		{
			if (pOldWeapon == NULL)
			{
				return TryCreateAsyncDAO<CPlayerWeaponInsertDAO>(this, pWeapon);
			}

			TryCreateAsyncDAO<CPlayerWeaponUpdateDAO>(this, pWeapon);
		}
		else if (pOldWeapon != NULL)
		{
			TryCreateAsyncDAO<CPlayerWeaponDeleteDAO>(this, pOldWeapon);
		}
	}

}

void CHL2RP_Player::OnSetWeaponClip1(CBaseCombatWeapon* pWeapon)
{
	TrySyncWeapon(pWeapon);
}

void CHL2RP_Player::OnSetWeaponClip2(CBaseCombatWeapon* pWeapon)
{
	TrySyncWeapon(pWeapon);
}

void CHL2RP_Player::SetHealth(int health)
{
	BaseClass::SetHealth(health);
	TrySyncMainData(EDALMainPropSelection::Health);
}

void CHL2RP_Player::SetAmmoCount(int count, int ammoIndex)
{
	int prevCount = m_iAmmo[ammoIndex];
	BaseClass::SetAmmoCount(count, ammoIndex);

	if (m_DALSyncCaps.IsFlagSet(EDALSyncCap::SaveData))
	{
		if (count > 0)
		{
			if (prevCount < 1)
			{
				return TryCreateAsyncDAO<CPlayerAmmoInsertDAO>(this, ammoIndex);
			}

			return TryCreateAsyncDAO<CPlayerAmmoUpdateDAO>(this, ammoIndex);
		}

		return TryCreateAsyncDAO<CPlayerAmmoDeleteDAO>(this, ammoIndex);
	}
}

void CHL2RP_Player::UpdateWeaponPosture()
{
	// Am I active?
	if (GetMoveTime() > 0.0f || m_nButtons != 0 || GetCurrentCommand()->mousedx != 0
		|| GetCurrentCommand()->mousedy != 0 || GetCurrentCommand()->weaponselect != 0)
	{
		// Yes, raise the weapon and update the time I should lower weapon as long as I keep inactive from now
		if (Weapon_Ready())
		{
			m_LowerWeaponTimer.Set(HL2RP_PLAYER_LOWER_WEAPON_WAIT_SECONDS);
		}
	}
	else if (m_LowerWeaponTimer.Expired())
	{
		Weapon_Lower();
	}
}

void CHL2RP_Player::OnTalkConditionsPassed(CBasePlayer* pListener, bool isTeamOnly)
{
	if (isTeamOnly)
	{
		// Regional communication. Feedback speaker with an effect on listener.
		CSingleUserRecipientFilter filter(this);
		Color color = HL2RP_COLOR_HUD_REGION;
		te->BeamRingPoint(filter, 0.0f, pListener->GetAbsOrigin(), 0.0f,
			HL2RP_PLAYER_REGION_TALK_BEAMRING_END_RADIUS, HL2RPRules()->m_iBeamModelIndex,
			HL2RPRules()->m_iBeamModelIndex, 0, 1, HL2RP_PLAYER_REGION_TALK_BEAMRING_END_DURATION,
			HL2RP_PLAYER_REGION_TALK_BEAMRING_WIDTH, 0, 1.0f, color.r(), color.g(), color.b(),
			color.a(), 0);
	}
}

void CHL2RP_Player::SetModel(const char* pModelPath)
{
	if (HL2RPRules()->GetJobTeamIndexForModel(pModelPath) == m_JobTeamIndex)
	{
		BaseClass::SetModel(pModelPath);
	}
}

void CHL2RP_Player::SetArmorValue(int armorValue)
{
	BaseClass::SetArmorValue(armorValue);
	TrySyncMainData(EDALMainPropSelection::Armor);
}

void CHL2RP_Player::DisplayDialog(CPlayerDialog* pDialog)
{
	delete m_pCurrentDialog;
	m_pCurrentDialog = pDialog;
	pDialog->Display(this);
}

void CHL2RP_Player::DisplayWelcomeHUD()
{
	const char* pHUDText = GetHL2RPAutoLocalizer().Localize(this, "#HL2RP_HUD_Welcome",
		HL2RP_PLAYER_INVENTORY_OPEN_USE_KEY_SECONDS);

	// This message should be quite long, so use a HUD channel-less central message instead of invading the alerts HUD
	ClientPrint(this, HUD_PRINTCENTER, pHUDText);
}

void CHL2RP_Player::DisplayStickyWalkingHint()
{
	if (m_iRemainingStickyWalkHints > 0)
	{
		m_iRemainingStickyWalkHints--;

#ifdef HL2DM_RP
		const char* pHUDText = GetHL2RPAutoLocalizer().
			Localize(this, HL2RP_LOC_TOKEN_HUD_HINT_STICKY_WALK);
		AddAlertHUDLine(pHUDText, HL2RP_PLAYER_STICKY_WALK_HINT_TIME, CHUDChannelLine::StickyWalkHint);
#else
		UTIL_HudHintText(this, HL2RP_LOC_TOKEN_HUD_HINT_STICKY_WALK);
#endif // HL2DM_RP
	}
}

void CHL2RP_Player::OnEnterStickyWalking()
{
	m_iRemainingStickyWalkHints = 0;
}

void CHL2RP_Player::PlayTimeContextThink()
{
	m_iSeconds = Clamp(m_iSeconds + 1, m_iSeconds, INT_MAX);
	m_iCrime = ::Max(0, m_iCrime - 1);
	TrySyncMainData(EDALMainPropSelection::Seconds | EDALMainPropSelection::Crime);
	SetNextThink(gpGlobals->curtime + HL2RP_PLAYER_HUD_INTERVAL, HL2RP_PLAYER_PLAYTIME_THINKCONTEXT_STRING);
}

void CHL2RP_Player::SetIdealActivity(Activity idealActivity, int sequence)
{
	SetActivity(idealActivity);
	SetPlaybackRate(1.0f);
	ResetSequence(sequence);
	SetCycle(0.0f);
}

void CHL2RP_Player::TrySetIdealActivityOrGesture(Activity standActivity, Activity crouchedActivity,
	Activity standGesture, Activity crouchedGesture, float duration)
{
	Activity finalActivity, finalGesture;

	if (GetFlags() & FL_ANIMDUCKING)
	{
		// We accept transitioning to ducked in order to activate the crouched activity variation
		finalActivity = crouchedActivity;

		finalGesture = crouchedGesture;
	}
	else
	{
		finalActivity = standActivity;
		finalGesture = standGesture;
	}

	// Try setting a complete activity, checking afterwards if we succeeded.
	// It should be a non-walking activity, so, our velocity must be zero first.
	if (GetAbsVelocity().IsZero())
	{
		int sequence = HL2RPRules()->GetPlayerActRemapSequence(this, finalActivity);

		if (sequence != ACTIVITY_NOT_AVAILABLE)
		{
			SetIdealActivity(finalActivity, sequence);

			if (duration > 0.0f)
			{
				m_flLockedPlaybackRate = SequenceDuration() / duration;
				SetPlaybackRate(m_flLockedPlaybackRate);
			}

			// Configure hidden gesture and exit
			m_HiddenGesture.m_Activity = finalGesture;
			m_HiddenGesture.m_flStartTime = gpGlobals->curtime;
			return;
		}
	}

	// Fallback to the gesture
	int sequence = HL2RPRules()->GetPlayerActRemapSequence(this, finalGesture);
	int layer = AddGestureSequence(sequence);

	if (duration > 0.0f)
	{
		SetLayerPlaybackRate(layer, GetLayerDuration(layer) / duration);
	}
}

void CHL2RP_Player::TrySyncWeapon(CBaseCombatWeapon* pWeapon)
{
	if (m_DALSyncCaps.IsFlagSet(EDALSyncCap::SaveData))
	{
		TryCreateAsyncDAO<CPlayerWeaponUpdateDAO>(this, pWeapon);
	}
}

const char* CHL2RP_Player::GetNetworkIDString()
{
	if (Q_strcmp(m_szNetworkIDString, "") == 0)
	{
		// It isn't loaded yet, retrieve value from engine and re-check.
		// Default Source code does not copy it upon a proper callback.
		BaseClass::GetNetworkIDString();
	}

	return m_szNetworkIDString;
}

bool CHL2RP_Player::LateLoadMainData(const char* pNetworkIdString)
{
	if (Q_strcmp(GetNetworkIDString(), pNetworkIdString) == 0)
	{
		if (m_DALSyncCaps.IsFlagSet(EDALSyncCap::LoadData))
		{
			TryCreateAsyncDAO<CPlayerLoadDAO>(this);
			m_DALSyncCaps.ClearFlag(EDALSyncCap::LoadData);
		}

		return true;
	}

	return false;
}

void CHL2RP_Player::SetCrime(int crime)
{
	m_iCrime = crime;
	TrySyncMainData(EDALMainPropSelection::Crime);
}

void CHL2RP_Player::SetAccessFlags(int accessFlags)
{
	m_AccessFlags.ClearAllFlags();
	m_AccessFlags.SetFlag(accessFlags);
	TrySyncMainData(EDALMainPropSelection::AccessFlags);
}

void CHL2RP_Player::AddAlertHUDLine(const char* pLine, float duration, CHUDChannelLine::EType type)
{
	CHUDChannelLine lineInfo(type, pLine);

	// Try replacing duplicated element, ensure a free slot otherwise
	if (!m_SharedHUDAlertList.FindAndRemove(lineInfo) &&
		m_SharedHUDAlertList.Count() >= HL2RP_PLAYER_HUD_ALERTS_MAX_LINES)
	{
		m_SharedHUDAlertList.RemoveMultipleFromTail(1);
	}

	m_SharedHUDAlertList.Insert(lineInfo);
	char fullText[MAX_DISPLAYABLE_USER_MSG_DATA];
	int fullTextLen = 0;

	FOR_EACH_VEC(m_SharedHUDAlertList, i)
	{
		fullTextLen += Q_snprintf(fullText + fullTextLen, sizeof(fullText) - fullTextLen,
			"%s\n", m_SharedHUDAlertList[i].m_Text);
	}

	UTIL_HUDMessage(this, EHUDChannelIndex::Alerts, -1.0f, HL2RP_PLAYER_HUD_ALERTS_Y_POS,
		duration, HL2RP_COLOR_HUD_ALERTS, fullText);
	SetContextThink(&CHL2RP_Player::CenterHUDLinesClearContextThink,
		m_flNextHUDChannelTime[EHUDChannelIndex::Alerts], HL2RP_PLAYER_HUD_ALERTS_CLEAR_CONTEXT);
}

void CHL2RP_Player::SetupSoundsByJobTeam()
{
	switch (m_JobTeamIndex)
	{
	case EJobTeamIndex::Citizen:
	{
		m_iPlayerSoundType = PLAYER_SOUNDS_CITIZEN;
		break;
	}
	case EJobTeamIndex::Police:
	{
		// As for the Combine team, use only this one, which we prefer
		m_iPlayerSoundType = PLAYER_SOUNDS_METROPOLICE;
		break;
	}
	}
}

void CHL2RP_Player::TryGiveLoadedHpAndArmor(int health, int armor)
{
	// Only set health if value is acceptable. Otherwise leave as is. Simple.
	if (health > 0)
	{
		m_iHealth = health;
	}

	SetArmorValue(UTIL_EnsureAddition(ArmorValue(), armor));
}

CBaseCombatWeapon* CHL2RP_Player::TryGiveLoadedWeapon(const char* pWeaponClassName, int clip1, int clip2)
{
	// We check this just in case, even when I shouldn't have it if called from Spawn (loaded on death)
	CBaseCombatWeapon* pWeapon = Weapon_OwnsThisType(pWeaponClassName);

	if (pWeapon == NULL)
	{
		CBaseEntity* pEntity = CreateNoSpawn(pWeaponClassName, GetLocalOrigin(), vec3_angle);

		if (pEntity != NULL)
		{
			// Quickly ensure that it's a weapon before proceeding
			if (!pEntity->IsBaseCombatWeapon())
			{
				// Remove this entity as it's useless here
				UTIL_RemoveImmediate(pEntity);
				return NULL;
			}

			// Begin task: Assign loaded clips and reserve ammo to the weapon, and equip it
			pWeapon = pEntity->MyCombatWeaponPointer();
			pWeapon->AddSpawnFlags(SF_NORESPAWN);
			DispatchSpawn(pWeapon);

			// Check this to prevent HUD from showing 0 ammo, if we ended with clip1 != WEAPON_NOCLIP
			if (pWeapon->UsesClipsForAmmo1())
			{
				// NOTE: Since limits for weapon clips are defined, we don't want to exceed them
				clip1 = Clamp(clip1, 0, pWeapon->GetMaxClip1());
				pWeapon->SetClip1(clip1);
			}

			// Check this to prevent HUD from showing 0 ammo, if we ended with clip2 != WEAPON_NOCLIP
			if (pWeapon->UsesClipsForAmmo2())
			{
				// NOTE: Since limits for weapon clips are defined, we don't want to exceed them
				clip2 = Clamp(clip2, 0, pWeapon->GetMaxClip2());
				pWeapon->SetClip2(clip2);
			}

			pWeapon->SetPrimaryAmmoCount(0);
			pWeapon->SetSecondaryAmmoCount(0);
			Weapon_Equip(pWeapon); // Don't replace by Touch, to avoid weapon not giving when player is stuck
		}
	}
	else
	{
		// Check this to prevent HUD from showing 0 ammo, if we ended with clip1 != WEAPON_NOCLIP
		if (pWeapon->UsesClipsForAmmo1())
		{
			// NOTE: Since limits for weapon clips are defined, we don't want to exceed them
			clip1 = UTIL_EnsureAddition(pWeapon->Clip1(), clip1);
			pWeapon->SetClip1(Min(clip1, pWeapon->GetMaxClip1()));
		}

		// Check this to prevent HUD from showing fake 0 ammo, if we ended with clip2 != WEAPON_NOCLIP
		if (pWeapon->UsesClipsForAmmo2())
		{
			// NOTE: Since limits for weapon clips are defined, we don't want to exceed them
			clip2 = UTIL_EnsureAddition(pWeapon->Clip2(), clip2);
			pWeapon->SetClip2(Min(clip2, pWeapon->GetMaxClip2()));
		}
	}

	return pWeapon;
}

// Input: propSelectionMask - A mask from EDALMainPropSelection values
void CHL2RP_Player::TrySyncMainData(int propSelectionMask)
{
	if (m_DALSyncCaps.IsFlagSet(EDALSyncCap::SaveData))
	{
		TryCreateAsyncDAO<CPlayerUpdateDAO>(this, propSelectionMask);
	}
}

// Attempts to set main attack activity, or otherwise its corresponding layered one (gesture).
// Input: duration - Forced duration to apply on the successful sequences. 0.0f to don't alter it.
void CHL2RP_Player::SetAttack1Animation(Activity standActivity, float duration)
{
	Weapon_SetActivity(Weapon_TranslateActivity(ACT_RANGE_ATTACK1), 0.0f); // Simply imitating default CHL2MP_Player code
	TrySetIdealActivityOrGesture(standActivity, ACT_RANGE_ATTACK1_LOW, ACT_GESTURE_RANGE_ATTACK1,
		ACT_GESTURE_RANGE_ATTACK1_LOW, duration);
}
