# Publish FRONTIER v1.3.1 with GUI loader
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$rel = Join-Path (Split-Path -Parent $root) "frontier-releases"
$tag = "v1.3.1"

& (Join-Path $PSScriptRoot "build-loader-gui.ps1")
& (Join-Path $PSScriptRoot "stage-loader.ps1")
& (Join-Path $PSScriptRoot "stage-zip.ps1") -Version $tag

$dist = Join-Path $root "dist"
$zip = Join-Path $dist "Frontier-$tag.zip"
if (-not (Test-Path $zip)) { throw "Missing $zip" }
if (-not (Test-Path $rel)) { throw "Missing $rel" }

Copy-Item $zip (Join-Path $rel "Frontier-$tag.zip") -Force
Copy-Item (Join-Path $dist "FrontierLoader.exe") (Join-Path $rel "FrontierLoader.exe") -Force
Copy-Item (Join-Path $dist "usermode\Frontier.exe") (Join-Path $rel "Frontier.exe") -Force
Get-ChildItem $dist -Filter "*.dll" | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $rel $_.Name) -Force
}
$config = Join-Path $dist "FrontierLoader.exe.config"
if (Test-Path $config) {
    Copy-Item $config (Join-Path $rel "FrontierLoader.exe.config") -Force
}

$kernel = Join-Path $dist "kernel\Frontier.exe"
if (Test-Path $kernel) {
    Copy-Item $kernel (Join-Path $rel "Frontier-Kernel.exe") -Force
} else {
    Write-Host "Reusing existing kernel asset if present"
}

$driver = Join-Path $dist "kernel\driver\FrontierDrv.sys"
if (Test-Path $driver) {
    Copy-Item $driver (Join-Path $rel "FrontierDrv.sys") -Force
} else {
    Write-Host "Reusing existing driver asset if present"
}

$assets = @(
    "Frontier-$tag.zip",
    "FrontierLoader.exe",
    "FrontierLoader.exe.config",
    "Frontier.exe",
    "Frontier-Kernel.exe",
    "FrontierDrv.sys"
) | ForEach-Object { Join-Path $rel $_ } | Where-Object { Test-Path $_ }

Get-ChildItem $rel -Filter "*.dll" | ForEach-Object { $assets += $_.FullName }

$notes = @"
## v1.3.1
- New Guna GUI loader (KeyAuth-style menu, ahead.best license API)
- Loader zip includes required .NET dependencies
- Menu crash fixes from v1.3.0 retained
"@

Push-Location $rel
$releaseExists = $false
try {
    gh release view $tag --repo binx-ux/frontier-releases 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) { $releaseExists = $true }
} catch { $releaseExists = $false }

if (-not $releaseExists) {
    gh release create $tag --repo binx-ux/frontier-releases --title "FRONTIER $tag" --notes $notes @assets
} else {
    gh release upload $tag --repo binx-ux/frontier-releases --clobber @assets
}
Pop-Location

Write-Host "Published $tag to https://github.com/binx-ux/frontier-releases/releases/tag/$tag"
