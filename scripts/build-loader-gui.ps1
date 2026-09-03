# Build the C# Guna loader (FrontierLoader.exe)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$guiDir = Join-Path $root "External\loader-gui\FrontierLoader"

Push-Location $guiDir
dotnet restore
dotnet build -c Release
if ($LASTEXITCODE -ne 0) { Pop-Location; exit $LASTEXITCODE }
Pop-Location

$out = Join-Path $guiDir "bin\Release\net48\FrontierLoader.exe"
if (-not (Test-Path $out)) {
    Write-Error "Build succeeded but missing $out"
}
Write-Host "Built $out"
