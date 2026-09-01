#pragma once

#include <Windows.h>

namespace LoaderConfig {

    inline constexpr int kLocalVersion = 26;
    inline constexpr const char* kDisplayVersion = "v1.2.23";
    inline constexpr const char* kProductLine = "AHEAD Products";

    inline constexpr const char* kTraceLoaderUrl =
        "https://ahead.best/ahead-load.lua";
    inline constexpr const char* kTraceCardUrl =
        "https://ahead.best/brand/trace-aimbot.png?v=4";
    inline constexpr const char* kFrontierCardUrl =
        "https://ahead.best/brand/frontier-ui.png?v=3";
    inline constexpr const char* kTraceMascotUrl =
        "https://ahead.best/brand/trace-chan.png?v=2";
    inline constexpr const char* kFrontierMascotUrl =
        "https://ahead.best/brand/frontier-chan.png?v=2";

    inline constexpr const wchar_t* kAppName = L"FRONTIER";

    inline constexpr const wchar_t* kUsermodeExe = L"Frontier.exe";
    inline constexpr const wchar_t* kKernelExe = L"Frontier.exe";

    inline constexpr const char* kManifestUrl =
        "https://ahead.best/releases/manifest.json";
    inline constexpr const char* kFallbackUsermodeUrl =
        "https://github.com/binx-ux/frontier-releases/releases/download/v1.2.21/Frontier.exe";
    inline constexpr const char* kReleasesPage =
        "https://github.com/binx-ux/frontier-releases/releases/latest";
    inline constexpr const char* kDiscordInvite =
        "https://discord.gg/zHGKqd92Pz";

    inline constexpr const char* kLicenseApi =
        "https://trace-host.vercel.app/api/frontier-license";
    inline constexpr const char* kAccessApi =
        "https://trace-host.vercel.app/api/trace-auth";
    inline constexpr const char* kDiscordOAuthApi =
        "https://trace-host.vercel.app/api/discord-oauth";
    inline constexpr int kOAuthPort = 38473;
    inline constexpr const char* kOAuthRedirectUri =
        "http://127.0.0.1:38473/callback";

    inline constexpr bool kDiscordOAuthEnabled = false;
    inline constexpr bool kKernelModeOffered = true;
    inline constexpr const char* kKernelModeTag = "Testing";

    // Reference loader dimensions (Ezeross/Cheat-Loader-Menu-KeyAuth-GUI)
    inline constexpr int kWindowW = 580;
    inline constexpr int kWindowH = 400;
    inline constexpr int kSplitX = 0;
    inline constexpr int kMarginX = 24;
    inline constexpr int kContentW = kWindowW - kMarginX * 2;

    inline constexpr COLORREF kBgTop = RGB(10, 10, 10);
    inline constexpr COLORREF kBgBottom = RGB(10, 10, 10);
    inline constexpr COLORREF kBg = RGB(10, 10, 10);
    inline constexpr COLORREF kPanel = RGB(10, 10, 10);
    inline constexpr COLORREF kPanelLeft = RGB(10, 10, 10);
    inline constexpr COLORREF kCard = RGB(10, 10, 10);
    inline constexpr COLORREF kCardInner = RGB(17, 17, 17);
    inline constexpr COLORREF kCardHover = RGB(20, 20, 22);
    inline constexpr COLORREF kShadow = RGB(21, 19, 21);

    inline constexpr COLORREF kAccent = RGB(48, 104, 194);
    inline constexpr COLORREF kAccentHover = RGB(58, 124, 214);
    inline constexpr COLORREF kAccentDim = RGB(36, 78, 150);
    inline constexpr COLORREF kAccentLight = RGB(94, 148, 255);
    inline constexpr COLORREF kAccentGlow = RGB(24, 36, 64);

    inline constexpr COLORREF kText = RGB(255, 255, 255);
    inline constexpr COLORREF kTextDim = RGB(160, 160, 160);
    inline constexpr COLORREF kTextMuted = RGB(128, 128, 128);
    inline constexpr COLORREF kTextPlaceholder = RGB(78, 76, 78);

    inline constexpr COLORREF kBorder = RGB(23, 23, 23);
    inline constexpr COLORREF kBorderActive = RGB(94, 148, 255);
    inline constexpr COLORREF kSuccess = RGB(109, 179, 91);
    inline constexpr COLORREF kWarning = RGB(230, 170, 90);
    inline constexpr COLORREF kBarTrack = RGB(23, 23, 23);
    inline constexpr COLORREF kInputBg = RGB(23, 23, 23);
    inline constexpr COLORREF kInputBorder = RGB(23, 23, 23);
}
