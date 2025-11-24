// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "prop_ration_dispenser.h"
#include <ration.h>
#include <engine/IEngineSound.h>

#ifdef HL2RP_CLIENT_OR_LEGACY
#include "hl2rp_localizer.h"

#ifdef GAME_DLL
#include <hl2_roleplayer.h>
#else
#include <c_hl2_roleplayer.h>
#endif // GAME_DLL
#endif // HL2RP_CLIENT_OR_LEGACY

LINK_ENTITY_TO_CLASS(prop_ration_dispenser, CRationDispenserProp)

#ifdef HL2RP_FULL
IMPLEMENT_HL2RP_NETWORKCLASS(RationDispenserProp)
#ifdef GAME_DLL
SendPropInt(SENDINFO(m_spawnflags)),
SendPropBool(SENDINFO(mIsLocked)),
SendPropTime(SENDINFO(mNextTimeAvailable))
#else
RecvPropInt(RECVINFO(m_spawnflags)),
RecvPropBool(RECVINFO(mIsLocked)),
RecvPropTime(RECVINFO(mNextTimeAvailable))
#endif // GAME_DLL
END_NETWORK_TABLE()
#endif // HL2RP_FULL

void CRationDispenserProp::Spawn()
{
	SetModelName(MAKE_STRING(RATION_DISPENSER_MODEL_PATH));
	SetSolid(SOLID_VPHYSICS);
	BaseClass::Spawn();

	if (FBitSet(m_spawnflags, RATION_DISPENSER_SF_COMBINE_CONTROLLED))
	{
		mIsLocked = true;
	}
}

void CRationDispenserProp::OnContainedRationPickup()
{
	mNextTimeAvailable = gpGlobals->curtime + RATION_DISPENSER_USE_COOLDOWN;
	mhContainedRation.Term();

	if (!IsSequenceFinished())
	{
		mNextTimeAvailable += SequenceDuration() * (1.0f - GetCycle());
	}
}

#ifdef HL2RP_CLIENT_OR_LEGACY
void CRationDispenserProp::GetHUDInfo(CHL2Roleplayer* pPlayer, CLocalizeFmtStr<>& text)
{
	if (SequenceDuration() <= 0.0f)
	{
		if (mIsLocked)
		{
			text.Format("%t\n%t", "#HL2RP_Dispenser_Locked", pPlayer->HasCombineGrants(FBitSet(m_spawnflags,
				RATION_DISPENSER_SF_COMBINE_CONTROLLED)) ? "#HL2RP_Dispenser_Unlock" : "#HL2RP_Dispenser_LockedEx");
		}
		else if (mNextTimeAvailable > gpGlobals->curtime)
		{
			text.Localize("#HL2RP_Dispenser_Restoring",
				UTIL_FormatDuration(pPlayer, mNextTimeAvailable - gpGlobals->curtime));
		}
		else
		{
			text.Localize("#HL2RP_Dispenser_Available");

			if (pPlayer->mFaction == EFaction::Citizen)
			{
				text.Format("\n%t", "#HL2RP_Dispenser_UseHint");
			}

			if (pPlayer->HasCombineGrants(FBitSet(m_spawnflags, RATION_DISPENSER_SF_COMBINE_CONTROLLED)))
			{
				int timeLeft = mNextTimeAvailable + RATION_DISPENSER_LOCK_COOLDOWN - gpGlobals->curtime;

				if (timeLeft > 0)
				{
					SRelativeTime relTime(timeLeft);
					text.Format("\n%t", "#HL2RP_Dispenser_LockWait", relTime.mMinutes, relTime.mSeconds);
					return;
				}

				text.Format("\n%t", "#HL2RP_Dispenser_Lock");
			}
		}
	}
}
#endif // HL2RP_CLIENT_OR_LEGACY
