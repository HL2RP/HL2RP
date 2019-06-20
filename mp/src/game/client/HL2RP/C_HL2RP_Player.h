#ifndef C_HL2RP_PLAYER_H
#define C_HL2RP_PLAYER_H
#pragma once

#include <c_hl2mp_player.h>
#include <HL2RP_Player_shared.h>

class C_HL2RP_Player : public C_HL2MP_Player
{
	DECLARE_CLASS(C_HL2RP_Player, C_HL2MP_Player);
	DECLARE_CLIENTCLASS();

	void SharedSpawn() OVERRIDE;
	void EndHandleSpeedChanges(int buttonsChanged) OVERRIDE;
	bool ShouldCollide(int collisionGroup, int contentsMask) const OVERRIDE;

	void DisplayStickyWalkingHint() { }
	void OnEnterStickyWalking() { }

	CHL2RP_PlayerSharedState m_SharedState;
};

#endif // !C_HL2RP_PLAYER_H
