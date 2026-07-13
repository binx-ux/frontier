# Match-Ware External

Open-source Roblox **external** (Windows x64 overlay). Free, no key, MIT.

> **Repo:** https://github.com/binx-ux/open-source-roblox-cheat-

---

## Read this first

This touches Roblox process memory. Using it on live accounts can get you **banned**.  
That risk is on you. Code is as-is, no warranty, not affiliated with Roblox.

Full text: [WARNING.md](WARNING.md) · [LICENSE](LICENSE)

---

## What’s in here

| Area | Notes |
|------|--------|
| Aimbot / trigger / silent | Game-dependent |
| ESP / radar / chams | Overlay draw |
| Gun mods | Where offsets allow |
| Movement helpers | Disabled on some places (e.g. BloxStrike profile) |
| ImGui menu | Insert / RCtrl |

Supported place IDs live in `External/src/core/games/arsenal.h`.

---

## Build

**Need:** VS with C++ desktop workload, Windows SDK, MASM.

1. Open `External.sln`
2. **Release | x64**
3. Build

Binary lands at `External\x64\Release\External.exe` (gitignored).

---

## Run

1. Join a supported experience on a client build that matches the offsets
2. Start `External.exe`
3. **Insert** or **Right Ctrl** — menu  
4. **Delete** — panic off (if enabled)

No telemetry. No Discord webhook. Nothing phones home.

---

## Offsets

Roblox updates break these constantly.

| Path | What |
|------|------|
| `External/src/sdk/offsets.h` | Header the build uses |
| `offsets/` | Dump backups |

Update after every client bump or expect a bad attach.

---

## Layout

```
External/           main app + ImGui
  src/core/         aimbot, esp, cache, config…
  src/sdk/          instances, offsets, w2s
  ext/imgui/        Dear ImGui
offsets/            dump json
```

---

## Contributing

PRs welcome for offsets, builds, and fixes.

- Don’t commit secrets, webhooks, or license servers
- Keep ban-risk stuff labeled in the PR
- See [SECURITY.md](SECURITY.md)

---

## Credits

- Base ideas from [metixud/RobloxExternalBase](https://github.com/metixud/RobloxExternalBase)
- Offsets: community dumpers / [offsets.imtheo.lol](https://offsets.imtheo.lol)-style dumps
- UI: Dear ImGui  
- Maintainer: [binx-ux](https://github.com/binx-ux)

---

Educational / research use. Ban risk is real. Don’t be weird with it.
