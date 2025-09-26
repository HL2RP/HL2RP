// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_gamerules.h"

#define LOGIC_ATMOS_SF_ENABLED          1 // Used to detect controlling CVar changes. Internal only.
#define LOGIC_ATMOS_SF_ACTIVATE_HANDLED 2 // Prevents refiring OnActivate, for lifetime. Internal only.
#define LOGIC_ATMOS_SF_DAY_HANDLED      4 // Prevents refiring OnDay, within period. Internal only.
#define LOGIC_ATMOS_SF_NIGHT_HANDLED    8 // Prevents refiring OnNight, within period. Internal only.

#define LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT "UpdateLightValues"
#define LOGIC_ATMOS_LIGHT_UPDATE_PERIOD  300

SCOPED_ENUM(EPeakLightTime,
	MidDay,
	MidNight
)

extern ConVar gDayNightChangeHourCVar;

static ConVar sAtmosEnableCVar("sv_enable_atmos", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Enable computing atmospheric light values and firing outputs based on server time");

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
	void ForceUpdateLightValues(inputdata_t&);
	void FireOutput(COutputEvent&);
	void FireDayNightOutput(COutputEvent&, int handleSF, int unhandleSF);
	byte ComputeLightValue(const byte(&range)[EPeakLightTime::_Count], float scale);

	int mLastHour; // Last known server hour, to detect time transitions
	byte mLightAlpha[EPeakLightTime::_Count];
	string_t mLightPattern[EPeakLightTime::_Count];
	COutputInt mOnLightAlpha, mOnInverseLightAlpha;
	COutputString mOnLightPattern;
	COutputEvent mOnActivate, mOnDay, mOnNight, mOnMidDay, mOnMidNight;
};

LINK_ENTITY_TO_CLASS(logic_atmos, CLogicAtmos)

BEGIN_DATADESC(CLogicAtmos)
DEFINE_KEYFIELD_NOT_SAVED(mLightAlpha[EPeakLightTime::MidDay], FIELD_CHARACTER, "midday_alpha"),
DEFINE_KEYFIELD_NOT_SAVED(mLightAlpha[EPeakLightTime::MidNight], FIELD_CHARACTER, "midnight_alpha"),
DEFINE_KEYFIELD_NOT_SAVED(mLightPattern[EPeakLightTime::MidDay], FIELD_STRING, "midday_pattern"),
DEFINE_KEYFIELD_NOT_SAVED(mLightPattern[EPeakLightTime::MidNight], FIELD_STRING, "midnight_pattern"),

DEFINE_INPUTFUNC(FIELD_VOID, "ForceUpdateLightValues", ForceUpdateLightValues),

DEFINE_OUTPUT(mOnActivate, "OnActivate"),
DEFINE_OUTPUT(mOnLightAlpha, "OnLightAlpha"),
DEFINE_OUTPUT(mOnInverseLightAlpha, "OnInverseLightAlpha"),
DEFINE_OUTPUT(mOnLightPattern, "OnLightPattern"),

// Outputs called any time a transition happens or the entity is activated (once per 12-hour period)
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

	if (sAtmosEnableCVar.GetBool())
	{
		FullyActivate();
	}
}

void CLogicAtmos::FullyActivate()
{
	tm curTime;
	UTIL_GetServerTime(curTime);
	mLastHour = curTime.tm_hour;

	if (!HasSpawnFlags(LOGIC_ATMOS_SF_ACTIVATE_HANDLED))
	{
		FireOutput(mOnActivate);
		SetContextThink(&ThisClass::UpdateLightValues, TICK_INTERVAL, LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT);
	}

	AddSpawnFlags(LOGIC_ATMOS_SF_ENABLED | LOGIC_ATMOS_SF_ACTIVATE_HANDLED);

	// Setup main time period state via OnDay/OnNight outputs, preventing refiring same output within its 12-hour period
	if (HL2RPRules()->IsDayTime(curTime))
	{
		if (!HasSpawnFlags(LOGIC_ATMOS_SF_DAY_HANDLED))
		{
			FireDayNightOutput(mOnDay, LOGIC_ATMOS_SF_DAY_HANDLED, LOGIC_ATMOS_SF_NIGHT_HANDLED);
		}
	}
	else if (!HasSpawnFlags(LOGIC_ATMOS_SF_NIGHT_HANDLED))
	{
		FireDayNightOutput(mOnNight, LOGIC_ATMOS_SF_NIGHT_HANDLED, LOGIC_ATMOS_SF_DAY_HANDLED);
	}
}

void CLogicAtmos::Think()
{
	SetNextThink(TICK_INTERVAL);

	if (!sAtmosEnableCVar.GetBool())
	{
		if (HasSpawnFlags(LOGIC_ATMOS_SF_ENABLED))
		{
			RemoveSpawnFlags(LOGIC_ATMOS_SF_ENABLED);
			SetNextThink(TICK_NEVER_THINK, LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT);
		}

		return;
	}
	else if (!HasSpawnFlags(LOGIC_ATMOS_SF_ENABLED))
	{
		return FullyActivate(); // NOTE: We don't need to check for time outputs (below) now
	}

	tm curTime;
	UTIL_GetServerTime(curTime);

	if (curTime.tm_hour != mLastHour)
	{
		mLastHour = curTime.tm_hour;

		if (mLastHour == gDayNightChangeHourCVar.GetInt())
		{
			FireDayNightOutput(mOnDay, LOGIC_ATMOS_SF_DAY_HANDLED, LOGIC_ATMOS_SF_NIGHT_HANDLED);
		}
		else if (mLastHour == (gDayNightChangeHourCVar.GetInt() + 12) % 24)
		{
			FireDayNightOutput(mOnNight, LOGIC_ATMOS_SF_NIGHT_HANDLED, LOGIC_ATMOS_SF_DAY_HANDLED);
		}

		if (mLastHour == 12)
		{
			FireOutput(mOnMidDay);
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
	float scale = float(curTime.tm_hour % 12 * 60 + curTime.tm_min) / HALF_DAY_IN_MINUTES, inverseScale = 1.0f - scale;

	// If going midday, invert the scale (only midday-to-midnight transition fits the original scaling)
	if (curTime.tm_hour < 12)
	{
		V_swap(scale, inverseScale);
	}

	mOnLightAlpha.Set(ComputeLightValue(mLightAlpha, scale), this, this);
	mOnInverseLightAlpha.Set(ComputeLightValue(mLightAlpha, inverseScale), this, this);

	byte patternRange[] = { (byte)*mLightPattern[EPeakLightTime::MidDay].ToCStr(),
		(byte)*mLightPattern[EPeakLightTime::MidNight].ToCStr() };
	CFmtStrN<2> pattern("%c", ComputeLightValue(patternRange, scale));
	mOnLightPattern.Set(AllocPooledString(pattern), this, this);

	SetNextThink(gpGlobals->curtime + LOGIC_ATMOS_LIGHT_UPDATE_PERIOD, LOGIC_ATMOS_LIGHT_UPDATE_CONTEXT);
}

void CLogicAtmos::ForceUpdateLightValues(inputdata_t&)
{
	if (HasSpawnFlags(LOGIC_ATMOS_SF_ENABLED))
	{
		UpdateLightValues();
	}
}

void CLogicAtmos::FireOutput(COutputEvent& event)
{
	event.FireOutput(this, this);
}

void CLogicAtmos::FireDayNightOutput(COutputEvent& event, int handleSF, int unhandleSF)
{
	FireOutput(event);
	AddSpawnFlags(handleSF);
	RemoveSpawnFlags(unhandleSF);
}

byte CLogicAtmos::ComputeLightValue(const byte(&range)[EPeakLightTime::_Count], float scale)
{
	return (range[EPeakLightTime::MidDay] + (range[EPeakLightTime::MidNight] - range[EPeakLightTime::MidDay]) * scale);
}
