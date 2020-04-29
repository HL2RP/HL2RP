#include "cbase.h"
#include "CPropRationDispenser.h"
#include "CHL2RP.h"
#include "CHL2RP_Player.h"
#include "CHL2RP_PlayerDialog.h"
#include "weapon_citizenpackage.h"

const char DISPENSER_MODEL_PATH[] = "models/props_combine/combine_dispenser.mdl";

const float DISPENSER_RATION_PICKUP_WAIT_SECONDS = 1800.0f;
const float DISPENSER_CRIMINAL_USE_WAIT_SECONDS = 5.0f;
const char DISPENSER_SUPPLY_SOUND[] = "buttons/button5.wav";
const float DISPENSER_SUPPLY_GLOW_SPRITE_SIZE = 0.1f;
const char DISPENSER_CRIMINAL_ALARM_SOUND[] = "npc/attack_helicopter/aheli_damaged_alarm1.wav";
const float DISPENSER_CRIMINAL_GLOW_SPRITE_SIZE = 0.5f;
const char DISPENSER_INPUT_DISABLE_RATION_PICKUP_STRING[] = "DisableRationPickup";

LINK_ENTITY_TO_CLASS(prop_ration_dispenser, CPropRationDispenser);

BEGIN_DATADESC(CPropRationDispenser)
DEFINE_INPUTFUNC(FIELD_VOID, DISPENSER_INPUT_DISABLE_RATION_PICKUP_STRING, InputDisableRationPickup)
END_DATADESC()

CPropRationDispenser::CPropRationDispenser() : m_iDatabaseID(INVALID_DATABASE_ID), m_hContainedRation(NULL)
{
	// Don't become instantly filled, to look a bit more serious
	m_flNextReadyTime = gpGlobals->curtime + DISPENSER_RATION_PICKUP_WAIT_SECONDS;
	inputdata_t inputdata;
	CUtlString sInputString("OnAnimationDone !self,");
	sInputString += DISPENSER_INPUT_DISABLE_RATION_PICKUP_STRING;
	sInputString += ",0,0,-1";
	inputdata.value.SetString(MAKE_STRING(sInputString));
	InputAddOutput(inputdata);
}

void CPropRationDispenser::CHL2RP_PlayerUse(const CHL2RP_Player &player)
{
	if ((m_iDatabaseID != INVALID_DATABASE_ID) && (m_flNextReadyTime <= gpGlobals->curtime))
	{
		if (player.m_iCrime < 1)
		{
			PropSetAnim("dispense_package");

			if (m_hContainedRation == NULL)
			{
				if (engine->GetEntityCount() + 1 < MAX_EDICTS)
				{
					int iAttachment = LookupAttachment("Package_attachment");

					if (iAttachment > 0)
					{
						m_hContainedRation = static_cast<CWeaponCitizenPackage *>(CreateNoSpawn("weapon_citizenpackage", vec3_origin, GetAbsAngles()));
						m_hContainedRation->m_hParentDispenser = this;
						m_hContainedRation->AddSpawnFlags(SF_NORESPAWN);
						DispatchSpawn(m_hContainedRation);
						m_hContainedRation->SetSecondaryAmmoCount(1);
						m_hContainedRation->SetParent(this); // Passing attachment on overloaded version does attach it
						inputdata_t inputdata;
						inputdata.value.SetString(MAKE_STRING("Package_attachment"));
						m_hContainedRation->InputSetParentAttachment(inputdata);
					}
				}
			}
			else
			{
				m_hContainedRation->RemoveSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
			}

			if (m_hContainedRation != NULL)
			{
				m_flNextReadyTime = gpGlobals->curtime + SequenceDuration();

				// Begin ration supply effects
				CPASAttenuationFilter soundFilter;
				soundFilter.AddAllPlayers();
				enginesound->EmitSound(soundFilter, entindex(), CHAN_AUTO, DISPENSER_SUPPLY_SOUND, VOL_NORM, SNDLVL_NORM);

				int iCoverBone = LookupBone("Combine_Dispenser.CoverBone");

				if (iCoverBone != -1)
				{
					static int iDispenserSupplySprite = PrecacheModel("sprites/greenglow1.vmt");
					Vector glowSpriteOrigin;
					QAngle auxAngle;
					GetBonePosition(iCoverBone, glowSpriteOrigin, auxAngle);
					CBroadcastRecipientFilter glowSpriteFilter;
					glowSpriteFilter.SetIgnorePredictionCull(true);
					te->GlowSprite(glowSpriteFilter, 0.0f, &glowSpriteOrigin, iDispenserSupplySprite,
						SequenceDuration(), DISPENSER_SUPPLY_GLOW_SPRITE_SIZE, 255);
				}
			}
		}
		else
		{
			// Begin criminal response
			m_flNextReadyTime = gpGlobals->curtime + DISPENSER_CRIMINAL_USE_WAIT_SECONDS;

			CPASAttenuationFilter soundFilter;
			soundFilter.AddAllPlayers();
			enginesound->EmitSound(soundFilter, entindex(), CHAN_AUTO, DISPENSER_CRIMINAL_ALARM_SOUND, VOL_NORM, SNDLVL_NORM);
			int iCoverBone = LookupBone("Combine_Dispenser.CoverBone");

			if (iCoverBone != -1)
			{
				static int dispenserDenySprite = PrecacheModel("sprites/redglow1.vmt");
				Vector glowSpriteOrigin;
				QAngle auxAngle;
				GetBonePosition(iCoverBone, glowSpriteOrigin, auxAngle);
				CBroadcastRecipientFilter glowSpriteFilter;
				glowSpriteFilter.SetIgnorePredictionCull(true);
				te->GlowSprite(glowSpriteFilter, 0.0f, &glowSpriteOrigin, dispenserDenySprite,
					DISPENSER_CRIMINAL_USE_WAIT_SECONDS, DISPENSER_CRIMINAL_GLOW_SPRITE_SIZE, 255);
			}
		}
	}
}

void CPropRationDispenser::OnContainedRationPickup()
{
	// This has been set to the absolute dispense_package anim finish time, so add up the refresh interval
	m_flNextReadyTime += DISPENSER_RATION_PICKUP_WAIT_SECONDS;
}

void CPropRationDispenser::Precache()
{
	PrecacheModel(DISPENSER_MODEL_PATH);
	enginesound->PrecacheSound(DISPENSER_SUPPLY_SOUND);
	enginesound->PrecacheSound(DISPENSER_CRIMINAL_ALARM_SOUND);
	BaseClass::Precache();
}

void CPropRationDispenser::Spawn()
{
	SetModelName(MAKE_STRING(DISPENSER_MODEL_PATH));
	SetSolid(SOLID_VPHYSICS);
	BaseClass::Spawn();
}

// Purpose: Prevent ration from being picked once the dispense animation is done
void CPropRationDispenser::InputDisableRationPickup(inputdata_t &inputdata)
{
	// Only do it if input was activated by itself
	if ((inputdata.pCaller == this) && (m_hContainedRation != NULL))
	{
		m_hContainedRation->AddSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
	}
}

CDispenserMapTxn::CDispenserMapTxn() : m_sDispenserMapName(gpGlobals->mapname.ToCStr())
{

}

CDispenserSerialTxn::CDispenserSerialTxn(CPropRationDispenser *pDispenser) : m_hDispenserHandle(pDispenser->GetRefEHandle())
{
	Assert(pDispenser != NULL);
}

bool CDispenserSerialTxn::IsValidDispenserTxnAndIDEquals(const ThisClass *pTxn) const
{
	return ((pTxn != NULL) && (pTxn->m_sDispenserMapName == m_sDispenserMapName)
		&& (pTxn->m_hDispenserHandle == m_hDispenserHandle));
}

// Purpose: Replace derived transactions between themselves. It assumes they are safe candidates!
bool CDispenserSerialTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	return IsValidDispenserTxnAndIDEquals(txn.ToTxnClass(this));
}

CDispenserCoordsTxn::CDispenserCoordsTxn(CPropRationDispenser *pDispenser) :
m_vecCoords(pDispenser->GetAbsOrigin()), m_flYaw(pDispenser->GetAbsAngles().y), CDispenserSerialTxn(pDispenser)
{
	Assert(pDispenser != NULL);
}

CDispenserDatabaseIDTxn::CDispenserDatabaseIDTxn(CPropRationDispenser *pDispenser) : m_iDispenserDatabaseID(pDispenser->m_iDatabaseID)
{
	Assert(pDispenser != NULL);
}

CSetUpDispensersTxn::CSetUpDispensersTxn()
{

}

bool CSetUpDispensersTxn::ShouldUsePreparedStatements() const
{
	return false;
}

void CSetUpDispensersTxn::BuildSQLiteQueries()
{
	AddQuery("CREATE TABLE IF NOT EXISTS Dispenser "
		"(id INTEGER NOT NULL, map VARCHAR (" V_STRINGIFY(MAX_MAP_NAME) ") NOT NULL, x DOUBLE, y DOUBLE, z DOUBLE, yaw DOUBLE, PRIMARY KEY (id), CHECK (id >= 0));");
}

void CSetUpDispensersTxn::BuildMySQLQueries()
{
	AddQuery("CREATE TABLE IF NOT EXISTS Dispenser "
		"(id INTEGER NOT NULL AUTO_INCREMENT, map VARCHAR (" V_STRINGIFY(MAX_MAP_NAME) ") NOT NULL, x DOUBLE, y DOUBLE, z DOUBLE, yaw DOUBLE, PRIMARY KEY (id), CHECK (id >= 0));");
}

CLoadDispensersTxn::CLoadDispensersTxn()
{
	AddQuery("SELECT * FROM Dispenser WHERE map = ?;");
}

void CLoadDispensersTxn::BindParametersGeneric(int iQueryId, const void *pStmt) const
{
	CHL2RP::s_pSQL->BindString(pStmt, m_sDispenserMapName, 1);
}

void CLoadDispensersTxn::HandleQueryResults() const
{
	if (m_sDispenserMapName == gpGlobals->mapname.ToCStr())
	{
		while (CHL2RP::s_pSQL->FetchNextRow())
		{
			const Vector origin(CHL2RP::s_pSQL->FetchFloat("x"), CHL2RP::s_pSQL->FetchFloat("y"), CHL2RP::s_pSQL->FetchFloat("z"));
			const QAngle angles(0.0f, CHL2RP::s_pSQL->FetchFloat("yaw"), 0.0f);
			CPropRationDispenser *dispenser = static_cast<CPropRationDispenser *>(CBaseEntity::Create("prop_ration_dispenser", origin, angles));
			dispenser->m_iDatabaseID = CHL2RP::s_pSQL->FetchInt("id");
		}
	}
}

bool CLoadDispensersTxn::ShouldBeReplacedBy(const CAsyncSQLTxn &txn) const
{
	// Accept replacement simply if cast succeeds, because the map contained within
	// the incoming transaction is always more up-to-date than this transaction's
	return (txn.ToTxnClass(this) != NULL);
}

CInsertDispenserTxn::CInsertDispenserTxn(CPropRationDispenser *pDispenser) :
CDispenserCoordsTxn(pDispenser)
{
	AddQuery("INSERT INTO Dispenser (map, x, y, z, yaw) VALUES (?, ?, ?, ?, ?);");
	AddQuery("SELECT id FROM DISPENSER ORDER BY id DESC LIMIT 1;");
}

void CInsertDispenserTxn::BindParametersGeneric(int iQueryId, const void *pStmt) const
{
	if (!iQueryId)
	{
		CHL2RP::s_pSQL->BindString(pStmt, m_sDispenserMapName, 1);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[X_INDEX], 2);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Y_INDEX], 3);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Z_INDEX], 4);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_flYaw, 5);
	}
}

void CInsertDispenserTxn::HandleQueryResults() const
{
	// Results should correspond to the dispenser inserted
	// (if driver isolation is set to a decent minimum)
	if (CHL2RP::s_pSQL->FetchNextRow())
	{
		CPropRationDispenser *pDispenser = static_cast<CPropRationDispenser *>(m_hDispenserHandle.Get());

		if (pDispenser != NULL)
		{
			pDispenser->m_iDatabaseID = CHL2RP::s_pSQL->FetchInt("id");
		}
	}
}

bool CInsertDispenserTxn::ShouldRemoveBoth(const CAsyncSQLTxn &txn) const
{
	return IsValidDispenserTxnAndIDEquals(txn.ToTxnClass<CDeleteDispenserTxn>());
}

CUpsertDispenserTxn::CUpsertDispenserTxn(CPropRationDispenser *pDispenser) :
CDispenserCoordsTxn(pDispenser), CDispenserDatabaseIDTxn(pDispenser)
{

}

void CUpsertDispenserTxn::BuildSQLiteQueries()
{
	AddQuery("UPDATE Dispenser SET x = ?, y = ?, z = ?, yaw = ? WHERE id = ? AND map = ?;");
	AddQuery("INSERT OR IGNORE INTO Dispenser (id, map, x, y, z, yaw) VALUES (?, ?, ?, ?, ?, ?);");
}

void CUpsertDispenserTxn::BuildMySQLQueries()
{
	AddQuery("INSERT INTO Dispenser (id, map, x, y, z, yaw) VALUES (?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE x = ?, y = ?, z = ?, yaw = ?;");
}

void CUpsertDispenserTxn::BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const
{
	if (!iQueryId)
	{
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[X_INDEX], 1);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Y_INDEX], 2);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Z_INDEX], 3);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_flYaw, 4);
		CHL2RP::s_pSQL->BindInt(pStmt, m_iDispenserDatabaseID, 5);
		CHL2RP::s_pSQL->BindString(pStmt, m_sDispenserMapName, 6);
	}
	else
	{
		CHL2RP::s_pSQL->BindInt(pStmt, m_iDispenserDatabaseID, 1);
		CHL2RP::s_pSQL->BindString(pStmt, m_sDispenserMapName, 2);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[X_INDEX], 3);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Y_INDEX], 4);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Z_INDEX], 5);
		CHL2RP::s_pSQL->BindFloat(pStmt, m_flYaw, 6);
	}
}

void CUpsertDispenserTxn::BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const
{
	CHL2RP::s_pSQL->BindInt(pStmt, m_iDispenserDatabaseID, 1);
	CHL2RP::s_pSQL->BindString(pStmt, m_sDispenserMapName, 2);
	CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[X_INDEX], 3, 7);
	CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Y_INDEX], 4, 8);
	CHL2RP::s_pSQL->BindFloat(pStmt, m_vecCoords[Z_INDEX], 5, 9);
	CHL2RP::s_pSQL->BindFloat(pStmt, m_flYaw, 6, 10);
}

CDeleteDispenserTxn::CDeleteDispenserTxn(CPropRationDispenser *pDispenser) :
CDispenserSerialTxn(pDispenser), CDispenserDatabaseIDTxn(pDispenser)
{
	AddQuery("DELETE FROM Dispenser WHERE id = ?;");
}

void CDeleteDispenserTxn::BindParametersGeneric(int iQueryId, const void *pStmt) const
{
	CHL2RP::s_pSQL->BindInt(pStmt, m_iDispenserDatabaseID, 1);
}