// Created by Crowbar 0.58

$modelname "weapons/w_package.mdl"

$bodygroup "studio"
{
	studio "w_package_package_reference.smd"
}

$lod 27
{
	replacemodel "w_package_package_reference.smd" "w_package_package_reference_lod1.smd"
	nofacial
}

$surfaceprop "weapon"

$contents "solid"

$cdmaterials "models\weapons\w_package\"

$cbox 0 0 0 0 0 0

$definebone "ValveBiped.Bip01_R_Hand" "" 6.592096 -6.717985 -0.644930 10.879530 92.496854 -176.250342 0.000000 0.000000 0.000000 -0.000001 -0.000000 0.000000

$sequence "idle" {
	"w_package_anims\idle.smd"
	activity "ACT_VM_IDLE" 1
	fadein 0.2
	fadeout 0.2
	fps 30
	loop
}

$collisionmodel "w_package_package_reference.smd"
{
	$mass 8
	$inertia 1
	$damping 0
	$rotdamping 0
	$rootbone " "
	$concave
	$maxconvexpieces 1
}
