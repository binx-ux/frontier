#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../../common/url.h"

#pragma comment(lib, "winhttp.lib")

namespace SessionGate {

    inline std::wstring ExeDir()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring dir(path);
        size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            dir.resize(slash);
        return dir;
    }

    inline bool ReadSessionFile(char* token, size_t tokenSz, char* hwid, size_t hwidSz)
    {
        if (token && tokenSz) token[0] = 0;
        if (hwid && hwidSz) hwid[0] = 0;
        std::wstring path = ExeDir() + L"\\frontier.session";
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"r") != 0 || !f)
            return false;
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            char* nl = strchr(line, '\n');
            if (nl) *nl = 0;
            if (strncmp(line, "token=", 6) == 0 && token && tokenSz)
                strncpy_s(token, tokenSz, line + 6, _TRUNCATE);
            else if (strncmp(line, "hwid=", 5) == 0 && hwid && hwidSz)
                strncpy_s(hwid, hwidSz, line + 5, _TRUNCATE);
        }
        fclose(f);
        return token && token[0] != 0;
    }

    inline bool HttpGetSession(const char* url, std::string& outBody)
    {
        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);
        wchar_t host[256]{}, path[512]{};
        uc.lpszHostName = host;
        uc.dwHostNameLength = _countof(host);
        uc.lpszUrlPath = path;
        uc.dwUrlPathLength = _countof(path);

        int wlen = MultiByteToWideChar(CP_UTF8, 0, url, -1, nullptr, 0);
        std::wstring wurl(wlen - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl.data(), wlen);

        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc))
            return false;

        HINTERNET ses = WinHttpOpen(L"FRONTIER/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!ses) return false;

        HINTERNET con = WinHttpConnect(ses, host, uc.nPort ? uc.nPort : INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!con) {
            WinHttpCloseHandle(ses);
            return false;
        }

        HINTERNET req = WinHttpOpenRequest(con, L"GET", path, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!req) {
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            return false;
        }

        BOOL ok = WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (ok) ok = WinHttpReceiveResponse(req, nullptr);

        if (ok) {
            DWORD avail = 0;
            do {
                if (!WinHttpQueryDataAvailable(req, &avail)) break;
                if (!avail) break;
                std::vector<char> chunk(avail);
                DWORD read = 0;
                if (!WinHttpReadData(req, chunk.data(), avail, &read)) break;
                outBody.append(chunk.data(), read);
            } while (avail > 0);
        }

        WinHttpCloseHandle(req);
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);
        return !outBody.empty();
    }

    inline bool ValidateSession(std::string& errMsg)
    {
        char token[768]{}, hwid[64]{};
        if (!ReadSessionFile(token, sizeof(token), hwid, sizeof(hwid))) {
            errMsg = "Launch through FrontierLoader after activating your license.";
            return false;
        }

        char url[1200];
        std::string encToken = Net::UrlEncode(token);
        std::string encHwid = Net::UrlEncode(hwid[0] ? hwid : "");
        sprintf_s(url, "https://trace-host.vercel.app/api/frontier-license?token=%s&hwid=%s",
            encToken.c_str(), encHwid.c_str());

        std::string body;
        if (!HttpGetSession(url, body)) {
            errMsg = "Could not verify license (offline). Check your connection.";
            return false;
        }
        if (body.find("\"allowed\":true") == std::string::npos &&
            body.find("\"allowed\": true") == std::string::npos) {
            errMsg = "License invalid, expired, or bound to another PC.";
            return false;
        }
        return true;
    }
}
