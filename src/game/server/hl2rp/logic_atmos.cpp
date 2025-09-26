// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gamerules.h"

#define LOGIC_ATMOS_SF_DISABLED 1 // Used to detect global CVar activation. Internal only.

#define LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT "UpdateLightValues"
#define LOGIC_ATMOS_LIGHT_UPDATE_PERIOD  300

#define HALF_DAY_IN_MINUTES 720

SCOPED_ENUM(EPeakLightTime,
	MidDay,
	MidNight
);

extern ConVar gDayNightChangeHourCVar;

static ConVar sAtmosEventsEnableCVar("sv_enable_atmos_events", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Enable computing atmospheric light values and generating outputs, based on server time");

// Entity that computes atmospheric values based on server's IRL time,
// and is able to forward them to environment-like entities, ideally.
// Also fires specific outputs at relevant times, without parameters.
class CLogicAtmos : public CLogicalEntity
{
	DECLARE_CLASS(CLogicAtmos, CLogicalEntity)
	DECLARE_DATADESC()

	void Activate() OVERRIDE;
	void Think() OVERRIDE;

	void FullyActivate();
	void UpdateLightValues();
	void FireOutput(COutputEvent&, float delay = 0.0f);
	byte ComputeLightValue(const tm&, float scale, const byte(&peakValues)[EPeakLightTime::_Count]);

	int mLastHour;
	byte mLightAlpha[EPeakLightTime::_Count];
	string_t mLightPattern[EPeakLightTime::_Count];
	COutputInt mOnLightAlpha;
	COutputString mOnLightPattern;
	COutputEvent mOnActivate, mOnDay, mOnNight, mOnMidDay, mOnMidNight;
};

LINK_ENTITY_TO_CLASS(logic_atmos, CLogicAtmos)

BEGIN_DATADESC(CLogicAtmos)
DEFINE_KEYFIELD_NOT_SAVED(mLightAlpha[EPeakLightTime::MidDay], FIELD_CHARACTER, "midday_alpha"),
DEFINE_KEYFIELD_NOT_SAVED(mLightAlpha[EPeakLightTime::MidNight], FIELD_CHARACTER, "midnight_alpha"),
DEFINE_KEYFIELD_NOT_SAVED(mLightPattern[EPeakLightTime::MidDay], FIELD_STRING, "midday_pattern"),
DEFINE_KEYFIELD_NOT_SAVED(mLightPattern[EPeakLightTime::MidNight], FIELD_STRING, "midnight_pattern"),

DEFINE_OUTPUT(mOnActivate, "OnActivate"),
DEFINE_OUTPUT(mOnLightAlpha, "OnLightAlpha"),
DEFINE_OUTPUT(mOnLightPattern, "OnLightPattern"),

// Outputs called any time the transition happens or the entity is activated
DEFINE_OUTPUT(mOnDay, "OnDay"),
DEFINE_OUTPUT(mOnNight, "OnNight"),

// Outputs called only upon transition (strict)
DEFINE_OUTPUT(mOnMidDay, "OnMidDay"),
DEFINE_OUTPUT(mOnMidNight, "OnMidNight")
END_DATADESC()

// NOTE: Used instead of Spawn to ensure target entities from outputs are ready
void CLogicAtmos::Activate()
{
	BaseClass::Activate();
	SetNextThink(TICK_INTERVAL);

	if (sAtmosEventsEnableCVar.GetBool())
	{
		FullyActivate();
	}
}

void CLogicAtmos::FullyActivate()
{
	tm curTime;
	UTIL_GetServerTime(curTime);
	mLastHour = curTime.tm_hour;

	FireOutput(mOnActivate, TICK_INTERVAL);
	SetContextThink(&ThisClass::UpdateLightValues, TICK_INTERVAL, LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT);

	// Setup main state via OnDay/OnNight outputs
	FireOutput(HL2RPRules()->IsDayTime(curTime) ? mOnDay : mOnMidNight, TICK_INTERVAL);
}

void CLogicAtmos::Think()
{
	SetNextThink(TICK_INTERVAL);

	if (!sAtmosEventsEnableCVar.GetBool())
	{
		AddSpawnFlags(LOGIC_ATMOS_SF_DISABLED);
		return SetNextThink(TICK_NEVER_THINK, LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT);
	}
	else if (HasSpawnFlags(LOGIC_ATMOS_SF_DISABLED))
	{
		RemoveSpawnFlags(LOGIC_ATMOS_SF_DISABLED);
		FullyActivate();
	}

	tm curTime;
	UTIL_GetServerTime(curTime);

	if (curTime.tm_hour != mLastHour)
	{
		mLastHour = curTime.tm_hour;

		if (mLastHour == gDayNightChangeHourCVar.GetInt())
		{
			FireOutput(mOnDay);
		}
		else if (mLastHour == 12)
		{
			FireOutput(mOnMidDay);
		}
		else if (mLastHour == gDayNightChangeHourCVar.GetInt() + 12)
		{
			FireOutput(mOnNight);
		}
		else if (mLastHour < 1)
		{
			FireOutput(mOnMidNight);
		}
	}
}

void CLogicAtmos::UpdateLightValues()
{
	tm curTime;
	UTIL_GetServerTime(curTime);
	float scale = float(curTime.tm_hour % 12 * 60 + curTime.tm_min) / HALF_DAY_IN_MINUTES;

	// If going midday, reverse the scale (only midday-to-midnight transition fits the original scaling)
	if (curTime.tm_hour < 12)
	{
		scale = 1.0f - scale;
	}

	byte alpha = ComputeLightValue(curTime, scale, mLightAlpha), patternRange[]
		= { *mLightPattern[EPeakLightTime::MidDay].ToCStr(), *mLightPattern[EPeakLightTime::MidNight].ToCStr() };
	mOnLightAlpha.Set(alpha, this, this);
	CFmtStrN<2> pattern("%c", ComputeLightValue(curTime, scale, patternRange));
	mOnLightPattern.Set(AllocPooledString(pattern), this, this);

	SetNextThink(gpGlobals->curtime + LOGIC_ATMOS_LIGHT_UPDATE_PERIOD, LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT);
}

void CLogicAtmos::FireOutput(COutputEvent& event, float delay)
{
	event.FireOutput(this, this, delay);
}

byte CLogicAtmos::ComputeLightValue(const tm& targetTime, float scale, const byte(&range)[EPeakLightTime::_Count])
{
	return (range[EPeakLightTime::MidDay] + (range[EPeakLightTime::MidNight] - range[EPeakLightTime::MidDay]) * scale);
}
