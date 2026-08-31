#pragma once

#include <Windows.h>

namespace LoaderConfig {

    inline constexpr int kLocalVersion = 7;

    inline constexpr const wchar_t* kAppName = L"FRONTIER";

    inline constexpr const wchar_t* kUsermodeExe = L"Frontier.exe";
    inline constexpr const wchar_t* kKernelExe = L"Frontier.exe";

    inline constexpr const char* kManifestUrl =
        "https://trace-host.vercel.app/releases/manifest.json";
    inline constexpr const char* kFallbackUsermodeUrl =
        "https://github.com/binx-ux/frontier-releases/releases/download/v1.1/Frontier.exe";
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

    // Kernel stays in the loader for testing; Usermode is the supported default.
    inline constexpr bool kKernelModeOffered = true;
    inline constexpr const char* kKernelModeTag = "Testing";

    // Premium split layout
    inline constexpr int kWindowW = 760;
    inline constexpr int kWindowH = 440;
    inline constexpr int kSplitX = 272;

    inline constexpr COLORREF kBgTop = RGB(22, 24, 28);
    inline constexpr COLORREF kBgBottom = RGB(12, 13, 16);
    inline constexpr COLORREF kBg = RGB(16, 17, 20);
    inline constexpr COLORREF kPanel = RGB(20, 22, 26);
    inline constexpr COLORREF kPanelLeft = RGB(14, 15, 18);
    inline constexpr COLORREF kCard = RGB(28, 30, 36);
    inline constexpr COLORREF kCardInner = RGB(22, 24, 28);
    inline constexpr COLORREF kCardHover = RGB(34, 36, 42);
    inline constexpr COLORREF kAccent = RGB(91, 110, 245);
    inline constexpr COLORREF kAccentHover = RGB(112, 128, 255);
    inline constexpr COLORREF kAccentDim = RGB(62, 74, 180);
    inline constexpr COLORREF kAccentLight = RGB(140, 152, 255);
    inline constexpr COLORREF kAccentGlow = RGB(48, 58, 120);
    inline constexpr COLORREF kText = RGB(248, 249, 252);
    inline constexpr COLORREF kTextDim = RGB(118, 124, 138);
    inline constexpr COLORREF kTextMuted = RGB(82, 88, 100);
    inline constexpr COLORREF kBorder = RGB(42, 46, 54);
    inline constexpr COLORREF kBorderActive = RGB(91, 110, 245);
    inline constexpr COLORREF kSuccess = RGB(64, 196, 145);
    inline constexpr COLORREF kWarning = RGB(230, 170, 90);
    inline constexpr COLORREF kBarTrack = RGB(32, 34, 40);
    inline constexpr COLORREF kInputBg = RGB(18, 20, 24);
    inline constexpr COLORREF kInputBorder = RGB(50, 54, 64);
}
