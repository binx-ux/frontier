# FRONTIER

**See ahead. Built open.**

Open-source Roblox **external** for Windows x64 — overlay ESP, aimbot, ImGui menu. Part of [AHEAD](https://trace-host.vercel.app). MIT, no keys, no telemetry.

> **Repo:** https://github.com/binx-ux/frontier  
> **Site:** https://trace-host.vercel.app  
> **Status:** Maintained by [binx-ux](https://github.com/binx-ux). Offsets may be stale — verify before building. Full release target: **May 10, 2028**.

---

## Read this first

This reads Roblox process memory from outside the client. Using it on live accounts can get you **banned**. That risk is on you. Not affiliated with Roblox.

Full text: [WARNING.md](WARNING.md) · [LICENSE](LICENSE)

---

## What’s in here

| Area | Notes |
|------|--------|
| Aimbot / trigger / silent | Game-dependent |
| ESP / radar / chams | Overlay draw |
| Gun mods | Where offsets allow |
| Movement helpers | Disabled on some places |
| ImGui menu | Insert / Right Ctrl |

Supported place IDs: `External/src/core/games/arsenal.h`

---

## Build

**Need:** Visual Studio with C++ desktop workload, Windows SDK, MASM.

1. Open `External.sln`
2. **Release | x64**
3. Build

Output: `External\x64\Release\Frontier.exe`

---

## Run

1. Join a supported experience on a client build that matches the offsets
2. Start `Frontier.exe`
3. **Insert** or **Right Ctrl** — menu
4. **Delete** — panic off (if enabled)

No telemetry. No Discord webhook. Nothing phones home. Build from source or grab [GitHub Releases](https://github.com/binx-ux/frontier/releases) when published.

---

## Offsets

Roblox updates break these constantly. **FRONTIER** mirrors [theo's offsets](https://offsets.imtheo.lol/) in this repo — no build required to download them.

| Where | URL |
|-------|-----|
| Live upstream | https://offsets.imtheo.lol |
| This repo | [`offsets/`](./offsets/) · `node scripts/sync-offsets.js` |
| AHEAD catalog | https://trace-host.vercel.app/api/frontier-offsets |
| Example JSON | https://trace-host.vercel.app/api/frontier-offsets?file=Offsets.json |

| Path | What |
|------|------|
| `offsets/offsets.json` | Latest JSON mirror |
| `offsets/offsets.hpp` | Latest C++ header mirror |
| `offsets/sources.json` | Full route manifest |
| `offsets/active.json` | Active Roblox client version |
| `External/src/sdk/offsets.h` | Header used when you build |

See [offsets/README.md](./offsets/README.md). Update after every client bump or expect a failed attach.

---

## Layout

```
External/           main app + ImGui
  src/core/         aimbot, esp, cache, config…
  src/sdk/          instances, offsets, w2s
  ext/imgui/        Dear ImGui
offsets/            dump json
```

Config saves to `%USERPROFILE%\Documents\FRONTIER\`

---

## Contributing

PRs welcome for offsets, fixes, and docs.

- Don’t commit secrets, webhooks, or license servers
- Keep ban-risk stuff labeled in the PR
- See [SECURITY.md](SECURITY.md)

---

## Credits

- Base ideas from [metixud/RobloxExternalBase](https://github.com/metixud/RobloxExternalBase)
- Offsets: [theo's offsets](https://offsets.imtheo.lol) · mirrored in `offsets/`
- UI: Dear ImGui
- Maintainer: [binx-ux](https://github.com/binx-ux) · [AHEAD](https://trace-host.vercel.app)
- Original co-dev: [astwdnya](https://github.com/astwdnya)

---

Educational / research use. Ban risk is real.
