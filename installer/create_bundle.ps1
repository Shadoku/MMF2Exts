Param(
    [string]$Version,
    [string]$PythonEmbedZip,
    [string]$OutputZip = "releases/fusionpy-bundle-$Version.zip"
)

$ErrorActionPreference = "Stop"

if (-not $Version) { throw "--Version required" }
$root = Split-Path -Parent $PSScriptRoot
$bundleStaging = Join-Path $PSScriptRoot "bundle"
Remove-Item -Recurse -Force $bundleStaging -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $bundleStaging | Out-Null

Copy-Item $PythonEmbedZip $bundleStaging
Copy-Item (Join-Path $root "releases" "x64" "fusionpy_host-$Version-x64.zip") $bundleStaging
Copy-Item (Join-Path $root "releases" "Win32" "fusionpy_host-$Version-Win32.zip") $bundleStaging
Copy-Item (Get-ChildItem -Path (Join-Path $root "dist") -Filter "fusionpy_sdk-$Version-*.whl" | Select-Object -First 1) $bundleStaging

Compress-Archive -Path (Join-Path $bundleStaging '*') -DestinationPath (Join-Path $root $OutputZip) -Force
Write-Host "Created bundle at $OutputZip"
