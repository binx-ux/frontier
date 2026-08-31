# FRONTIER



**See ahead.**



Roblox **external** for Windows x64 — overlay, aimbot, ImGui menu. Part of [AHEAD](https://trace-host.vercel.app).



> **Product:** closed source — distributed via loader / private releases  

> **Base:** open (MIT) — memory I/O, SDK, kernel driver source, offsets  

> **Site:** https://trace-host.vercel.app



---



## Open vs closed



| | Public (MIT) | Private (proprietary) |

|---|--------------|------------------------|

| Memory / syscalls / driver source | ✅ | — |

| SDK + offsets | ✅ | — |

| Aimbot, ESP, exploits, menu | — | ✅ |

| FrontierLoader + Discord gate | — | ✅ |

| Release binaries | — | ✅ |



Full split: [docs/OPEN_VS_CLOSED.md](docs/OPEN_VS_CLOSED.md)



To publish the public base only:



```powershell

.\scripts\export-public-base.ps1

# → ..\frontier-base\

```



---



## Read this first



This reads Roblox process memory from outside the client. Using it on live accounts can get you **banned**. That risk is on you. Not affiliated with Roblox.



Full text: [WARNING.md](WARNING.md) · Base: [LICENSE](LICENSE) · Product: [LICENSE-PROPRIETARY.md](LICENSE-PROPRIETARY.md)



---



## What's in the product (closed)



| Area | Notes |

|------|--------|

| Aimbot / trigger / silent | Game-dependent |

| ESP / radar / chams | Overlay draw |

| Gun mods | Where offsets allow |

| Movement helpers | Disabled on some places |

| ImGui menu | Insert / Right Ctrl |



Supported place IDs: private tree — `External/src/core/games/`



---



## Build (developers with private repo access)



**Need:** Visual Studio with C++ desktop workload, Windows SDK, MASM.



1. Open `External.sln`

2. **Release | x64**

3. Build



Output: `External\x64\Release\Frontier.exe`



---



## Run (users)



### Loader (recommended)



1. Get **FrontierLoader.exe** from releases (Discord / private channel — not the public base repo)

2. Join [Discord](https://discord.gg/zHGKqd92Pz) and run `/verify YourRobloxUsername`

3. Open the loader → **Sign in with Discord** → **Launch**



See [External/loader/README.md](External/loader/README.md) (private repo).



### Direct exe



Shipped only with private builds — not built from the public base export alone.



---



## Usermode vs Kernel



| | Usermode | Kernel |

|---|----------|--------|

| Driver | None | `.sys` (source open, binaries private) |

| Open base | Memory + SDK | + `kernel/driver/` source |

| Product binary | Closed | Closed |



Kernel driver source: [kernel/driver/README.md](kernel/driver/README.md)



---



## Offsets (public)



Roblox updates break these constantly. Mirrors [theo's offsets](https://offsets.imtheo.lol/) in `offsets/`.



| Path | What |

|------|------|

| `offsets/offsets.json` | Latest JSON mirror |

| `offsets/offsets.hpp` | Latest C++ header mirror |

| `External/src/sdk/offsets.h` | Header used when you build |



See [offsets/README.md](./offsets/README.md).



---



## Layout



```

kernel/             driver source + IOCTL (open)

offsets/            dump mirrors (open)

External/src/memory/   attach + read/write (open)

External/src/sdk/      instances, w2s (open)

External/           full app + features (closed — private repo)

  loader/           FrontierLoader (closed)

docs/               OPEN_VS_CLOSED.md

base/               public export manifest

scripts/            export-public-base.ps1, sync-offsets.js

```



Config saves to `%USERPROFILE%\Documents\FRONTIER\`



---



## Contributing



**Public base:** PRs welcome for offsets, memory/SDK fixes, driver docs.



**Closed product:** not accepting public PRs — contact maintainer.



- Don't commit secrets, webhooks, or license servers

- See [SECURITY.md](SECURITY.md)



---



## Credits



- Base ideas from [metixud/RobloxExternalBase](https://github.com/metixud/RobloxExternalBase)

- Offsets: [theo's offsets](https://offsets.imtheo.lol)

- Maintainer: [binx-ux](https://github.com/binx-ux) · [AHEAD](https://trace-host.vercel.app)



---



Educational / research use for the **open base**. Product use at your own risk.

