#include <cbase.h>
#include "Suitcase.h"
#include <HL2RP_Player.h>
#include <in_buttons.h>
#include <Dialogs/PublicDialogs.h>

LINK_ENTITY_TO_CLASS(suitcase, CSuitcase);
PRECACHE_WEAPON_REGISTER(suitcase);

acttable_t CSuitcase::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SLAM, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SLAM, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SLAM, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SLAM, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SLAM, false },
	{ ACT_CROUCH, ACT_CROUCH, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_SUITCASE, false },
	{ ACT_JUMP, ACT_JUMP, false },
	{ ACT_RUN_AIM, ACT_RUN, false },
	{ ACT_WALK_AIM, ACT_WALK_SUITCASE, false },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH, false }
};

IMPLEMENT_ACTTABLE(CSuitcase);

#ifndef HL2DM_RP
// On HL2DM, this would block players from connecting to the server.
// Excluding it cancels client-side initializations, but it's fine.
IMPLEMENT_SERVERCLASS_ST(CSuitcase, DT_Suitcase)
END_SEND_TABLE();
#endif

void CSuitcase::SecondaryAttack()
{
	if (m_ActiveState == EActiveState::Deployed)
	{
		CHL2RP_Player* pPlayer = ToHL2RP_Player(GetOwner());

		if (pPlayer != NULL)
		{
			SendWeaponAnim(ACT_VM_HOLSTER);
			pPlayer->DisplayDialog<CMainMenu>();
			WeaponSound(SPECIAL1);
			m_ActiveState = EActiveState::Holstering;
		}
	}
}

void CSuitcase::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	switch (m_ActiveState)
	{
	case EActiveState::Holstering:
	{
		CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

		if (pPlayer != NULL && (pPlayer->m_afButtonReleased & IN_ATTACK2))
		{
			if (IsViewModelSequenceFinished())
			{
				// In current case, only the following activity reverts to the deployed pose smoothly
				SendWeaponAnim(ACT_VM_DRAW);
			}
			else
			{
				// In current case, only the following activity reverts to the deployed pose smoothly
				SendWeaponAnim(ACT_VM_IDLE);
			}

			m_ActiveState = EActiveState::Deployed;
		}

		break;
	}
	}
}

void CSuitcase::OnActiveStateChanged(int oldState)
{
	BaseClass::OnActiveStateChanged(oldState);

	if (m_iState == WEAPON_IS_ACTIVE)
	{
		m_ActiveState = EActiveState::Deployed;
	}
}

bool CSuitcase::CanBeSelected()
{
#ifdef HL2DM_RP
	// HACK: On HL2DM, the special weapons can't be assigned a proper slot
	return false;
#endif // HL2DM

	return BaseClass::CanBeSelected();
}
