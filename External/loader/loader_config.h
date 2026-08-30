#pragma once
#include <Windows.h>

namespace LoaderConfig {

    inline constexpr int kLocalVersion = 0;
    inline constexpr const wchar_t* kAppName = L"FRONTIER";
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

    inline constexpr int kWindowW = 520;
    inline constexpr int kWindowH = 680;
    inline constexpr COLORREF kDiscord = RGB(88, 101, 242);

    // Monochrome dark — matches in-app Karpiware-style UI
    inline constexpr COLORREF kBg = RGB(14, 14, 16);
    inline constexpr COLORREF kCard = RGB(22, 22, 26);
    inline constexpr COLORREF kBrand = RGB(245, 245, 248);
    inline constexpr COLORREF kText = RGB(240, 240, 245);
    inline constexpr COLORREF kTextDim = RGB(120, 120, 132);
    inline constexpr COLORREF kBorder = RGB(48, 48, 56);
    inline constexpr COLORREF kAccent = RGB(88, 101, 242);

}
