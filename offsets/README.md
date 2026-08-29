# FRONTIER offsets

Synced from **[theo's offsets](https://offsets.imtheo.lol/)** — not built here, just mirrored for contributors and download tools.

## Quick links

| Format | Upstream (live) | GitHub (this repo) | AHEAD proxy |
|--------|-----------------|--------------------|-------------|
| JSON | [Offsets.json](https://offsets.imtheo.lol/Offsets.json) | [offsets.json](https://raw.githubusercontent.com/binx-ux/frontier/main/offsets/offsets.json) | [api](https://trace-host.vercel.app/api/frontier-offsets?file=Offsets.json) |
| C++ header | [Offsets.hpp](https://offsets.imtheo.lol/Offsets.hpp) | [offsets.hpp](https://raw.githubusercontent.com/binx-ux/frontier/main/offsets/offsets.hpp) | [api](https://trace-host.vercel.app/api/frontier-offsets?file=Offsets.hpp) |
| C# | [Offsets.cs](https://offsets.imtheo.lol/Offsets.cs) | [offsets.cs](https://raw.githubusercontent.com/binx-ux/frontier/main/offsets/offsets.cs) | [api](https://trace-host.vercel.app/api/frontier-offsets?file=Offsets.cs) |
| Text | [Offsets.txt](https://offsets.imtheo.lol/Offsets.txt) | [offsets.txt](https://raw.githubusercontent.com/binx-ux/frontier/main/offsets/offsets.txt) | [api](https://trace-host.vercel.app/api/frontier-offsets?file=Offsets.txt) |
| Hex JSON | [OffsetsHex.json](https://offsets.imtheo.lol/OffsetsHex.json) | [offsets-hex.json](https://raw.githubusercontent.com/binx-ux/frontier/main/offsets/offsets-hex.json) | [api](https://trace-host.vercel.app/api/frontier-offsets?file=OffsetsHex.json) |

Full manifest: [sources.json](./sources.json) · AHEAD catalog: [trace-host.vercel.app/api/frontier-offsets](https://trace-host.vercel.app/api/frontier-offsets)

## Sync locally

```bash
node scripts/sync-offsets.js
```

This downloads the latest dumps from imtheo, updates `offsets/*`, sets `active.json`, and refreshes `External/src/sdk/offsets.h` for source builds (no compile step).

## Build note

Roblox client updates break offsets often. Always match **Roblox Version** in the JSON header to your client before attaching.
