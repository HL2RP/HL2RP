//============ Copyright Valve Corporation, All rights reserved. ============//
//
// HL2MP_WEAPON_REGISTER.CPP
//
// Purpose: Decouple weapons linkage from original source files, to re-assign
//
//===========================================================================//

#include <cbase.h>
#include "weapon_357.h"
#include "weapon_ar2.h"
#include "weapon_crossbow.h"
#include "weapon_crowbar.h"
#include "weapon_frag.h"
#include "weapon_physcannon.h"
#include "weapon_pistol.h"
#include "weapon_rpg.h"
#include "weapon_shotgun.h"
#include "weapon_slam.h"
#include "weapon_smg1.h"
#include "weapon_stunstick.h"

LINK_ENTITY_TO_CLASS(weapon_357, CWeapon357);
LINK_ENTITY_TO_CLASS(weapon_ar2, CWeaponAR2);
LINK_ENTITY_TO_CLASS(weapon_crossbow, CWeaponCrossbow);
LINK_ENTITY_TO_CLASS(weapon_crowbar, CWeaponCrowbar);
LINK_ENTITY_TO_CLASS(weapon_frag, CWeaponFrag);
LINK_ENTITY_TO_CLASS(weapon_physcannon, CWeaponPhysCannon);
LINK_ENTITY_TO_CLASS(weapon_pistol, CWeaponPistol);
LINK_ENTITY_TO_CLASS(weapon_rpg, CWeaponRPG);
LINK_ENTITY_TO_CLASS(weapon_shotgun, CWeaponShotgun);
LINK_ENTITY_TO_CLASS(weapon_slam, CWeapon_SLAM);
LINK_ENTITY_TO_CLASS(weapon_smg1, CWeaponSMG1);
LINK_ENTITY_TO_CLASS(weapon_stunstick, CWeaponStunStick);
