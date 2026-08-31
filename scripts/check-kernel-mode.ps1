# Report whether this PC can load the unsigned FrontierDrv.sys driver.

Write-Host "FRONTIER kernel mode readiness" -ForegroundColor Cyan
Write-Host ""

try {
    $sb = Confirm-SecureBootUEFI
    if ($sb) {
        Write-Host "Secure Boot:  ON  (blocks test signing)" -ForegroundColor Yellow
    } else {
        Write-Host "Secure Boot:  OFF" -ForegroundColor Green
    }
} catch {
    Write-Host "Secure Boot:  unknown ($($_.Exception.Message))"
}

$ts = bcdedit /enum "{current}" 2>$null | Select-String -Pattern "testsigning"
if ($ts) {
    Write-Host "Test signing: $ts"
} else {
    Write-Host "Test signing: unknown (run elevated for full bcdedit status)"
}

Write-Host ""
$svc = sc.exe query FrontierDrv 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Host "FrontierDrv service:"
    $svc | ForEach-Object { Write-Host "  $_" }
} else {
    Write-Host "FrontierDrv service: not installed yet (normal before first kernel launch)"
}

Write-Host ""
Write-Host "Kernel bundle:"
$root = Split-Path -Parent $PSScriptRoot
$paths = @(
    (Join-Path $root "dist\kernel\Frontier.exe"),
    (Join-Path $root "dist\kernel\driver\FrontierDrv.sys")
)
foreach ($p in $paths) {
    if (Test-Path $p) {
        $i = Get-Item $p
        Write-Host ("  OK  {0} ({1} bytes)" -f $i.FullName, $i.Length)
    } else {
        Write-Host "  missing $p" -ForegroundColor Yellow
    }
}
