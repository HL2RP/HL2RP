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

void CServerGameDLL::LevelInit_ParseAllEntities(const char* pMapEntities)
{
	// Load exported HL2RP entities separate from BSP into the current map
	char path[MAX_PATH];
	V_sprintf_safe(path, "maps/entities/%s.txt", STRING(gpGlobals->mapname));
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

				DispatchSpawn(pEntity);
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