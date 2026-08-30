# Kernel mode bundle

Kernel mode is **not included** in the open-source repository.

When you publish a private kernel build, place it here:

```
kernel/
  Frontier.exe      ← kernel client (launcher starts this)
  driver/           ← optional .sys + mapper (your distribution)
```

Set `kernel_available: true` in `releases/manifest.json` and point `kernel_url` at your release asset.

The loader will enable the **Kernel** card when either:

- `kernel\Frontier.exe` exists locally, or
- the remote manifest marks kernel as available (after update).

## Requirements (typical)

- Test-signing or a proper code-signing cert for the driver
- Separate kernel client build (often still C++ usermode talking to `.sys`)
- HVCI / Secure Boot may block unsigned drivers

Do not commit unsigned drivers to a public repo.
