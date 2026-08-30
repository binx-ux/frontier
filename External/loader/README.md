# FRONTIER Loader

Pre-launch updater, Discord gate, and mode picker for FRONTIER.

## What it does

1. **Requires Discord access** — join the server and run `/verify YourRobloxName` before launch
2. **Checks for updates** — reads `releases/manifest.json` from GitHub
3. **Downloads** `usermode\Frontier.exe` (and optional `kernel\` bundle)
4. **Lets users pick** Usermode vs Kernel before launch
5. **Remembers** last mode and verified account in `loader.ini`

## Discord access flow

1. Run `FrontierLoader.exe`
2. Click **Join Discord** → https://discord.gg/zHGKqd92Pz
3. In Discord, run: `/verify YourRobloxUsername`
4. Back in the loader, enter the same Roblox username and click **Verify**
5. Once verified, **Launch** unlocks

Verification hits `https://trace-host.vercel.app/api/trace-auth?userId=…` — the same whitelist used by TRACE scripts. Set `"access_required": false` in the manifest only if you want to disable the gate for a private build.

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
  "access_required": true,
  "discord_invite": "https://discord.gg/zHGKqd92Pz",
  "usermode_url": "https://github.com/binx-ux/frontier/releases/download/v37/Frontier.exe",
  "kernel_url": ""
}
```

Push to `main` so the loader can fetch it from:

`https://raw.githubusercontent.com/binx-ux/frontier/main/releases/manifest.json`

---

## Language options (what to use for what)

Pick the language for the **layer** you're building — not one language for everything.

| Layer | Language | When to use it |
|-------|----------|----------------|
| **Cheat / overlay / memory** | **C++** | DirectX11, ImGui, ReadProcessMemory, low latency — **this is FRONTIER today** |
| **Loader + updater (native)** | **C++** or **Rust** | Small Win32 shell, HTTP downloads; Rust if you want safer parsing/crypto |
| **Loader UI (fancy installer)** | **C# (WinUI 3 / WPF)** | Animations, login wizards, MSI-style flows — wraps the same `Frontier.exe` payload |
| **Internal / executor scripts** | **Luau (Roblox)** | In-game logic only — not for the external EXE |
| **Automation / CI / staging** | **Python**, **Go**, **Node/TS** | Zip staging, manifest bumps, release uploads, offset sync scripts |
| **Auth / whitelist / Discord bot** | **TypeScript (Node)** | Vercel API (`trace-auth`), Discord bot `/verify` — loader calls this, cheat does not |
| **Web dashboard / docs site** | **TypeScript**, **HTML/CSS** | trace-host, preview pages, offset catalog |
| **Kernel driver (`.sys`)** | **C / C++ (WDK)** | Ring-0 memory access — Windows Driver Kit only |
| **Kernel mapper / manual map** | **C++**, **Assembly (x64)** | Shellcode, PE mapping — keep isolated from main cheat source |
| **Reverse engineering / offsets** | **C++**, **Python**, **IDA/Ghidra scripts** | Dump tools, pattern scans, header generation |
| **Config / data pipelines** | **JSON**, **Python**, **Node** | Offset mirrors, manifest files, build metadata |
| **Cross-platform tooling** | **Rust**, **Go** | Portable CLIs that don't need Win32 — good for updaters on other OSes later |
| **Game-specific modules** | **C++ headers** | Per-place logic in `External/src/core/games/` |

### Quick picks

| Goal | Best choice |
|------|-------------|
| Keep shipping FRONTIER as-is | **C++** everywhere in `External/` |
| Safer HTTP loader rewrite | **Rust** (`reqwest` + `winapi`) |
| Pretty installer with license screen | **C# WinUI 3** downloading C++ binary |
| Discord verify + whitelist API | **TypeScript** on Vercel (already done) |
| Nightly offset sync | **Python** or **Node** (`scripts/sync-offsets.js`) |
| Private kernel build | **C++ WDK** — separate repo, never mix with open usermode |

**Recommendation:** Keep **C++** for usermode cheat + loader. Add **Rust or C#** only if you want a fancier installer — they wrap the same `usermode\Frontier.exe`, they don't replace it.

---

## Kernel vs Usermode — what's the difference?

Both builds run the same cheat features (ESP, aimbot, menu). The difference is **where** they read game memory from and **what** you need installed.

### Usermode (default — this repo)

Runs as a normal Windows program (`Frontier.exe`). It uses `OpenProcess` + `ReadProcessMemory` (or equivalent) to read Roblox from **outside** the game, then draws an overlay on top.

| | |
|---|---|
| **Privilege** | Ring 3 — same as any app |
| **Driver** | None |
| **Install** | Unzip and run |
| **How it reads memory** | OS API into Roblox's process |
| **Detection surface** | Higher — external handles and overlay are visible to anti-cheat |
| **Stability** | Usually fine; breaks on Roblox updates (offsets), not driver signing |
| **Open source** | ✅ Yes — `External/` builds this |

**Use usermode if:** you want the public build, easy setup, no driver headaches, or you're learning how externals work.

### Kernel (optional — not in public releases)

Uses a **kernel driver** (`.sys`) to read/write memory from **Ring 0**, below normal process protections. Often paired with a mapper to load the unsigned driver.

| | |
|---|---|
| **Privilege** | Ring 0 — kernel |
| **Driver** | `.sys` + loader/mapper |
| **Install** | Driver load, test signing or vulnerable driver exploit |
| **How it reads memory** | Direct physical/virtual reads from kernel |
| **Detection surface** | Lower *if* the driver stays hidden — but drivers themselves get signature-scanned |
| **Stability** | BSOD risk, Windows updates break drivers, signing is a legal/ops problem |
| **Open source** | ❌ Placeholder only — distributed separately if at all |

**Use kernel if:** you operate a private build, accept driver maintenance, and need lower-level access — **not** for the default GitHub download.

### Side-by-side

| | Usermode | Kernel |
|---|----------|--------|
| Driver required | No | Yes (`.sys`) |
| Setup time | Seconds | Minutes + reboot/signing |
| Ban / detection risk | Higher | Lower in theory, driver bans possible |
| BSOD risk | Low | Non-zero |
| Matches this repo | ✅ `usermode\Frontier.exe` | ❌ `kernel\` folder placeholder |
| Loader mode | **Usermode** card | **Kernel** card (only if `kernel\Frontier.exe` exists) |

The loader lets users pick a mode before launch. **Usermode is the default open-source path.** Kernel is optional and only appears when a kernel bundle is present locally or in the release manifest.
