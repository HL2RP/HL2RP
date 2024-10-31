// Combine dispenser, used to distribute rations in HL2RP

$modelname "props_combine\combine_dispenser.mdl"

$cdmaterials "models\props_combine"

$body studio "combine_dispenser_reference"

$illumposition 1.386 10.36 19.096

$attachment "Package_attachment"	"Combine_Dispenser.Package_Attachment" 0 0 0 rotate -90 -90 0
$attachment "Ration_attachment"		"Combine_Dispenser.Package_Attachment" -6 -1.5 12 rotate -90 -90 0
$attachment "Sprite_Attachment"		"Combine_Dispenser.Base" 1.5 22 8

$sequence idle "combine_dispenser_reference"

$sequence dispense_package "combine_dispenser_anims\dispense_package" {
	event 0 28 // Fully open
	event 1 82 // Fully closed
}

// Builds a convex collision model, which was missing by default
$collisionmodel "combine_dispenser_reference"
{
	$automass
}
