# Copy build outputs into a release-style folder next to the loader project
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$loaderOut = Join-Path $root "External\loader\x64\Release\FrontierLoader.exe"
$cheatOut = Join-Path $root "External\x64\Release\Frontier.exe"
$stage = Join-Path $root "dist"

if (-not (Test-Path $cheatOut)) {
    Write-Error "Build External Release|x64 first. Missing: $cheatOut"
    exit 1
}

New-Item -ItemType Directory -Force -Path (Join-Path $stage "usermode") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stage "kernel") | Out-Null

Copy-Item $cheatOut (Join-Path $stage "usermode\Frontier.exe") -Force
if (Test-Path $loaderOut) {
    Copy-Item $loaderOut (Join-Path $stage "FrontierLoader.exe") -Force
} else {
    Write-Warning "Loader not built yet — only usermode staged"
}

Write-Host "Staged to $stage"
Write-Host "  usermode\Frontier.exe"
if (Test-Path (Join-Path $stage "FrontierLoader.exe")) { Write-Host "  FrontierLoader.exe" }
