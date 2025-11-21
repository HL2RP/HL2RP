// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2_roleplayer.h"
#include <hl2rp_localizer.h>
#include <hudelement.h>
#include <iclientmode.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>

#ifdef WIN32
#define wcstok wcstok_s
#endif // WIN32

using namespace vgui;

class CHL2RPHUD : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE(CHL2RPHUD, Panel)

	CPanelAnimationVar(Color, mTextColor, "textColor", "");
	int mTextPosition[2]; // Desired text position (relative to screen), equivalent to the loaded panel's

protected:
	CHL2RPHUD(const char* pElementName, const char* pPanelName);

	bool IsSettingEmpty(KeyValues* pSettings, const char*);
	void ApplySettings(KeyValues* pSettings, float defaultXPos, float defaultYPos, const Color& defaultTextColor);
	void Paint(wchar_t*, const Color&);
	void Paint(wchar_t*, const Color& headerColor, const Color& linesColor, int topSpacersCount = 0);
};

CHL2RPHUD::CHL2RPHUD(const char* pElementName, const char* pPanelName)
	: CHudElement(pElementName), BaseClass(g_pClientMode->GetViewport(), pPanelName)
{
	SetPaintBackgroundEnabled(false);
}

bool CHL2RPHUD::IsSettingEmpty(KeyValues* pSettings, const char* pSetting)
{
	return FStrEq(pSettings->GetString(pSetting), "");
}

void CHL2RPHUD::ApplySettings(KeyValues* pSettings, float defaultXPos, float defaultYPos, const Color& defaultTextColor)
{
	int screenSize[2];
	GetHudSize(screenSize[0], screenSize[1]);

	if (IsSettingEmpty(pSettings, "xPos"))
	{
		// Set a scale-based position (unless requesting center) to prevent base class from computing a wrong one
		pSettings->SetString("xPos",
			VarArgs("%s%f", defaultXPos == HL2RP_CENTER_HUD_SPECIAL_POS ? "" : "p", defaultXPos));
	}
	else
	{
		defaultXPos = pSettings->GetFloat("xPos");
	}

	if (IsSettingEmpty(pSettings, "yPos"))
	{
		pSettings->SetString("yPos", VarArgs("p%f", defaultYPos)); // Similar to above
	}

	BaseClass::ApplySettings(pSettings);
	GetPos(mTextPosition[0], mTextPosition[1]);

	if (defaultXPos == HL2RP_CENTER_HUD_SPECIAL_POS)
	{
		mTextPosition[0] = defaultXPos;
	}

	// Match screen bounds as otherwise updating them to minimally wrap the
	// text in Paint creates flickering and ephemeral incorrect positions
	SetBounds(0, 0, screenSize[0], screenSize[1]);

	if (IsSettingEmpty(pSettings, "textColor"))
	{
		mTextColor = defaultTextColor;
	}
}

void CHL2RPHUD::Paint(wchar_t* pText, const Color& color)
{
	Paint(pText, color, color);
}

void CHL2RPHUD::Paint(wchar_t* pText, const Color& headerColor, const Color& linesColor, int topSpacersCount)
{
	int screenSize[2], textSize[2];
	GetHudSize(screenSize[0], screenSize[1]);
	surface()->GetTextSize(g_hFontTrebuchet24, pText, textSize[0], textSize[1]);
	surface()->DrawSetTextFont(g_hFontTrebuchet24);
	surface()->DrawSetTextColor(headerColor);
	int yPos = Min(mTextPosition[1] + surface()->GetFontTall(g_hFontTrebuchet24)
		* topSpacersCount, screenSize[1] - textSize[1]);

	// Draw each line delimited by new line characters separately, since VGUI2 doesn't handle this
	for (wchar_t* pLine = pText; (pLine = wcstok(NULL, L"\n", &pText)) != NULL; yPos += textSize[1])
	{
		surface()->GetTextSize(g_hFontTrebuchet24, pLine, textSize[0], textSize[1]);
		surface()->DrawSetTextPos(mTextPosition[0] == HL2RP_CENTER_HUD_SPECIAL_POS ?
			(screenSize[0] - textSize[0]) / 2 : Min(mTextPosition[0], screenSize[0] - textSize[0]), yPos);
		surface()->DrawPrintText(pLine, Q_wcslen(pLine));
		surface()->DrawSetTextColor(linesColor);
	}
}

class CHL2RPMainHUD : public CHL2RPHUD
{
	DECLARE_CLASS(CHL2RPMainHUD, CHL2RPHUD)

	void ApplySettings(KeyValues*) OVERRIDE;
	void Paint() OVERRIDE;

	CPanelAnimationVar(Color, mCriminalTextColor, "criminalTextColor", "");

public:
	CHL2RPMainHUD(const char* pElementName) : BaseClass(pElementName, "MainHUD") {}
};

void CHL2RPMainHUD::ApplySettings(KeyValues* pSettings)
{
	BaseClass::ApplySettings(pSettings, HL2RP_MAIN_HUD_DEFAULT_X_POS,
		HL2RP_MAIN_HUD_DEFAULT_Y_POS, HL2RP_MAIN_HUD_DEFAULT_NORMAL_TEXT_COLOR);

	if (IsSettingEmpty(pSettings, "criminalTextColor"))
	{
		mCriminalTextColor = HL2RP_MAIN_HUD_DEFAULT_CRIMINAL_TEXT_COLOR;
	}
}

void CHL2RPMainHUD::Paint()
{
	CLocalizeFmtStr<> text;
	GetLocalHL2Roleplayer()->GetMainHUD(text);
	Color& color = (GetLocalHL2Roleplayer()->mCrime > 0) ? mCriminalTextColor : mTextColor;
	BaseClass::Paint(text.mDest, color);
}

class CHL2RPZoneHUD : public CHL2RPHUD
{
	DECLARE_CLASS(CHL2RPZoneHUD, CHL2RPHUD)

	void ApplySettings(KeyValues*) OVERRIDE;
	void Paint() OVERRIDE;

public:
	CHL2RPZoneHUD(const char* pElementName) : BaseClass(pElementName, "ZoneHUD") {}
};

void CHL2RPZoneHUD::ApplySettings(KeyValues* pSettings)
{
	BaseClass::ApplySettings(pSettings, HL2RP_CENTER_HUD_SPECIAL_POS,
		HL2RP_ZONE_HUD_DEFAULT_Y_POS, HL2RP_ZONE_HUD_DEFAULT_COLOR);
}

void CHL2RPZoneHUD::Paint()
{
	CLocalizeFmtStr<> text;

	if (GetLocalHL2Roleplayer()->IsAlive() && GetLocalHL2Roleplayer()->GetZoneHUD(text))
	{
		BaseClass::Paint(text.mDest, mTextColor);
	}
}

class CHL2RPAimingEntityHUD : public CHL2RPHUD
{
	DECLARE_CLASS(CHL2RPAimingEntityHUD, CHL2RPHUD)

	void ApplySettings(KeyValues*) OVERRIDE;
	void ProcessInput() OVERRIDE;
	void Paint() OVERRIDE;

	CPanelAnimationVar(Color, mPlayerTextColor, "playerTextColor", "");
	EHANDLE mhEntity;
	float mEntityDistance;

public:
	CHL2RPAimingEntityHUD(const char* pElementName) : BaseClass(pElementName, "AimingEntityHUD") {}
};

void CHL2RPAimingEntityHUD::ApplySettings(KeyValues* pSettings)
{
	BaseClass::ApplySettings(pSettings, HL2RP_CENTER_HUD_SPECIAL_POS,
		HL2RP_AIMING_HUD_DEFAULT_Y_POS, HL2RP_AIMING_HUD_DEFAULT_GENERAL_COLOR);

	if (IsSettingEmpty(pSettings, "playerTextColor"))
	{
		mPlayerTextColor = HL2RP_AIMING_HUD_DEFAULT_PLAYER_COLOR;
	}
}

void CHL2RPAimingEntityHUD::ProcessInput()
{
	mhEntity.Term();

	if (GetLocalHL2Roleplayer()->IsAlive())
	{
		SPlayerAimInfo info;
		GetLocalHL2Roleplayer()->GetAimInfo(info);
		mhEntity = info.mhMainEntity;
		mEntityDistance = info.mEndDistance;
	}
}

void CHL2RPAimingEntityHUD::Paint()
{
	if (mhEntity != NULL && mEntityDistance < PLAYER_USE_RADIUS)
	{
		CLocalizeFmtStr<> text;
		mhEntity->GetHUDInfo(GetLocalHL2Roleplayer(), text);

		if (text.mLength > 0)
		{
			BaseClass::Paint(text.mDest, mhEntity->IsPlayer() ? mPlayerTextColor : mTextColor);
		}
	}
}

class CHL2RPRegionHUD : public CHL2RPHUD
{
	DECLARE_CLASS(CHL2RPRegionHUD, CHL2RPHUD)

	void ApplySettings(KeyValues*) OVERRIDE;
	void ProcessInput() OVERRIDE;
	void Paint() OVERRIDE;

	CPanelAnimationVar(Color, mHeaderTextColor, "headerTextColor", "");
	CUtlVector<C_BasePlayer*> mPlayers;

public:
	CHL2RPRegionHUD(const char* pElementName) : BaseClass(pElementName, "RegionHUD") {}
};

void CHL2RPRegionHUD::ApplySettings(KeyValues* pSettings)
{
	BaseClass::ApplySettings(pSettings, HL2RP_COMM_HUD_DEFAULT_X_POS,
		HL2RP_COMM_HUD_DEFAULT_Y_POS, HL2RP_REGION_HUD_PLAYERS_DEFAULT_COLOR);

	if (IsSettingEmpty(pSettings, "headerTextColor"))
	{
		mHeaderTextColor = HL2RP_REGION_HUD_HEADER_DEFAULT_COLOR;
	}
}

void CHL2RPRegionHUD::ProcessInput()
{
	mPlayers.Purge();

	if (GetLocalHL2Roleplayer()->IsAlive())
	{
		GetLocalHL2Roleplayer()->GetPlayersInRegion(mPlayers);
	}
}

void CHL2RPRegionHUD::Paint()
{
	CLocalizeFmtStr<> text;

	if (!mPlayers.IsEmpty())
	{
		int count = GetLocalHL2Roleplayer()->GetRegionHUD(mPlayers, text);
		BaseClass::Paint(text.mDest, mHeaderTextColor, mTextColor, HL2_ROLEPLAYER_REGION_MAX_PLAYERS - count);
	}
}

DECLARE_HUDELEMENT(CHL2RPMainHUD)
DECLARE_HUDELEMENT(CHL2RPZoneHUD)
DECLARE_HUDELEMENT(CHL2RPAimingEntityHUD)
DECLARE_HUDELEMENT(CHL2RPRegionHUD)
