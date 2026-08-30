#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "loader_config.h"
#include "loader_update.h"

namespace LoaderAuth {

    inline bool ParseUserIdFromAuthJson(const std::string& json, int& outId)
    {
        const char* key = "\"userId\":";
        size_t p = json.find(key);
        if (p == std::string::npos) return false;
        p += strlen(key);
        outId = atoi(json.c_str() + p);
        return outId > 0;
    }

    inline bool ParseAllowedFromAuthJson(const std::string& json)
    {
        size_t p = json.find("\"allowed\"");
        if (p == std::string::npos) return false;
        p = json.find(':', p);
        if (p == std::string::npos) return false;
        p++;
        while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) p++;
        return json.compare(p, 4, "true") == 0;
    }

    inline bool ResolveRobloxUserId(const char* username, int& outId, std::string& err)
    {
        outId = 0;
        if (!username || !username[0]) {
            err = "Enter your Roblox username";
            return false;
        }

        std::string body = "{\"usernames\":[\"" + std::string(username) + "\"],\"excludeBannedUsers\":true}";
        std::string url = "https://users.roblox.com/v1/usernames/users";

        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);
        wchar_t host[256]{}, path[512]{};
        uc.lpszHostName = host;
        uc.dwHostNameLength = _countof(host);
        uc.lpszUrlPath = path;
        uc.dwUrlPathLength = _countof(path);
        std::wstring wurl = LoaderUpdate::ToWide(url.c_str());
        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
            err = "Bad Roblox API URL";
            return false;
        }

        HINTERNET ses = WinHttpOpen(L"FRONTIER-Loader/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!ses) { err = "Network unavailable"; return false; }

        HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0);
        if (!con) { WinHttpCloseHandle(ses); err = "Connect failed"; return false; }

        HINTERNET req = WinHttpOpenRequest(con, L"POST", path, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!req) {
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            err = "Request failed";
            return false;
        }

        const wchar_t* hdrs = L"Content-Type: application/json\r\n";
        BOOL ok = WinHttpSendRequest(req, hdrs, (DWORD)-1L,
            (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
        if (ok) ok = WinHttpReceiveResponse(req, nullptr);
        std::string resp;
        if (ok) {
            DWORD avail = 0;
            do {
                if (!WinHttpQueryDataAvailable(req, &avail)) break;
                if (!avail) break;
                std::vector<char> chunk(avail);
                DWORD read = 0;
                if (!WinHttpReadData(req, chunk.data(), avail, &read)) break;
                resp.append(chunk.data(), read);
            } while (avail > 0);
        }
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);

        if (resp.empty()) {
            err = "Roblox lookup failed";
            return false;
        }

        size_t idPos = resp.find("\"id\":");
        if (idPos == std::string::npos) {
            err = "Username not found on Roblox";
            return false;
        }
        outId = atoi(resp.c_str() + idPos + 5);
        if (outId <= 0) {
            err = "Username not found";
            return false;
        }
        return true;
    }

    inline bool CheckAccess(int userId, bool& allowed, bool& verifyRequired, std::string& err)
    {
        allowed = false;
        verifyRequired = true;
        char url[256];
        sprintf_s(url, "%s?userId=%d", LoaderConfig::kAccessApi, userId);
        std::string body;
        if (!LoaderUpdate::HttpGet(url, body, err))
            return false;
        verifyRequired = body.find("\"verifyRequired\":true") != std::string::npos
            || body.find("\"verifyRequired\": true") != std::string::npos;
        allowed = ParseAllowedFromAuthJson(body);
        if (!verifyRequired)
            allowed = true;
        return true;
    }

    inline void LoadSavedAuth(int& userId, char* username, size_t usernameSz, bool& verified)
    {
        userId = 0;
        verified = false;
        if (username && usernameSz) username[0] = 0;
        std::wstring ini = LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini");
        userId = GetPrivateProfileIntW(L"access", L"userId", 0, ini.c_str());
        if (username && usernameSz) {
            wchar_t wname[64]{};
            GetPrivateProfileStringW(L"access", L"username", L"", wname, 64, ini.c_str());
            WideCharToMultiByte(CP_UTF8, 0, wname, -1, username, (int)usernameSz, nullptr, nullptr);
        }
        verified = GetPrivateProfileIntW(L"access", L"verified", 0, ini.c_str()) != 0 && userId > 0;
    }

    inline void SaveAuth(int userId, const char* username, bool verified)
    {
        std::wstring ini = LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini");
        wchar_t buf[32];
        swprintf_s(buf, L"%d", userId);
        WritePrivateProfileStringW(L"access", L"userId", buf, ini.c_str());
        swprintf_s(buf, L"%d", verified ? 1 : 0);
        WritePrivateProfileStringW(L"access", L"verified", buf, ini.c_str());
        if (username && username[0]) {
            wchar_t wname[64]{};
            MultiByteToWideChar(CP_UTF8, 0, username, -1, wname, 64);
            WritePrivateProfileStringW(L"access", L"username", wname, ini.c_str());
        }
    }
}
