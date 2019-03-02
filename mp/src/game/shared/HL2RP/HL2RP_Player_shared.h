#ifndef HL2RP_PLAYER_SHARED
#define HL2RP_PLAYER_SHARED
#pragma once

#ifdef CLIENT_DLL
#define CHL2RP_Player	C_HL2RP_Player
#define CHL2RP_PlayerSharedState	C_HL2RP_PlayerSharedState
#endif // CLIENT_DLL

class CHL2RP_Player;

class CHL2RP_PlayerSharedState
{
public:
	void StopWalking(CHL2RP_Player* pPlayer);
	void StartWalking(CHL2RP_Player* pPlayer);

	bool m_bIsStickyWalk; // Sticky walking mode that doesn't require to hold key

	// Max. time player must finish pressing walk key twice to enable sticky walk
	float m_flStickyWalkRefuseTime;
};

#endif // !HL2RP_PLAYER_SHARED
