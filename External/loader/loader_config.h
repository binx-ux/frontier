#pragma once



#include <Windows.h>



namespace LoaderConfig {



    inline constexpr int kLocalVersion = 1;

    inline constexpr const char* kDisplayVersion = "v1.3.3";

    inline constexpr const char* kProductLine = "AHEAD Products";



    inline constexpr const char* kTraceLoaderUrl = "https://ahead.best/ahead-load.lua";



    inline constexpr const wchar_t* kAppName = L"FRONTIER";

    inline constexpr const wchar_t* kUsermodeExe = L"Frontier.exe";

    inline constexpr const wchar_t* kKernelExe = L"Frontier.exe";



    inline constexpr const char* kManifestUrl = "https://www.ahead.best/releases/manifest.json";

    inline constexpr const char* kFallbackUsermodeUrl =

        "https://github.com/binx-ux/frontier-releases/releases/download/v1.3.0/Frontier.exe";

    inline constexpr const char* kReleasesPage =

        "https://github.com/binx-ux/frontier-releases/releases/latest";

    inline constexpr const char* kDiscordInvite = "https://discord.gg/zHGKqd92Pz";

    // Open-source: license / OAuth APIs disabled
    inline constexpr const char* kLicenseApi = "";
    inline constexpr const char* kAccessApi = "";
    inline constexpr const char* kDiscordOAuthApi = "";
    inline constexpr int kOAuthPort = 38473;
    inline constexpr const char* kOAuthRedirectUri = "http://127.0.0.1:38473/callback";
    inline constexpr bool kDiscordOAuthEnabled = false;

    inline constexpr bool kKernelModeOffered = true;

    inline constexpr bool kFrontierMaintenance = false;

    inline constexpr const char* kFrontierMaintenanceMsg =

        "FRONTIER is temporarily down. Check ahead.best for status.";

    inline constexpr const char* kKernelModeTag = "Kernel";



    // MwByte glass layout

    inline constexpr int kWindowW = 800;

    inline constexpr int kWindowH = 460;

    inline constexpr int kBrandPanelW = 248;

    inline constexpr int kMarginX = 28;

    inline constexpr int kHeaderH = 36;



    inline constexpr COLORREF kBgTop = RGB(8, 12, 20);

    inline constexpr COLORREF kBgBottom = RGB(5, 8, 14);

    inline constexpr COLORREF kBg = RGB(8, 12, 20);

    inline constexpr COLORREF kBrandPanel = RGB(10, 14, 22);

    inline constexpr COLORREF kPanel = RGB(12, 16, 26);

    inline constexpr COLORREF kCard = RGB(14, 18, 28);

    inline constexpr COLORREF kCardHover = RGB(18, 24, 36);

    inline constexpr COLORREF kCardInner = RGB(16, 20, 32);

    inline constexpr COLORREF kShadow = RGB(0, 0, 0);



    inline constexpr COLORREF kAccent = RGB(56, 189, 248);

    inline constexpr COLORREF kAccentHover = RGB(96, 205, 255);

    inline constexpr COLORREF kAccentDim = RGB(30, 120, 170);

    inline constexpr COLORREF kAccentLight = RGB(186, 230, 253);



    inline constexpr COLORREF kTraceAccent = RGB(129, 140, 248);

    inline constexpr COLORREF kFrontierAccent = RGB(56, 189, 248);



    inline constexpr COLORREF kText = RGB(244, 247, 251);

    inline constexpr COLORREF kTextDim = RGB(139, 149, 168);

    inline constexpr COLORREF kTextMuted = RGB(98, 108, 128);

    inline constexpr COLORREF kTextPlaceholder = RGB(72, 82, 98);



    inline constexpr COLORREF kBorder = RGB(42, 48, 62);

    inline constexpr COLORREF kBorderActive = RGB(56, 189, 248);

    inline constexpr COLORREF kSuccess = RGB(74, 222, 128);

    inline constexpr COLORREF kWarning = RGB(251, 191, 36);

    inline constexpr COLORREF kBarTrack = RGB(18, 22, 32);

    inline constexpr COLORREF kInputBg = RGB(10, 12, 18);

    inline constexpr COLORREF kInputBorder = RGB(48, 54, 68);

}

