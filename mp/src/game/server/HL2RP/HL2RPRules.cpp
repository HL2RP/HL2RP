#include <cbase.h>
#include "HL2RPRules.h"
#include <ai_basenpc.h>
#include "AutoLocalizer.h"
#include "DAL/IDALEngine.h"
#include "HL2RPProperty.h"
#include "HL2RP_Defs.h"
#include "HL2RP_Player.h"
#include <networkstringtable_gamedll.h>

#ifdef HL2DM_RP
#include <tier2/fileutils.h>
#include <filesystem_helpers.h>
#include <utlbuffer.h>
#endif // HL2DM_RP

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define DOWNLOADABLE_FILE_TABLENAME	"downloadables"

#define HL2RP_RULES_PLAYER_ACT_REMAP_RELATIVE_PATH	"scripts/player_actremap.txt"
#define HL2RP_RULES_PLAYER_ACT_REMAP_ROOT_KEYNAME	"player"
#define HL2RP_RULES_NPCS_ACT_REMAP_RELATIVE_PATH	"scripts/ai_basenpc_actremap.txt"
#define HL2RP_RULES_NPCS_ACT_REMAP_ROOT_KEYNAME	"AI_BaseNPC"
#define HL2RP_RULES_HUD_DEATHNOTICE_TIME	6.0f

// We use a new path ID for the Downloader to consistently filter the files we really want
#define HL2RP_RULES_UPLOAD_SEARCH_PATH	"upload"

#define HL2RP_LOC_TOKEN_CHAT_OOC	"#HL2RP_Chat_OOC"

#ifdef HL2DM_RP
#define HL2RP_RULES_UPLOAD_LIST_RELATIVE_PATH	"uploadlist.txt"

void __CreateGameRules_CHL2RPRules()
{
	new CHL2RPRules;
}

// Links to CHL2RPRules while remaining compatible with HL2DM client connects
static CGameRulesRegister __g_GameRulesRegister_CHL2RPRules
("CHL2MPRules", __CreateGameRules_CHL2RPRules);
#endif // HL2DM_RP

SJob::SJob()
{

}

SJob::SJob(const char* pModelPath, bool isReadOnly)
	: m_IsReadOnly(isReadOnly)
{
	Q_strcpy(m_ModelPath, pModelPath);
}

CHL2RPRules::CHL2RPRules() : m_iBeamModelIndex(-1)
{
	SharedConstructor();

	// Proceed to register default job infos
	const char* ppDefaultCitizenJobPairs[][2] =
	{
		{ "Peaceful Male 1", "models/humans/group01/male_01.mdl" },
		{ "Peaceful Male 2", "models/humans/group01/male_02.mdl" },
		{ "Peaceful Male 3", "models/humans/group01/male_03.mdl" },
		{ "Peaceful Male 4", "models/humans/group01/male_04.mdl" },
		{ "Peaceful Male 5", "models/humans/group01/male_05.mdl" },
		{ "Peaceful Male 6", "models/humans/group01/male_06.mdl" },
		{ "Peaceful Male 7", "models/humans/group01/male_07.mdl" },
		{ "Peaceful Male 8", "models/humans/group01/male_08.mdl" },
		{ "Peaceful Male 9", "models/humans/group01/male_09.mdl" },
		{ "Peaceful Female 1", "models/humans/group01/female_01.mdl" },
		{ "Peaceful Female 2", "models/humans/group01/female_02.mdl" },
		{ "Peaceful Female 3", "models/humans/group01/female_03.mdl" },
		{ "Peaceful Female 4", "models/humans/group01/female_04.mdl" },
		{ "Peaceful Female 6", "models/humans/group01/female_06.mdl" },
		{ "Peaceful Female 7", "models/humans/group01/female_07.mdl" },
		{ "Intermediate Male 1", "models/humans/group01/male_01.mdl" },
		{ "Intermediate Male 2", "models/humans/group01/male_02.mdl" },
		{ "Intermediate Male 3", "models/humans/group01/male_03.mdl" },
		{ "Intermediate Male 4", "models/humans/group01/male_04.mdl" },
		{ "Intermediate Male 5", "models/humans/group01/male_05.mdl" },
		{ "Intermediate Male 6", "models/humans/group01/male_06.mdl" },
		{ "Intermediate Male 7", "models/humans/group01/male_07.mdl" },
		{ "Intermediate Male 8", "models/humans/group01/male_08.mdl" },
		{ "Intermediate Male 9", "models/humans/group01/male_09.mdl" },
		{ "Intermediate Female 1", "models/humans/group01/female_01.mdl" },
		{ "Intermediate Female 2", "models/humans/group01/female_02.mdl" },
		{ "Intermediate Female 3", "models/humans/group01/female_03.mdl" },
		{ "Intermediate Female 4", "models/humans/group01/female_04.mdl" },
		{ "Intermediate Female 6", "models/humans/group01/female_06.mdl" },
		{ "Intermediate Female 7", "models/humans/group01/female_07.mdl" },
		{ "Rebel Male 1", "models/humans/group03/male_01.mdl" },
		{ "Rebel Male 2", "models/humans/group03/male_02.mdl" },
		{ "Rebel Male 3", "models/humans/group03/male_03.mdl" },
		{ "Rebel Male 4", "models/humans/group03/male_04.mdl" },
		{ "Rebel Male 5", "models/humans/group03/male_05.mdl" },
		{ "Rebel Male 6", "models/humans/group03/male_06.mdl" },
		{ "Rebel Male 7", "models/humans/group03/male_07.mdl" },
		{ "Rebel Male 8", "models/humans/group03/male_08.mdl" },
		{ "Rebel Male 9", "models/humans/group03/male_09.mdl" },
		{ "Rebel Female 1", "models/humans/group03/female_01.mdl" },
		{ "Rebel Female 2", "models/humans/group03/female_02.mdl" },
		{ "Rebel Female 3", "models/humans/group03/female_03.mdl" },
		{ "Rebel Female 4", "models/humans/group03/female_04.mdl" },
		{ "Rebel Female 6", "models/humans/group03/female_06.mdl" },
		{ "Rebel Female 7", "models/humans/group03/female_07.mdl" },
	};

	for (const char** ppAuxCitizenJobModel : ppDefaultCitizenJobPairs)
	{
		SJob auxJob(ppAuxCitizenJobModel[1], true);
		CBaseEntity::PrecacheModel(ppAuxCitizenJobModel[1]);
		m_JobTeamsModels[EJobTeamIndex::Citizen].Insert(ppAuxCitizenJobModel[0], auxJob);
	}

	m_JobTeamsModels[EJobTeamIndex::Police].Insert("Police Recruit", "models/police.mdl");
}

// Actions done here should better not include map/entity stuff,
// do that on LevelInitPostEntity -where map is loaded- to feel safer
bool CHL2RPRules::Init()
{
	if (!BaseClass::Init())
	{
		return false;
	}

	m_iBeamModelIndex = CBaseEntity::PrecacheModel("sprites/laserbeam.vmt");
	m_Localizer.Init();
	m_DAL.Init();
	return true;
}

void CHL2RPRules::LevelInitPostEntity()
{
	BaseClass::LevelInitPostEntity();
	UTIL_LoadActivityRemapFile(HL2RP_RULES_PLAYER_ACT_REMAP_RELATIVE_PATH,
		HL2RP_RULES_PLAYER_ACT_REMAP_ROOT_KEYNAME, m_PlayerActRemap);
	UTIL_LoadActivityRemapFile(HL2RP_RULES_NPCS_ACT_REMAP_RELATIVE_PATH, HL2RP_RULES_NPCS_ACT_REMAP_ROOT_KEYNAME,
		m_NPCsActRemap);

	// Prepare and add downloadables
	unsigned int time0 = Plat_MSTime();
	FileFindHandle_t handle;
	const char* pFileName = filesystem->FindFirstEx("*", HL2RP_RULES_UPLOAD_SEARCH_PATH, &handle);
	char relativePath[MAX_PATH];
	relativePath[0] = '\0';
	INetworkStringTable* pDownloadables = networkstringtable->FindTable(DOWNLOADABLE_FILE_TABLENAME);
	int numFilesAdded = AddDownloadables(handle, pFileName, relativePath, pDownloadables);

#ifdef HL2DM_RP
	// Register the custom assets, as clients may not have them by default (unlike standalone)
	CBaseFile file;
	file.Open(HL2RP_RULES_UPLOAD_LIST_RELATIVE_PATH, "r");
	CUtlBuffer buf;
	file.ReadFile(buf);
	char downloadEntry[MAX_PATH];

	for (const char* pRemainingText = ParseFile((char*)buf.Base(), downloadEntry, NULL);
		pRemainingText != NULL; pRemainingText = ParseFile(pRemainingText, downloadEntry, NULL))
	{
		if (pDownloadables->AddString(true, downloadEntry) != INVALID_STRING_INDEX)
		{
			numFilesAdded++;
		}
	}
#endif // HL2DM_RP

	DevMsg("CMultiplayRules: LevelInitPostEntity() took %s ms to register %s downloadable files\n",
		Q_pretifynum(Plat_MSTime() - time0), Q_pretifynum(numFilesAdded));
	m_DAL.LevelInitPostEntity();
}

void CHL2RPRules::LevelShutdown()
{
	m_DAL.LevelShutdown();
}

void CHL2RPRules::Think()
{
	BaseClass::Think();
	m_DAL.Think();
}

void CHL2RPRules::DeathNotice(CBasePlayer* pVictim, const CTakeDamageInfo& info)
{
	DeathNotice(pVictim->MyCombatCharacterPointer(), info);
}

void CHL2RPRules::ClientSettingsChanged(CBasePlayer* pPlayer)
{
	const char* pDesiredModelPath = engine->GetClientConVarValue(pPlayer->entindex(), "cl_playermodel");

	if (Q_stricmp(STRING(pPlayer->GetModelName()), pDesiredModelPath) != 0)
	{
		pPlayer->SetModel(pDesiredModelPath); // Let our overriden method validate the model
	}

	BaseClass::ClientSettingsChanged(pPlayer);
	ToHL2RPPlayer_Fast(pPlayer)->SetupSoundsByJobTeam();
}

void CHL2RPRules::ClientDisconnected(edict_t* pEdict)
{
	CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(CBaseEntity::Instance(pEdict));

	if (pPlayer != NULL)
	{
		// Weapons/ammo are going to be removed (on this BaseClass chain). Must disable syncing while disconnecting.
		pPlayer->m_DALSyncCaps.ClearAllFlags();

		// ONLY AFTER syncing is off, clear safely the active weapon so BaseClass chain doesn't drop it
		pPlayer->ClearActiveWeapon();
	}

	BaseClass::ClientDisconnected(pEdict);
}

const char* CHL2RPRules::GetChatFormat(bool isTeamOnly, CBasePlayer* pPlayer)
{
	if (pPlayer != NULL) // Filter out dedicated server output
	{
		if (isTeamOnly)
		{
			return m_Localizer.Localize(pPlayer, "#HL2RP_Chat_Region");
		}
		else
		{
#ifdef HL2DM_RP
			return m_Localizer.Localize(pPlayer, HL2RP_LOC_TOKEN_CHAT_OOC);
#else
			// As this prefix doesn't require custom colors, we let the client format the phrase to save bandwidth
			return HL2RP_LOC_TOKEN_CHAT_OOC;
#endif // HL2DM_RP
		}
	}

	return NULL;
}

// We assume this is called during a teamonly chat evaluation. So, we evaluate region conditions.
bool CHL2RPRules::PlayerCanHearChat(CBasePlayer* pListener, CBasePlayer* pSpeaker)
{
	if (pSpeaker->GetAbsOrigin().DistToSqr(pListener->GetAbsOrigin())
		<= HL2RP_RULES_REGION_PLAYER_INTER_RADIUS_SQR)
	{
		return true;
	}

	return false;
}

int CHL2RPRules::AddDownloadables(FileFindHandle_t handle, const char* pFileName, char relativePath[MAX_PATH],
	INetworkStringTable* pDownloadables)
{
	if (handle != FILESYSTEM_INVALID_FIND_HANDLE)
	{
		int numFilesAdded = 0;

		if (pFileName == NULL)
		{
			filesystem->FindClose(handle);
			return 0;
		}
		else if (pFileName[0] != '.')
		{
			int initialLen = Q_strlen(relativePath);

			if (filesystem->FindIsDirectory(handle))
			{
				int finderLen = initialLen + Q_snprintf(relativePath + initialLen, MAX_PATH - initialLen, "%s/*", pFileName);
				FileFindHandle_t auxHandle;
				const char* pAuxFileName = filesystem->FindFirstEx(relativePath, HL2RP_RULES_UPLOAD_SEARCH_PATH, &auxHandle);
				relativePath[finderLen - 1] = '\0';
				numFilesAdded += AddDownloadables(auxHandle, pAuxFileName, relativePath, pDownloadables);
			}
			else
			{
				Q_strncpy(relativePath + initialLen, pFileName, MAX_PATH - initialLen);

				if (pDownloadables->AddString(true, relativePath) != INVALID_STRING_INDEX)
				{
					numFilesAdded++;
				}
			}

			relativePath[initialLen] = '\0';
		}

		pFileName = filesystem->FindNext(handle);
		return (numFilesAdded + AddDownloadables(handle, pFileName, relativePath, pDownloadables));
	}

	return 0;
}

IGameEvent* CHL2RPRules::BuildPlayerDeathEvent(int victimId, int killerId, const char* pKillerWeaponName)
{
	IGameEvent* pEvent = gameeventmanager->CreateEvent("player_death");

	if (pEvent != NULL)
	{
		pEvent->SetString("weapon", pKillerWeaponName);
		pEvent->SetInt("priority", 7);
		pEvent->SetInt("userid", victimId);
		pEvent->SetInt("attacker", killerId);
	}

	return pEvent;
}

void CHL2RPRules::HandleNPCActRemapGesture(CAI_BaseNPC* pNPC, CActivityRemap& actRemap)
{
	if (actRemap.GetExtraKeyValueBlock() != NULL)
	{
		const char* pRequestedGestureName = actRemap.GetExtraKeyValueBlock()->GetString("gesture", NULL);

		if (pRequestedGestureName != NULL)
		{
			Activity requestedGestureIndex = static_cast<Activity>(ActivityList_IndexForName(pRequestedGestureName));
			Activity weaponGesture = pNPC->Weapon_TranslateActivity(requestedGestureIndex);
			pNPC->RestartGesture(weaponGesture);
		}
	}
}

int CHL2RPRules::GetActRemapSequence(CUtlVector<CActivityRemap>& actRemapList, CBaseCombatCharacter* pCharacter,
	Activity initialActivity, bool isRequired, int& actRemapIndexOut)
{
	Assert(pCharacter != NULL);
	int fallbackSequence = ACTIVITY_NOT_AVAILABLE;

	FOR_EACH_VEC(actRemapList, i)
	{
		actRemapIndexOut = i;
		CActivityRemap& actRemap = actRemapList[actRemapIndexOut];

		if (actRemap.activity == initialActivity)
		{
			bool auxIsRequired = isRequired;
			Activity auxActivity = pCharacter->Weapon_TranslateActivity(actRemap.mappedActivity, &auxIsRequired);
			int auxSequence = pCharacter->SelectWeightedSequence(auxActivity);

			if (auxSequence == ACTIVITY_NOT_AVAILABLE)
			{
				if (auxActivity == actRemap.mappedActivity)
				{
					continue;
				}

				auxSequence = pCharacter->SelectWeightedSequence(actRemap.mappedActivity);

				if (auxSequence == ACTIVITY_NOT_AVAILABLE)
				{
					continue;
				}
			}

			// Pass if activity got translated to a different one or we don't require that
			if (auxActivity != actRemap.mappedActivity || !auxIsRequired)
			{
				return auxSequence;
			}

			fallbackSequence = auxSequence;
		}
	}

	if (fallbackSequence == ACTIVITY_NOT_AVAILABLE)
	{
		// Fallback to the initial activity
		Activity activity = pCharacter->Weapon_TranslateActivity(initialActivity);
		fallbackSequence = pCharacter->SelectWeightedSequence(activity);
		actRemapIndexOut = ACT_INVALID;

		if (fallbackSequence == ACTIVITY_NOT_AVAILABLE && activity != initialActivity)
		{
			fallbackSequence = pCharacter->SelectWeightedSequence(initialActivity);
		}
	}

	return fallbackSequence;
}

void CHL2RPRules::NotifyPlayerVsNPCDeath(CBaseEntity* pAttacker, CBaseEntity* pVictim, const char* pKillerWeaponName)
{
#ifdef HL2DM_RP
	CUtlVector<CHL2RP_Player*> playerList;
	CollectHumanPlayers(&playerList, TEAM_ANY, true);

	FOR_EACH_VEC(playerList, i)
	{
		const char* pDeathInfoLine;

		if (pAttacker->IsPlayer())
		{
			pDeathInfoLine = m_Localizer.Localize(playerList[i], "HL2RP_HUD_Player_Killed_NPC",
				ToHL2RPPlayer_Fast(pAttacker)->GetPlayerName(), pVictim->GetEntityName(), pKillerWeaponName);
		}
		else if (playerList[i] != pVictim)
		{
			pDeathInfoLine = m_Localizer.Localize(playerList[i], "HL2RP_HUD_NPC_Killed_Player",
				pAttacker->GetEntityName(), ToHL2RPPlayer_Fast(pVictim)->GetPlayerName(), pKillerWeaponName);
		}
		else
		{
			// Discard for alive victim player within the list
			continue;
		}

		playerList[i]->AddAlertHUDLine(pDeathInfoLine, HL2RP_RULES_HUD_DEATHNOTICE_TIME);
	}
#else
	IGameEvent* pEvent = gameeventmanager->CreateEvent("player_npc_death");

	if (pEvent != NULL)
	{
		pEvent->SetInt("entindex_attacker", pAttacker->entindex());
		pEvent->SetInt("entindex_killed", pVictim->entindex());
		pEvent->SetString("weapon", pKillerWeaponName);
		pEvent->SetInt("priority", 7);

		if (pAttacker->IsPlayer())
		{
			pEvent->SetString("npcname", STRING(pVictim->GetEntityName()));
		}
		else
		{
			pEvent->SetString("npcname", STRING(pAttacker->GetEntityName()));
		}

		gameeventmanager->FireEvent(pEvent);
	}
#endif // HL2DM_RP
}

void CHL2RPRules::NetworkIdValidated(const char* pNetworkIdString)
{
	// Make a copy because we are eventually going to call the engine for each player,
	// and it writes on same string than parameter's, so we don't compare false positives
	char networkIdStringCopy[MAX_NETWORKID_LENGTH];
	Q_strcpy(networkIdStringCopy, pNetworkIdString);

	// Search the player with the validated network ID string
	for (int i = gpGlobals->maxClients; i >= 1; i--)
	{
		CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_PlayerByIndex(i));

		if (pPlayer != NULL && pPlayer->LateLoadMainData(networkIdStringCopy))
		{
			break;
		}
	}
}

int CHL2RPRules::GetJobTeamIndexForModel(const char* pModelPath)
{
	for (int i = 0; i < JobTeamIndex_Max; i++)
	{
		FOR_EACH_DICT_FAST(m_JobTeamsModels[i], j)
		{
			if (Q_stricmp(m_JobTeamsModels[i][j].m_ModelPath, pModelPath) == 0)
			{
				return i;
			}
		}
	}

	return EJobTeamIndex::Invalid;
}

void CHL2RPRules::DeathNotice(CBaseCombatCharacter* pVictim, const CTakeDamageInfo& info)
{
	// Work out what killed the player, and send a message to all clients about it
	const char* pKillerWeaponName = "world"; // By default, the player is killed by the world
	int killerId = 0, victimId = 0;

	// Find the killer & the scorer
	CBaseEntity* pInflictor = info.GetInflictor(), * pKiller = info.GetAttacker();
	CBasePlayer* pScorer = GetDeathScorer(pKiller, pInflictor);

	// Custom kill type?
	if (info.GetDamageCustom() != 0)
	{
		pKillerWeaponName = GetDamageCustomString(info);
	}
	else
	{
		// Is the killer a client?
		if (pScorer != NULL)
		{
			if (pInflictor == pScorer)
			{
				// If the inflictor is the killer, then it must be their current weapon doing the damage
				if (pScorer->GetActiveWeapon())
				{
					pKillerWeaponName = pScorer->GetActiveWeapon()->GetClassname();
				}
			}
			else if (pInflictor != NULL)
			{
				pKillerWeaponName = pInflictor->GetClassname(); // It's just that easy
			}
		}
		else if (pInflictor == pKiller && pKiller->IsNPC()
			&& pKiller->MyNPCPointer()->GetActiveWeapon() != NULL)
		{
			pKillerWeaponName = pKiller->MyNPCPointer()->GetActiveWeapon()->GetClassname();
		}
		else
		{
			pKillerWeaponName = pInflictor->GetClassname();
		}

		// Strip the NPC_* or weapon_* from the inflictor's classname
		if (Q_strncmp(pKillerWeaponName, "weapon_", 7) == 0)
		{
			pKillerWeaponName += 7;
		}
		else if (Q_strncmp(pKillerWeaponName, "npc_", 4) == 0)
		{
			pKillerWeaponName += 4;

			if (Q_strcmp(pKillerWeaponName, "satchel") == 0 || Q_strcmp(pKillerWeaponName, "tripmine") == 0)
			{
				pKillerWeaponName = "slam";
			}
		}
		else if (Q_strncmp(pKillerWeaponName, "func_", 5) == 0)
		{
			pKillerWeaponName += 5;
		}
		else if (Q_strstr(pKillerWeaponName, "physics") != NULL)
		{
			pKillerWeaponName = "physics";
		}
		else if (Q_strcmp(pKillerWeaponName, "prop_combine_ball") == 0)
		{
			pKillerWeaponName = "combine_ball";
		}
		else if (Q_strcmp(pKillerWeaponName, "grenade_ar2") == 0)
		{
			pKillerWeaponName = "smg1_grenade";
		}
	}

	if (pScorer != NULL)
	{
		killerId = pScorer->GetUserID();

		if (pVictim->IsPlayer())
		{
			victimId = ToHL2RPPlayer_Fast(pVictim)->GetUserID();
		}
		else
		{
			if (pVictim->IsNPC())
			{
				NotifyPlayerVsNPCDeath(pScorer, pVictim, pKillerWeaponName);
			}

			return; // Event is out of interest
		}
	}
	else if (pVictim->IsPlayer()) // pScorer == NULL
	{
		victimId = ToHL2RPPlayer_Fast(pVictim)->GetUserID();

		if (pKiller != NULL && pKiller->IsNPC())
		{
			return NotifyPlayerVsNPCDeath(pKiller, pVictim, pKillerWeaponName);
		}
	}
	else
	{
		return; // Event is out of interest
	}

	IGameEvent* pEvent = BuildPlayerDeathEvent(victimId, killerId, pKillerWeaponName);

	if (pEvent != NULL)
	{
		gameeventmanager->FireEvent(pEvent);
	}
}

int CHL2RPRules::GetPlayerActRemapSequence(CBasePlayer* pPlayer, Activity initialActivity)
{
	int actRemapIndex;
	return GetActRemapSequence(m_PlayerActRemap, pPlayer, initialActivity, true, actRemapIndex);
}

Activity CHL2RPRules::GetNPCActRemapActivity(CAI_BaseNPC* pNPC, Activity initialActivity)
{
	int actRemapIndex;
	GetActRemapSequence(m_NPCsActRemap, pNPC, initialActivity, false, actRemapIndex);

	if (actRemapIndex != ACT_INVALID)
	{
		HandleNPCActRemapGesture(pNPC, m_NPCsActRemap[actRemapIndex]);
		return m_NPCsActRemap[actRemapIndex].mappedActivity;
	}

	return initialActivity;
}

CHL2RPPropertyDoor* GetHL2RPPropertyDoor(CBaseEntity* pEntity)
{
	if (pEntity != NULL)
	{
		int entHandleIndex = pEntity->GetRefEHandle().ToInt();
		CHandle<CHL2RPPropertyDoor>* pDoorHandle = HL2RPRules()->
			m_PropertyDoorTable.GetPtr(entHandleIndex);

		if (pDoorHandle != NULL)
		{
			return *pDoorHandle;
		}
	}

	return NULL;
}

class CUserInputDAO : public IDAO
{
public:
	CUserInputDAO(const char* pQueryText);

private:
	void Execute(CSQLEngine* pSQL) OVERRIDE;

	char m_QueryText[DAL_ENGINE_SQL_QUERY_SIZE];
};

CUserInputDAO::CUserInputDAO(const char* pQueryText)
{
	Q_strncpy(m_QueryText, pQueryText, sizeof(m_QueryText));
}

void CUserInputDAO::Execute(CSQLEngine* pSQL)
{
	pSQL->ExecuteQuery(m_QueryText);
}

CON_COMMAND(fake_client, "Creates fake client")
{
	if (UTIL_IsCommandIssuedByServerAdmin())
	{
		edict_t* pEdict = engine->CreateFakeClientEx("Fake Player", false);

		if (pEdict != NULL)
		{
			CBaseEntity::Instance(pEdict)->AddFlag(FL_FAKECLIENT);
		}
	}
}

CON_COMMAND(restart_map, "Restarts current map")
{
	if (UTIL_IsCommandIssuedByServerAdmin())
	{
		engine->ChangeLevel(STRING(gpGlobals->mapname), NULL);
	}
}

CON_COMMAND(set_admin, "<userid> - Sets a player admin")
{
	if (UTIL_IsCommandIssuedByServerAdmin())
	{
		CHL2RP_Player* pPlayer = ToHL2RPPlayer_Fast(UTIL_PlayerByUserId(atoi(args.Arg(1))));

		if (pPlayer != NULL && !pPlayer->IsAdmin())
		{
			pPlayer->SetAccessFlags(CHL2RP_Player::EAccessFlag::Admin);
			const char* pMsg = GetHL2RPAutoLocalizer().Localize(pPlayer, "HL2RP_Chat_Admin_Status_Gained");
			ClientPrint(pPlayer, HUD_PRINTTALK, pMsg);
		}
	}
}

CON_COMMAND(sql_query, "Sends a query to the current database")
{
	TryCreateAsyncDAO<CUserInputDAO>(args.Arg(1));
}