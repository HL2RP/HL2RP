#ifndef HL2RP_CONFIGURATION_H
#define HL2RP_CONFIGURATION_H
#pragma once

class COtherSettingsDialog;

class CHL2RPConfiguration : CAutoGameSystemPerFrame
{
	void PostInit() OVERRIDE;
	void Shutdown() OVERRIDE;

	CON_COMMAND_MEMBER_F(CHL2RPConfiguration, "cl_showothersettings", CmdDisplayOtherSettingsDialog,
		"Displays the extended settings dialog", FCVAR_HIDDEN);

public:
	CHL2RPConfiguration();

	KeyValuesAD mUserData; // Contains automatic user data not suitable to be handled through CVars
	COtherSettingsDialog* mpOtherSettingsDialog = NULL;
};

extern CHL2RPConfiguration gHL2RPConfiguration;

#endif // !HL2RP_CONFIGURATION_H
