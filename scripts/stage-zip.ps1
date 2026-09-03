# Build Frontier release zip (loader + usermode + optional kernel)
param(
    [string]$Version = "v1.3.0"
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$dist = Join-Path $root "dist"
$stage = Join-Path $dist "zip-stage"
$zipOut = Join-Path $dist "Frontier-$Version.zip"

$loader = Join-Path $dist "FrontierLoader.exe"
$cheat = Join-Path $dist "usermode\Frontier.exe"
$ini = Join-Path $dist "loader.ini"
$kernelExe = Join-Path $dist "kernel\Frontier.exe"
$driverSys = Join-Path $dist "kernel\driver\FrontierDrv.sys"

if (-not (Test-Path $loader)) { Write-Error "Build loader first: scripts\build-loader-gui.ps1" }
if (-not (Test-Path $cheat)) { Write-Error "Stage cheat first: scripts\stage-loader.ps1" }

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path (Join-Path $stage "usermode") | Out-Null
Copy-Item $loader (Join-Path $stage "FrontierLoader.exe") -Force
if (Test-Path $ini) { Copy-Item $ini (Join-Path $stage "loader.ini") -Force }
Copy-Item $cheat (Join-Path $stage "usermode\Frontier.exe") -Force
$assetsDir = Join-Path $dist "usermode\assets"
if (Test-Path $assetsDir) {
    $destAssets = Join-Path $stage "usermode\assets"
    New-Item -ItemType Directory -Force -Path $destAssets | Out-Null
    Copy-Item (Join-Path $assetsDir "*") $destAssets -Force
}

if (Test-Path $kernelExe) {
    New-Item -ItemType Directory -Force -Path (Join-Path $stage "kernel\driver") | Out-Null
    Copy-Item $kernelExe (Join-Path $stage "kernel\Frontier.exe") -Force
    if (Test-Path $driverSys) {
        Copy-Item $driverSys (Join-Path $stage "kernel\driver\FrontierDrv.sys") -Force
    }
}

if (Test-Path $zipOut) { Remove-Item $zipOut -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zipOut -Force
Remove-Item $stage -Recurse -Force

Write-Host "Created $zipOut"
