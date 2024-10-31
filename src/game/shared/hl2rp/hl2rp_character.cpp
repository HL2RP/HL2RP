#include <cbase.h>
#include "hl2rp_character.h"

#ifdef GAME_DLL
#include <hl2_roleplayer.h>
#endif // GAME_DLL

#ifdef GAME_DLL
bool CBaseCombatCharacter::HL2RP_OnTakeDamage(const CTakeDamageInfo& info)
{
	CHL2Roleplayer* pAttackPlayer = ToHL2Roleplayer(info.GetAttacker());

	if (pAttackPlayer != NULL && pAttackPlayer != this)
	{
		if (pAttackPlayer->IsDamageProtected())
		{
			return false;
		}
		else if (pAttackPlayer->mFaction == EFaction::Citizen)
		{
			pAttackPlayer->mCrime += info.GetDamage() * HL2_ROLEPLAYER_CRIME_PER_HP;
		}
	}

	return true;
}
#endif // GAME_DLL
