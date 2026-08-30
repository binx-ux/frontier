# Kernel mode bundle

Kernel mode uses a **driver** (`.sys`) + **kernel client** (`Frontier.exe`) that talks to the driver instead of `OpenProcess`.

## Layout

```
kernel/
  Frontier.exe              ← ReleaseKernel build (FRONTIER_KERNEL)
  driver/
    FrontierDrv.sys         ← WDK driver (private build, do not commit)
```

Loader distribution:

```
FrontierLoader.exe
usermode/Frontier.exe       ← normal Release|x64 build
kernel/Frontier.exe         ← ReleaseKernel|x64 build
kernel/driver/FrontierDrv.sys
```

## Build kernel client

```powershell
.\scripts\build-kernel.ps1
```

Or in Visual Studio: **ReleaseKernel | x64** on the **External** project.

Output: `kernel\Frontier.exe`

## Build driver

See [driver/README.md](driver/README.md). Source: `kernel/driver/frontier_drv/frontier_drv.c`

Requires WDK. Copy built `FrontierDrv.sys` to `kernel\driver\`.

## Run (local)

1. Enable test signing (unsigned driver) or use a signed `.sys`
2. Run **FrontierLoader as Administrator**
3. Pick **Kernel** mode → **Launch**
4. Kernel client loads `driver\FrontierDrv.sys` and attaches via IOCTL

## Publish via loader manifest

```json
{
  "kernel_available": true,
  "kernel_url": "https://github.com/you/frontier/releases/download/v37/Frontier-Kernel.exe",
  "driver_url": "https://github.com/you/frontier/releases/download/v37/FrontierDrv.sys"
}
```

Push to `releases/manifest.json` on `main`.

## vs Usermode

| | Usermode | Kernel |
|---|----------|--------|
| Build config | Release \| x64 | ReleaseKernel \| x64 |
| Memory | Direct syscalls (`luck.asm`) | Driver IOCTL |
| Admin | No | Yes (driver load) |
| Driver | None | `FrontierDrv.sys` |

## Shared headers

- `kernel/shared/frontier_ioctl.h` — IOCTL layout for driver + client

Do **not** commit unsigned `.sys` files to the public GitHub repo. Ship via private GitHub Releases.
