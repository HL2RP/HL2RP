// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gamerules.h"
#include "hl2rp_gameinterface.h"
#include "hl2_roleplayer.h"
#include <hl2rp_localizer.h>
#include <hl2rp_property.h>
#include <activitylist.h>
#include <gameinterface.h>
#include <networkstringtable_gamedll.h>
#include <sourcehook.h>
#include <tier2/fileutils.h>
#include <usermessages.h>

#define DOWNLOADABLE_FILE_TABLENAME "downloadables"

#define HL2RP_RULES_UPLOAD_PATH_ID   "upload"
#define HL2RP_RULES_UPLOAD_LIST_FILE (HL2RP_CONFIG_PATH "upload.txt")

#define HL2RP_RULES_DAYNIGHT_MAPCHANGE_THINK_CONTEXT "DayNightMapChangeThink"

extern CServerGameDLL g_ServerGameDLL;
extern SourceHook::ISourceHook* g_SHPtr;
extern ConVar mp_chattime, nextlevel, gMaxHomeInactivityDaysCVar, gRegionMaxRadiusCVar;

ConVar gDayNightChangeHourCVar("sv_daynight_change_hour", "7", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Hour at which to switch between maps from day/night cycle", true, 0.0f, true, 12.0f);

static ConVar sDayNightMapCycleEnableCVar("sv_enable_daynight_mapcycle", "1",
	FCVAR_ARCHIVE | FCVAR_NOTIFY, "Whether to switch maps from day/night cycle automatically"),
	sPoliceWaveCountCVar("sv_police_wave_count", "5", FCVAR_ARCHIVE | FCVAR_NOTIFY,
		"Amount of police NPCs to spawn or keep in each wave"),
	sPoliceWavePeriodCVar("sv_police_wave_period", "60", FCVAR_ARCHIVE | FCVAR_NOTIFY,
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

bool SMoneyPropData::CLess::Less(SMoneyPropData* pLeft, SMoneyPropData* pRight, void*)
{
	return (pLeft->mAmount < pRight->mAmount);
}

CHL2RPRules::CSeasonData::CSeasonData()
{
	for (int i = 0; i < EMapCycleTime::_Count; ++i)
	{
		mEligibleMaps[i].SetLessFunc(CaselessStringLessThan);
		mNonEligibleMaps[i].SetLessFunc(CaselessStringLessThan);
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
	KeyValuesAD mapGroupsKV(""), seasonsKV(""), dayNightMapCycleKV(""), currencyKV(""), modelsKV(""), jobsKV("");
	CAutoLessFuncAdapter<CUtlRBTree<const char*>> seasonNames; // Correlative with loaded seasons

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

	// Load seasons
	if (seasonsKV->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "seasons.txt"))
	{
		FOR_EACH_TRUE_SUBKEY(seasonsKV, pSeasonKV)
		{
			if (!seasonNames.HasElement(pSeasonKV->GetName()))
			{
				auto pDateRange = mSeasons.AddToTailGetPtr()->mDateRange;
				const char* partNames[] = { "start", "end" };
				int i = 0;

				for (; i < ARRAYSIZE(partNames); ++i)
				{
					sscanf(pSeasonKV->GetString(partNames[i]), "%i-%i",
						&pDateRange[i][ESeasonDatePart::Month], &pDateRange[i][ESeasonDatePart::Day]);
					pDateRange[i][ESeasonDatePart::Month] -= 1; // Accommodate to tm's 0-index based month range

					// Validate time parts are non-negative at least, to produce clean DevMsgs (eg. no double minus sign)
					if (Min(pDateRange[i][ESeasonDatePart::Month], pDateRange[i][ESeasonDatePart::Day]) < 0)
					{
						Msg("HL2RPRules: Found invalid %s date format '%s' (MM-DD) for season '%s'. Discarded season.\n",
							partNames[i], pSeasonKV->GetString(partNames[i]), pSeasonKV->GetName());
						break;
					}
				}

				if (i < ARRAYSIZE(partNames))
				{
					mSeasons.RemoveMultipleFromTail(1);
					continue;
				}

				seasonNames.Insert(pSeasonKV->GetName());
				DevMsg("HL2RPRules: Loaded season '%s', scheduled between %02i-%02i and %02i-%02i (MM-DD)\n",
					pSeasonKV->GetName(), pDateRange[ESeasonRangePart::Start][ESeasonDatePart::Month] + 1,
					pDateRange[ESeasonRangePart::Start][ESeasonDatePart::Day],
					pDateRange[ESeasonRangePart::End][ESeasonDatePart::Month] + 1,
					pDateRange[ESeasonRangePart::End][ESeasonDatePart::Day]);
			}
		}
	}

	// Load day/night mapcycle
	if (dayNightMapCycleKV->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "daynight_mapcycle.txt"))
	{
		FOR_EACH_TRUE_SUBKEY(dayNightMapCycleKV, pMapGroupKV)
		{
			if (mMapGroups.HasElement(pMapGroupKV->GetName()))
			{
				FOR_EACH_TRUE_SUBKEY(pMapGroupKV, pSeasonKV)
				{
					int index = seasonNames.Find(pSeasonKV->GetName());

					if (!seasonNames.IsValidIndex(index))
					{
						// Continue except for "Default" season, which is allowed to be used as a fallback
						if (Q_stricmp(pSeasonKV->GetName(), "Default") != 0)
						{
							continue;
						}

						index = mSeasons.AddToTail();
						mSeasons[index].mIsTimeLess = true;
					}

					FOR_EACH_SUBKEY(pSeasonKV, pMapKV)
					{
						bool eligible = true;
						const char* pTime = pMapKV->GetString();
						EMapCycleTime cycleTime = EMapCycleTime::Day;

						if (pMapKV->GetFirstSubKey() != NULL)
						{
							pTime = pMapKV->GetString("time");
							eligible = pMapKV->GetBool("eligible", true);
						}

						if (Q_stricmp(pTime, "night") == 0)
						{
							cycleTime = EMapCycleTime::Night;
						}
						else if (Q_stricmp(pTime, "day") != 0)
						{
							continue;
						}

						auto pMaps = eligible ? mSeasons[index].mEligibleMaps : mSeasons[index].mNonEligibleMaps;

						if (pMaps[cycleTime].InsertIfNotFound(pMapKV->GetName()) != pMaps[cycleTime].InvalidIndex())
						{
							DevMsg("HL2RPRules: Loaded time '%s' for map '%s' under '%s' season (eligible = %i)\n",
								pTime, pMapKV->GetName(), pSeasonKV->GetName(), eligible);
						}
					}
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
						CPlainAutoPtr<CPlayerModelDict> subModels(new CPlayerModelDict);

						if (pGroupKV->GetFirstSubKey() == NULL)
						{
							subModels->AddModel(pGroupKV, pModelPrecache);
						}
						else
						{
							FOR_EACH_VALUE(pGroupKV, pModelKV)
							{
								subModels->AddModel(pModelKV, pModelPrecache);
							}
						}

						if (subModels->Count() > 0)
						{
							subModels->mType = (EPlayerModelType)i;
							int index = mPlayerModelsByGroup.Insert(pGroupKV->GetName(), subModels.Detach());
							mPlayerModelsByGroup.mGroupIndices[i].AddToTail(index);
						}
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

	// Load currency data (money props setup)
	if (currencyKV->LoadFromFile(filesystem, HL2RP_CONFIG_PATH "currency.txt"))
	{
		FOR_EACH_TRUE_SUBKEY(currencyKV, amountKV)
		{
			int amount = Q_atoi(amountKV->GetName());

			if (amount > 0) // Early prevent negative money affecting economy
			{
				// TODO: Considerar sustituir el CUtlSortVector por un CUtlMap<int, SMoneyPropData*>, lo cual permitiria
				// prescindir de hacer un "new SMoneyPropData" aqui al no necesitarse para insertar ordenadamente.
				// Ademas, permitiria eliminar el mAmount y el constructor, resultando mas coherente la nomenclatura de
				// la clase, y eliminando el CLess.
				CPlainAutoPtr<SMoneyPropData> data(new SMoneyPropData(amount));
				int index = mMoneyPropsData.InsertIfNotFound(data.Detach());

				if (mMoneyPropsData.IsValidIndex(index))
				{
					FOR_EACH_VALUE(amountKV, dataKV)
					{
						mMoneyPropsData[index]->mFieldByKey
							.InsertOrReplace(dataKV->GetName(), SUtlField::FromKeyValues(dataKV));
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
				CPlainAutoPtr<CActivityList> fallbackActivities(new CActivityList);

				if (pBaseActivityKV->GetFirstSubKey() == NULL)
				{
					fallbackActivities->AddActivity(pBaseActivityKV);
				}
				else
				{
					FOR_EACH_VALUE(pBaseActivityKV, pFallbackActivityKV)
					{
						fallbackActivities->AddActivity(pFallbackActivityKV);
					}
				}

				if (!fallbackActivities->IsEmpty())
				{
					mActivityFallbacksMap.Insert(baseActivity, fallbackActivities.Detach());
				}
			}
		}
	}
}

void CHL2RPRules::InitDefaultAIRelationships()
{
	CBaseCombatCharacter::AllocateDefaultRelationships();
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER, D_NU, DEF_RELATIONSHIP_PRIORITY);
}

bool CHL2RPRules::IsDayTime(const tm& targetTime)
{
	return (targetTime.tm_hour >= gDayNightChangeHourCVar.GetInt()
		&& targetTime.tm_hour < gDayNightChangeHourCVar.GetInt() + 12);
}

const char* CHL2RPRules::GetIdealMapAlias()
{
	return (mMapGroups.Count() == 1 ? mMapGroups[0] : STRING(gpGlobals->mapname));
}

bool CHL2RPRules::IsSeasonApplicable(const CSeasonData& season, const tm& targetTime)
{
	auto pDateRange = season.mDateRange;
	CUtlPair<int, int> monthsRange(pDateRange[ESeasonRangePart::Start][ESeasonDatePart::Month],
		pDateRange[ESeasonRangePart::End][ESeasonDatePart::Month]);

	// First, check within months range, accounting for year breaks between
	if (monthsRange.second >= monthsRange.first ?
		(targetTime.tm_mon >= monthsRange.first && targetTime.tm_mon <= monthsRange.second)
		: (targetTime.tm_mon >= monthsRange.first || targetTime.tm_mon <= monthsRange.second))
	{
		// Now, check day only if month lays in any of the bounds
		if (targetTime.tm_mon == monthsRange.first)
		{
			return (targetTime.tm_mday >= pDateRange[ESeasonRangePart::Start][ESeasonDatePart::Day]);
		}

		return (targetTime.tm_mon != monthsRange.second
			|| targetTime.tm_mday <= pDateRange[ESeasonRangePart::End][ESeasonDatePart::Day]);
	}

	return false;
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

	if (!g_fGameOver)
	{
		if (sDayNightMapCycleEnableCVar.GetBool())
		{
			CHL2RPRulesProxy* pProxy = static_cast<CHL2RPRulesProxy*>
				(gEntList.FindEntityByClassname(NULL, "hl2rp_gamerules"));

			if (mpNextDayNightMap == NULL)
			{
				tm targetTime;
				UTIL_GetServerTime(targetTime, mp_chattime.GetInt());

				for (auto& season : mSeasons)
				{
					if (season.mIsTimeLess || IsSeasonApplicable(season, targetTime))
					{
						EMapCycleTime cycleTime = IsDayTime(targetTime) ? EMapCycleTime::Day : EMapCycleTime::Night;

						if (season.mEligibleMaps[cycleTime].HasElement(STRING(gpGlobals->mapname))
							|| season.mNonEligibleMaps[cycleTime].HasElement(STRING(gpGlobals->mapname)))
						{
							break; // Current map is already valid for this season; stop
						}
						else if (season.mEligibleMaps[cycleTime].Count() > 0)
						{
							char path[MAX_PATH];
							int index = RandomInt(0, season.mEligibleMaps[cycleTime].Count() - 1);
							V_sprintf_safe(path, "maps/%s.bsp", season.mEligibleMaps[cycleTime][index]);

							if (filesystem->FileExists(path))
							{
								if (pProxy != NULL)
								{
									pProxy->SetContextThink(&CHL2RPRulesProxy::DayNightMapChangeThink,
										gpGlobals->curtime + 1.0f, HL2RP_RULES_DAYNIGHT_MAPCHANGE_THINK_CONTEXT);
								}

								mpNextDayNightMap = season.mEligibleMaps[cycleTime][index];
								mDayNightMapChangeTime = gpGlobals->realtime + mp_chattime.GetFloat();
								SendDayNightMapTimeLeft(true);
								Msg("%s\n", CLocalizeFmtStr<>().Localize("#HL2RP_DayNight_MapChange",
									mpNextDayNightMap, mp_chattime.GetInt()));
								break;
							}
						}
					}
				}
			}
			else if (pProxy == NULL || g_ServerGameDLL.m_bIsHibernating)
			{
				EndDayNightMapChange();
			}
		}
		else
		{
			mpNextDayNightMap = NULL; // Fully end countdown, so we'll create a new one once cvar is re-enabled
		}

		if (mPoliceWaveTimer.Expired())
		{
			mPoliceWaveTimer.Set(sPoliceWavePeriodCVar.GetInt());
			CUtlVector<CBaseEntity*> spawnPoints;
			CBaseEntity* pSpawnPoint = NULL;
			for (; (pSpawnPoint = gEntList.FindEntityByClassname(pSpawnPoint, "info_police_start")) != NULL;
				spawnPoints.AddToTail(pSpawnPoint));

			if (!spawnPoints.IsEmpty())
			{
				for (int i = mWavePolices.Count(); i < sPoliceWaveCountCVar.GetInt(); ++i)
				{
					pSpawnPoint = spawnPoints.Random();
					mWavePolices.Insert(CBaseEntity::Create("npc_hl2rp_police",
						pSpawnPoint->GetAbsOrigin(), pSpawnPoint->GetAbsAngles()));
				}
			}
		}
	}

	if (gMaxHomeInactivityDaysCVar.GetInt() > 0)
	{
		long curTime;
		VCRHook_Time(&curTime);
		int maxInactivitySeconds = gMaxHomeInactivityDaysCVar.GetInt() * ONE_DAY_IN_SECONDS;

		FOR_EACH_DICT_FAST(mProperties, i)
		{
			if (mProperties[i]->HasOwner() && curTime >= mProperties[i]->mOwnerLastSeenTime + maxInactivitySeconds
				&& UTIL_PlayerBySteamID(mProperties[i]->mOwnerSteamIdNumber) == NULL)
			{
				mProperties[i]->Disown(NULL, 100);
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

void CHL2RPRules::SendDayNightMapTimeLeft(bool useChat)
{
	CNumStr timeLeft(Ceil2Int(mDayNightMapChangeTime - gpGlobals->realtime));

	ForEachRoleplayer([&](CHL2Roleplayer* pPlayer)
	{
		if (useChat)
		{
			pPlayer->Print(HUD_PRINTTALK, "#HL2RP_DayNight_MapChange", mpNextDayNightMap, timeLeft);
		}

		pPlayer->Print(HUD_PRINTCENTER, "#HL2RP_DayNight_MapChange", mpNextDayNightMap, timeLeft);
	});
}

bool CHL2RPRules::EndDayNightMapChange()
{
	if (gpGlobals->realtime >= mDayNightMapChangeTime)
	{
		char path[MAX_PATH];
		V_sprintf_safe(path, "maps/%s.bsp", mpNextDayNightMap);

		if (engine->IsMapValid(path))
		{
			// Adapt to CHL2MPRules way
			g_fGameOver = true;
			nextlevel.SetValue(mpNextDayNightMap);

			// Ensure SM's next map won't supersede ours during SM's ChangeLevel hook
			ConVarRef("sm_nextmap", true).SetValue(mpNextDayNightMap);
		}

		mpNextDayNightMap = NULL;
		return true;
	}

	return false;
}

void CHL2RPRulesProxy::DayNightMapChangeThink()
{
	if (HL2RPRules()->mpNextDayNightMap != NULL && !HL2RPRules()->EndDayNightMapChange())
	{
		HL2RPRules()->SendDayNightMapTimeLeft(false);
		SetNextThink(gpGlobals->curtime + 1.0f, HL2RP_RULES_DAYNIGHT_MAPCHANGE_THINK_CONTEXT);
	}
}

ConVar gCopVIPSkinsAllowCVar("sv_allow_cop_vip_skins", "0", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Allow usage of VIP Citizen models playing as Combine."
	" By default, this is off to prevent other players from confusing wearer's faction.");
