#ifndef HL2RP_GAMERULES_H
#define HL2RP_GAMERULES_H
#pragma once

#include <hl2rp_gamerules_shared.h>
#include <GameEventListener.h>

class CHL2RPRules : public CBaseHL2RPRules, CGameEventListener
{
	DECLARE_CLASS(CHL2RPRules, CBaseHL2RPRules)

	void LevelInitPostEntity() OVERRIDE;
	void FireGameEvent(IGameEvent*) OVERRIDE;
	bool AllowDamage(CBaseEntity*, const CTakeDamageInfo&) OVERRIDE;
	void ClientDisconnected(edict_t*) OVERRIDE;
	void ClientCommandKeyValues(edict_t*, KeyValues*) OVERRIDE;

	void PlayerDisconnected(CBasePlayer*);

public:
	CHL2RPRules();
};

CHL2RPRules* HL2RPRules();

#endif // !HL2RP_GAMERULES_H
