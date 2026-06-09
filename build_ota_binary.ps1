# Copy compiled app binary to releases/ with version from TenZillaVersion.h
# Run from project root (Tenzilla folder). Binary is taken from build folder after Compile/Verify.

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$versionFile = "src\TenZillaVersion.h"
$releasesDir = "releases"

# Arduino IDE 2 puts the app binary in build/esp32.esp32.esp32s3/Tenzilla.ino.bin (not in sketch root)
$buildBin = "build\esp32.esp32.esp32s3\Tenzilla.ino.bin"
$rootBin = "Tenzilla.bin"

if (-not (Test-Path $versionFile)) {
    Write-Error "Not found: $versionFile (run from project root)"
}

$sourceBin = $null
if (Test-Path $buildBin) {
    $sourceBin = $buildBin
} elseif (Test-Path $rootBin) {
    $sourceBin = $rootBin
}
if (-not $sourceBin) {
    Write-Host "Binary not found. Tried: $buildBin and $rootBin"
    Write-Host "Do: Sketch -> Verify/Compile in Arduino IDE, then run this script again."
    exit 1
}
Write-Host "Using: $sourceBin"

$content = Get-Content $versionFile -Raw
if ($content -match 'TENZILLA_RELEASE_NUMBER\s+"([^"]+)"') {
    $ver = $matches[1].Trim()
} else {
    Write-Error "TENZILLA_RELEASE_NUMBER not found in $versionFile"
}

if (-not (Test-Path $releasesDir)) {
    New-Item -ItemType Directory -Path $releasesDir | Out-Null
}

$destBin = Join-Path $releasesDir "Tenzilla_$ver.bin"
Copy-Item -Path $sourceBin -Destination $destBin -Force
Write-Host "OTA binary saved: $destBin"
