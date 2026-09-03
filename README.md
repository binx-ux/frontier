# FRONTIER

**See ahead. Built open.**

Roblox **external** for Windows x64 — overlay, aimbot, ImGui menu, usermode + optional kernel driver.

- **License:** MIT (entire tree)
- **Keys:** none
- **Telemetry:** none
- **Repo:** https://github.com/binx-ux/frontier

> Using this on live Roblox accounts can get you banned. That risk is yours. Not affiliated with Roblox.

Full warning: [WARNING.md](WARNING.md)

---

## What's included

| Area | Path |
|------|------|
| Usermode external | `External/` |
| FrontierLoader (Win32) | `External/loader/` |
| FrontierLoader (WinForms) | `External/loader-gui/` |
| Kernel driver source | `kernel/` |
| Offsets mirrors | `offsets/` |
| Build / publish scripts | `scripts/` |

---

## Build (usermode)

1. Open `External.sln` in Visual Studio 2022 (x64 Release).
2. Build **External** → `Frontier.exe`.
3. Optional: build loader under `External/loader/` or `External/loader-gui/`.

Kernel mode requires a signed/test-signed driver — see `kernel/README.md`.

---

## Open source policy

No license keys, Discord gates, HWID binds, or launch telemetry.

Session / license code paths are stubs that always succeed so you can fork and run without ahead.best auth.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Keep secrets out of the tree (`.env`, webhooks, tokens).
