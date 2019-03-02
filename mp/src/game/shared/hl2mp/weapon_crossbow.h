#ifndef WEAPON_CROSSBOW_H
#define WEAPON_CROSSBOW_H
#pragma once

#include "weapon_hl2mpbasehlmpcombatweapon.h"

//-----------------------------------------------------------------------------
// CWeaponCrossbow
//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL
#define CWeaponCrossbow C_WeaponCrossbow
#else
#include "Sprite.h"
#endif

class CWeaponCrossbow : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponCrossbow, CBaseHL2MPCombatWeapon );
public:

	CWeaponCrossbow( void );
	
	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	Reload( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual bool	SendWeaponAnim( int iActivity );

#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	
	void	SetSkin( int skinNum );
	void	CheckZoomToggle( void );
	void	FireBolt( void );
	void	ToggleZoom( void );
	
	// Various states for the crossbow's charger
	enum ChargerState_t
	{
		CHARGER_STATE_START_LOAD,
		CHARGER_STATE_START_CHARGE,
		CHARGER_STATE_READY,
		CHARGER_STATE_DISCHARGE,
		CHARGER_STATE_OFF,
	};

	void	CreateChargerEffects( void );
	void	SetChargerState( ChargerState_t state );
	void	DoLoadEffect( void );

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	
	// Charger effects
	ChargerState_t		m_nChargeState;

#ifndef CLIENT_DLL
	CHandle<CSprite>	m_hChargerSprite;
#endif

	CNetworkVar( bool,	m_bInZoom );
	CNetworkVar( bool,	m_bMustReload );

	CWeaponCrossbow( const CWeaponCrossbow & );
};

#endif // !WEAPON_CROSSBOW_H
