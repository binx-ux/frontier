#pragma once
// Asset downloads removed from UI path — images caused GDI+ crashes during paint.
// Product screen is text-only; no bitmap cache needed.

namespace LoaderAssets {
    inline void EnsureGdiplus() {}
    inline void ShutdownGdiplus() {}
    inline void RefreshAssetPaths() {}
    inline void EnsureProductArt() {}
}
