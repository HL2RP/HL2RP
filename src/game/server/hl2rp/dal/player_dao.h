#ifndef PLAYER_DAO_H
#define PLAYER_DAO_H
#pragma once

#include "idao.h"
#include <hl2_roleplayer_shared.h>

#define PLAYER_DAO_MAIN_COLLECTION_NAME "Player"

class CPlayerLoadDAO : public CLoadDAO
{
	bool MergeFrom(IDAO*) OVERRIDE;
	void HandleCompletion() OVERRIDE;

	uint64 mSteamIdNumber;

public:
	CPlayerLoadDAO(CBasePlayer*);
};

class CPlayersMainDataSaveDAO : public CSaveDAO
{
	CRecordNodeDTO* CreateRecord(CBasePlayer*);
	void AddField(CRecordNodeDTO*, const SUtlField&, EPlayerDatabasePropType);

public:
	CPlayersMainDataSaveDAO(CHL2Roleplayer*);
	CPlayersMainDataSaveDAO(CBasePlayer*, const SUtlField&, EPlayerDatabasePropType);
};

class CPlayersAmmunitionSaveDAO : public CSaveDeleteDAO
{
public:
	CPlayersAmmunitionSaveDAO(CBasePlayer*, int index, int count);
};

class CPlayersWeaponsSaveDAO : public CSaveDeleteDAO
{
public:
	CPlayersWeaponsSaveDAO(CBasePlayer*, CBaseCombatWeapon*, bool save);
};

#endif // !PLAYER_DAO_H
