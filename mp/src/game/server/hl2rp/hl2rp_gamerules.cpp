// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gamerules.h"
#include "hl2rp_gameinterface.h"
#include "hl2_roleplayer.h"
#include <hl2rp_property.h>
#include <activitylist.h>
#include <networkstringtable_gamedll.h>
#include <sourcehook.h>
#include <tier2/fileutils.h>
#include <usermessages.h>

#define DOWNLOADABLE_FILE_TABLENAME "downloadables"

#define HL2RP_RULES_UPLOAD_PATH_ID   "upload"
#define HL2RP_RULES_UPLOAD_LIST_FILE (HL2RP_CONFIG_PATH "upload.txt")

#define ONE_DAY_IN_SECONDS 86400

extern SourceHook::ISourceHook* g_SHPtr;
extern ConVar gMaxHomeInactivityDays, gRegionMaxRadiusCVar;

ConVar gPoliceWaveCountCVar("sv_police_wave_count", "5", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Amount of police NPCs to spawn or keep in each wave"),
	gPoliceWavePeriodCVar("sv_police_wave_period", "60", FCVAR_ARCHIVE | FCVAR_NOTIFY,
		"Period to (re)create police NPCs in waves");

#ifdef HL2RP_FULL
void InstallGameRules()
{
	CreateGameRulesObject("CHL2RPRules");
}
#else
static void CreateHL2RPRules()
{
	new CHL2RPRules;
}

static CGameRulesRegister sHL2RPRulesRegister("CHL2MPRules", CreateHL2RPRules);
#endif // HL2RP_FULL

bf_write* CHUDMsgInterceptor::OnUserMessageBegin(IRecipientFilter* pFilter, int type)
{
	if (FStrEq(usermessages->GetUserMessageName(type), "HudMsg"))
	{
		mpWriter = META_RESULT_ORIG_RET(bf_write*);
		mpRecipientFilter = pFilter;
	}

	RETURN_META_VALUE(MRES_IGNORED, mpWriter);
}

void CHUDMsgInterceptor::OnMessageEnd()
{
	if (mpWriter != NULL)
	{
		for (int i = 0; i < mpRecipientFilter->GetRecipientCount(); ++i)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex(mpRecipientFilter->GetRecipientIndex(i));
			ToHL2Roleplayer(pPlayer)->OnPreSendHUDMessage(mpWriter);
		}

		mpWriter = NULL;
	}
}

CJobData::CJobData(KeyValues* pJobKV, int faction) : CPlayerEquipment(pJobKV->GetInt("health"),
	pJobKV->GetInt("armor"), true), mIsGod(pJobKV->GetBool("god"))
{
	if (pJobKV->GetBool("root"))
	{
		mRequiredAccessFlag.SetBit(EPlayerAccessFlag::Root);
	}
	else if (pJobKV->GetBool("admin"))
	{
		mRequiredAccessFlag.SetBit(EPlayerAccessFlag::Admin);
	}
	else if (pJobKV->GetBool("vip"))
	{
		mRequiredAccessFlag.SetBit(faction == EFaction::Citizen ?
			EPlayerAccessFlag::VIPCitizen : EPlayerAccessFlag::VIPCombine);
	}

	AddModelGroup(pJobKV->GetString("model"));
	KeyValues* pModelGroupsKV = pJobKV->FindKey("models"), * pWeapons = pJobKV->FindKey("Weapons");

	if (pModelGroupsKV != NULL)
	{
		FOR_EACH_VALUE(pModelGroupsKV, pModelGroupKV)
		{
			AddModelGroup(pModelGroupKV->GetString());
		}
	}

	if (pWeapons != NULL)
	{
		FOR_EACH_SUBKEY(pWeapons, pWeapon)
		{
			AddWeapon(pWeapon->GetName(), pWeapon->GetInt("clip1"), pWeapon->GetInt("clip2"));
			WEAPON_FILE_INFO_HANDLE handle;

			if (ReadWeaponDataFromFileForSlot(filesystem, pWeapon->GetName(), &handle))
			{
				FileWeaponInfo_t* pInfo = GetFileWeaponInfoFromHandle(handle);
				mAmmoCountByIndex.InsertOrReplace(pInfo->iAmmoType, pWeapon->GetInt("ammo1"));
				mAmmoCountByIndex.InsertOrReplace(pInfo->iAmmo2Type, pWeapon->GetInt("ammo2"));
			}
		}
	}
}

void CJobData::AddModelGroup(const char* pGroup)
{
	int index = HL2RPRules()->mPlayerModelsByGroup.Find(pGroup);

	if (HL2RPRules()->mPlayerModelsByGroup.IsValidIndex(index))
	{
		mModelGroupIndices.InsertIfNotFound(index);
	}
}

void CHL2RPRules::CPlayerModelDict::AddModel(KeyValues* pModelKV, INetworkStringTable* pModelPrecache)
{
	CFmtStrN<MAX_PATH> path("models/%s", pModelKV->GetString());

	// Try loading from a potential wildcard search path
	if (strchr(pModelKV->GetString(), '*') != NULL)
	{
		FileFindHandle_t findHandle;
		const char* pName = filesystem->FindFirst(path, &findHandle);

		if (pName != NULL)
		{
			Q_StripFilename(path.Access());
			path.SetLength(Q_strlen(path));

			do
			{
				char alias[MAX_PATH];
				Q_StrSubst(pName, "_", " ", alias, sizeof(alias));
				Q_StripExtension(alias, alias, sizeof(alias));
				Q_snprintf(path.Access() + path.Length(), path.GetMaxLength() - path.Length(), "/%s", pName);
				AddExactModel(alias, path, pModelPrecache);
			} while ((pName = filesystem->FindNext(findHandle)) != NULL);
		}

		return filesystem->FindClose(findHandle);
	}

	AddExactModel(pModelKV->GetName(), path, pModelPrecache);
}

void CHL2RPRules::CPlayerModelDict::AddExactModel(const char* pAlias,
	const char* pPath, INetworkStringTable* pModelPrecache)
{
	if (!HasElement(pAlias))
	{
		const char* pExtension = Q_GetFileExtension(pPath);

		if (pExtension != NULL && Q_stricmp(pExtension, "mdl") == 0) // Check for safety
		{
			int index = CBaseEntity::PrecacheModel(pPath);
			Insert(pAlias, pModelPrecache->GetString(index)); // Save allocated engine string
		}
	}
}

void CHL2RPRules::CActivityList::AddActivity(KeyValues* pActivityKV)
{
	int activity = ActivityList_IndexForName(pActivityKV->GetString());

	if (activity != kActivityLookup_Missing)
	{
		AddToTail((Activity)activity);
	}
}

CHL2RPRules::CHL2RPRules() : mExcludedUploadExts(CaselessStringLessThan),
mpPlayerResponseSystem(PrecacheCustomResponseSystem("scripts/player_responses.txt"))
{
	mExcludedUploadExts.Insert("bz2");
	mExcludedUploadExts.Insert("cache");
	mExcludedUploadExts.Insert("ztmp");
	mMapGroups.SetLessFunc(CaselessStringLessThan);
	mPlayerModelsByGroup.SetLessFunc(CaselessStringLessThan);

	for (auto& jobs : mJobByName)
	{
		jobs.SetLessFunc(CaselessStringLessThan);
	}

#ifdef HL2RP_FULL
	if (!engine->IsDedicatedServer())
	{
		// Enable this to overcome ClientDisconnected() not being called for listenserver's host
		ListenForGameEvent("player_disconnect");
	}
#endif // HL2RP_FULL

	// Register custom user assets for download
	uint startTime = Plat_MSTime();
	INetworkStringTable* pModelPrecache = networkstringtable->FindTable("modelprecache"),
		* pDownloadables = networkstringtable->FindTable(DOWNLOADABLE_FILE_TABLENAME);
	int oldDownloadablesCount = pDownloadables->GetNumStrings();
	CFmtStrN<MAX_PATH> path;
	RegisterDownloadableFiles(path, FILESYSTEM_INVALID_FIND_HANDLE, pDownloadables);

#ifdef HL2RP_LEGACY
	// Register default HL2RP assets for download (only needed in HL2DM)
	CInputTextFile uploadsList(HL2RP_RULES_UPLOAD_LIST_FILE);

	for (char entry[MAX_PATH]; uploadsList.ReadLine(entry, sizeof(entry)) != NULL;)
	{
		Q_StripPrecedingAndTrailingWhitespace(entry);
		Q_FixSlashes(entry);

		if (pDownloadables->AddString(true, entry) == INVALID_STRING_INDEX)
		{
			break;
		}
	}
#endif // HL2RP_LEGACY

	DevMsg("HL2RPRules: Took %s ms to register %s downloadable files\n", Q_pretifynum(Plat_MSTime() - startTime),
		Q_pretifynum(pDownloadables->GetNumStrings() - oldDownloadablesCount));
	KeyValuesAD mapGroupsKV(""), modelsKV(""), jobsKV("");

	// Load related map groups
	if (mapGroupsKV->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "mapgroups.txt"))
	{
		FOR_EACH_TRUE_SUBKEY(mapGroupsKV, pMapGroupKV)
		{
			FOR_EACH_VALUE(pMapGroupKV, pMapKV)
			{
				if (Q_stricmp(pMapKV->GetString(), STRING(gpGlobals->mapname)) == 0)
				{
					mMapGroups.InsertIfNotFound(pMapGroupKV->GetName());
					break;
				}
			}
		}
	}

	// Load player models
	if (modelsKV->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "models.txt"))
	{
		const char* typeNames[EPlayerModelType::_Count] = { "Job", "Public", "VIP", "Admin", "Root" };

		for (int i = 0; i < ARRAYSIZE(typeNames); ++i)
		{
			KeyValues* pTypeKV = modelsKV->FindKey(typeNames[i]);

			if (pTypeKV != NULL)
			{
				FOR_EACH_SUBKEY(pTypeKV, pGroupKV)
				{
					if (!mPlayerModelsByGroup.IsValidIndex(mPlayerModelsByGroup.Find(pGroupKV->GetName())))
					{
						CPlayerModelDict* pSubModels = new CPlayerModelDict;

						if (pGroupKV->GetFirstSubKey() == NULL)
						{
							pSubModels->AddModel(pGroupKV, pModelPrecache);
						}
						else
						{
							FOR_EACH_VALUE(pGroupKV, pModelKV)
							{
								pSubModels->AddModel(pModelKV, pModelPrecache);
							}
						}

						if (pSubModels->Count() < 1)
						{
							delete pSubModels;
							continue;
						}

						pSubModels->mType = (EPlayerModelType)i;
						int index = mPlayerModelsByGroup.Insert(pGroupKV->GetName(), pSubModels);
						mPlayerModelsByGroup.mGroupIndices[i].AddToTail(index);
					}
				}
			}
		}
	}

	// Load jobs data
	if (jobsKV->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "jobs.txt"))
	{
		const char* factionNames[EFaction::_Count] = { "Citizen", "Combine" };

		for (int i = 0; i < ARRAYSIZE(factionNames); ++i)
		{
			KeyValues* pFactionKV = jobsKV->FindKey(factionNames[i]);

			if (pFactionKV != NULL)
			{
				FOR_EACH_SUBKEY(pFactionKV, pJobKV)
				{
					if (!mJobByName[i].IsValidIndex(mJobByName[i].Find(pJobKV->GetName())))
					{
						mJobByName[i].Insert(pJobKV->GetName(), new CJobData(pJobKV, i));
					}
				}
			}
		}
	}
}

CHL2RPRules::~CHL2RPRules()
{
	FOR_EACH_DICT_FAST(mProperties, i)
	{
		delete mProperties[i];
	}
}

void CHL2RPRules::RegisterDownloadableFiles(CFmtStrN<MAX_PATH>& path, FileFindHandle_t findHandle,
	INetworkStringTable* pDownloadables)
{
	int dirLen = path.Length();

	for (const char* pNextFileName = filesystem->FindFirstEx(path += "*", HL2RP_RULES_UPLOAD_PATH_ID, &findHandle);
		pNextFileName != NULL; pNextFileName = filesystem->FindNext(findHandle))
	{
		path.SetLength(dirLen);

		if (filesystem->FindIsDirectory(findHandle))
		{
			if (*pNextFileName != '.')
			{
				path.AppendFormat("%s%c", pNextFileName, CORRECT_PATH_SEPARATOR);
				RegisterDownloadableFiles(path, findHandle, pDownloadables);
			}
		}
		else if (!mExcludedUploadExts.HasElement(Q_GetFileExtension(pNextFileName))
			&& pDownloadables->AddString(true, path += pNextFileName) == INVALID_STRING_INDEX)
		{
			break; // Can't register more files
		}
	}

	filesystem->FindClose(findHandle);
}

void CHL2RPRules::LevelInitPostEntity()
{
	FOR_EACH_DICT_FAST(mMapGroups, i)
	{
		LoadExportableEntsFile(mMapGroups[i]);
	}

	KeyValuesAD activityFallbacksKV("");

	// Load activity fallbacks
	if (activityFallbacksKV->LoadFromFile(filesystem, "scripts/activity_fallbacks.txt"))
	{
		FOR_EACH_SUBKEY(activityFallbacksKV, pBaseActivityKV)
		{
			Activity baseActivity = (Activity)ActivityList_IndexForName(pBaseActivityKV->GetName());

			if (!mActivityFallbacksMap.IsValidIndex(mActivityFallbacksMap.Find(baseActivity)))
			{
				CActivityList* pFallbackActivities = new CActivityList;

				if (pBaseActivityKV->GetFirstSubKey() == NULL)
				{
					pFallbackActivities->AddActivity(pBaseActivityKV);
				}
				else
				{
					FOR_EACH_VALUE(pBaseActivityKV, pFallbackActivityKV)
					{
						pFallbackActivities->AddActivity(pFallbackActivityKV);
					}
				}

				if (pFallbackActivities->IsEmpty())
				{
					delete pFallbackActivities;
					continue;
				}

				mActivityFallbacksMap.Insert(baseActivity, pFallbackActivities);
			}
		}
	}
}

void CHL2RPRules::InitDefaultAIRelationships()
{
	CBaseCombatCharacter::AllocateDefaultRelationships();
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER, D_NU, DEF_RELATIONSHIP_PRIORITY);
}

const char* CHL2RPRules::GetIdealMapAlias()
{
	return (mMapGroups.Count() == 1 ? mMapGroups[0] : STRING(gpGlobals->mapname));
}

const char* CHL2RPRules::GetChatFormat(bool teamOnly, CBasePlayer* pPlayer)
{
	if (pPlayer != NULL)
	{
		return (teamOnly ? "#HL2RP_Chat_Region" : "#HL2RP_Chat_OOC");
	}

	return NULL;
}

// Called when request is team only
bool CHL2RPRules::PlayerCanHearChat(CBasePlayer* pListener, CBasePlayer* pSpeaker)
{
	return ToHL2Roleplayer(pListener)->IsWithinDistance(pSpeaker, gRegionMaxRadiusCVar.GetFloat(), false);
}

bool CHL2RPRules::CanPlayerHearVoice(CBasePlayer* pListener, CBasePlayer* pSpeaker, bool allTalk)
{
	CBitFlags<> miscFlags = ToHL2Roleplayer(pListener)->mMiscFlags | ToHL2Roleplayer(pSpeaker)->mMiscFlags;
	return ((!miscFlags.IsBitSet(EPlayerMiscFlag::AllowsRegionVoiceOnly)
		&& (allTalk || pListener->GetTeam() == pSpeaker->GetTeam())) || PlayerCanHearChat(pListener, pSpeaker));
}

Activity CHL2RPRules::GetBestTranslatedActivity(CBaseCombatCharacter* pCharacter,
	Activity baseActivity, bool weaponActStrict, int& sequence)
{
	Activity fallbackActivity = baseActivity,
		currentActivity = TranslateActivity(pCharacter, baseActivity, fallbackActivity, weaponActStrict, sequence);
	int index = mActivityFallbacksMap.Find(baseActivity);

	if (currentActivity > ACT_INVALID)
	{
		return currentActivity;
	}
	else if (mActivityFallbacksMap.IsValidIndex(index))
	{
		for (auto activity : *mActivityFallbacksMap[index])
		{
			if ((currentActivity = TranslateActivity(pCharacter, activity,
				fallbackActivity, weaponActStrict, sequence)) > ACT_INVALID)
			{
				return currentActivity;
			}
		}
	}

	return fallbackActivity;
}

// Returns an appropriate translated activity when available or ACT_INVALID otherwise.
// Input - fallbackActivity: First activity we had a valid sequence for in previous calls, but we discarded.
Activity CHL2RPRules::TranslateActivity(CBaseCombatCharacter* pCharacter,
	Activity baseActivity, Activity& fallbackActivity, bool weaponActStrict, int& sequence)
{
	Activity translatedActivity = pCharacter->Weapon_TranslateActivity(baseActivity);
	int auxSequence = pCharacter->SelectWeightedSequence(translatedActivity);

	if (auxSequence > 0)
	{
		if (translatedActivity != baseActivity || !weaponActStrict)
		{
			sequence = auxSequence;
			return translatedActivity;
		}
	}
	else if (translatedActivity != baseActivity
		&& (auxSequence = pCharacter->SelectWeightedSequence(baseActivity)) > 0 && !weaponActStrict)
	{
		sequence = auxSequence;
		return baseActivity;
	}

	if (auxSequence > 0 && sequence < 1)
	{
		sequence = auxSequence;
		fallbackActivity = baseActivity;
	}

	return ACT_INVALID;
}

void CHL2RPRules::Think()
{
	BaseClass::Think();

	if (gMaxHomeInactivityDays.GetInt() > 0)
	{
		long curTime;
		VCRHook_Time(&curTime);
		int maxInactivitySeconds = gMaxHomeInactivityDays.GetInt() * ONE_DAY_IN_SECONDS;

		FOR_EACH_DICT_FAST(mProperties, i)
		{
			if (mProperties[i]->HasOwner() && curTime >= mProperties[i]->mOwnerLastSeenTime + maxInactivitySeconds
				&& UTIL_PlayerBySteamID(mProperties[i]->mOwnerSteamIdNumber) == NULL)
			{
				mProperties[i]->Disown(NULL, 100);
			}
		}
	}

	if (mPoliceWaveTimer.Expired())
	{
		mPoliceWaveTimer.Set(gPoliceWavePeriodCVar.GetInt());
		CUtlVector<CBaseEntity*> spawnPoints;
		CBaseEntity* pSpawnPoint = NULL;
		for (; (pSpawnPoint = gEntList.FindEntityByClassname(pSpawnPoint, "info_police_start")) != NULL;
			spawnPoints.AddToTail(pSpawnPoint));

		if (!spawnPoints.IsEmpty())
		{
			for (int i = mWavePolices.Count(); i < gPoliceWaveCountCVar.GetInt(); ++i)
			{
				pSpawnPoint = spawnPoints.Random();
				mWavePolices.Insert(CBaseEntity::Create("npc_hl2rp_police",
					pSpawnPoint->GetAbsOrigin(), pSpawnPoint->GetAbsAngles()));
			}
		}
	}
}

void CHL2RPRules::FireGameEvent(IGameEvent* pEvent)
{
#ifdef HL2RP_FULL
	if (Q_strcmp(pEvent->GetName(), "player_disconnect") == 0)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByUserId(pEvent->GetInt("userid"));

		if (pPlayer != NULL && pPlayer->entindex() < 2)
		{
			ToHL2Roleplayer(pPlayer)->OnDisconnect();
		}
	}
#endif // HL2RP_FULL
}

void CHL2RPRules::ClientDisconnected(edict_t* pClient)
{
	CHL2Roleplayer* pPlayer = static_cast<CHL2Roleplayer*>(CBaseEntity::Instance(pClient));

	if (pPlayer != NULL)
	{
		pPlayer->OnDisconnect();
	}

	BaseClass::ClientDisconnected(pClient);
}

void CHL2RPRules::ClientCommandKeyValues(edict_t* pClient, KeyValues* pKeyValues)
{
#ifdef HL2RP_FULL
	if (Q_strcmp(pKeyValues->GetName(), HL2RP_LEARNED_HUD_HINTS_UPDATE_EVENT) == 0)
	{
		CHL2Roleplayer* pPlayer = ToHL2Roleplayer(CBaseEntity::Instance(pClient));

		if (pPlayer != NULL)
		{
			pPlayer->mLearnedHUDHints = pKeyValues->GetInt(HL2RP_LEARNED_HUD_HINTS_FIELD_NAME);
		}

		return;
	}
#endif // HL2RP_FULL

	BaseClass::ClientCommandKeyValues(pClient, pKeyValues);
}

void CHL2RPRules::AddPlayerName(uint64 steamIdNumber, const char* pName)
{
	SendPlayerName(mPlayerNameBySteamIdNum.InsertOrReplace(steamIdNumber, pName));
}

#ifdef HL2RP_FULL
void CHL2RPRules::SendPlayerName(int index, CRecipientFilter&& filter)
{
	filter.MakeReliable();
	UserMessageBegin(filter, HL2RP_PLAYER_NAME_SAVE_MESSAGE);
	WRITE_VARINT64(mPlayerNameBySteamIdNum.Key(index));
	WRITE_STRING(mPlayerNameBySteamIdNum[index]);
	MessageEnd();
}
#endif // HL2RP_FULL

ConVar gCopVIPSkinsAllowCVar("sv_allow_cop_vip_skins", "0", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Allow usage of VIP Citizen models playing as Combine."
	" By default, this is off to prevent other players from confusing wearer's faction.");
