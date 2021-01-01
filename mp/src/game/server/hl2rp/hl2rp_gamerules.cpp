// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gamerules.h"
#include "hl2_roleplayer.h"
#include <fmtstr.h>
#include <networkstringtable_gamedll.h>
#include <tier2/fileutils.h>

#define DOWNLOADABLE_FILE_TABLENAME "downloadables"

#define HL2RP_RULES_LEGACY_DOWNLOADS_LIST_FILE "cfg/base_downloads.txt"
#define HL2RP_RULES_USER_DOWNLOADS_PATH_ID     "upload"

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

CHL2RPRules::CHL2RPRules()
{
	if (!engine->IsDedicatedServer())
	{
		// Enable player disconnect detection, since ClientDisconnected() isn't called on listenserver
		ListenForGameEvent("player_disconnect");
	}
}

void CHL2RPRules::LevelInitPostEntity()
{
	// Register custom user assets for download
	uint now = Plat_MSTime();
	INetworkStringTable* pDownloadables = networkstringtable->FindTable(DOWNLOADABLE_FILE_TABLENAME);
	int oldDownloadablesCount = pDownloadables->GetNumStrings();
	FileFindHandle_t curFindHandle;
	CFmtStrN<MAX_PATH> path;
	CUtlVector<int> dirsLengths;

	for (const char* pFileName = filesystem->FindFirstEx("*", HL2RP_RULES_USER_DOWNLOADS_PATH_ID, &curFindHandle);;)
	{
		if (pFileName == NULL)
		{
			filesystem->FindClose(curFindHandle);

			if (--curFindHandle <= FILESYSTEM_INVALID_FIND_HANDLE)
			{
				break;
			}
			else if (dirsLengths.Count() > 0)
			{
				dirsLengths.RemoveMultipleFromTail(1);
				path.SetLength(dirsLengths.Count() > 0 ? dirsLengths.Tail() : 0);
			}
		}
		else if (filesystem->FindIsDirectory(curFindHandle))
		{
			if (pFileName[0] != '.')
			{
				path.AppendFormat("%s/", pFileName);
				int len = path.Length();
				dirsLengths.AddToTail(path.Length());
				path += "*";
				pFileName = filesystem->FindFirstEx(path, HL2RP_RULES_USER_DOWNLOADS_PATH_ID, &curFindHandle);
				path.SetLength(len);
				continue;
			}
		}
		else
		{
			int len = path.Length();
			path += pFileName;

			if (pDownloadables->AddString(true, path) == INVALID_STRING_INDEX)
			{
				break;
			}

			path.SetLength(len);
		}

		pFileName = filesystem->FindNext(curFindHandle);
	}

#ifdef HL2RP_LEGACY
	// Register default HL2RP assets for download (only needed in HL2DM)
	CInputTextFile downloadsList(HL2RP_RULES_LEGACY_DOWNLOADS_LIST_FILE);

	for (char entry[MAX_PATH]; downloadsList.ReadLine(entry, sizeof(entry)) != NULL;)
	{
		Q_StripPrecedingAndTrailingWhitespace(entry);

		if (pDownloadables->AddString(true, entry) == INVALID_STRING_INDEX)
		{
			break;
		}
	}
#endif // HL2RP_LEGACY

	DevMsg("HL2RPRules::LevelInitPostEntity: Took %s ms to register %s downloadable files\n",
		Q_pretifynum(Plat_MSTime() - now), Q_pretifynum(pDownloadables->GetNumStrings() - oldDownloadablesCount));
}

void CHL2RPRules::FireGameEvent(IGameEvent* pEvent)
{
	if (Q_strcmp(pEvent->GetName(), "player_disconnect") == 0)
	{
		PlayerDisconnected(UTIL_PlayerByUserId(pEvent->GetInt("userid")));
	}
}

bool CHL2RPRules::AllowDamage(CBaseEntity*, const CTakeDamageInfo& info)
{
	CHL2Roleplayer* pPlayerAttacker = ToHL2Roleplayer(info.GetAttacker());

	if (pPlayerAttacker != NULL && !pPlayerAttacker->mDatabaseIOFlags.IsBitSet(EPlayerDatabaseIOFlag::IsLoaded))
	{
		return false;
	}

	return true;
}

void CHL2RPRules::ClientDisconnected(edict_t* pClient)
{
	PlayerDisconnected(ToBasePlayer(CBaseEntity::Instance(pClient)));
	BaseClass::ClientDisconnected(pClient);
}

void CHL2RPRules::PlayerDisconnected(CBasePlayer* pPlayer)
{
	CHL2Roleplayer* pRoleplayer = ToHL2Roleplayer(pPlayer);

	if (pPlayer != NULL)
	{
		pRoleplayer->mDatabaseIOFlags.ClearBit(EPlayerDatabaseIOFlag::IsLoaded);
		pRoleplayer->ClearActiveWeapon();

		// Clear the player data that would get duplicated when retrying to localhost (player instance is reused)
		pRoleplayer->mSeconds = 0;
		pRoleplayer->mCrime = 0;
	}
}

void CHL2RPRules::ClientCommandKeyValues(edict_t* pClient, KeyValues* pKeyValues)
{
#ifdef HL2RP_FULL
	if (Q_strcmp(pKeyValues->GetName(), HL2RP_SENT_HUD_HINTS_UPDATE_EVENT_NAME) == 0)
	{
		CHL2Roleplayer* pPlayer = ToHL2Roleplayer(CBaseEntity::Instance(pClient));

		if (pPlayer != NULL)
		{
			pPlayer->mSentHUDHints = pKeyValues->GetInt(HL2RP_SENT_HUD_HINTS_UPDATE_EVENT_DATA_KEY);
		}
	}
#endif // HL2RP_FULL
}

CHL2RPRules* HL2RPRules()
{
	return static_cast<CHL2RPRules*>(GameRules());
}
