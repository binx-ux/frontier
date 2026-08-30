# FrontierDrv (WDK)

Kernel driver source for FRONTIER kernel mode memory reads/writes.

## Build (requires Windows Driver Kit)

1. Install **Visual Studio** + **Windows Driver Kit (WDK)**
2. Create a new **Kernel Mode Driver (KMDF)** empty project, or add `frontier_drv.c` to a WDK driver project
3. Target **x64 / Release**
4. Output: `FrontierDrv.sys`
5. Copy to:

```
kernel/driver/FrontierDrv.sys
```

Or next to the loader:

```
dist/kernel/driver/FrontierDrv.sys
```

## Load requirements

- Run **FrontierLoader** or **kernel\Frontier.exe** as **Administrator**
- Enable **test signing** for unsigned builds:
  ```
  bcdedit /set testsigning on
  ```
  Reboot required.
- Or sign the driver with a proper code-signing cert

The kernel client auto-loads `driver\FrontierDrv.sys` via the Windows service manager on first attach.

## Device

- Device path: `\\.\FrontierDrv`
- IOCTLs: see `kernel/shared/frontier_ioctl.h`

## Do not commit

- `*.sys`, `*.pdb` from driver builds — keep in private releases only
