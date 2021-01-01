#ifndef PLAYER_DAO_H
#define PLAYER_DAO_H
#pragma once

#include "idao.h"
#include <hl2_roleplayer_shared.h>

class CPlayerLoadDAO : public CLoadDAO
{
	bool MergeFrom(IDAO*) OVERRIDE;
	void HandleCompletion() OVERRIDE;

	uint64 mSteamIdNumber;

public:
	CPlayerLoadDAO(CHL2Roleplayer*);
};

class CPlayersMainDataSaveDAO : public CSaveDAO
{
	CRecordNodeDTO* CreateRecord(CHL2Roleplayer*);
	void AddField(CRecordNodeDTO*, const SFieldDTO&, EPlayerDatabasePropType);

public:
	CPlayersMainDataSaveDAO(CHL2Roleplayer*);
	CPlayersMainDataSaveDAO(CHL2Roleplayer*, const SFieldDTO&, EPlayerDatabasePropType);
};

class CPlayersAmmunitionSaveDAO : public CSaveDeleteDAO
{
public:
	CPlayersAmmunitionSaveDAO(CHL2Roleplayer*, int index, int count);
};

class CPlayersWeaponsSaveDAO : public CSaveDeleteDAO
{
public:
	CPlayersWeaponsSaveDAO(CHL2Roleplayer*, CBaseCombatWeapon*, bool save);
};

#endif // !PLAYER_DAO_H
