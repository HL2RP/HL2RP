#!/bin/bash

# Find out the correct VPK path for either Linux or Windows (default)
VPK_PATH=$(command -v 'vpk_linux32');

if [ -z $VPK_PATH ]
then
	VPK_PATH=$(command -v 'vpk.exe');
else
	# Export required Source Engine bin directory, expected to be the VPK's parent
	export LD_LIBRARY_PATH=$(dirname "$VPK_PATH");
fi

pushd $(dirname "$0");
"$VPK_PATH" generate_keypair "hl2rp_key";

# Delete previous VPKs
rm -f mod_hl2rp/hl2rp_pak*.vpk;
rm -f hl2rp/hl2rp_pak*.vpk;
rm -f hl2mp/hl2rp_pak*.vpk;

# Create empty VPK for HL2RP Standalone (retail)
"$VPK_PATH" -M -k "hl2rp_key.publickey.vdf" -K "hl2rp_key.privatekey.vdf" \
	a "mod_hl2rp/hl2rp_pak";

# Create empty VPK for HL2RP Standalone (development)
"$VPK_PATH" -M -k "hl2rp_key.publickey.vdf" -K "hl2rp_key.privatekey.vdf" \
	a "hl2rp/hl2rp_pak";

# Create empty VPK for HL2DM RP version
"$VPK_PATH" -M -k "hl2rp_key.publickey.vdf" -K "hl2rp_key.privatekey.vdf" \
	a "hl2mp/hl2rp_pak";

# Fix changed endianness by Git after certain actions for localization files
for file in mod_*/resource/*.txt
do
	# Reliably check that it has the wrong endianness (UTF-16BE)
	if [ $(head -c 2 $file) = $(printf "\xFE\xFF") ]
	then
		printf "Fixing endianness of localization file '$file'...\n";
		iconv -f UTF-16BE -t UTF-16LE -c $file > $file.utf16le;
		mv $file.utf16le $file;
	fi
done

# Compile models
pushd mod_hl2rp/modelsrc;
studiomdl.exe -nop4 -verbose "combine_soldier_anims.qc";
studiomdl.exe -nop4 -verbose "humans/female_shared.qc";
studiomdl.exe -nop4 -verbose "humans/male_shared.qc";
studiomdl.exe -nop4 -verbose "police_animations.qc";
studiomdl.exe -nop4 -verbose "props_combine/combine_dispenser.qc";
studiomdl.exe -nop4 -verbose "weapons/w_grenade.qc";
studiomdl.exe -nop4 -verbose "weapons/w_package.qc";
popd;

# Start: Effectively pack all assets into the VPKs.
# NOTE: As of date, multiple #includes aren't supported within the keyvalues
# control files, so we deal with it by adding exceeded files on more VPK calls.

# Pack assets into the HL2RP Standalone (development) VPK
"$VPK_PATH" -M k "mod_hl2rp/hl2rp_pak" "mod_hl2rp/resource/controlfile.txt";
"$VPK_PATH" -M k "mod_hl2rp/hl2rp_pak" "mod_hl2rp/controlfile_standalone.txt";

# Pack assets into the HL2RP Standalone (retail) VPK
"$VPK_PATH" -M k "hl2rp/hl2rp_pak" "hl2rp/controlfile.txt";

# Pack assets into the HL2DM RP version VPK
"$VPK_PATH" -M k "hl2mp/hl2rp_pak" "mod_hl2rp/resource/controlfile.txt";
"$VPK_PATH" -M k "hl2mp/hl2rp_pak" "hl2mp/controlfile.txt"

popd;
