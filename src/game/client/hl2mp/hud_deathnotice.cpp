//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

// Player entries in a death notice
struct DeathNoticeEntity
{
	char		szName[MAX_PLAYER_NAME_LENGTH];
	int			iEntIndex;

#ifdef HL2RP
	int mTeamNum; // To safely preserve team color once related NPC (if any) dies
#endif // HL2RP
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticeEntity	Killer;
	DeathNoticeEntity   Victim;
	CHudTexture *iconDeath;
	int			iSuicide;
	float		flDisplayTime;
	bool		bHeadshot;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDeathNotice, vgui::Panel );

public:
	CHudDeathNotice( const char *pElementName );

	void Init( void );
	void VidInit( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	void SetColorForNoticeEntity(const DeathNoticeEntity&);
	void RetireExpiredDeathNotices( void );
	
	virtual void FireGameEvent( IGameEvent * event );

private:
	void GetEventEntityInfo(IGameEvent*, DeathNoticeEntity&, const char* pEntIndexKey);

	CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "15", "proportional_float" );

	CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "4" );

	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudNumbersTimer" );

	// Texture for skull symbol
	CHudTexture		*m_iconD_skull;  
	CHudTexture		*m_iconD_headshot;  

	CUtlVector<DeathNoticeItem> m_DeathNotices;
};

using namespace vgui;

DECLARE_HUDELEMENT( CHudDeathNotice );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDeathNotice::CHudDeathNotice( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_iconD_headshot = NULL;
	m_iconD_skull = NULL;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );

	SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Init( void )
{
	ListenForGameEvent( "player_death" );

#ifdef HL2RP
	ListenForGameEvent("npc_death");
#endif // HL2RP
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::VidInit( void )
{
	m_iconD_skull = gHUD.GetIcon( "d_skull" );
	m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::SetColorForNoticeEntity(const DeathNoticeEntity& entityData)
{
	int team = 0;

#ifdef HL2RP
	if (entityData.iEntIndex > gpGlobals->maxClients)
	{
		CBaseEntity* pCharacter = ToBaseCombatCharacter(ClientEntityList().GetBaseEntity(entityData.iEntIndex));
		team = (pCharacter != NULL) ? pCharacter->GetTeamNumber() : entityData.mTeamNum;
	}
	else
#endif // HL2RP
	{
		if (g_PR != NULL)
		{
			team = g_PR->GetTeam(entityData.iEntIndex);
		}
	}

	surface()->DrawSetTextColor( GameResources()->GetTeamColor( team ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Paint()
{
	if ( !m_iconD_skull )
		return;

	int yStart = GetClientModeHL2MPNormal()->GetDeathMessageStartHeight();

	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( GameResources()->GetTeamColor( 0 ) );


	int iCount = m_DeathNotices.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CHudTexture *icon = m_DeathNotices[i].iconDeath;
		if ( !icon )
			continue;

		wchar_t victim[ 256 ];
		wchar_t killer[ 256 ];

		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Killer.szName, killer, sizeof( killer ) );

		int nLinePadding = vgui::scheme()->GetProportionalScaledValue( 4 );

		// Get the local position for this notice
		int len = UTIL_ComputeStringWidth( m_hTextFont, victim );
		int y = yStart + ( ( m_flLineHeight + nLinePadding ) * i);

		int iconWide;
		int iconTall;

		if( icon->bRenderUsingFont )
		{
			iconWide = surface()->GetCharacterWidth( icon->hFont, icon->cCharacterInFont );
			iconTall = surface()->GetFontTall( icon->hFont );
		}
		else
		{
			float scale = ( (float)ScreenHeight() / 480.0f );	//scale based on 640x480
			iconWide = (int)( scale * (float)icon->Width() );
			iconTall = (int)( scale * (float)icon->Height() );
		}

		// misyl: Looks bad all crunched up in the corner.
		int nPadding = vgui::scheme()->GetProportionalScaledValue( 16 );

		int x;
		if ( m_bRightJustify )
		{
			x =	GetWide() - len - iconWide - nPadding;
		}
		else
		{
			x = nPadding;
		}
		
		// Only draw killers name if it wasn't a suicide
		if ( !m_DeathNotices[i].iSuicide )
		{
			if ( m_bRightJustify )
			{
				x -= UTIL_ComputeStringWidth( m_hTextFont, killer );
			}

			SetColorForNoticeEntity(m_DeathNotices[i].Killer);

			// Draw killer's name
			surface()->DrawSetTextPos( x, y );
			surface()->DrawSetTextFont( m_hTextFont );
			surface()->DrawUnicodeString( killer );
			surface()->DrawGetTextPos( x, y );
		}

		Color iconColor( 255, 80, 0, 255 );

		// Draw death weapon
		//If we're using a font char, this will ignore iconTall and iconWide
		icon->DrawSelf( x, y, iconWide, iconTall, iconColor );
		x += iconWide;		

		SetColorForNoticeEntity(m_DeathNotices[i].Victim);

		// Draw victims name
		surface()->DrawSetTextPos( x, y );
		surface()->DrawSetTextFont( m_hTextFont );	//reset the font, draw icon can change it
		surface()->DrawUnicodeString( victim );
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudDeathNotice::RetireExpiredDeathNotices( void )
{
	// Loop backwards because we might remove one
	int iSize = m_DeathNotices.Size();
	for ( int i = iSize-1; i >= 0; i-- )
	{
		if ( m_DeathNotices[i].flDisplayTime < gpGlobals->curtime )
		{
			m_DeathNotices.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CHudDeathNotice::FireGameEvent( IGameEvent * event )
{
	if (!g_PR)
		return;

	if ( hud_deathnotice_time.GetFloat() == 0 )
		return;

	DeathNoticeItem deathMsg;

	// the event should be "player_death"
	deathMsg.Killer.iEntIndex = engine->GetPlayerForUserID( event->GetInt("attacker") );
	deathMsg.Victim.iEntIndex = engine->GetPlayerForUserID( event->GetInt("userid") );
	const char *killedwith = event->GetString( "weapon" );

	char fullkilledwith[128];
	if ( killedwith && *killedwith )
	{
		Q_snprintf( fullkilledwith, sizeof(fullkilledwith), "death_%s", killedwith );
	}
	else
	{
		fullkilledwith[0] = 0;
	}

	// Do we have too many death messages in the queue?
	if ( m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() >= (int)m_flMaxDeathNotices )
	{
		// Remove the oldest one in the queue, which will always be the first
		m_DeathNotices.Remove(0);
	}

	GetEventEntityInfo(event, deathMsg.Killer, "killer_entindex");
	GetEventEntityInfo(event, deathMsg.Victim, "victim_entindex");
	deathMsg.flDisplayTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.iSuicide = (deathMsg.Killer.iEntIndex == 0 || deathMsg.Killer.iEntIndex == deathMsg.Victim.iEntIndex);

	// Try and find the death identifier in the icon list
	deathMsg.iconDeath = gHUD.GetIcon( fullkilledwith );

	if ( !deathMsg.iconDeath || deathMsg.iSuicide )
	{
		// Can't find it, so use the default skull & crossbones icon
		deathMsg.iconDeath = m_iconD_skull;
	}

	// Add it to our list of death notices
	m_DeathNotices.AddToTail( deathMsg );

	char sDeathMsg[512];

	// Record the death notice in the console
	if ( deathMsg.iSuicide )
	{
		if ( !strcmp( fullkilledwith, "d_worldspawn" ) )
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s died", deathMsg.Victim.szName );
		}
		else	//d_world
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s suicided", deathMsg.Victim.szName );
		}
	}
	else
	{
		int len = Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s killed %s", deathMsg.Killer.szName, deathMsg.Victim.szName );

		if ( Q_strlen(fullkilledwith) > 6 )
		{
			Q_snprintf(sDeathMsg + len, sizeof(sDeathMsg) - len, " with %s", fullkilledwith + 6);
		}
	}

	Msg( "%s.\n", sDeathMsg );
}

void CHudDeathNotice::GetEventEntityInfo(IGameEvent* pEvent, DeathNoticeEntity& entityData, const char* pEntIndexKey)
{
	*entityData.szName = '\0';
	const char* pName = NULL;

#ifdef HL2RP
	if (entityData.iEntIndex < 1)
	{
		int entIndex = pEvent->GetInt(pEntIndexKey);
		CBaseCombatCharacter* pCharacter = ToBaseCombatCharacter(ClientEntityList().GetBaseEntity(entIndex));

		if (pCharacter != NULL)
		{
			entityData.iEntIndex = entIndex;
			entityData.mTeamNum = pCharacter->GetTeamNumber();
			pName = g_pVGuiLocalize->FindAsUTF8(pCharacter->GetDisplayName());
		}
	}
	else
#endif // HL2RP
	{
		pName = g_PR->GetPlayerName(entityData.iEntIndex);
	}

	if (pName != NULL)
	{
		V_strcpy_safe(entityData.szName, pName);
	}
}
