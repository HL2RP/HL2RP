// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gameinterface.h"
#include <hl2mp_gameinterface.h>
#include "hl2_roleplayer.h"
#include <filesystem.h>

const char* GetGameDescription()
{
	return HL2RP_TITLE;
}

void HL2RP_PostInit()
{
	// Add specific executable paths to allow loading SQL drivers (when required) if available there
	char path[MAX_PATH];

	// Attempt to add mod's parent bin path, which may be needed when running from local repository.
	// Search in MOD_WRITE as it'll likely contain a single path, and other locations are unneeded.
	if (filesystem->RelativePathToFullPath_safe(".."
		CORRECT_PATH_SEPARATOR_S PLATFORM_BIN_DIR, "MOD_WRITE", path, FILTER_CULLPACK) != NULL)
	{
		filesystem->AddSearchPath(path, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);
	}

	// Add GAMEBIN path to support drivers install location from thirdparty version
	if (filesystem->GetSearchPath_safe("GAMEBIN", false, path) > 0)
	{
		filesystem->AddSearchPath(path, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);
	}
}

void CServerGameDLL::LevelInit_ParseAllEntities(const char*)
{
	LoadExportableEntsFile(STRING(gpGlobals->mapname));
}

void LoadExportableEntsFile(const char* pFileName)
{
	char path[MAX_PATH];
	V_sprintf_safe(path, "maps/entities/%s.txt", pFileName);
	KeyValuesAD entities("");

	if (entities->LoadFromFile(filesystem, path))
	{
		FOR_EACH_TRUE_SUBKEY(entities, pKey)
		{
			CBaseEntity* pEntity = CreateEntityByName(pKey->GetString("classname"));

			if (pEntity != NULL)
			{
				FOR_EACH_VALUE(pKey, pValue)
				{
					pEntity->KeyValue(pValue->GetName(), pValue->GetString());
				}

				pEntity->KeyValue(HL2RP_MAP_ALIAS_FIELD_NAME, pFileName);
				KeyValues* pSolids = pKey->FindKey("Solids");

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
							->ConvexFromPlanes(planes.Base()->m_Normal.Base(), planes.Size(), 0.0f));
					}

					CPhysCollide* pCollide = physcollision->ConvertConvexToCollide(convexes.Base(), convexes.Size());

					if (pCollide != NULL)
					{
						objectparams_t params{};
						IPhysicsObject* pObject = physenv->CreatePolyObjectStatic(pCollide, -1,
							pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), &params);

						if (pObject != NULL)
						{
							pEntity->VPhysicsSetObject(pObject);
						}
					}
				}

				DispatchSpawn(pEntity);
			}
		}
	}
}

void CServerGameClients::NetworkIDValidated(const char*, const char* pNetworkId)
{
	// Backup the Steam ID since engine's GetNetworkIDString() overwrites parameter
	char networkId[MAX_NETWORKID_LENGTH];
	V_strcpy_safe(networkId, pNetworkId);

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer != NULL && Q_strcmp(pPlayer->GetNetworkIDString(), networkId) == 0)
		{
			ToHL2Roleplayer(pPlayer)->LoadFromDatabase();
			break;
		}
	}
}
