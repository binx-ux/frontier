#pragma once

#include <Windows.h>

namespace LoaderConfig {

    inline constexpr int kLocalVersion = 35;
    inline constexpr const char* kDisplayVersion = "v1.2.31";
    inline constexpr const char* kProductLine = "AHEAD Products";

    inline constexpr const char* kTraceLoaderUrl = "https://ahead.best/ahead-load.lua";

    inline constexpr const wchar_t* kAppName = L"FRONTIER";
    inline constexpr const wchar_t* kUsermodeExe = L"Frontier.exe";
    inline constexpr const wchar_t* kKernelExe = L"Frontier.exe";

    inline constexpr const char* kManifestUrl = "https://ahead.best/releases/manifest.json";
    inline constexpr const char* kFallbackUsermodeUrl =
        "https://github.com/binx-ux/frontier-releases/releases/download/v1.2.31/Frontier.exe";
    inline constexpr const char* kReleasesPage =
        "https://github.com/binx-ux/frontier-releases/releases/latest";
    inline constexpr const char* kDiscordInvite = "https://discord.gg/zHGKqd92Pz";

    inline constexpr const char* kLicenseApi = "https://ahead.best/api/frontier-license";
    inline constexpr const char* kAccessApi = "https://ahead.best/api/trace-auth";
    inline constexpr const char* kDiscordOAuthApi = "https://ahead.best/api/discord-oauth";
    inline constexpr int kOAuthPort = 38473;
    inline constexpr const char* kOAuthRedirectUri = "http://127.0.0.1:38473/callback";

    inline constexpr bool kDiscordOAuthEnabled = false;
    inline constexpr bool kKernelModeOffered = true;
    inline constexpr bool kFrontierMaintenance = true;
    inline constexpr const char* kFrontierMaintenanceMsg =
        "FRONTIER is temporarily down. ETA ~2 weeks.";
    inline constexpr const char* kKernelModeTag = "Testing";

    // erium-inspired split layout
    inline constexpr int kWindowW = 760;
    inline constexpr int kWindowH = 440;
    inline constexpr int kBrandPanelW = 260;
    inline constexpr int kMarginX = 24;
    inline constexpr int kHeaderH = 36;

    inline constexpr COLORREF kBgTop = RGB(11, 12, 16);
    inline constexpr COLORREF kBgBottom = RGB(7, 8, 11);
    inline constexpr COLORREF kBg = RGB(11, 12, 16);
    inline constexpr COLORREF kBrandPanel = RGB(9, 10, 14);
    inline constexpr COLORREF kPanel = RGB(15, 16, 22);
    inline constexpr COLORREF kCard = RGB(18, 19, 26);
    inline constexpr COLORREF kCardHover = RGB(24, 25, 34);
    inline constexpr COLORREF kCardInner = RGB(20, 21, 28);
    inline constexpr COLORREF kShadow = RGB(0, 0, 0);

    inline constexpr COLORREF kAccent = RGB(93, 95, 239);
    inline constexpr COLORREF kAccentHover = RGB(115, 117, 245);
    inline constexpr COLORREF kAccentDim = RGB(62, 64, 180);
    inline constexpr COLORREF kAccentLight = RGB(160, 162, 255);

    inline constexpr COLORREF kTraceAccent = RGB(139, 92, 246);
    inline constexpr COLORREF kFrontierAccent = RGB(248, 113, 113);

    inline constexpr COLORREF kText = RGB(236, 237, 242);
    inline constexpr COLORREF kTextDim = RGB(148, 152, 166);
    inline constexpr COLORREF kTextMuted = RGB(98, 102, 118);
    inline constexpr COLORREF kTextPlaceholder = RGB(72, 76, 90);

    inline constexpr COLORREF kBorder = RGB(34, 36, 48);
    inline constexpr COLORREF kBorderActive = RGB(93, 95, 239);
    inline constexpr COLORREF kSuccess = RGB(52, 211, 153);
    inline constexpr COLORREF kWarning = RGB(251, 191, 36);
    inline constexpr COLORREF kBarTrack = RGB(22, 23, 32);
    inline constexpr COLORREF kInputBg = RGB(13, 14, 20);
    inline constexpr COLORREF kInputBorder = RGB(40, 42, 56);
}
