# FrontierLoader (WinForms)

Dark WinForms loader UI for FRONTIER.

Open-source build: **no license keys**. Launch panel opens immediately.

## Build

```powershell
.\scripts\build-loader-gui.ps1
```

Or open `External/loader-gui/FrontierLoader.sln` in Visual Studio.

## Flow

1. Loader opens → launch panel (no key gate)
2. Choose Usermode / Kernel
3. Update downloads payloads from the release manifest when needed
4. Launch `Frontier.exe`
