# FRONTIER Loader

Pre-launch updater and mode picker for FRONTIER.

## What it does

1. **Checks for updates** — reads `releases/manifest.json` from GitHub
2. **Downloads** `usermode\Frontier.exe` (and optional `kernel\` bundle)
3. **Lets users pick** Usermode vs Kernel before launch
4. **Remembers** last mode in `loader.ini`

## Build

1. Open `External.sln` in Visual Studio 2022
2. Build **Loader** and **External** — both are in the solution
3. Stage a release folder:

```
FrontierLoader.exe          ← from External\loader\x64\Release\
usermode\Frontier.exe       ← from External\x64\Release\Frontier.exe
kernel\                     ← optional private build
loader.ini                  ← created at runtime
```

Or run `scripts\stage-loader.ps1` after building.

## Release manifest

Edit `releases/manifest.json` on each release:

```json
{
  "version": 37,
  "display": "FRONTIER v37",
  "kernel_available": false,
  "usermode_url": "https://github.com/binx-ux/frontier/releases/download/v37/Frontier.exe",
  "kernel_url": ""
}
```

Push to `main` so the loader can fetch it from:

`https://raw.githubusercontent.com/binx-ux/frontier/main/releases/manifest.json`

## Language options (what to use for what)

| Layer | Best fit | Why |
|-------|----------|-----|
| **Cheat / overlay** | **C++** (current) | DirectX11, ImGui, memory reads, low latency — keep this |
| **Loader shell + updater** | **C++** (this loader) or **Rust** | Native Win32, small binary; Rust adds safer HTTP/crypto if you rewrite later |
| **Pretty loader UI fast** | **C# (WinUI 3 / WPF)** | Easier animations, auth UI, installer wizard — separate exe that downloads C++ payload |
| **Auto-update CLI / CI** | **Go** or **Node** | Simple HTTP, zip extract, good for build pipelines |
| **Auth / keys / Discord** | **TypeScript** (Vercel) | Already have trace-host — loader calls your API, not the cheat |
| **Kernel driver** | **C / C++** | WDK only — no Rust/Go for `.sys` in practice |

**Recommendation:** Keep **C++** for usermode cheat + loader. Add **Rust or C#** only if you want a fancier installer later — they would wrap the same `usermode\Frontier.exe` payload, not replace it.

## Kernel vs Usermode

| | Usermode | Kernel |
|---|----------|--------|
| Driver | None | `.sys` + mapper |
| Detection surface | Higher | Lower (if driver stable) |
| Setup | Unzip and run | Driver install, signing |
| This repo | ✅ `Frontier.exe` | ❌ placeholder only |

Usermode is the default open-source path. Kernel is optional and distributed separately.
