#include <cbase.h>
#include <CHL2RPRules.h>
#include <ammodef.h>

#ifdef HL2DM_RP
#include <CHL2RP.h>
#endif

#ifdef HL2RP
REGISTER_GAMERULES_CLASS(CHL2RPRules)
#else
// HACK: Fake CHL2RPRules networked name on HL2DM to match client-side table (allow connection)
void __CreateGameRules_CHL2RPRules() { new CHL2RPRules; };
static CGameRulesRegister __g_GameRulesRegister_CHL2RPRules(V_STRINGIFY(CHL2MPRules), __CreateGameRules_CHL2RPRules);
#endif

CAmmoDef *HL2MP_GetAmmoDef();

CAmmoDef *GetAmmoDef()
{
	CAmmoDef &def = *HL2MP_GetAmmoDef();
	def.AddAmmoType("molotov", DMG_BURN, TRACER_NONE, 0, 0, 1, 0, 0);
	def.AddAmmoType("citizenpackage", DMG_GENERIC, TRACER_NONE, 0, 0, INT_MAX, 0, 0);
	def.AddAmmoType("citizensuitcase", DMG_GENERIC, TRACER_NONE, 0, 0, 1, 0, 0);
	return &def;
}

void CHL2RPRules::DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	// Do nothing. The more generic overloaded DeathNotice should be called instead.
}

bool CHL2RPRules::DoDeathNoticeCommons(CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info,
	IGameEvent *&pEventOut, int &scorerEntIndex, int &victimUserID, int &attackerUserID)
{
#ifndef CLIENT_DLL
	// BEGIN: Work out what killed the character, and send a message to all clients about it
	// By default, the character is killed by the world
	const char *pKillerWeaponName = "world";

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pPlayerScorer = GetDeathScorer(pKiller, pInflictor);
	CBaseCombatCharacter *pCharacterScorer;

	if (pPlayerScorer == NULL)
	{
		pCharacterScorer = pKiller->MyCombatCharacterPointer();
		scorerEntIndex = pKiller->entindex();
	}
	else
	{
		pCharacterScorer = pPlayerScorer;
		scorerEntIndex = pPlayerScorer->entindex();
	}

	// Custom kill type?
	if (info.GetDamageCustom())
	{
		pKillerWeaponName = GetDamageCustomString(info);
	}
	else
	{
		// Is the killer a character?
		if (pCharacterScorer != NULL)
		{
			if (pInflictor != NULL)
			{
				if (pInflictor == pCharacterScorer)
				{
					// If the inflictor is the killer, then it must be their current weapon doing the damage
					if (pCharacterScorer->GetActiveWeapon() != NULL)
					{
						pKillerWeaponName = pCharacterScorer->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					pKillerWeaponName = pInflictor->GetClassname(); // It's just that easy
				}
			}
		}
		else
		{
			pKillerWeaponName = pInflictor->GetClassname();
		}

		// Strip the NPC_* or weapon_* from the inflictor's classname
		if (!Q_strncmp(pKillerWeaponName, "weapon_", 7))
		{
			pKillerWeaponName += 7;
		}
		else if (!Q_strncmp(pKillerWeaponName, "npc_", 4))
		{
			pKillerWeaponName += 4;

			if (!Q_strcmp(pKillerWeaponName, "satchel") || !Q_strcmp(pKillerWeaponName, "tripmine"))
			{
				pKillerWeaponName = "slam";
			}
		}
		else if (!Q_strncmp(pKillerWeaponName, "func_", 5))
		{
			pKillerWeaponName += 5;
		}
		else if (Q_strstr(pKillerWeaponName, "physics") != NULL)
		{
			pKillerWeaponName = "physics";
		}
		else if (!Q_strcmp(pKillerWeaponName, "prop_combine_ball"))
		{
			pKillerWeaponName = "combine_ball";
		}
		else if (!Q_strcmp(pKillerWeaponName, "grenade_ar2"))
		{
			pKillerWeaponName = "smg1_grenade";
		}
	}

	pEventOut = gameeventmanager->CreateEvent("player_death");

	if (pEventOut == NULL)
	{
		return false;
	}

	CBaseCombatCharacter *pMaybeAICharacter = pCharacterScorer;
	CBasePlayer *pVictimPlayer = ToBasePlayer(pVictim);
	int numPlayers = 0;

	if (pVictimPlayer != NULL)
	{
		numPlayers++;
		victimUserID = pVictimPlayer->GetUserID();
		pEventOut->SetInt("userid", victimUserID);
	}
	else
	{
		victimUserID = 0;
	}

	if (pPlayerScorer != NULL)
	{
		pMaybeAICharacter = pVictim;
		numPlayers++;
		attackerUserID = pPlayerScorer->GetUserID();
		pEventOut->SetInt("attacker", attackerUserID);
	}
	else
	{
		attackerUserID = 0;
	}

	if (numPlayers < 1)
	{
		// It is a kill between non-player entities, cancel event out of interest
		return false;
	}
	else if (numPlayers == 1)
	{
		// BEGIN: Pass generic beautified names to the event by the AI character class
		const char *pNPCName;

		switch (pMaybeAICharacter->Classify())
		{
		case CLASS_METROPOLICE:
		{
			pNPCName = "Metropolice";
			break;
		}
		case CLASS_COMBINE:
		{
			pNPCName = "Combine Soldier";
			break;
		}
		case CLASS_CITIZEN_PASSIVE:
		case CLASS_CITIZEN_REBEL:
		case CLASS_PLAYER_ALLY:
		case CLASS_PLAYER_ALLY_VITAL:
		{
			pNPCName = "Citizen";
			break;
		}
		default:
		{
			pNPCName = "Enemy Character";
		}
		}

		pEventOut->SetString("npc_name", pNPCName);
	}

	pEventOut->SetString("weapon", pKillerWeaponName);
	pEventOut->SetInt("priority", 7);
#endif

	return true;
}

#ifdef HL2RP
void CHL2RPRules::DeathNotice(CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info)
{
#ifndef CLIENT_DLL
	IGameEvent *pEvent;
	int scorerEntIndex, victimUserID, attackerUserID;

	if (DoDeathNoticeCommons(pVictim, info, pEvent, scorerEntIndex, victimUserID, attackerUserID))
	{
		pEvent->SetInt("entindex_killed", pVictim->entindex());
		pEvent->SetInt("entindex_attacker", scorerEntIndex);
		gameeventmanager->FireEvent(pEvent);
	}
#endif
}
#else
void CHL2RPRules::DeathNotice(CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info)
{
#ifndef CLIENT_DLL
	IGameEvent *pEvent;
	int scorerEntIndex, victimUserID, attackerUserID;

	if (DoDeathNoticeCommons(pVictim, info, pEvent, scorerEntIndex, victimUserID, attackerUserID))
	{
		if ((victimUserID < 1) || (attackerUserID < 1))
		{
			CBasePlayer *pDeathNoticeBot = CHL2RP::s_hDeathNoticeBotHandle;

			if (pDeathNoticeBot != NULL)
			{

				if (victimUserID < 1)
				{
					pEvent->SetInt("userid", pDeathNoticeBot->GetUserID());
				}
				else
				{
					pEvent->SetInt("attacker", pDeathNoticeBot->GetUserID());
				}

				// A bot is involved, event will fire later (see CHL2RP::Think)
				CHL2RP::s_SpecialDeathNoticeEvents.AddToTail(pEvent);
				return;
			}
		}

		gameeventmanager->FireEvent(pEvent);
	}
#endif
}
#endif

void CHL2RPRules::ClientSettingsChanged(CBasePlayer *pPlayer)
{
#ifndef CLIENT_DLL
	Assert(pPlayer != NULL);

	const char *pModelName = engine->GetClientConVarValue(pPlayer->entindex(), "cl_playermodel");

	// Prevent VGUI model bug later changing to the wrong team:
	if (!Q_strcmp(pModelName, "models"))
	{
		return;
	}

#ifdef HL2DM_RP
	CHL2MP_Player *pHL2MPPlayer = static_cast<CHL2MP_Player *>(pPlayer);

	if (pHL2MPPlayer->GetNextModelChangeTime() < gpGlobals->curtime)
	{
		// Determine team first from "CompatModel" list
		int compatModelTypeIndex = CHL2RP::GetCompatModelTypeIndex(pModelName);

		if (compatModelTypeIndex == COMPAT_ANIM_REBEL_MODEL_INDEX)
		{
			pHL2MPPlayer->ChangeTeam(TEAM_REBELS);
		}
		else if (compatModelTypeIndex == COMPAT_ANIM_COMBINE_MODEL_INDEX)
		{
			pHL2MPPlayer->ChangeTeam(TEAM_COMBINE);
		}
	}
#endif

	BaseClass::ClientSettingsChanged(pPlayer);
#endif
}
