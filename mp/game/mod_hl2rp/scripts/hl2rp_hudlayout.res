#base "hudlayout.res"

"resource/hl2rp_hudlayout.res"
{
	"MainHUD"
	{
		"xPos"              "10"
		"yPos"              "15"
		"textColor"         "0 0 255 255" // Normal (non-criminal)
		"criminalTextColor" "255 0 0 255"
	}

	"AimingEntityHUD"
	{
		"xPos"            "-1" // Auto center
		"yPos"            "375"
		"textColor"       "0 255 0 255" // General entities
		"playerTextColor" "255 255 255 255"
	}

	"HUDHintKeyDisplay"
	{
		"xPos"                "r120"
		"yPos"                "r340"
		"wide"                "100"
		"tall"                "200"
		"textColor"           "255 170 0 220"
		"paintBackgroundType" "2"
	}
}
