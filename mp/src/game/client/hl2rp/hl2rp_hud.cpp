// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2_roleplayer.h"
#include <prop_ration_dispenser.h>
#include <hudelement.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include <iclientmode.h>

using namespace vgui;

class CHL2RPHUD : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHL2RPHUD, EditablePanel)

	CPanelAnimationVar(Color, mTextColor, "textColor", "");
	bool mCenterHorizontally = false;

protected:
	CHL2RPHUD(const char* pElementName, const char* pPanelName);

	bool IsSettingEmpty(KeyValues* pSettings, const char*);
	void ApplySettings(KeyValues* pSettings, int defaultXPos, int defaultYPos, const Color& defaultTextColor);
	void Paint(const wchar_t* pText);
	void Paint(const wchar_t* pText, const Color&);

};

CHL2RPHUD::CHL2RPHUD(const char* pElementName, const char* pPanelName) : CHudElement(pElementName),
BaseClass(g_pClientMode->GetViewport(), pPanelName)
{
	SetPaintBackgroundEnabled(false);
}

bool CHL2RPHUD::IsSettingEmpty(KeyValues* pSettings, const char* pSetting)
{
	return FStrEq(pSettings->GetString(pSetting), "");
}

void CHL2RPHUD::ApplySettings(KeyValues* pSettings, int defaultXPos, int defaultYPos, const Color& defaultTextColor)
{
	if (IsSettingEmpty(pSettings, "xpos"))
	{
		pSettings->SetInt("xpos", defaultXPos);
	}

	if (IsSettingEmpty(pSettings, "ypos"))
	{
		pSettings->SetInt("ypos", defaultYPos);
	}

	BaseClass::ApplySettings(pSettings);

	if (GetXPos() < 0)
	{
		mCenterHorizontally = true;
	}

	if (IsSettingEmpty(pSettings, "TextColor"))
	{
		mTextColor = defaultTextColor;
	}
}

void CHL2RPHUD::Paint(const wchar_t* pText)
{
	Paint(pText, mTextColor);
}

void CHL2RPHUD::Paint(const wchar_t* pText, const Color& color)
{
	int wide, tall;
	surface()->GetTextSize(g_hFontTrebuchet24, pText, wide, tall);
	SetSize(wide, tall);
	surface()->DrawSetTextFont(g_hFontTrebuchet24);
	surface()->DrawSetTextColor(color);

	if (mCenterHorizontally)
	{
		SetPos((ScreenWidth() - wide) / 2, GetYPos());
	}

	// Draw each line delimited by new line characters separately, since VGUI2 doesn't handle this
	for (int yOffset = 0; pText != NULL; yOffset += surface()->GetFontTall(g_hFontTrebuchet24))
	{
		const wchar_t* pLineBreak = wcschr(pText, L'\n'), * pLine = pText;
		int lineLen;

		if (pLineBreak != NULL)
		{
			lineLen = pLineBreak - pLine;
			pText = pLineBreak + 1;
		}
		else
		{
			lineLen = Q_wcslen(pLine);
			pText = NULL;
		}

		surface()->DrawSetTextPos(0, yOffset);
		surface()->DrawPrintText(pLine, lineLen);
	}
}

class CHL2RPMainHUD : public CHL2RPHUD
{
	DECLARE_CLASS(CHL2RPMainHUD, CHL2RPHUD)

	void ApplySettings(KeyValues*) OVERRIDE;
	void Paint() OVERRIDE;

	CPanelAnimationVar(Color, mCriminalTextColor, "criminalTextColor", "");

public:
	CHL2RPMainHUD(const char* pElementName);
};

CHL2RPMainHUD::CHL2RPMainHUD(const char* pElementName) : CHL2RPHUD(pElementName, "MainHUD")
{

}

void CHL2RPMainHUD::ApplySettings(KeyValues* pSettings)
{
	BaseClass::ApplySettings(pSettings, HL2RP_MAIN_HUD_DEFAULT_X_POS, HL2RP_MAIN_HUD_DEFAULT_Y_POS,
		HL2RP_MAIN_HUD_DEFAULT_NORMAL_TEXT_COLOR);

	if (IsSettingEmpty(pSettings, "CriminalTextColor"))
	{
		mCriminalTextColor = HL2RP_MAIN_HUD_DEFAULT_CRIMINAL_TEXT_COLOR;
	}
}

void CHL2RPMainHUD::Paint()
{
	localizebuf_t text;
	GetLocalHL2Roleplayer()->ComputeMainHUD(text);
	Color& color = (GetLocalHL2Roleplayer()->mCrime > 0) ? mCriminalTextColor : mTextColor;
	BaseClass::Paint(text, color);
}

class CHL2RPAimingEntityHUD : public CHL2RPHUD
{
	DECLARE_CLASS(CHL2RPAimingEntityHUD, CHL2RPHUD)

	void ApplySettings(KeyValues*) OVERRIDE;
	void Paint() OVERRIDE;

	CPanelAnimationVar(Color, mPlayerTextColor, "playerTextColor", "");

public:
	CHL2RPAimingEntityHUD(const char* pElementName);
};

CHL2RPAimingEntityHUD::CHL2RPAimingEntityHUD(const char* pElementName) : CHL2RPHUD(pElementName, "AimingEntityHUD")
{

}

void CHL2RPAimingEntityHUD::ApplySettings(KeyValues* pSettings)
{
	BaseClass::ApplySettings(pSettings, HL2RP_CENTER_HUD_SPECIAL_POS, HL2RP_AIMING_HUD_DEFAULT_Y_POS,
		HL2RP_AIMING_HUD_DEFAULT_GENERAL_COLOR);

	if (IsSettingEmpty(pSettings, "PlayerTextColor"))
	{
		mPlayerTextColor = HL2RP_AIMING_HUD_DEFAULT_PLAYER_COLOR;
	}
}

void CHL2RPAimingEntityHUD::Paint()
{
	C_HL2Roleplayer* pPlayer = GetLocalHL2Roleplayer();
	localizebuf_t text;

	if (pPlayer->ComputeAimingEntityAndHUD(text))
	{
		if (pPlayer->mhAimingEntity->IsPlayer())
		{
			return BaseClass::Paint(text, mPlayerTextColor);
		}

		BaseClass::Paint(text);
	}
}

DECLARE_HUDELEMENT(CHL2RPMainHUD);
DECLARE_HUDELEMENT(CHL2RPAimingEntityHUD);
