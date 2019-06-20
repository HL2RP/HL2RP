#include <cbase.h>
#include "HL2RP_Player_shared.h"
#include <in_buttons.h>

#ifdef CLIENT_DLL
#include <C_HL2RP_Player.h>
#else
#include <HL2RP_Player.h>
#endif // CLIENT_DLL

#define HL2RP_PLAYER_WALK_SPEED	80.0f

// Max. interval player must press walk key twice within to enable sticky walk
#define HL2RP_PLAYER_STICKY_WALK_MAX_ENABLING_DELAY	0.3f

LINK_ENTITY_TO_CLASS(player, CHL2RP_Player);

void CHL2RP_PlayerSharedState::StartWalking(CHL2RP_Player* pPlayer)
{
	pPlayer->StartWalking();
	pPlayer->SetMaxSpeed(HL2RP_PLAYER_WALK_SPEED);
}

void CHL2RP_PlayerSharedState::StopWalking(CHL2RP_Player* pPlayer)
{
	pPlayer->StopWalking();
	m_bIsStickyWalk = false;
}

void CHL2RP_Player::SharedSpawn()
{
	BaseClass::SharedSpawn();

	// Clear sticky walking
	if (m_SharedState.m_bIsStickyWalk)
	{
		m_SharedState.StopWalking(this);
	}
}

// Sticky walking: Player must press walk, instead of once, for safety (eg. ALT-TAB conflict).
void CHL2RP_Player::EndHandleSpeedChanges(int buttonsChanged)
{
	// Precondition: Am I allowed to walk?
	if (!IsSprinting() && !(m_nButtons & IN_DUCK))
	{
		if (m_afButtonPressed & IN_WALK)
		{
			if (IsWalking())
			{
				m_SharedState.StopWalking(this);
			}
			else
			{
				m_SharedState.StartWalking(this);

				if (gpGlobals->curtime < m_SharedState.m_flStickyWalkRefuseTime)
				{
					m_SharedState.m_bIsStickyWalk = IsWalking();
					OnEnterStickyWalking();
				}
				else
				{
					m_SharedState.m_flStickyWalkRefuseTime = gpGlobals->curtime
						+ HL2RP_PLAYER_STICKY_WALK_MAX_ENABLING_DELAY;
					DisplayStickyWalkingHint();
				}
			}
		}
		else if ((m_afButtonReleased & IN_WALK) && !m_SharedState.m_bIsStickyWalk)
		{
			m_SharedState.StopWalking(this);
		}
	}
	else if (IsWalking())
	{
		m_SharedState.StopWalking(this);
	}
}

bool CHL2RP_Player::ShouldCollide(int collisionGroup, int contentsMask) const
{
	// Cancel if I'm stuck with a player already or I'm a fake client, and the other is a player.
	// Fake clients don't handle m_StuckLast and make it hard to detect intersection in some frames.
	if ((m_StuckLast != 0 || (GetFlags() & FL_FAKECLIENT)) && contentsMask == MASK_PLAYERSOLID)
	{
		return false;
	}

	return BaseClass::ShouldCollide(collisionGroup, contentsMask);
}
