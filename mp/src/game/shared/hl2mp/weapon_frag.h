#ifndef WEAPON_FRAG_H
#define WEAPON_FRAG_H
#pragma once

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponFrag C_WeaponFrag
#endif

#define GRENADE_TIMER	2.5f //Seconds
#define GRENADE_DAMAGE_RADIUS 250.0f

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponFrag: public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponFrag, CBaseHL2MPCombatWeapon );
public:

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponFrag();

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame( void );

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	
	bool	Reload( void );

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	void	ThrowGrenade( CBasePlayer *pPlayer );
	bool	IsPrimed( bool ) { return ( m_AttackPaused != 0 );	}
	
private:

	void	RollGrenade( CBasePlayer *pPlayer );
	void	LobGrenade( CBasePlayer *pPlayer );
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	CNetworkVar( bool,	m_bRedraw );	//Draw the weapon again after throwing a grenade
	
	CNetworkVar( int,	m_AttackPaused );
	CNetworkVar( bool,	m_fDrawbackFinished );

	CWeaponFrag( const CWeaponFrag & );

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif
};

#endif // !WEAPON_FRAG_H
