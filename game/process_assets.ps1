Param([bool]$Compile = $True)
$StopWatch = [System.Diagnostics.Stopwatch]::StartNew()
pushd $PSScriptRoot
$PublicKeyFile = "hl2rp.publickey.vdf"
$PrivateKeyFile = "hl2rp.privatekey.vdf"
$TempPaths = @()
$ControlFileDataByMod = @{"mod_hl2rp" = ''; "hl2rp" = ''; "hl2mp" = ''}
$CRLF="`r`n"
$TrustedKeysData = '"trusted_key_list"' + "$CRLF{"
$LegacyDownloadsData = ''
Set-Alias VPK bin\x64\vpk.exe
rm *\*.vpk
mkdir hl2rp -Force > $null

if ($Compile)
{
	.\compile_models.ps1
}

Enum AssetType
{
	Default
	VGUI2Localization
	Downloadable
}

function CreateTempFile
{
	$tempPath = New-TemporaryFile
	$script:TempPaths += $tempPath
	return $tempPath
}

function AddToControlFile
{
	Param([string]$Mod, [string]$SourcePath, [string]$DestPath)
	$ControlFileDataByMod[$Mod] += "`"$SourcePath`" { `"destpath`" `"$DestPath`" }$CRLF"
}

# Generate VPK signing keypair if either key file doesn't exist
if (!(Test-Path $PublicKeyFile) -or !(Test-Path $PrivateKeyFile))
{
	VPK generate_keypair hl2rp
}

# Add the VPK keypair to the server trusted keys custom file for the full versions
(cat $PublicKeyFile) -split $CRLF | foreach `
{
	$TrustedKeysData += "$CRLF`t$_"
}

$TrustedKeysData += "$CRLF}"
$TempTrustedKeysFile = CreateTempFile
Set-Content -Path $TempTrustedKeysFile -Value $TrustedKeysData
$TrustedKeysPath = "cfg\trusted_keys.txt"
AddToControlFile -mod mod_hl2rp -sourcePath $TempTrustedKeysFile -destPath $TrustedKeysPath
AddToControlFile -mod hl2rp -sourcePath $TempTrustedKeysFile -destPath $TrustedKeysPath

Import-Csv assets_metadata.csv | foreach `
{
	$sourcePath = $_.Mod + "\" + $_.SourcePath
	$assetMetaData = $_

	# If the asset is a VGUI2 localization file, fix its UTF-16 endianness to LE, which Git may have changed to BE
	if ($_.Type -eq [int][AssetType]::VGUI2Localization)
	{
		Set-Content -Path $sourcePath -Value (cat $sourcePath) -Encoding Unicode
	}

	if ($_.Mod -eq 'hl2mp')
	{
		$_.DestLegacyPath = $_.SourcePath
	}
	else
	{
		if ($_.Mod -eq 'hl2rp')
		{
			$_.DestFullReleasePath = $_.SourcePath
		}
		else
		{
			# Prepare asset redirection for full release version
			AddToControlFile -mod mod_hl2rp -sourcePath $sourcePath -destPath $_.SourcePath

			if ([string]::IsNullOrEmpty($_.DestFullReleasePath))
			{
				$_.DestFullReleasePath = $_.SourcePath
			}
		}

		AddToControlFile -mod hl2rp -sourcePath $sourcePath -destPath $_.DestFullReleasePath

		# Prepare asset redirection for legacy server. When path equals '!', we must ignore the asset.
		if ($_.DestLegacyPath -eq '!')
		{
			return
		}
		elseif ([string]::IsNullOrEmpty($_.DestLegacyPath))
		{
			$_.DestLegacyPath = $_.DestFullReleasePath
		}
	}

	if ($_.Type -eq [int][AssetType]::Downloadable)
	{
		# Add the asset to the downloads list data, and create a compressed BZ2 file for sv_downloadurl hosting
		$LegacyDownloadsData += $assetMetaData.destLegacyPath + $CRLF
		$bz2LegacyPath = "hl2mp\" + $assetMetaData.destLegacyPath + ".bz2"
		rm $bz2LegacyPath -ErrorAction Ignore
		bin\x64\7z.exe a -mx9 -tbzip2 $bz2LegacyPath $sourcePath
	}

	AddToControlFile -mod hl2mp -sourcePath $sourcePath -destPath $_.DestLegacyPath
}

# Create and register the uploads list file containing the downloadable assets for legacy server
$TempDownloadsFile = CreateTempFile
Set-Content -Path $TempDownloadsFile -Value $LegacyDownloadsData -NoNewline
AddToControlFile -mod hl2mp -sourcePath $TempDownloadsFile -destPath cfg\hl2rp\upload.txt

# Pack the listed assets into a VPK for each mod
$ControlFileDataByMod.Keys | foreach `
{
	echo ''
	$tempControlFile = CreateTempFile
	Set-Content -Path $tempControlFile -Value $ControlFileDataByMod[$_]
	VPK -P -k $PublicKeyFile -K $PrivateKeyFile k $_\hl2rp_pak $tempControlFile
}

$TempPaths | foreach `
{
	rm $_
}

popd
echo ($CRLF + "Script completed in " + $StopWatch.Elapsed.TotalSeconds + " seconds")
