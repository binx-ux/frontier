<div align="center">

```
    ███████╗██╗  ██╗████████╗███████╗██████╗ ███╗   ██╗ █████╗ ██╗
    ██╔════╝╚██╗██╔╝╚══██╔══╝██╔════╝██╔══██╗████╗  ██║██╔══██╗██║
    █████╗   ╚███╔╝    ██║   █████╗  ██████╔╝██╔██╗ ██║███████║██║
    ██╔══╝   ██╔██╗    ██║   ██╔══╝  ██╔══██╗██║╚██╗██║██╔══██║██║
    ███████╗██╔╝ ██╗   ██║   ███████╗██║  ██║██║ ╚████║██║  ██║███████╗
    ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝
                              ▀   ▀   ▀   ▀   ▀   ▀   ▀   ▀   ▀   ▀
                                 B A S E   L A Y E R
```

**Open MIT foundation for Roblox externals on Windows x64**

[![License: MIT](https://img.shields.io/badge/License-MIT-red?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-111?style=for-the-badge&logo=windows)](https://github.com/binx-ux/external-base)
[![Part of AHEAD](https://img.shields.io/badge/AHEAD-trace--host-251708?style=for-the-badge)](https://trace-host.vercel.app)

*memory I/O · SDK · kernel driver source · offsets*

[Website](https://trace-host.vercel.app) · [Open vs Closed](docs/OPEN_VS_CLOSED.md) · [Security](SECURITY.md) · [Warning](WARNING.md)

</div>

---

## What is this?

**External Base** is the public, MIT-licensed plumbing behind [FRONTIER](https://trace-host.vercel.app).

Think of it as the engine room — not the whole ship:

| You get here (open) | Lives in the closed product |
|---------------------|-----------------------------|
| Process attach + read/write | Aimbot, trigger, silent aim |
| `luck.asm` usermode syscalls | ESP, chams, skeleton |
| Roblox SDK + W2S + scanner | ImGui menu + theme |
| Kernel driver **source** | FrontierLoader + Discord gate |
| Offset mirrors + sync scripts | Release binaries |

```
   ┌─────────────┐         ┌──────────────────┐
   │ External    │  MIT    │  FRONTIER product │
   │ Base (here) │ ──────► │  (closed source)  │
   │ memory·sdk  │  links  │  features·menu    │
   └─────────────┘         └──────────────────┘
```

---

## Repo map

```
external-base/
├── External/src/memory/     attach, luck.asm, kernel driver link
├── External/src/sdk/        instances, offsets.h, w2s, scanner
├── kernel/                  FrontierDrv source + IOCTL headers
├── offsets/                 mirrored dumps (theo sync)
├── scripts/                 sync-offsets.js, export-public-base.ps1
└── docs/                    what's open vs closed
```

---

## Quick start

### Sync offsets

```powershell
node scripts/sync-offsets.js
```

### Export from the private monorepo

If you maintain the full FRONTIER tree locally:

```powershell
.\scripts\export-public-base.ps1
# → ..\external-base\
```

### Kernel driver (optional)

Driver **source** is included. Built `.sys` files are **never** committed.

See [kernel/driver/README.md](kernel/driver/README.md) and [kernel/README.md](kernel/README.md).

---

## Stack at a glance

| Layer | Tech | Open? |
|-------|------|-------|
| Memory I/O | C++ · MASM (`luck.asm`) | yes |
| SDK | C++ headers | yes |
| Kernel driver | C · WDK | source yes, binaries no |
| Offsets | JSON / HPP mirrors | yes |
| Cheat features | C++ · ImGui | no |
| Loader | C++ Win32 | no |

---

## Contributing

PRs welcome for:

- offset mirror fixes
- SDK / memory layer bugs
- driver docs and IOCTL clarity
- security hardening in **open paths only**

Please read [SECURITY.md](SECURITY.md) before opening a PR.

Do **not** PR closed product code (menu, aimbot, loader) into this repo.

---

## License

| File | Covers |
|------|--------|
| [LICENSE](LICENSE) | This open base (MIT) |
| [LICENSE-PROPRIETARY.md](LICENSE-PROPRIETARY.md) | FRONTIER product (closed) |

---

<div align="center">

**See ahead.**

Maintained by [binx-ux](https://github.com/binx-ux) · [AHEAD](https://trace-host.vercel.app)

```
     ▄▀▀▀▄  open base — build on it
     ▀▄▄▄▀  closed product — ship separately
```

</div>
