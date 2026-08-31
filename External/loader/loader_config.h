#pragma once



#include <Windows.h>



namespace LoaderConfig {



    inline constexpr int kLocalVersion = 10;



    inline constexpr const wchar_t* kAppName = L"FRONTIER";



    inline constexpr const wchar_t* kUsermodeExe = L"Frontier.exe";

    inline constexpr const wchar_t* kKernelExe = L"Frontier.exe";



    inline constexpr const char* kManifestUrl =

        "https://trace-host.vercel.app/releases/manifest.json";

    inline constexpr const char* kFallbackUsermodeUrl =

        "https://github.com/binx-ux/frontier-releases/releases/download/v1.2.7/Frontier.exe";

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



    // Flat utility layout — no glow / gradient chrome
    inline constexpr int kWindowW = 780;
    inline constexpr int kWindowH = 460;
    inline constexpr int kSplitX = 248;

    inline constexpr COLORREF kBgTop = RGB(18, 18, 20);
    inline constexpr COLORREF kBgBottom = RGB(14, 14, 16);
    inline constexpr COLORREF kBg = RGB(16, 16, 18);
    inline constexpr COLORREF kPanel = RGB(20, 20, 22);
    inline constexpr COLORREF kPanelLeft = RGB(12, 12, 14);
    inline constexpr COLORREF kCard = RGB(22, 22, 24);
    inline constexpr COLORREF kCardInner = RGB(22, 22, 24);
    inline constexpr COLORREF kCardHover = RGB(28, 28, 32);
    inline constexpr COLORREF kAccent = RGB(235, 45, 35);
    inline constexpr COLORREF kAccentHover = RGB(255, 70, 58);
    inline constexpr COLORREF kAccentDim = RGB(140, 28, 22);
    inline constexpr COLORREF kAccentLight = RGB(255, 120, 110);
    inline constexpr COLORREF kAccentGlow = RGB(40, 40, 44);

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

