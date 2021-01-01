// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gameinterface.h"
#include <hl2mp_gameinterface.h>
#include "hl2_roleplayer.h"
#include <filesystem.h>
#include <language.h>

extern IPhysicsCollision* physcollision;

// The MOTD guide file, used only to allow registering localized variants via LoadSpecificMOTDMsg.
// It is global to prevent crashing when leaving the scope if declared locally.
static ConVar sLangMOTDGuideFileCVar("motdguidefile", NULL, FCVAR_UNREGISTERED | FCVAR_HIDDEN);

const char* GetGameDescription()
{
	return HL2RP_TITLE;
}

void CServerGameDLL::LevelInit_ParseAllEntities(const char* pMapEntities)
{
	FOR_EACH_LANGUAGE(language)
	{
		const char* pLangShortName = GetLanguageShortName((ELanguage)language);
		const char* pLangMOTDGuideFileName = UTIL_VarArgs("%s_%s.html", HL2RP_MOTD_GUIDE_BASE_NAME, pLangShortName);
		UTIL_VarArgs("%s_%s.html", HL2RP_MOTD_GUIDE_BASE_NAME, pLangShortName);
		sLangMOTDGuideFileCVar.SetValue(pLangMOTDGuideFileName);
		pLangMOTDGuideFileName = UTIL_VarArgs("%s_%s", HL2RP_MOTD_GUIDE_BASE_NAME, pLangShortName);
		LoadSpecificMOTDMsg(sLangMOTDGuideFileCVar, pLangMOTDGuideFileName);
	}

	// Load exported HL2RP entities separate from BSP into the current map
	char path[MAX_PATH];
	V_sprintf_safe(path, "maps/%s_entities.txt", STRING(gpGlobals->mapname));
	KeyValuesAD entities("");

	if (entities->LoadFromFile(filesystem, path))
	{
		FOR_EACH_TRUE_SUBKEY(entities, pEntityKey)
		{
			CBaseEntity* pEntity = CreateEntityByName(pEntityKey->GetString("classname"));

			if (pEntity != NULL)
			{
				FOR_EACH_VALUE(pEntityKey, pValue)
				{
					pEntity->KeyValue(pValue->GetName(), pValue->GetString());
				}

				DispatchSpawn(pEntity);
				KeyValues* pSolids = pEntityKey->FindKey("Solids");

				if (pSolids != NULL)
				{
					CUtlVector<CPhysConvex*> convexes;

					FOR_EACH_TRUE_SUBKEY(pSolids, pSolid)
					{
						CUtlVector<VPlane> planes;

						FOR_EACH_VALUE(pSolid, pPlane)
						{
							int index = planes.AddToTail();
							UTIL_StringToFloatArray(planes[index].m_Normal.Base(), 4, pPlane->GetString());
						}

						convexes.AddToTail(physcollision
							->ConvexFromPlanes(planes.Base()->m_Normal.Base(), planes.Count(), 0.0f));
					}

					pEntity->m_iHammerID = 0; // Prevent any Hammer ID in the file from leaking in
					CPhysCollide* pCollide = physcollision->ConvertConvexToCollide(convexes.Base(), convexes.Count());

					if (pCollide != NULL)
					{
						objectparams_t params{};
						IPhysicsObject* pObject = physenv->CreatePolyObjectStatic(pCollide,
							-1, pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), &params);

						if (pObject != NULL)
						{
							pEntity->VPhysicsSetObject(pObject);
						}
					}
				}
			}
		}
	}
}

void CServerGameClients::NetworkIDValidated(const char*, const char* pNetworkIdString)
{
	// Copy the Network ID since engine's GetNetworkIDString() reuses parameter one
	char searchNetworkId[MAX_NETWORKID_LENGTH];
	Q_strcpy(searchNetworkId, pNetworkIdString);

	CUtlVector<CBasePlayer*> players;
	CollectPlayers(&players, TEAM_ANY, false, APPEND_PLAYERS);

	for (auto pPlayer : players)
	{
		const char* pAuxNetworkIdString = pPlayer->GetNetworkIDString();

		if (Q_strcmp(pAuxNetworkIdString, searchNetworkId) == 0)
		{
			ToHL2Roleplayer(pPlayer)->LoadFromDatabase();
			break;
		}
	}
}
