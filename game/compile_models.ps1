pushd "$PSScriptRoot\mod_hl2rp"
$genericModelPaths = @()

dir "modelsrc\*.qc" -Recurse | foreach `
{
	..\bin\studiomdl.exe -nop4 $_

	# Compute the model's base path and register it with any extension
	$baseModelPath = (cat $_) | Select-String -Pattern "`$modelname" -SimpleMatch | Select-Object -Last 1
	$baseModelPath = ($baseModelPath -split '\s+')[1]
	$baseModelPath = $baseModelPath.Trim('"').Replace('\', '/')
	$baseModelPath = ($baseModelPath -split '\.mdl$')[0]
	$genericModelPaths += "/$baseModelPath" + '.*'
}

# Write all listed generic model paths into .gitignore
Set-Content -Path "models\.gitignore" -Value ("# Built models", $genericModelPaths)

popd
