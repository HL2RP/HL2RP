#ifndef WEAPON_CUBEMAP_H
#define WEAPON_CUBEMAP_H

class CWeaponCubemap : public CBaseCombatWeapon
{
public:

	DECLARE_CLASS(CWeaponCubemap, CBaseCombatWeapon);

	void	Precache(void);

	bool	HasAnyAmmo(void)	{ return true; }

	void	Spawn(void);

	DECLARE_SERVERCLASS();
};

#endif // WEAPON_CUBEMAP_H
