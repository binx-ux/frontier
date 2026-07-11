#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <cstring>
#include <cstdio>

#pragma comment(lib, "winhttp.lib")

// Optional anonymous launch ping. Disabled unless you set a webhook path below.
// Never put secrets in public forks. Payload never includes HWID, username, key, or PC name.
namespace Telemetry {

    inline std::atomic<bool> consentPending{ false };
    inline std::atomic<bool> consented{ false };
    inline std::atomic<bool> decided{ false };
    inline std::atomic<bool> sentThisSession{ false };
    inline float animT = 0.f;

    // Leave empty for open-source builds (no network ping).
    inline constexpr const char* kWebhookHost = "discord.com";
    inline constexpr const char* kWebhookPath = "";

    inline bool WebhookConfigured()
    {
        return kWebhookPath != nullptr && kWebhookPath[0] != '\0';
    }

    inline std::string PrefPath()
    {
        char* appdata = nullptr;
        size_t len = 0;
        if (_dupenv_s(&appdata, &len, "APPDATA") != 0 || !appdata)
            return "mw_telemetry.dat";
        std::string dir = std::string(appdata) + "\\MatchWare";
        free(appdata);
        CreateDirectoryA(dir.c_str(), nullptr);
        return dir + "\\telemetry.dat";
    }

    inline void Load()
    {
        if (!WebhookConfigured()) {
            decided = true;
            consented = false;
            consentPending = false;
            return;
        }
        std::ifstream f(PrefPath());
        if (!f) {
            decided = false;
            consented = false;
            consentPending = true;
            return;
        }
        std::string line;
        std::getline(f, line);
        if (line == "1" || line == "agree") {
            decided = true;
            consented = true;
            consentPending = false;
        }
        else if (line == "0" || line == "deny") {
            decided = true;
            consented = false;
            consentPending = false;
        }
        else {
            decided = false;
            consented = false;
            consentPending = true;
        }
    }

    inline void Save(bool agree)
    {
        decided = true;
        consented = agree;
        consentPending = false;
        std::ofstream f(PrefPath(), std::ios::trunc);
        if (f) f << (agree ? "1" : "0");
    }

    inline void PostLaunchPing()
    {
        if (!WebhookConfigured()) return;
        if (!consented.load()) return;
        if (sentThisSession.exchange(true)) return;

        std::thread([]() {
            const char* body = "{\"content\":\"Match-Ware External launched\"}";

            HINTERNET session = WinHttpOpen(L"MatchWare/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!session) return;

            wchar_t host[64]{};
            wchar_t path[512]{};
            MultiByteToWideChar(CP_UTF8, 0, kWebhookHost, -1, host, 64);
            MultiByteToWideChar(CP_UTF8, 0, kWebhookPath, -1, path, 512);

            HINTERNET connect = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (!connect) { WinHttpCloseHandle(session); return; }

            HINTERNET request = WinHttpOpenRequest(connect, L"POST", path, nullptr,
                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (!request) {
                WinHttpCloseHandle(connect);
                WinHttpCloseHandle(session);
                return;
            }

            const wchar_t* headers = L"Content-Type: application/json\r\n";
            DWORD bodyLen = (DWORD)strlen(body);
            WinHttpSendRequest(request, headers, (DWORD)-1,
                (LPVOID)body, bodyLen, bodyLen, 0);
            WinHttpReceiveResponse(request, nullptr);

            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
        }).detach();
    }

    inline void Agree()
    {
        Save(true);
        PostLaunchPing();
    }

    inline void Deny()
    {
        Save(false);
    }

    inline void OnReady()
    {
        Load();
        if (decided.load() && consented.load())
            PostLaunchPing();
        else if (!decided.load())
            consentPending = true;
    }
}
