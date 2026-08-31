# FrontierDrv (WDK)

Kernel driver source for FRONTIER kernel mode memory reads/writes.

## Build (requires Windows Driver Kit)

1. Install **Visual Studio** + **Windows Driver Kit (WDK)**:
   ```powershell
   winget install Microsoft.WindowsWDK.10.0.26100
   ```
2. Build from repo root:
   ```powershell
   .\scripts\build-driver.ps1
   ```
   Or open `kernel\driver\frontier_drv\frontier_drv.vcxproj` in Visual Studio (**Release | x64**).
3. Output: `kernel\driver\FrontierDrv.sys`

## Load requirements

- Run **FrontierLoader** or **kernel\Frontier.exe** as **Administrator**
- **Secure Boot must be OFF** in BIOS/UEFI. If `bcdedit` says *"protected by Secure Boot policy"*, disable Secure Boot first (Settings → Recovery → Advanced startup → UEFI Firmware Settings).
- Enable **test signing** for unsigned builds (required — service exit code **577** means signing is blocked):
  ```powershell
  # Elevated PowerShell:
  .\scripts\enable-kernel-mode.ps1
  ```
  Check status anytime:
  ```powershell
  .\scripts\check-kernel-mode.ps1
  ```
  **Reboot required** after enabling test signing. You should see "Test Mode" on the desktop.

**One boot without BIOS changes:** Shift+Restart → Troubleshoot → Advanced → Startup Settings → Restart → press **7** (Disable driver signature enforcement). Must repeat every boot.

- Or sign the driver with a proper EV code-signing certificate (works with Secure Boot on)

If you cannot disable Secure Boot, use **Usermode** in the loader — it does not need a driver.

## Device

- Device path: `\\.\FrontierDrv`
- IOCTLs: see `kernel/shared/frontier_ioctl.h`

## Do not commit

- `*.sys`, `*.pdb` from driver builds — keep in private releases only
