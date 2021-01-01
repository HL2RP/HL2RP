#ifndef HL2_ROLEPLAYER_SHARED_H
#define HL2_ROLEPLAYER_SHARED_H
#pragma once

#include "hl2rp_character.h"
#include "hl2rp_localizer.h"
#include "hl2rp_shareddefs.h"
#include "hl2rp_util_shared.h"
#include <bitflags.h>

#ifdef GAME_DLL
#include <hl2mp_player.h>
#include <idto.h>
#else
#include <c_hl2mp_player.h>
#include <simtimer.h>

#define CBaseHL2Roleplayer C_BaseHL2Roleplayer
#endif // GAME_DLL

#define CALL_PLAYER_DATABASE_PROP_CHANGE_FUNC(VarType, pProp, enumValue) \
	OnDatabasePropChanged((SPlayerDatabasePropProxy<VarType>::Inner&)pProp, enumValue);

#define CPlayerDatabaseNetworkProp(Type, name, enumValue) \
	NETWORK_VAR_START(Type, name) \
	NETWORK_VAR_END(Type, name, CPlayerDatabasePropBase, NetworkStateChanged) \
	{ CALL_PLAYER_DATABASE_PROP_CHANGE_FUNC(Type, name, enumValue) }

#define DATABASE_PROP_STRING(value) STRING((string_t)value)

template<class T, typename S>
struct SPlayerDatabasePropProxyBase
{
	using Outer = T;
	using Inner = S;
};

template<typename T, class Listener = CDefaultNetworkVarListener>
struct SPlayerDatabasePropProxy : SPlayerDatabasePropProxyBase<CNetworkVarBase<T, Listener>, T>
{

};

template<class Listener>
struct SPlayerDatabasePropProxy<int, Listener> : SPlayerDatabasePropProxyBase<CPositiveVar<int, Listener>, int>
{

};

template<class Listener>
struct SPlayerDatabasePropProxy<float, Listener> : SPlayerDatabasePropProxyBase<CPositiveVar<float, Listener>, float>
{

};

template<class Listener>
struct SPlayerDatabasePropProxy<CBitFlags<>, Listener>
	: SPlayerDatabasePropProxyBase<CBitFlags<CNetworkVarBase<int, Listener>>, int>
{

};

template<typename T, class Listener = CDefaultNetworkVarListener>
using CPlayerDatabasePropBase = typename SPlayerDatabasePropProxy<T, Listener>::Outer;

#define HL2_ROLEPLAYER_REGION_MAX_PLAYERS 5

#define HL2_ROLEPLAYER_USE_KEEP_MAX_DIST      (PLAYER_USE_RADIUS * 2.0f) // Max. distance to allow further USEs on an entity
#define HL2_ROLEPLAYER_CITIZEN_AIM_TRACE_DIST PLAYER_USE_RADIUS
#define HL2_ROLEPLAYER_COMBINE_AIM_TRACE_DIST 600.0f

#define HL2_ROLEPLAYER_DOUBLE_KEYPRESS_MAX_DELAY 0.3f

#define HL2_ROLEPLAYER_BEAMS_PATH "materials/sprites/laserbeam.vmt"

SCOPED_ENUM(EPlayerDatabasePropType,
	Name,
	Seconds,
	Crime,
	Faction,
	Job,
	ModelGroup,
	ModelAlias,
	Health,
	Armor,
	AccessFlags,
	MiscFlags,
	LearnedHUDHints
)

SCOPED_ENUM(EPlayerHUDHintType,
	StickyWalking,
	RationDeployed,
	RationThrowing
)

SCOPED_ENUM(EPlayerAccessFlag, // NOTE: Don't change order
	Combine,
	VIPCitizen,
	VIPCombine,
	Admin, // NOTE: Auth level based sorting assumed from here
	Root
)

SCOPED_ENUM(EPlayerMiscFlag,
	IsRegionListEnabled,
	AllowsRegionVoiceOnly
)

class CBaseHL2Roleplayer : public CHL2RPCharacter<CHL2MP_Player>
{
	DECLARE_CLASS(CBaseHL2Roleplayer, CHL2RPCharacter)

	void Precache() OVERRIDE;

#ifdef GAME_DLL
	virtual void OnDatabasePropChanged(const SFieldDTO&, EPlayerDatabasePropType) = 0;
#else
	void OnDatabasePropChanged(...) {}
#endif // GAME_DLL

public:
	bool IsAdmin();
	bool HasCombineGrants(bool extraCombineCheck = true);
	bool IsWithinDistance(CBaseEntity*, float maxDistance, bool fromEye = false);
	CBaseEntity* GetAimingEntity(float& distance);
	void GetPlayersInRegion(CUtlVector<CBasePlayer*>&);
	int ComputeRegionHUD(const CUtlVector<CBasePlayer*>&, CLocalizeFmtStr<>&); // Returns the player lines count

	CNetworkVar(bool, mIsInStickyWalkMode);
	CPlayerDatabaseNetworkProp(int, mSeconds, EPlayerDatabasePropType::Seconds);
	CPlayerDatabaseNetworkProp(int, mCrime, EPlayerDatabasePropType::Crime);
	CPlayerDatabaseNetworkProp(int, mFaction, EPlayerDatabasePropType::Faction);
	CPlayerDatabaseNetworkProp(CBitFlags<>, mAccessFlags, EPlayerDatabasePropType::AccessFlags);
	CPlayerDatabaseNetworkProp(CBitFlags<>, mMiscFlags, EPlayerDatabasePropType::MiscFlags);

protected:
	void Spawn() OVERRIDE;

	void StartWalking();
	void StopWalking();

	// Controls the limit time that a second walk key press allows entering into sticky walk mode
	CSimpleSimTimer mStickyWalkChanceTimer;
};

CHL2Roleplayer* ToHL2Roleplayer(CBasePlayer*);
CHL2Roleplayer* ToHL2Roleplayer(CBaseEntity*);

template<class F>
void ForEachRoleplayer(F functor)
{
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer != NULL)
		{
			functor(ToHL2Roleplayer(pPlayer));
		}
	}
}

#endif // !HL2_ROLEPLAYER_SHARED_H
