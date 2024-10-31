// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include <prop_ration_dispenser.h>
#include "hl2_roleplayer.h"
#include <player_dialogs.h>
#include <ration.h>
#include <npcevent.h>

#define RATION_DISPENSER_GLOW_SPRITE_SIZE 0.1f

#define RATION_DISPENSER_CRIME_EFFECTS_COOLDOWN 5.0f

BEGIN_DATADESC(CRationDispenserProp)
DEFINE_KEYFIELD_NOT_SAVED(mpMapAlias, FIELD_STRING, HL2RP_MAP_ALIAS_FIELD_NAME),
DEFINE_KEYFIELD_NOT_SAVED(mRationsAmmo, FIELD_INTEGER, "rations")
END_DATADESC()

void CRationDispenserProp::UnlockThink()
{
	if (mNextTimeAvailable > gpGlobals->curtime)
	{
		SetContextThink(&ThisClass::UnlockThink, gpGlobals->curtime + HL2_ROLEPLAYER_HUD_THINK_PERIOD, "UnlockThink");

		ForEachRoleplayer([&](CHL2Roleplayer* pPlayer)
		{
			if (pPlayer->IsAlive() && pPlayer->mCrime < 1 && pPlayer->mFaction == EFaction::Citizen)
			{
				pPlayer->SendBeam(GetAbsOrigin(), COLOR_GREEN);
			}
		});
	}
}

void CRationDispenserProp::Use(CBaseEntity* pActivator, CBaseEntity*, USE_TYPE type, float)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(pActivator);

	if (pPlayer != NULL && SequenceDuration() <= 0.0f)
	{
		switch (type)
		{
		case USE_TOGGLE:
		{
			if (pPlayer->mFaction == EFaction::Citizen)
			{
				if (mIsLocked)
				{
					DoReactionEffects(RATION_DISPENSER_LOCKED_SOUND);
				}
				else if (gpGlobals->curtime >= mNextTimeAvailable)
				{
					if (pPlayer->mCrime > 0)
					{
						if (mCrimeEffectsTimer.Expired())
						{
							DoReactionEffects(RATION_DISPENSER_DENY_SOUND, RATION_DISPENSER_DENY_SPRITE_PATH);
							mCrimeEffectsTimer.Set(RATION_DISPENSER_CRIME_EFFECTS_COOLDOWN);
						}

						return;
					}
					else if (mhContainedRation == NULL)
					{
						// Require the presence of ration attachment within plate to spawn, as a good minimum
						int attachment = LookupAttachment("Ration_attachment");
						Vector origin;

						if (GetAttachment(attachment, origin))
						{
#ifdef HL2RP_LEGACY
							// HL2DM clients may not have the new ration attachment, so, before parenting,
							// switch to the legacy one for the ration to follow the dispenser's plate
							attachment = LookupAttachment("Package_attachment");
#endif // HL2RP_LEGACY

							CRation* pRation = static_cast<CRation*>(Create("ration", origin, GetAbsAngles()));
							pRation->AddSpawnFlags(SF_NORESPAWN);
							pRation->SetParent(this, attachment);
							pRation->SetMoveType(MOVETYPE_NONE); // Prevent VPhysics warnings when parented
							pRation->SetTotalUnits(mRationsAmmo);
							pRation->mhParentDispenser = this;
							mhContainedRation = pRation;
						}
					}

					PropSetAnim("dispense_package");
					DoReactionEffects(RATION_DISPENSER_SUPPLY_SOUND, RATION_DISPENSER_SUPPLY_SPRITE_PATH);
				}
			}

			break;
		}
		case USE_SPECIAL1:
		{
			if (pPlayer->HasCombineGrants(HasSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED)))
			{
				if (mIsLocked)
				{
					mNextTimeAvailable = gpGlobals->curtime + RATION_DISPENSER_LOCK_COOLDOWN;
					UnlockThink();

					ForEachRoleplayer([](CHL2Roleplayer* pTarget)
					{
						if (pTarget->mCrime < 1 && pTarget->mFaction == EFaction::Citizen)
						{
							pTarget->SendHUDMessage(EPlayerHUDType::Alert, "#HL2RP_Dispenser_Unlocked",
								HL2RP_CENTER_HUD_SPECIAL_POS, HL2RP_ALERT_HUD_DEFAULT_Y_POS,
								HL2RP_ALERT_HUD_DEFAULT_COLOR, HL2_ROLEPLAYER_ALERT_HUD_HOLD_TIME);
						}
					});
				}
				else if (mNextTimeAvailable + RATION_DISPENSER_LOCK_COOLDOWN > gpGlobals->curtime)
				{
					break;
				}

				mIsLocked = !mIsLocked;
				DoReactionEffects(RATION_DISPENSER_LOCK_SOUND);
			}

			break;
		}
		case USE_SPECIAL2:
		{
			if (pPlayer->HasCombineGrants(HasSpawnFlags(RATION_DISPENSER_SF_COMBINE_CONTROLLED)))
			{
				pPlayer->SendRootDialog(new CDispensersMenu(pPlayer, this));
			}

			break;
		}
		}
	}
}

void CRationDispenserProp::DoReactionEffects(const char* pSound, const char* pSprite)
{
	Vector origin;

	if (pSprite != NULL && GetAttachment("Sprite_attachment", origin))
	{
		CPASFilter filter(origin);
		filter.SetIgnorePredictionCull(true);
		te->GlowSprite(filter, 0.0f, &origin, PrecacheModel(pSprite),
			SequenceDuration(), RATION_DISPENSER_GLOW_SPRITE_SIZE, 255);
	}

	// Play original sound limiting attenuation, to prevent hearing too far away (e.g. deny sound)
	CSoundParameters params;
	GetParametersForSound(pSound, params, NULL);
	engine->EmitAmbientSound(GetSoundSourceIndex(), vec3_origin, (*params.soundname != '\0') ?
		params.soundname : pSound, params.volume, Min(SNDLVL_NORM, params.soundlevel), SND_NOFLAGS, params.pitch);
}

void CRationDispenserProp::HandleAnimEvent(animevent_t* pEvent)
{
	switch (pEvent->event)
	{
	case EPositionEvent::FullyOpen:
	{
		if (mhContainedRation != NULL)
		{
			mhContainedRation->AllowPickup();
		}

		break;
	}
	case EPositionEvent::FullyClosed:
	{
		if (mhContainedRation != NULL)
		{
			mhContainedRation->DisallowPickup();
		}

		PropSetAnim("idle");
	}
	}
}
