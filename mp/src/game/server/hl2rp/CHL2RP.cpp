#include "cbase.h"
#include "CHL2RP.h"
#include "CHL2RP_Colors.h"
#include "CHL2RP_Player.h"
#include "CHL2RP_PlayerDialog.h"

#ifdef HL2DM_RP
#include "filesystem.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CHL2RP_SQL *CHL2RP::s_pSQL = NULL;
KeyValues *CHL2RP::s_pPhrases = NULL;
CUtlVector<CHL2RP_Zone> CHL2RP::s_Zones;

#ifdef HL2DM_RP
const float ESTIMATED_NAME_UPDATE_TRANSMIT_SECONDS = 0.5f;

CUtlVector<IGameEvent *> CHL2RP::s_SpecialDeathNoticeEvents;
CBasePlayerHandle CHL2RP::s_hDeathNoticeBotHandle;
CUtlDict<CUtlString> CHL2RP::s_CompatAnimationModelPaths[NUM_COMPAT_ANIM_MODEL_TYPES];
#endif

// Purpose:	Find an entity's offset marked as changed by CBaseEdict, belonging to a given network prop
// Input:	pNetworkProp - Network prop's absolute address
//			i - Index which to start searching from
//			pValueIndexOut - Local index of the offset's value, only if iNetworkPropValueCount is greater than 1
// Return:	Index of the offset if found (0 <= index < MAX_CHANGE_OFFSETS), or MAX_CHANGE_OFFSETS if no one was found
unsigned short CHL2RP::FindChangedEntityOffset(const CBaseEntity &entity, void *pNetworkProp, int iNetworkPropValueSize,
	int iNetworkPropValueCount, unsigned short i, int *piValueIndexOut)
{
	Assert(entity.edict() != NULL);
	Assert(0 < iNetworkPropValueSize);
	Assert(0 <= i);

	CEdictChangeInfo &changeInfo = g_pSharedChangeInfo->m_ChangeInfos[entity.edict()->GetChangeInfo()];
	unsigned short usStartOffset = ((char*)pNetworkProp - (char*)&entity);

	for (; i < changeInfo.m_nChangeOffsets; i++)
	{
		if (iNetworkPropValueCount <= 1)
		{
			// Network prop contains a single value. Fail if searched offset is different than first
			if (changeInfo.m_ChangeOffsets[i] != usStartOffset)
			{
				continue;
			}
		}
		else
		{
			// Network prop has an array of values. Fail if searched offset is outside array address range
			if (changeInfo.m_ChangeOffsets[i] < usStartOffset)
			{
				continue;
			}

			int iLocalValueIndex = ((changeInfo.m_ChangeOffsets[i] - usStartOffset) / iNetworkPropValueSize);

			if (iNetworkPropValueCount <= iLocalValueIndex)
			{
				continue;
			}
			else if (piValueIndexOut != NULL)
			{
				// Caller wants the index of the value in the array
				*piValueIndexOut = iLocalValueIndex;
			}
		}

		// Search successful
		return i;
	}

	return MAX_CHANGE_OFFSETS;
}

// Actions done here should better not include map/entity stuff,
// do that on ServerActivate -where map is loaded- to feel safer
void CHL2RP::PostInit()
{
	CHL2RP_Phrases::Init();
	CHL2RP_SQL::Init();
}

void CHL2RP::ServerActivate()
{
	CHL2RP_SQL::ServerActivate();
}

void CHL2RP::Think()
{
#ifdef HL2DM_RP
	if (s_hDeathNoticeBotHandle.IsValid())
	{
		if (s_SpecialDeathNoticeEvents.Count())
		{
			CBasePlayer *deathNoticeBot = s_hDeathNoticeBotHandle;

			if (deathNoticeBot != NULL)
			{
				IGameEvent *event = s_SpecialDeathNoticeEvents[0];
				static float fireTime = FLT_MAX;

				if (fireTime == FLT_MAX)
				{
					const char *characterName = event->GetString("npc_name"),
						*botName = engine->GetClientConVarValue(deathNoticeBot->entindex(), "name");

					// If bot name already equals AI's character, mark event to fire on this frame
					if (!Q_strcmp(botName, characterName))
					{
						fireTime = gpGlobals->curtime;
					}
					else
					{
						engine->SetFakeClientConVarValue(deathNoticeBot->edict(), "name", characterName);
						fireTime = gpGlobals->curtime + ESTIMATED_NAME_UPDATE_TRANSMIT_SECONDS;
					}
				}

				if (fireTime <= gpGlobals->curtime)
				{
					gameeventmanager->FireEvent(event);
					s_SpecialDeathNoticeEvents.Remove(0);
					fireTime = FLT_MAX;
				}
			}
			else
			{
				// Pointed player got removed, free the list
				s_SpecialDeathNoticeEvents.PurgeAndDeleteElements();
			}
		}
	}
	else
	{
		// Haven't found any server spawn function to create bot without it being removed upon hibernation
		// Create the bot only if saved handle has not been configured.
		// Don't re-create it if it was removed.
		CBasePlayer *deathNoticeBot = static_cast<CBasePlayer *>(CBaseEntity::Instance(engine->CreateFakeClient("Server Bot")));
		s_hDeathNoticeBotHandle = deathNoticeBot->GetRefEHandle();
		deathNoticeBot->ChangeTeam(TEAM_UNASSIGNED);
	}
#endif

	CHL2RP_SQL::Think();
}

#ifdef HL2DM_RP
int CHL2RP::GetCompatModelTypeIndex(const char *model)
{
	char fixedPath[MAX_VARCHAR_KEY_LENGTH];
	V_FixupPathName(fixedPath, sizeof fixedPath, model);

	for (int i = 0; i < ARRAYSIZE(s_CompatAnimationModelPaths); i++)
	{
		for (int j = s_CompatAnimationModelPaths[i].First(); s_CompatAnimationModelPaths[i].IsValidIndex(j); j = s_CompatAnimationModelPaths[i].Next(j))
		{
			if (!Q_stricmp(fixedPath, s_CompatAnimationModelPaths[i][j]))
			{
				return i;
			}
		}
	}

	return INVALID_COMPAT_ANIM_INDEX;
}

CCompatAnimModelTxn::CCompatAnimModelTxn(const char szCompatAnimModelID[])
{
	Q_strcpy(this->m_szCompatAnimModelID, szCompatAnimModelID);
}

// Purpose: Replace derived transactions between themselves. It assumes they are safe candidates!
bool CCompatAnimModelTxn::ShouldBeReplacedBy(const BaseClass &txn) const
{
	ThisClass *pThisClassOtherTxn = txn.ToTxnClass(this);
	return ((pThisClassOtherTxn != NULL) && !Q_strcmp(pThisClassOtherTxn->m_szCompatAnimModelID, m_szCompatAnimModelID));
}

CSetUpCompatAnimModelsTxn::CSetUpCompatAnimModelsTxn()
{
	AddFormattedQuery("CREATE TABLE IF NOT EXISTS HL2MPCompatHL2SPAnimationModel (id VARCHAR(%i) NOT NULL, path VARCHAR(%i) NOT NULL, type INTEGER, "
		"PRIMARY KEY (id), UNIQUE (path), CHECK (type >= 0 AND type < %i));", MENU_ITEM_LENGTH, MAX_VARCHAR_KEY_LENGTH, NUM_COMPAT_ANIM_MODEL_TYPES);
}

bool CSetUpCompatAnimModelsTxn::ShouldUsePreparedStatements() const
{
	return false;
}

CLoadCompatAnimModelsTxn::CLoadCompatAnimModelsTxn() : CAsyncSQLTxn("SELECT * FROM HL2MPCompatHL2SPAnimationModel;")
{

}

bool CLoadCompatAnimModelsTxn::ShouldUsePreparedStatements() const
{
	return false;
}

void CLoadCompatAnimModelsTxn::HandleQueryResults() const
{
	while (CHL2RP::s_pSQL->FetchNextRow())
	{
		char id[MENU_ITEM_LENGTH], path[MAX_VARCHAR_KEY_LENGTH], fixedPath[MAX_VARCHAR_KEY_LENGTH];

		CHL2RP::s_pSQL->FetchString("id", id, sizeof id);
		CHL2RP::s_pSQL->FetchString("path", path, sizeof path);
		V_FixupPathName(fixedPath, sizeof fixedPath, path);

		if (filesystem->FileExists(path))
		{
			CBaseEntity::PrecacheModel(path);
			int type = CHL2RP::s_pSQL->FetchInt("type");

			if (type <= 0 && type < ARRAYSIZE(CHL2RP::s_CompatAnimationModelPaths))
			{
				CHL2RP::s_CompatAnimationModelPaths[type].Insert(id, CUtlString(fixedPath));
			}
		}
	}
}

CUpsertCompatAnimModelTxn::CUpsertCompatAnimModelTxn(const char *compatAnimModelID, const char *compatAnimModelPath, int compatAnimModelType) :
compatAnimModelType(compatAnimModelType), CCompatAnimModelTxn(compatAnimModelID)
{
	// Note: Even though no internal changes happen to a single model ID on this server instance,
	// add queries to remove a colliding record that may have been inserted externally
	AddQuery("DELETE FROM HL2MPCompatHL2SPAnimationModel WHERE path = ?;");
	Q_strcpy(this->compatAnimModelPath, compatAnimModelPath);
}

void CUpsertCompatAnimModelTxn::BuildSQLiteQueries()
{
	AddQuery("UPDATE HL2MPCompatHL2SPAnimationModel SET path = ?, type = ? WHERE id = ?;");
	AddQuery("INSERT OR IGNORE INTO HL2MPCompatHL2SPAnimationModel (id, path, type) VALUES (?, ?, ?);");
}

void CUpsertCompatAnimModelTxn::BuildMySQLQueries()
{
	AddQuery("INSERT INTO HL2MPCompatHL2SPAnimationModel (id, path, type) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE path = ?, type = ?;");
}

void CUpsertCompatAnimModelTxn::BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const
{
	if (iQueryId > 0)
	{
		int dataStartParamIndex;

		if (iQueryId == 1)
		{
			dataStartParamIndex = 1;
			CHL2RP::s_pSQL->BindString(pStmt, m_szCompatAnimModelID, 3);
		}
		else
		{
			dataStartParamIndex = 2;
			CHL2RP::s_pSQL->BindString(pStmt, m_szCompatAnimModelID, 1);
		}

		CHL2RP::s_pSQL->BindString(pStmt, compatAnimModelPath, dataStartParamIndex);
		CHL2RP::s_pSQL->BindInt(pStmt, compatAnimModelType, ++dataStartParamIndex);
	}
	else
	{
		CHL2RP::s_pSQL->BindString(pStmt, compatAnimModelPath, 1);
	}
}

void CUpsertCompatAnimModelTxn::BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const
{
	if (iQueryId > 0)
	{
		CHL2RP::s_pSQL->BindString(pStmt, m_szCompatAnimModelID, 1);
		CHL2RP::s_pSQL->BindString(pStmt, compatAnimModelPath, 2, 4);
		CHL2RP::s_pSQL->BindInt(pStmt, compatAnimModelType, 3, 5);
	}
	else
	{
		CHL2RP::s_pSQL->BindString(pStmt, compatAnimModelPath, 1);
	}
}

CDeleteCompatAnimModelTxn::CDeleteCompatAnimModelTxn(const char *compatAnimModelID) : CCompatAnimModelTxn(compatAnimModelID)
{
	AddQuery("DELETE FROM HL2MPCompatHL2SPAnimationModel WHERE id = ?;");
}

void CDeleteCompatAnimModelTxn::BindParametersGeneric(int iQueryId, const void *pStmt) const
{
	CHL2RP::s_pSQL->BindString(pStmt, m_szCompatAnimModelID, 1);
}
#endif

CUserInputTxn::CUserInputTxn(const char *query) : CAsyncSQLTxn(query)
{

}

bool CUserInputTxn::ShouldUsePreparedStatements() const
{
	return false;
}

CON_COMMAND(fake_client, "Creates fake client")
{
	if (UTIL_IsCommandIssuedByServerAdmin())
		engine->CreateFakeClient("Fake Player");
}

CON_COMMAND(restart_map, "Restarts current map")
{
	if (UTIL_IsCommandIssuedByServerAdmin())
		engine->ChangeLevel(UTIL_VarArgs("%s\n", STRING(gpGlobals->mapname)), NULL);
}

CON_COMMAND(set_admin, "<userid> - Sets a player admin")
{
	if (UTIL_IsCommandIssuedByServerAdmin())
	{
		CHL2RP_Player *pPlayer = CHL2RP_Player::ToThisClassFast(UTIL_PlayerByUserId(atoi(args.Arg(1))));

		if (pPlayer != NULL && !pPlayer->IsAdmin())
		{
			pPlayer->m_AccessFlag = CHL2RP_Player::AccessFlag::ACCESS_FLAG_ADMIN;
			pPlayer->TrySyncMainData();

			char msg[MAX_VALVE_USER_MSG_TEXT_DATA];
			Q_snprintf(msg, sizeof(msg), DEEP_SKY_BLUE_CHAT_COLOR "You are now an admin");
			ClientPrint(pPlayer, HUD_PRINTTALK, msg);
		}
	}
}

static void CommandZoneHeightUp(const CCommand &args)
{
	CHL2RP_Player *pPlayer = CHL2RP_Player::ToThisClassFast(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		CHL2RP_PlayerZoneMenu *zoneMenu = dynamic_cast<CHL2RP_PlayerZoneMenu *>(pPlayer->m_pCurrentDialog);

		if (zoneMenu != NULL)
		{
			zoneMenu->m_HeightEditState = CHL2RP_PlayerZoneMenu::HeightEditState::RAISING;
		}
	}
}

static ConCommand zone_height_up("+zone_height_up", CommandZoneHeightUp, "Raise current zone height");

static void CommandZoneHeightDown(const CCommand &args)
{
	CHL2RP_Player *pPlayer = CHL2RP_Player::ToThisClassFast(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		CHL2RP_PlayerZoneMenu *zoneMenu = dynamic_cast<CHL2RP_PlayerZoneMenu *>(pPlayer->m_pCurrentDialog);

		if (zoneMenu != NULL)
		{
			zoneMenu->m_HeightEditState = CHL2RP_PlayerZoneMenu::HeightEditState::LOWERING;
		}
	}
}

static ConCommand zone_height_down("+zone_height_down", CommandZoneHeightDown, "Lowers current zone height");

static void CommandStopZoneHeightChange(const CCommand &args)
{
	CHL2RP_Player *pPlayer = CHL2RP_Player::ToThisClassFast(UTIL_GetCommandClient());

	if (pPlayer != NULL)
	{
		CHL2RP_PlayerZoneMenu *zoneMenu = dynamic_cast<CHL2RP_PlayerZoneMenu *>(pPlayer->m_pCurrentDialog);

		if (zoneMenu != NULL)
		{
			zoneMenu->m_HeightEditState = CHL2RP_PlayerZoneMenu::HeightEditState::STOPPED;
		}
	}
}

static ConCommand zone_height_up_stop("-zone_height_up", CommandStopZoneHeightChange, "Stops raising current zone height");
static ConCommand zone_height_down_stop("-zone_height_down", CommandStopZoneHeightChange, "Stops lowering current zone height");

CON_COMMAND(sql_query, "Sends a query to the current database")
{
	if (CHL2RP::s_pSQL != NULL)
	{
		CHL2RP::s_pSQL->AddAsyncTxn(new CUserInputTxn(args.Arg(1)));
	}
}
