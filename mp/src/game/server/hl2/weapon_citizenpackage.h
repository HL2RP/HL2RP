//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef WEAPON_CITIZENPACKAGE_H
#define WEAPON_CITIZENPACKAGE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef HL2RP
#include "basehlcombatweapon.h"
#else
#include "weapon_cubemap.h"
#endif

class CPropRationDispenser;

//=============================================================================
//
// Weapon - Citizen Package Class
//=============================================================================
#ifdef HL2RP
class CWeaponCitizenPackage : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponCitizenPackage, CBaseHLCombatWeapon);
#else
class CWeaponCitizenPackage : public CWeaponCubemap
{
	DECLARE_CLASS(CWeaponCitizenPackage, CWeaponCubemap);
#endif

public:
#ifdef HL2RP
	DECLARE_SERVERCLASS();
#endif
	DECLARE_DATADESC();	
	DECLARE_ACTTABLE();

#ifdef ROLEPLAY
	CWeaponCitizenPackage();
#endif

	void ItemPostFrame( void );
	void Drop( const Vector &vecVelocity );

#ifdef ROLEPLAY
private:
	void Spawn();
	void PrimaryAttack();
	void SecondaryAttack();
	void AllowPickup();
	bool FVisible(CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker) OVERRIDE;
	bool UsesClipsForAmmo1() const;
	bool UsesClipsForAmmo2() const override;
	int GetDefaultClip2() const;

public:
	void UpdateOnRemove() OVERRIDE;

	CHandle<CPropRationDispenser> m_hParentDispenser;
#endif
};

//-----------------------------------------------------------------------------
// Purpose: Citizen suitcase
//-----------------------------------------------------------------------------
#ifdef HL2RP
class CWeaponCitizenSuitcase : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponCitizenSuitcase, CBaseHLCombatWeapon);
#else
class CWeaponCitizenSuitcase : public CWeaponCubemap
{
	DECLARE_CLASS(CWeaponCitizenSuitcase, CWeaponCubemap);
#endif

public:
#ifdef HL2RP
	DECLARE_SERVERCLASS();
#endif
	DECLARE_DATADESC();
	DECLARE_ACTTABLE();

#ifdef ROLEPLAY
private:
	bool UsesClipsForAmmo1() const;
	bool UsesClipsForAmmo2() const;
	int GetDefaultClip2() const;
#endif
};

#endif // WEAPON_CITIZENPACKAGE_H