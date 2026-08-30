#pragma once
#include <Windows.h>

namespace LoaderConfig {

    inline constexpr int kLocalVersion = 0;
    inline constexpr const wchar_t* kAppName = L"FRONTIER Loader";
    inline constexpr const wchar_t* kUsermodeExe = L"Frontier.exe";
    inline constexpr const wchar_t* kKernelExe = L"Frontier.exe";
    inline constexpr const char* kManifestUrl =
        "https://raw.githubusercontent.com/binx-ux/frontier/main/releases/manifest.json";
    inline constexpr const char* kReleasesPage =
        "https://github.com/binx-ux/frontier/releases";
    inline constexpr const char* kDiscordInvite =
        "https://discord.gg/zHGKqd92Pz";
    inline constexpr const char* kAccessApi =
        "https://trace-host.vercel.app/api/trace-auth";
    inline constexpr const char* kDiscordOAuthApi =
        "https://trace-host.vercel.app/api/discord-oauth";
    inline constexpr const char* kOAuthRedirectUri =
        "http://127.0.0.1:38473/callback";
    inline constexpr int kOAuthPort = 38473;

    inline constexpr int kWindowW = 480;
    inline constexpr int kWindowH = 620;
    inline constexpr COLORREF kDiscord = RGB(88, 101, 242);

    // Theme — matches in-app FRONTIER red/dark
    inline constexpr COLORREF kBg = RGB(12, 12, 12);
    inline constexpr COLORREF kCard = RGB(21, 21, 21);
    inline constexpr COLORREF kBrand = RGB(235, 56, 71);
    inline constexpr COLORREF kText = RGB(240, 240, 245);
    inline constexpr COLORREF kTextDim = RGB(132, 132, 144);
    inline constexpr COLORREF kBorder = RGB(48, 48, 56);

}
