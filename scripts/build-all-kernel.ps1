# Build kernel client + driver + stage to dist
$root = Split-Path -Parent $PSScriptRoot
$ErrorActionPreference = "Stop"

Write-Host "=== Kernel client ===" -ForegroundColor Cyan
& (Join-Path $PSScriptRoot "build-kernel.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n=== Kernel driver ===" -ForegroundColor Cyan
& (Join-Path $PSScriptRoot "build-driver.ps1")
$driverOk = ($LASTEXITCODE -eq 0)

if (-not $driverOk) {
    Write-Host "Driver build skipped or failed - kernel client still usable if driver is placed manually." -ForegroundColor Yellow
}

Write-Host "`n=== Stage dist ===" -ForegroundColor Cyan
& (Join-Path $PSScriptRoot "stage-loader.ps1")

Write-Host "`nDone." -ForegroundColor Green
if ($driverOk) {
    Write-Host "Kernel bundle: dist\kernel\Frontier.exe + dist\kernel\driver\FrontierDrv.sys"
} else {
    Write-Host "Kernel client: dist\kernel\Frontier.exe (no .sys - install WDK and re-run build-driver.ps1)"
}
