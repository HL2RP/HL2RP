// Citizen package worldmodel

$modelname "weapons\w_ration.mdl"

$cdmaterials "models\weapons\w_package"

$body studio "w_package_reference"

$surfaceprop "weapon"

$sequence idle "w_package_reference" ACT_VM_IDLE 1

$collisionmodel "w_package_reference"
{
	$mass 8
}
