#ifndef HL2RP_CHARACTER_H
#define HL2RP_CHARACTER_H
#pragma once

template<class T = CBaseCombatCharacter>
class CHL2RPCharacter : public T
{
	DECLARE_CLASS(CHL2RPCharacter, T)

protected:
#ifdef GAME_DLL
	int OnTakeDamage(const CTakeDamageInfo& info) OVERRIDE
	{
		return ((!T::IsAlive() || T::HL2RP_OnTakeDamage(info)) ? T::OnTakeDamage(info) : 0);
	}

#ifdef HL2RP_FULL
	void Event_Killed(const CTakeDamageInfo& info) OVERRIDE
	{
		BaseClass::Event_Killed(info);
		GameRules()->DeathNotice(this, info);
	}
#endif // HL2RP_FULL
#endif // GAME_DLL
};

#endif // !HL2RP_CHARACTER_H
