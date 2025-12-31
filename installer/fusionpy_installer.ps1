Param(
    [string]$InstallDir = "$Env:ProgramFiles\fusionpy",
    [string]$FusionPath,
    [switch]$AddToPath
)

$ErrorActionPreference = "Stop"

if (-not $FusionPath) {
    Write-Error "FusionPath is required (path to Fusion.exe)."
}

$bundleRoot = Join-Path $PSScriptRoot "bundle"
$hostZip = Get-ChildItem -Path $bundleRoot -Filter "fusionpy_host-*-x64.zip" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$hostZip32 = Get-ChildItem -Path $bundleRoot -Filter "fusionpy_host-*-Win32.zip" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$wheel = Get-ChildItem -Path $bundleRoot -Filter "fusionpy_sdk-*-py3-none-any.whl" | Select-Object -First 1
$pythonEmbed = Get-ChildItem -Path $bundleRoot -Filter "python-embed-*.zip" | Select-Object -First 1

if (-not (Test-Path $bundleRoot)) { throw "Missing bundle payload" }

New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
Expand-Archive -Path $pythonEmbed.FullName -DestinationPath (Join-Path $InstallDir "python") -Force
Expand-Archive -Path $hostZip.FullName -DestinationPath (Join-Path $InstallDir "host\x64") -Force
Expand-Archive -Path $hostZip32.FullName -DestinationPath (Join-Path $InstallDir "host\Win32") -Force
Copy-Item $wheel.FullName -Destination (Join-Path $InstallDir "wheels") -Force

$cmdPath = Join-Path $InstallDir "fusionpy.cmd"
Set-Content -Path $cmdPath -Value "@echo off`nsetlocal`nset PYTHONHOME=%~dp0python`nset PYTHONPATH=%~dp0python`n" -Encoding ascii
Add-Content -Path $cmdPath -Value """%~dp0python\\python.exe"" -m pip install --no-deps --upgrade ""%~dp0wheels\$($wheel.Name)""" -Encoding ascii
Add-Content -Path $cmdPath -Value """%~dp0python\\python.exe"" -m fusionpy_sdk.cli %*" -Encoding ascii

if ($AddToPath) {
    $current = [Environment]::GetEnvironmentVariable("Path", "Machine")
    if (-not $current.Contains($InstallDir)) {
        [Environment]::SetEnvironmentVariable("Path", "$current;$InstallDir", "Machine")
    }
}

Write-Host "Installed fusionpy SDK to $InstallDir"
Write-Host "Fusion path: $FusionPath"

