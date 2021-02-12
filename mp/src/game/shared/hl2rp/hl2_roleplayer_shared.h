#ifndef HL2_ROLEPLAYER_SHARED_H
#define HL2_ROLEPLAYER_SHARED_H
#pragma once

#include "hl2rp_util_shared.h"
#include <generic.h>

#ifdef GAME_DLL
#include <hl2mp_player.h>
#else
#include <c_hl2mp_player.h>
#include <simtimer.h>

#define CBaseHL2Roleplayer C_BaseHL2Roleplayer
#define CHL2Roleplayer     C_HL2Roleplayer
#endif // GAME_DLL

#ifdef HL2RP_CLIENT_OR_LEGACY
#include "hl2rp_localizer.h"
#endif // HL2RP_CLIENT_OR_LEGACY

template<typename T, class Listener>
class CPlayerDatabasePropBase : public CPositiveVar<T, Listener>
{
public:
	CPlayerDatabasePropBase(T value = 0) : CPositiveVar<T, Listener>(value) {}
};

#define CPlayerDatabasePropListenerFunc(category, name) \
	static void OnValueChanged(void* pProp) \
	{ \
		PointerOf(ThisClass, pProp, name)->OnDatabasePropChanged(category); \
	} \

#define CPlayerDatabaseNetworkProp(Type, category, name) \
	NETWORK_VAR_START(Type, name) \
		CPlayerDatabasePropListenerFunc(category, name) \
	NETWORK_VAR_END(Type, name, CPlayerDatabasePropBase, NetworkStateChanged)

class CHL2Roleplayer;

ENUM(EPlayerDatabasePropType,
	Name,
	Seconds,
	Crime,
	AccessFlags,
	Health,
	Armor,
	MiscFlags,
	SentHUDHints
)

ENUM(EPlayerHUDHintType,
	StickyWalking,
	RationDeployed,
	RationThrowing
)

ENUM(EPlayerMovementFlag,
	Interpenetrating = 1, // Penetrating with other players (prevents getting stuck with them)
	InStickyWalkMode = 2
)

class CBaseHL2Roleplayer : public CHL2MP_Player
{
	DECLARE_CLASS(CBaseHL2Roleplayer, CHL2MP_Player)

	void PreThink() OVERRIDE;
	virtual void OnDatabasePropChanged(EPlayerDatabasePropType::_Value) {}

public:
#ifdef HL2RP_CLIENT_OR_LEGACY
	void ComputeMainHUD(localizebuf_t& dest);
#endif // HL2RP_CLIENT_OR_LEGACY

	CNetworkVar(int, mMovementFlags);
	EHANDLE mhAimingEntity;
	CPlayerDatabaseNetworkProp(int, EPlayerDatabasePropType::Seconds, mSeconds);
	CPlayerDatabaseNetworkProp(int, EPlayerDatabasePropType::Crime, mCrime);

protected:
	void Spawn() OVERRIDE;

	void StartWalking();
	void StopWalking();

	// Controls the limit time that a second walk key press allows entering into sticky walk mode
	CSimpleSimTimer mStickyWalkChanceExpireTimer;
};

CHL2Roleplayer* ToHL2Roleplayer(CBasePlayer*);
CHL2Roleplayer* ToHL2Roleplayer(CBaseEntity*);

#endif // !HL2_ROLEPLAYER_SHARED_H
