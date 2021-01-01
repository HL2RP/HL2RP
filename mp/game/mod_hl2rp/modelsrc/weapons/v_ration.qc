// Citizen package viewmodel

$modelname "weapons\v_ration.mdl"

$cdmaterials "models\weapons\v_hand" "models\weapons\w_package"

$bodygroup hands
{
	studio "hands_reference"
}

$bodygroup package
{
	studio "w_package_reference"
}

$origin 0 13 68

$pushd "v_ration_anims"
$sequence idle		"idle"		ACT_VM_IDLE		1
$sequence draw		"draw"		ACT_VM_DRAW		1 fps 4
$sequence holster	"holster"	ACT_VM_HOLSTER	1 fps 4
$sequence release	"release"	ACT_VM_RELEASE	1 fps 8
$sequence pullback	"pullback"	ACT_VM_PULLBACK	1 fps 16
$sequence throw		"throw"		ACT_VM_THROW	1 fps 8 { event 3005 3 }
$popd
