#ifndef HL2RP_GAMERULES_H
#define HL2RP_GAMERULES_H
#pragma once

#include <hl2mp_gamerules.h>

#ifdef CLIENT_DLL
#define CHL2RPRules C_HL2RPRules
#endif

class CHL2RPRules : public CHL2MPRules
{
private:
	DECLARE_CLASS(CHL2RPRules, CHL2MPRules)

	// Purpose: Set up common death notice params among standalone & HL2DM version
	// If all succeeds, it returns true and valid output parameters. False otherwise.
	bool DoDeathNoticeCommons(CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info,
		IGameEvent *&pEventOut, int &scorerEntIndex, int &victimUserID, int &attackerUserID);

	void DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info) OVERRIDE;
	virtual void DeathNotice(CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info);
	void ClientSettingsChanged(CBasePlayer *pPlayer) OVERRIDE;
};

#endif // HL2RP_GAMERULES_H
