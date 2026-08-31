#pragma once



#include <Windows.h>



namespace LoaderConfig {



    inline constexpr int kLocalVersion = 13;
    inline constexpr const char* kDisplayVersion = "v1.2.10";
    inline constexpr const char* kProductLine = "Roblox External";



    inline constexpr const wchar_t* kAppName = L"FRONTIER";



    inline constexpr const wchar_t* kUsermodeExe = L"Frontier.exe";

    inline constexpr const wchar_t* kKernelExe = L"Frontier.exe";



    inline constexpr const char* kManifestUrl =

        "https://trace-host.vercel.app/releases/manifest.json";

    inline constexpr const char* kFallbackUsermodeUrl =

        "https://github.com/binx-ux/frontier-releases/releases/download/v1.2.10/Frontier.exe";

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



    // Flat utility layout — refined spacing & motion-friendly size
    inline constexpr int kWindowW = 820;
    inline constexpr int kWindowH = 480;
    inline constexpr int kSplitX = 268;

    inline constexpr COLORREF kBgTop = RGB(14, 20, 36);
    inline constexpr COLORREF kBgBottom = RGB(8, 12, 24);
    inline constexpr COLORREF kBg = RGB(10, 14, 28);
    inline constexpr COLORREF kPanel = RGB(14, 20, 36);
    inline constexpr COLORREF kPanelLeft = RGB(8, 12, 26);
    inline constexpr COLORREF kCard = RGB(16, 24, 42);
    inline constexpr COLORREF kCardInner = RGB(18, 26, 46);
    inline constexpr COLORREF kCardHover = RGB(22, 32, 56);
    inline constexpr COLORREF kAccent = RGB(56, 118, 255);
    inline constexpr COLORREF kAccentHover = RGB(88, 148, 255);
    inline constexpr COLORREF kAccentDim = RGB(34, 72, 168);
    inline constexpr COLORREF kAccentLight = RGB(140, 182, 255);
    inline constexpr COLORREF kAccentGlow = RGB(24, 36, 64);

    inline constexpr COLORREF kText = RGB(248, 250, 255);

    inline constexpr COLORREF kTextDim = RGB(168, 178, 204);

    inline constexpr COLORREF kTextMuted = RGB(108, 118, 142);

    inline constexpr COLORREF kBorder = RGB(34, 44, 68);

    inline constexpr COLORREF kBorderActive = RGB(88, 148, 255);

    inline constexpr COLORREF kSuccess = RGB(64, 168, 240);

    inline constexpr COLORREF kWarning = RGB(230, 170, 90);

    inline constexpr COLORREF kBarTrack = RGB(18, 26, 44);

    inline constexpr COLORREF kInputBg = RGB(12, 12, 14);

    inline constexpr COLORREF kInputBorder = RGB(56, 56, 64);

}

