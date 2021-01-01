#ifndef PLAYER_DAO_H
#define PLAYER_DAO_H
#pragma once

#include "idao.h"
#include <bitflags.h>

class CHL2Roleplayer;

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
public:
	CPlayersMainDataSaveDAO(CHL2Roleplayer*, const CBitFlags<>& selectedProps = INT_MAX);
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
