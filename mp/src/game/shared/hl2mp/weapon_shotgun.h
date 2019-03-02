#ifndef WEAPON_SHOTGUN_H
#define WEAPON_SHOTGUN_H
#pragma once

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponShotgun C_WeaponShotgun
#endif

class CWeaponShotgun : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponShotgun, CBaseHL2MPCombatWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	CNetworkVar( bool,	m_bNeedPump );		// When emptied completely
	CNetworkVar( bool,	m_bDelayedFire1 );	// Fire primary when finished reloading
	CNetworkVar( bool,	m_bDelayedFire2 );	// Fire secondary when finished reloading
	CNetworkVar( bool,	m_bDelayedReload );	// Reload when finished pump

public:
	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_10DEGREES;
		return cone;
	}

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }

	bool StartReload( void );
	bool Reload( void );
	void FillClip( void );
	void FinishReload( void );
	void CheckHolsterReload( void );
	void Pump( void );
//	void WeaponIdle( void );
	void ItemHolsterFrame( void );
	void ItemPostFrame( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void DryFire( void );
	virtual float GetFireRate( void ) { return 0.7; };

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponShotgun(void);

private:
	CWeaponShotgun( const CWeaponShotgun & );
};

#endif // !WEAPON_SHOTGUN_H
