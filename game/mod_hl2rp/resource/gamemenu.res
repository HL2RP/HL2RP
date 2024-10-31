"GameMenu"
{
	"1"
	{
		"label"			"#GameUI_GameMenu_ResumeGame"
		"command"		"ResumeGame"
		"onlyInGame"	"1"
	}

	"2"
	{
		"label"			"#GameUI_GameMenu_Disconnect"
		"command"		"Disconnect"
		"onlyInGame"	"1"
	}

	"3"
	{
		"label"			"#GameUI_GameMenu_PlayerList"
		"command"		"OpenPlayerListDialog"
		"onlyInGame"	"1"
	}

	"4"
	{
		"label"			""
		"onlyInGame"	"1"
	}

	"5"
	{
		"label"		"#GameUI_GameMenu_FindServers"
		"command"	"OpenServerBrowser"
	}

	"6"
	{
		"label"		"#GameUI_GameMenu_CreateServer"
		"command"	"OpenCreateMultiplayerGameDialog"
	}

	"7"
	{
		"label"					"#GameUI_GameMenu_ActivateVR"
		"command"				"engine vr_activate"
		"inGameOrder"			"40"
		"onlyWhenVREnabled"		"1"
		"onlyWhenVRInactive"	"1"
	}

	"8"
	{
		"label"				"#GameUI_GameMenu_DeactivateVR"
		"command"			"engine vr_deactivate"
		"inGameOrder"		"40"
		"onlyWhenVREnabled"	"1"
		"onlyWhenVRActive"	"1"
	}

	"9"
	{
		"label"		"#GameUI_GameMenu_Options"
		"command"	"OpenOptionsDialog"
	}

	"10"
	{
		"label"		"ROLEPLAY"
		"command"	"engine cl_showroleplaymenu"
	}

	"11"
	{
		"label"		"#GameUI_GameMenu_Quit"
		"command"	"Quit"
	}
}
