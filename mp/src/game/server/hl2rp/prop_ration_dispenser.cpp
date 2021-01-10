// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include <prop_ration_dispenser.h>
#include "hl2_roleplayer.h"
#include <ration.h>
#include <npcevent.h>

#define RATION_DISPENSER_SF_COMBINE_CONTROLLED 512

#define RATION_DISPENSER_GLOW_SPRITE_SIZE 0.1f

#define RATION_DISPENSER_REACTION_EFFECTS_COOLDOWN 5.0f

ENUM(ERationDispenserEvent,
	FullyOpen,
	FullyClosed
)

void CRationDispenserProp::Use(CBaseEntity* pActivator, CBaseEntity*, USE_TYPE, float)
{
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(pActivator);

	if (pPlayer != NULL && gpGlobals->curtime >= mNextTimeAvailable && SequenceDuration() <= 0.0f)
	{
		if (pPlayer->mCrime < 1)
		{
			if (mhContainedRation == NULL)
			{
				// Require the presence of ration attachment within plate to spawn, as a good minimum
				int attachment = LookupAttachment("Ration_attachment");
				Vector origin;

				if (GetAttachment(attachment, origin))
				{
#ifdef HL2RP_LEGACY
					// HL2DM clients may not have the new ration attachment, so, before parenting,
					// we switch to the legacy one for the ration to follow the dispenser's plate
					attachment = LookupAttachment("Package_attachment");
#endif // HL2RP_LEGACY

					CRation* pRation = static_cast<CRation*>(Create("ration", origin, vec3_angle));
					pRation->AddSpawnFlags(SF_NORESPAWN);
					pRation->SetParent(this, attachment);
					pRation->SetMoveType(MOVETYPE_NONE); // Prevent VPhysics warnings when parented
					pRation->SetPrimaryAmmoCount(1);
					pRation->m_iClip1 = 0;
					pRation->mhParentDispenser = this;
					mhContainedRation = pRation;
				}
			}

			PropSetAnim("dispense_package");
			DoReactionEffects(RATION_DISPENSER_SUPPLY_GLOW_SPRITE_PATH, RATION_DISPENSER_SUPPLY_SOUND);
		}
		else if (mReactionEffectsAllowTimer.Expired())
		{
			// React to criminal
			DoReactionEffects(RATION_DISPENSER_DENY_GLOW_SPRITE_PATH, RATION_DISPENSER_DENY_SOUND);
			mReactionEffectsAllowTimer.Set(RATION_DISPENSER_REACTION_EFFECTS_COOLDOWN);
		}
	}
}

void CRationDispenserProp::DoReactionEffects(const char* pSprite, const char* pSound)
{
	Vector origin;

	if (GetAttachment("Sprite_attachment", origin))
	{
		CPVSFilter filter(origin);
		filter.SetIgnorePredictionCull(true);
		te->GlowSprite(filter, 0.0f, &origin, engine->PrecacheModel(pSprite),
			SequenceDuration(), RATION_DISPENSER_GLOW_SPRITE_SIZE, 255);
		engine->EmitAmbientSound(entindex(), vec3_origin, pSound, VOL_NORM, SNDLVL_NORM, SND_NOFLAGS, PITCH_NORM);
	}
}

void CRationDispenserProp::HandleAnimEvent(animevent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ERationDispenserEvent::FullyOpen:
	{
		if (mhContainedRation != NULL)
		{
			mhContainedRation->AllowPickup();
		}

		break;
	}
	case ERationDispenserEvent::FullyClosed:
	{
		if (mhContainedRation != NULL)
		{
			mhContainedRation->DisallowPickup();
		}

		PropSetAnim("idle");
	}
	}
}
