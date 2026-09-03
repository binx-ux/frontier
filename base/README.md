# FRONTIER Base (open)

Public, MIT-licensed building blocks for Roblox externals on Windows x64.

This is **not** the full FRONTIER cheat. The product (menu, aimbot, ESP, loader,
releases) is **closed source** and distributed separately.

## What's included

- **Memory I/O** — usermode syscalls (`luck.asm`) + kernel driver link
- **SDK** — instances, offsets, world-to-screen, pattern scanner
- **Kernel driver source** — WDK project under `kernel/driver/` (no built `.sys` in git)
- **Offsets** — mirrored dumps + sync script

See [docs/OPEN_VS_CLOSED.md](../docs/OPEN_VS_CLOSED.md) for the full split.

## Build (base only)

The open tree is intentionally **minimal**. It documents and ships infrastructure;
the closed `Frontier.exe` links against this layer in the private repo.

To work on the full product, use the private FRONTIER repository.

## Export

From the private monorepo:

```powershell
.\scripts\export-public-base.ps1
```

## License

MIT for paths listed in [MANIFEST.json](./MANIFEST.json). Product binaries and
feature source are proprietary — [LICENSE-PROPRIETARY.md](../LICENSE-PROPRIETARY.md).
