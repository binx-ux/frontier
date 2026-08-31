# Publish FRONTIER build to GitHub Releases (binx-ux/frontier-releases)
param(
    [string]$Tag = "v1.1",
    [string]$ZipName = "Frontier-v1.1.zip"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $root "dist"
$relRoot = Join-Path (Split-Path -Parent $root) "frontier-releases"

& (Join-Path $PSScriptRoot "stage-loader.ps1")
& (Join-Path $PSScriptRoot "stage-zip.ps1")

if (-not (Test-Path $relRoot)) {
    Write-Error "Missing $relRoot - clone https://github.com/binx-ux/frontier-releases first"
}

$zipSrc = Join-Path $root "..\trace-host\downloads\Frontier-v1.zip"
Copy-Item $zipSrc (Join-Path $relRoot $ZipName) -Force
Copy-Item (Join-Path $dist "FrontierLoader.exe") (Join-Path $relRoot "FrontierLoader.exe") -Force
Copy-Item (Join-Path $dist "usermode\Frontier.exe") (Join-Path $relRoot "Frontier.exe") -Force

Push-Location $relRoot
gh release view $Tag 2>$null
if ($LASTEXITCODE -ne 0) {
    gh release create $Tag --title "FRONTIER $Tag" --notes "See trace-host changelog." $ZipName FrontierLoader.exe Frontier.exe
} else {
    gh release upload $Tag --clobber $ZipName FrontierLoader.exe Frontier.exe
}
Pop-Location

Write-Host "Published $Tag to https://github.com/binx-ux/frontier-releases/releases/tag/$Tag"
