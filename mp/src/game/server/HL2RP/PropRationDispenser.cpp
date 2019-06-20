#include <cbase.h>
#include "PropRationDispenser.h"
#include "DAL/IDAO.h"
#include "HL2RP_Player.h"
#include "HL2RP_Util.h"
#include "Weapons/Ration.h"

#define DISPENSER_MODEL_PATH	"models/props_combine/combine_dispenser.mdl"
#define DISPENSER_SUPPLY_GLOW_SPRITE_PATH	"sprites/plasmaember.vmt"
#define DISPENSER_DENY_GLOW_SPRITE_PATH "sprites/redglow2.vmt"
#define DISPENSER_GLOW_SPRITE_SIZE	0.1f
#define DISPENSER_RATION_PICKUP_WAIT_SECONDS	1800.0f
#define DISPENSER_DENY_EFFECTS_REACTIVATE_DELAY	5.0f
#define DISPENSER_SUPPLY_SOUND	"buttons/button5.wav"
#define DISPENSER_DENY_SOUND	"npc/attack_helicopter/aheli_damaged_alarm1.wav"
#define DISPENSER_ANIMDONE_OUTPUT_TEXT	"OnAnimationDone !self,HandleClosed"

LINK_ENTITY_TO_CLASS(prop_ration_dispenser, CPropRationDispenser);

BEGIN_DATADESC(CPropRationDispenser)
DEFINE_INPUTFUNC(FIELD_VOID, "HandleClosed", InputHandleClosed),
DEFINE_KEYFIELD_NOT_SAVED(m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputEnable)
END_DATADESC()

CPropRationDispenser::CPropRationDispenser() : m_iDatabaseId(IDAO_INVALID_DATABASE_ID)
{
	inputdata_t inputdata;
	inputdata.value.SetString(MAKE_STRING(DISPENSER_ANIMDONE_OUTPUT_TEXT));
	InputAddOutput(inputdata);
}

void CPropRationDispenser::Precache()
{
	BaseClass::Precache();
	PrecacheModel(DISPENSER_MODEL_PATH);
	m_iSupplyGlowSpriteIndex = PrecacheModel(DISPENSER_SUPPLY_GLOW_SPRITE_PATH);
	m_iDenyGlowSpriteIndex = PrecacheModel(DISPENSER_DENY_GLOW_SPRITE_PATH);
	enginesound->PrecacheSound(DISPENSER_SUPPLY_SOUND);
	enginesound->PrecacheSound(DISPENSER_DENY_SOUND);
}

void CPropRationDispenser::Spawn()
{
	SetModelName(MAKE_STRING(DISPENSER_MODEL_PATH));
	SetSolid(SOLID_VPHYSICS);
	BaseClass::Spawn();
}

void CPropRationDispenser::CHL2RP_PlayerUse(const CHL2RP_Player& player)
{
	int glowSpriteIndex;
	Vector origin;

	if (m_bStartDisabled || !UTIL_IsSavedEntity(this, m_iDatabaseId)
		|| m_flNextUseableTime > gpGlobals->curtime || IsOpen())
	{
		return;
	}
	else if (player.m_iCrime < 1)
	{
		if (m_hContainedRation == NULL)
		{
			if (engine->GetEntityCount() + 1 >= MAX_EDICTS)
			{
				return;
			}

			// A minimum required to spawn, asumming this attachment is correctly placed within the plate
			int attachment = LookupAttachment("Ration_attachment");

			if (GetAttachment(attachment, origin))
			{
				m_hContainedRation = static_cast<CRation*>(CreateNoSpawn("ration", origin, GetAbsAngles()));

				if (m_hContainedRation == NULL)
				{
					return;
				}

#ifdef HL2DM_RP
				// HL2DM clients may not have the new 'Ration_attachment', so before parenting,
				// we switch to a default one for the Ration to appear aligned there
				attachment = LookupAttachment("Package_attachment");
#endif // HL2DM_RP

				m_hContainedRation->SetParent(this, attachment);
				m_hContainedRation->m_hParentDispenser = this;
				m_hContainedRation->AddSpawnFlags(SF_NORESPAWN);
				DispatchSpawn(m_hContainedRation);
				m_hContainedRation->SetContextThink(&CRation::EnsureVisible,
					gpGlobals->curtime + TICK_INTERVAL, RATION_ENSURE_VISIBLE_CONTEXT_NAME);
				m_hContainedRation->SetSecondaryAmmoCount(1);
			}
		}
		else
		{
			// Unprotect Ration
			m_hContainedRation->RemoveSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
		}

		PropSetAnim("dispense_package");
		CPASAttenuationFilter soundFilter;
		soundFilter.AddAllPlayers();
		enginesound->EmitSound(soundFilter, entindex(), CHAN_AUTO, DISPENSER_SUPPLY_SOUND,
			VOL_NORM, SNDLVL_NORM);
		glowSpriteIndex = m_iSupplyGlowSpriteIndex;
	}
	else if (m_flNextEffectsReadyTime <= gpGlobals->curtime)
	{
		// React to criminal
		CPASAttenuationFilter soundFilter;
		soundFilter.AddAllPlayers();
		enginesound->EmitSound(soundFilter, entindex(), CHAN_AUTO, DISPENSER_DENY_SOUND,
			VOL_NORM, SNDLVL_NORM);
		m_flNextEffectsReadyTime = gpGlobals->curtime + DISPENSER_DENY_EFFECTS_REACTIVATE_DELAY;
		glowSpriteIndex = m_iDenyGlowSpriteIndex;
	}
	else
	{
		return;
	}

	if (GetAttachment("Sprite_Attachment", origin))
	{
		CBroadcastRecipientFilter filter;
		filter.SetIgnorePredictionCull(true);
		te->GlowSprite(filter, 0.0f, &origin, glowSpriteIndex, SequenceDuration(),
			DISPENSER_GLOW_SPRITE_SIZE, 255);
	}
}

void CPropRationDispenser::OnContainedRationPickup()
{
	m_flNextUseableTime = gpGlobals->curtime + DISPENSER_RATION_PICKUP_WAIT_SECONDS;

	if (IsOpen())
	{
		m_flNextUseableTime += SequenceDuration();
	}
}

FORCEINLINE bool CPropRationDispenser::IsOpen()
{
	return (SequenceDuration() > 0.0f);
}

void CPropRationDispenser::InputEnable(inputdata_t& inputdata)
{
	m_bStartDisabled = false; // We treat it as current disabled value
}

void CPropRationDispenser::InputDisable(inputdata_t& inputdata)
{
	m_bStartDisabled = true; // We treat it as current disabled value
}

// Handles actions on closed, to avoid calculating the finish sequence time of 'dispense_package'
void CPropRationDispenser::InputHandleClosed(inputdata_t& inputdata)
{
	// Only proceed if input was self-activated
	if (inputdata.pCaller == this)
	{
		// Indicate we are now closed, for our logic
		ResetSequence(0);

		if (m_hContainedRation != NULL)
		{
			// Protect Ration against possible unmanaged 'dispense_package' triggers
			m_hContainedRation->AddSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP);
		}
	}
}
