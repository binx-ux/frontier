# Test FRONTIER license + launch with UI logging enabled
param(
    [string]$Key = "FRTR-2Q2R-7AK4-9QZU",
    [string]$TestDir = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $TestDir) {
    $TestDir = Join-Path $root "dist"
}

$exe = Join-Path $TestDir "usermode\Frontier.exe"
$loader = Join-Path $TestDir "FrontierLoader.exe"
$userDir = Join-Path $TestDir "usermode"
$logDir = Join-Path $userDir "frontier"
$logFile = Join-Path $logDir "frontier.log"

Write-Host "=== FRONTIER UI crash test ===" -ForegroundColor Cyan
Write-Host "Test dir: $TestDir"

if (-not (Test-Path $exe)) {
    Write-Host "Building + staging..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot "build-loader-gui.ps1")
    $msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path $msbuild)) {
        $msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
    }
    & $msbuild (Join-Path $root "External\External.vcxproj") /p:Configuration=Release /p:Platform=x64 /v:minimal
    & (Join-Path $PSScriptRoot "stage-loader.ps1")
}

if (-not (Test-Path $exe)) { throw "Missing $exe" }

# HWID (match loader)
$computer = $env:COMPUTERNAME
$vol = (Get-Volume -DriveLetter C -ErrorAction SilentlyContinue | ForEach-Object { $_.FileSystemLabel + $_.Size })
if (-not $vol) { $vol = "C" }
$sha = [System.Security.Cryptography.SHA256]::Create()
$bytes = [Text.Encoding]::UTF8.GetBytes("$computer-$vol")
$hash = ($sha.ComputeHash($bytes)[0..15] | ForEach-Object { $_.ToString("x2") }) -join ""

Write-Host "HWID: $hash"

$body = @{ key = $Key; hwid = $hash } | ConvertTo-Json
Write-Host "Activating license..."
$resp = Invoke-RestMethod -Uri "https://www.ahead.best/api/frontier-license" -Method Post -Body $body -ContentType "application/json"
Write-Host "License API: ok=$($resp.ok) licensed=$($resp.licensed) msg=$($resp.message)"

if (-not $resp.ok -or -not $resp.token) { throw "License activation failed" }

New-Item -ItemType Directory -Force -Path $userDir | Out-Null
$sessionPath = Join-Path $userDir "frontier.session"
Set-Content -Path $sessionPath -Value "token=$($resp.token)`nhwid=$hash`n" -Encoding ASCII
Write-Host "Wrote $sessionPath"

if (Test-Path $logFile) { Remove-Item $logFile -Force }
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$env:FRONTIER_UI_LOG = "1"
Write-Host "Launching Frontier.exe (UI log -> $logFile)"
Write-Host "Join Roblox, wait for attach, toggle menu (Insert / Right Ctrl). Log updates live." -ForegroundColor Yellow

$proc = Start-Process -FilePath $exe -WorkingDirectory $userDir -PassThru
Write-Host "PID $($proc.Id)"

for ($i = 0; $i - 120; $i++) {
    Start-Sleep -Seconds 2
    if ($proc.HasExited) {
        Write-Host "Process exited code $($proc.ExitCode)" -ForegroundColor Red
        break
    }
    if (Test-Path $logFile) {
        Write-Host "--- log tail ---" -ForegroundColor DarkGray
        Get-Content $logFile -Tail 25
        Write-Host "----------------" -ForegroundColor DarkGray
    } else {
        Write-Host "Waiting for log file..."
    }
}

if (-not $proc.HasExited) {
    Write-Host "Still running after 4 min. Check $logFile manually." -ForegroundColor Green
} elseif (Test-Path $logFile) {
    Write-Host "`nFull log:" -ForegroundColor Cyan
    Get-Content $logFile
}
