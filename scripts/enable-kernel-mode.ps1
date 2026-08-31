# Enable unsigned kernel driver loading (test signing). Requires Administrator + reboot.
#Requires -RunAsAdministrator

$ErrorActionPreference = 'Stop'

function Test-SecureBootEnabled {
    try {
        return (Confirm-SecureBootUEFI -ErrorAction Stop)
    } catch {
        return $null
    }
}

function Show-SecureBootInstructions {
    Write-Host ""
    Write-Host "Secure Boot is ON — Windows will not allow test signing while it is enabled." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To use kernel mode you must disable Secure Boot in firmware (BIOS/UEFI):" -ForegroundColor Cyan
    Write-Host "  1. Settings -> System -> Recovery -> Advanced startup -> Restart now"
    Write-Host "  2. Troubleshoot -> Advanced options -> UEFI Firmware Settings -> Restart"
    Write-Host "  3. Find Secure Boot (often under Security or Boot) and set it to Disabled"
    Write-Host "  4. Save and exit, boot back into Windows"
    Write-Host "  5. Run this script again from an elevated PowerShell"
    Write-Host ""
    Write-Host "One-session alternative (no BIOS change, must repeat every boot):" -ForegroundColor Cyan
    Write-Host "  Hold Shift -> Restart -> Troubleshoot -> Advanced options"
    Write-Host "  -> Startup Settings -> Restart -> press 7 (Disable driver signature enforcement)"
    Write-Host ""
    Write-Host "If you cannot disable Secure Boot, use Usermode in the loader instead." -ForegroundColor Green
}

$secureBoot = Test-SecureBootEnabled
if ($secureBoot -eq $true) {
    Show-SecureBootInstructions
    exit 2
}

Write-Host "Enabling test signing for FrontierDrv.sys ..." -ForegroundColor Cyan
$bcdOut = bcdedit /set testsigning on 2>&1 | Out-String
Write-Host $bcdOut

if ($LASTEXITCODE -ne 0) {
    if ($bcdOut -match 'Secure Boot') {
        Show-SecureBootInstructions
        exit 2
    }
    Write-Error "bcdedit failed. Run this script from an elevated PowerShell."
    exit 1
}

Write-Host ""
Write-Host "Test signing enabled. Reboot Windows, then:" -ForegroundColor Green
Write-Host "  1. Run dist\FrontierLoader.exe as Administrator"
Write-Host "  2. Choose Kernel mode"
Write-Host ""
Write-Host "After reboot you should see a 'Test Mode' watermark on the desktop."
Write-Host "To disable later: bcdedit /set testsigning off  (then reboot)"
