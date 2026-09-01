# Stage loader release folder (usermode + optional kernel)
$root = Split-Path -Parent $PSScriptRoot
$loaderOut = Join-Path $root "External\loader\x64\Release\FrontierLoader.exe"
$cheatOut = Join-Path $root "External\x64\Release\Frontier.exe"
if (-not (Test-Path $cheatOut)) {
    $cheatOut = Join-Path $root "x64\Release\Frontier.exe"
}
$kernelOut = Join-Path $root "kernel\Frontier.exe"
$stage = Join-Path $root "dist"

if (-not (Test-Path $cheatOut)) {
    Write-Error "Build External Release|x64 first. Missing: $cheatOut"
    exit 1
}

New-Item -ItemType Directory -Force -Path (Join-Path $stage "usermode") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stage "kernel") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stage "kernel\driver") | Out-Null

Copy-Item $cheatOut (Join-Path $stage "usermode\Frontier.exe") -Force
$assetsDir = Join-Path $root "External\assets"
if (Test-Path $assetsDir) {
    $destAssets = Join-Path $stage "usermode\assets"
    New-Item -ItemType Directory -Force -Path $destAssets | Out-Null
    Copy-Item (Join-Path $assetsDir "*") $destAssets -Force
    Write-Host "  usermode\assets\"
}
if (Test-Path $kernelOut) {
    Copy-Item $kernelOut (Join-Path $stage "kernel\Frontier.exe") -Force
    Write-Host "  kernel\Frontier.exe"
}
$driverSys = Join-Path $root "kernel\driver\FrontierDrv.sys"
if (Test-Path $driverSys) {
    Copy-Item $driverSys (Join-Path $stage "kernel\driver\FrontierDrv.sys") -Force
    Write-Host "  kernel\driver\FrontierDrv.sys"
}
if (Test-Path $loaderOut) {
    Copy-Item $loaderOut (Join-Path $stage "FrontierLoader.exe") -Force
} else {
    Write-Warning "Loader not built yet - only cheat staged"
}

Write-Host "Staged to $stage"
Write-Host "  usermode\Frontier.exe"
if (Test-Path (Join-Path $stage "FrontierLoader.exe")) { Write-Host "  FrontierLoader.exe" }
