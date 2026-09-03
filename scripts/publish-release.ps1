# Publish FRONTIER release (GUI loader)
param(
    [string]$Tag = "v1.3.2"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$rel = Join-Path (Split-Path -Parent $root) "frontier-releases"
$dist = Join-Path $root "dist"

& (Join-Path $PSScriptRoot "build-loader-gui.ps1")
& (Join-Path $PSScriptRoot "stage-loader.ps1")
& (Join-Path $PSScriptRoot "stage-zip.ps1") -Version $Tag

$zip = Join-Path $dist "Frontier-$Tag.zip"
if (-not (Test-Path $zip)) { throw "Missing $zip" }
if (-not (Test-Path $rel)) { throw "Missing $rel" }

Copy-Item $zip (Join-Path $rel "Frontier-$Tag.zip") -Force
Copy-Item (Join-Path $dist "FrontierLoader.exe") (Join-Path $rel "FrontierLoader.exe") -Force
Copy-Item (Join-Path $dist "usermode\Frontier.exe") (Join-Path $rel "Frontier.exe") -Force

$kernel = Join-Path $dist "kernel\Frontier.exe"
if (Test-Path $kernel) {
    Copy-Item $kernel (Join-Path $rel "Frontier-Kernel.exe") -Force
}
$driver = Join-Path $dist "kernel\driver\FrontierDrv.sys"
if (Test-Path $driver) {
    Copy-Item $driver (Join-Path $rel "FrontierDrv.sys") -Force
}

$assets = @(
    "Frontier-$Tag.zip",
    "FrontierLoader.exe",
    "Frontier.exe",
    "Frontier-Kernel.exe",
    "FrontierDrv.sys"
) | ForEach-Object { Join-Path $rel $_ } | Where-Object { Test-Path $_ }

$notes = @"
## $Tag
- Fix launch: loader now writes frontier.session before starting cheat
- Single FrontierLoader.exe (no loose DLL files)
"@

$releaseExists = $false
try {
    $null = gh release view $Tag --repo binx-ux/frontier-releases 2>&1
    if ($LASTEXITCODE -eq 0) { $releaseExists = $true }
} catch {
    $releaseExists = $false
}

if (-not $releaseExists) {
    gh release create $Tag --repo binx-ux/frontier-releases --title "FRONTIER $Tag" --notes $notes @assets
} else {
    gh release upload $Tag --repo binx-ux/frontier-releases --clobber @assets
}

Write-Host "Published $Tag"
