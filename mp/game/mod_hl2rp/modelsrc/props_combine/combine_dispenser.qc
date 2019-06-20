// Created by Crowbar 0.58

$modelname "props_combine/Combine_Dispenser.mdl"

$bodygroup "studio"
{
	studio "Combine_Dispenser_reference.smd"
}

$lod 25
{
	replacemodel "Combine_Dispenser_reference.smd" "Combine_Dispenser_reference_lod1.smd"
	replacebone "Combine_Dispenser.Package_Attachment" "Combine_Dispenser.ArmBone"
	nofacial
}

$lod 45
{
	replacemodel "Combine_Dispenser_reference.smd" "Combine_Dispenser_reference_lod2.smd"
	replacebone "Combine_Dispenser.ArmBone" "Combine_Dispenser.Base"
	replacebone "Combine_Dispenser.Package_Attachment" "Combine_Dispenser.ArmBone"
	nofacial
}

$surfaceprop "default"

$contents "solid"

$cdmaterials "models\props_combine\"

$attachment "Package_attachment" "Combine_Dispenser.Package_Attachment" 0 0 0 rotate -90 -90 0
$attachment "Ration_attachment" "Combine_Dispenser.Package_Attachment" -6 -1.5 12 rotate -90 -90 0
$attachment "Sprite_Attachment" "Combine_Dispenser.Base" 1.5 22 8

$cbox 0 0 0 0 0 0

$definebone "Combine_Dispenser.Base" "" 0 0 0 0 0 89.999983 0 0 0 0 0 0
$definebone "Combine_Dispenser.ArmBone" "Combine_Dispenser.Base" 0 -11.885058 -13.067817 0.000019 180.000007 180.000007 0 0 0 0 0 0
$definebone "Combine_Dispenser.Package_Attachment" "Combine_Dispenser.ArmBone" -5.487947 6.027283 3.838225 -0.389302 173.143125 177.667319 0 0 0 0 0 0
$definebone "Combine_Dispenser.CoverBone" "Combine_Dispenser.Base" 0 -6.033648 5.074395 0 0 0 0 0 0 0 0 0

$sequence "idle" {
	"combine_dispenser_anims\idle.smd"
	fadein 0.2
	fadeout 0.2
	fps 30
}

$sequence "dispense_package" {
	"combine_dispenser_anims\dispense_package.smd"
	fadein 0.2
	fadeout 0.2
	fps 30
}

// Build a convex collision model, which is missing on official VPK
$collisionmodel "combine_dispenser_reference.smd"
{
	$automass
}
