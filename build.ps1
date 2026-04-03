$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$outDir = Join-Path $root "out"
$buildConfig = if ($env:BUILD_CONFIGURATION) { $env:BUILD_CONFIGURATION } else { "Debug" }
$debugExe = Join-Path $outDir "Debug\\Telegram.exe"
$releaseExe = Join-Path $outDir "Release\\Telegram.exe"
$rootExe = Join-Path $root "Telegram.exe"
$cmakeHelpers = Join-Path $root "cmake\\CMakeLists.txt"
$librariesRoot = [System.IO.Path]::GetFullPath((Join-Path $root "..\\Libraries"))
$defaultApiId = "17349"
$defaultApiHash = "344583e45741c457fe1862106095a5eb"
$preferredCmakeCandidates = @(
	"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
	"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
	"C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
	"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
$configureMarkers = @(
	(Join-Path $outDir "CMakeCache.txt"),
	(Join-Path $outDir "build-Debug.ninja"),
	(Join-Path $outDir "build-Release.ninja"),
	(Join-Path $outDir "build.ninja"),
	(Join-Path $outDir "Telegram.slnx"),
	(Join-Path $outDir "Telegram.sln")
)

function Test-Command($name) {
	return $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

function Resolve-CMakeExecutable {
	foreach ($candidate in $preferredCmakeCandidates) {
		if (Test-Path $candidate) {
			return $candidate
		}
	}
	$command = Get-Command cmake -ErrorAction SilentlyContinue
	if ($null -ne $command) {
		return $command.Source
	}
	return $null
}

$cmakeExe = Resolve-CMakeExecutable
if ($cmakeExe) {
	$cmakeDir = Split-Path -Parent $cmakeExe
	if (-not ($env:PATH -split ';' | Where-Object { $_ -eq $cmakeDir })) {
		$env:PATH = "$cmakeDir;$env:PATH"
	}
}

function Configure-ClientBuild {
	$python = $null
	if (Test-Command "python") {
		$python = "python"
	}
	elseif (Test-Command "py") {
		$python = "py"
	}

	$missing = @()
	if (-not (Test-Path $cmakeHelpers)) {
		$missing += "missing client/cmake helpers"
	}
	if (-not (Test-Path $librariesRoot)) {
		$missing += "missing prepared Libraries folder at '$librariesRoot'"
	}
	if (-not $python) {
		$missing += "missing Python in PATH"
	}
	if (-not $env:TDESKTOP_API_ID) {
		$env:TDESKTOP_API_ID = $defaultApiId
	}
	if (-not $env:TDESKTOP_API_HASH) {
		$env:TDESKTOP_API_HASH = $defaultApiHash
	}

	if ($missing.Count -gt 0) {
		$details = $missing -join "; "
		throw "Unable to configure client build automatically: $details. Follow client/docs/building-win.md, then rerun build.ps1."
	}

	if (Test-Path $outDir) {
		Remove-Item -LiteralPath $outDir -Recurse -Force
	}

	# run configure with python interpreter (if only py is available, use py -3)
	$pythonExe = if (Test-Command python) { 'python' } elseif (Test-Command py) { 'py' } else { throw 'Python not found' }
	$pythonArgs = @()
	if ($pythonExe -eq 'py') { $pythonArgs += '-3' }

	$configureScript = Join-Path $root 'Telegram\configure.py'
	if (-not (Test-Path $configureScript)) {
		throw "Configure script not found: $configureScript"
	}

	$configureArgs = $pythonArgs + @(
		$configureScript,
		'x64',
		'-D', "TDESKTOP_API_ID=$($env:TDESKTOP_API_ID)",
		'-D', "TDESKTOP_API_HASH=$($env:TDESKTOP_API_HASH)",
		'-D', 'DESKTOP_APP_DISABLE_CRASH_REPORTS=ON'
	)

	$oldErrorAction = $ErrorActionPreference
	$ErrorActionPreference = 'Continue'
	try {
		& $pythonExe @configureArgs
	}
	finally {
		$ErrorActionPreference = $oldErrorAction
	}
	if ($LASTEXITCODE -ne 0) {
		throw "Client configure step failed with exit code $LASTEXITCODE."
	}
	if (-not (Test-Path $outDir)) {
		throw "Client configure step finished without creating '$outDir'."
	}
}

function Invoke-ClientBuild($target) {
	& $cmakeExe --build out --config $buildConfig --target $target -- /m:1 /p:UseMultiToolTask=false
	if ($LASTEXITCODE -ne 0) {
		throw "Client build failed with exit code $LASTEXITCODE while building target '$target'."
	}
}

function Update-LangGeneratedFiles {
	$codegenLangExe = Join-Path $outDir "Telegram\\codegen\\codegen\\lang\\$buildConfig\\codegen_lang.exe"
	$genDir = Join-Path $outDir "Telegram\\gen"
	$langFile = Join-Path $root "Telegram\\Resources\\langs\\lang.strings"
	$langTimestamp = Join-Path $genDir "lang_auto.timestamp"
	$langTargetDir = Join-Path $outDir "Telegram\\CMakeFiles"
	$langTargetMarker = Join-Path $langTargetDir "td_lang_lang"

	if (-not (Test-Path $codegenLangExe)) {
		throw "Expected code generator '$codegenLangExe' was not built."
	}

	New-Item -ItemType Directory -Path $genDir -Force | Out-Null
	New-Item -ItemType Directory -Path $langTargetDir -Force | Out-Null
	& $codegenLangExe "-o$genDir" $langFile
	if ($LASTEXITCODE -ne 0) {
		throw "Manual lang generation failed with exit code $LASTEXITCODE."
	}
	Set-Content -LiteralPath $langTimestamp -Value "" -NoNewline
	Set-Content -LiteralPath $langTargetMarker -Value "" -NoNewline
}

if (-not $cmakeExe) {
	throw "CMake is not available in PATH."
}

if (-not (Test-Path $outDir) -or -not ($configureMarkers | Where-Object { Test-Path $_ })) {
	Configure-ClientBuild
}

Push-Location $root
try {
	$existingCl = $env:CL
	$env:CL = if ([string]::IsNullOrWhiteSpace($existingCl)) { "/FS /Zm300" } else { "/FS /Zm300 $existingCl" }
	Invoke-ClientBuild "codegen_lang"
	Update-LangGeneratedFiles
	Invoke-ClientBuild "Telegram"

	$builtExe = Join-Path $outDir "$buildConfig\\Telegram.exe"
	if (Test-Path $builtExe) {
		Copy-Item -LiteralPath $builtExe -Destination $rootExe -Force
		Write-Host "Built $rootExe from $buildConfig output"
	}
	elseif (Test-Path $releaseExe) {
		Copy-Item -LiteralPath $releaseExe -Destination $rootExe -Force
		Write-Host "Built $rootExe from Release output"
	}
	elseif (Test-Path $debugExe) {
		Copy-Item -LiteralPath $debugExe -Destination $rootExe -Force
		Write-Host "Built $rootExe from Debug output"
	}
	else {
		throw "Build finished but Telegram.exe was not found in out\\Debug or out\\Release."
	}
}
finally {
	$env:CL = $existingCl
	Pop-Location
}
