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
        "https://github.com/binx-ux/frontier-releases/releases/download/v1.2.4/Frontier.exe";
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

    // Premium split layout — matches in-game red brand
    inline constexpr int kWindowW = 820;
    inline constexpr int kWindowH = 480;
    inline constexpr int kSplitX = 300;

    inline constexpr COLORREF kBgTop = RGB(14, 14, 16);
    inline constexpr COLORREF kBgBottom = RGB(8, 8, 10);
    inline constexpr COLORREF kBg = RGB(10, 10, 12);
    inline constexpr COLORREF kPanel = RGB(16, 16, 18);
    inline constexpr COLORREF kPanelLeft = RGB(11, 11, 13);
    inline constexpr COLORREF kCard = RGB(22, 22, 26);
    inline constexpr COLORREF kCardInner = RGB(17, 17, 20);
    inline constexpr COLORREF kCardHover = RGB(30, 30, 34);
    inline constexpr COLORREF kAccent = RGB(251, 27, 8);
    inline constexpr COLORREF kAccentHover = RGB(255, 58, 42);
    inline constexpr COLORREF kAccentDim = RGB(160, 18, 6);
    inline constexpr COLORREF kAccentLight = RGB(255, 120, 100);
    inline constexpr COLORREF kAccentGlow = RGB(72, 10, 6);
    inline constexpr COLORREF kText = RGB(252, 252, 254);
    inline constexpr COLORREF kTextDim = RGB(160, 162, 172);
    inline constexpr COLORREF kTextMuted = RGB(100, 102, 112);
    inline constexpr COLORREF kBorder = RGB(38, 38, 44);
    inline constexpr COLORREF kBorderActive = RGB(251, 27, 8);
    inline constexpr COLORREF kSuccess = RGB(64, 196, 145);
    inline constexpr COLORREF kWarning = RGB(230, 170, 90);
    inline constexpr COLORREF kBarTrack = RGB(24, 24, 28);
    inline constexpr COLORREF kInputBg = RGB(12, 12, 14);
    inline constexpr COLORREF kInputBorder = RGB(56, 56, 64);
}
