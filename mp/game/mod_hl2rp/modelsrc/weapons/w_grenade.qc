// Created by Crowbar 0.58

$modelname "weapons/W_grenade.mdl"

$bodygroup "studio"
{
	studio "w_grenade_HL2DM_grenade_reference.smd"
}

$lod 35
{
	replacemodel "w_grenade_HL2DM_grenade_reference.smd" "w_grenade_HL2DM_grenade_reference_lod1.smd"
	nofacial
}

$surfaceprop "grenade"

$contents "solid"

$cdmaterials "models\weapons\w_grenade\"

$attachment "fuse" "static_prop" 0 0 4.5 rotate 0 90 0

$cbox 0 0 0 0 0 0

$definebone "ValveBiped.Bip01_R_Hand" "" -2.297680 2.858660 -1.072850 -16.489055 -85.117614 -177.467755 0.000000 -0.000000 0.000000 0.000001 0.000000 -0.000000
$definebone "ValveBiped.bone" "ValveBiped.Bip01_R_Hand" 1.046934 -1.640485 -1.295040 10.445138 -65.417046 -79.575933 -0.000000 0.000000 0.000000 -0.000002 0.000002 0.000002
$definebone "static_prop" "" 0 0 0 0 0 0 0 0 0 0 0 0

$sequence "idle" {
	"w_grenade_anims\idle.smd"
	activity "ACT_VM_IDLE" 1
	fadein 0.2
	fadeout 0.2
	fps 30
	loop
}

$sequence "attack1" {
	"w_grenade_anims\attack1.smd"
	fadein 0.2
	fadeout 0.2
	fps 30
}

$collisionmodel "w_grenade_HL2DM_grenade_reference.smd"
{
	$mass 10
	$inertia 1
	$damping 0
	$rotdamping 0
	$rootbone " "
}
