#base "hudlayout.res"

"resource/hl2rp_hudlayout.res"
{
	"MainHUD"
	{
		"xPos"				"p0.015"
		"yPos"				"p0.03"
		"textColor"			"0 0 255 255" // Normal (non-criminal)
		"criminalTextColor"	"255 0 0 255"
	}

	"ZoneHUD"
	{
		"xPos"		"-1" // Auto center
		"yPos"		"p0.03"
		"textColor"	"0 255 0 255"
	}

	"AimingEntityHUD"
	{
		"xPos"				"-1" // Auto center
		"yPos"				"p0.78"
		"textColor"			"0 255 0 255" // General entities
		"playerTextColor"	"255 255 255 255"
	}

	"RegionHUD"
	{
		"xPos"				"p0.82"
		"yPos"				"p0.62"
		"headerTextColor"	"255 150 150 255"
		"textColor"			"255 100 100 255" // Player lines
	}

	"HUDHintKeyDisplay"
	{
		"xPos"					"r120"
		"yPos"					"r340"
		"wide"					"100"
		"tall"					"200"
		"textColor"				"255 170 0 220"
		"paintBackgroundType"	"2"
	}
}
