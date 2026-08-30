#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <ShlObj.h>
#include <Shellapi.h>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include "loader_config.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")

namespace LoaderUpdate {

    inline std::wstring ToWide(const char* s)
    {
        if (!s || !s[0]) return L"";
        int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (n <= 0) return L"";
        std::wstring w(n - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), n);
        return w;
    }

    struct Manifest {
        int version = 0;
        char display[64] = "unknown";
        char usermodeUrl[512] = "";
        char kernelUrl[512] = "";
        bool kernelAvailable = false;
    };

    inline std::wstring GetLoaderDir()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring dir(path);
        size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            dir.resize(slash);
        return dir;
    }

    inline std::wstring PathJoin(const std::wstring& dir, const wchar_t* leaf)
    {
        return dir + L"\\" + leaf;
    }

    inline bool FileExists(const std::wstring& path)
    {
        DWORD a = GetFileAttributesW(path.c_str());
        return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
    }

    inline bool EnsureDir(const std::wstring& path)
    {
        if (path.empty()) return false;
        if (CreateDirectoryW(path.c_str(), nullptr)) return true;
        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    inline int LoadSavedMode()
    {
        std::wstring ini = PathJoin(GetLoaderDir(), L"loader.ini");
        return GetPrivateProfileIntW(L"loader", L"mode", 0, ini.c_str());
    }

    inline void SaveMode(int mode)
    {
        std::wstring ini = PathJoin(GetLoaderDir(), L"loader.ini");
        wchar_t buf[8];
        swprintf_s(buf, L"%d", mode);
        WritePrivateProfileStringW(L"loader", L"mode", buf, ini.c_str());
    }

    inline bool HttpGet(const char* url, std::string& outBody, std::string& outErr)
    {
        outBody.clear();
        std::wstring wurl = ToWide(url);
        if (wurl.empty()) { outErr = "Bad URL"; return false; }

        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);
        wchar_t host[256]{}, path[2048]{};
        uc.lpszHostName = host;
        uc.dwHostNameLength = (DWORD)_countof(host);
        uc.lpszUrlPath = path;
        uc.dwUrlPathLength = (DWORD)_countof(path);
        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
            outErr = "Bad URL";
            return false;
        }

        HINTERNET ses = WinHttpOpen(L"FRONTIER-Loader/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!ses) { outErr = "WinHttpOpen failed"; return false; }

        bool https = uc.nScheme == INTERNET_SCHEME_HTTPS;
        HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0);
        if (!con) { WinHttpCloseHandle(ses); outErr = "Connect failed"; return false; }

        DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET req = WinHttpOpenRequest(con, L"GET", path, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!req) {
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            outErr = "OpenRequest failed";
            return false;
        }

        BOOL ok = WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (ok) ok = WinHttpReceiveResponse(req, nullptr);
        if (!ok) {
            WinHttpCloseHandle(req);
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            outErr = "Request failed";
            return false;
        }

        DWORD avail = 0;
        do {
            if (!WinHttpQueryDataAvailable(req, &avail)) break;
            if (avail == 0) break;
            std::vector<char> chunk(avail);
            DWORD read = 0;
            if (!WinHttpReadData(req, chunk.data(), avail, &read)) break;
            outBody.append(chunk.data(), read);
        } while (avail > 0);

        WinHttpCloseHandle(req);
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);
        return !outBody.empty();
    }

    inline bool HttpDownloadFile(const char* url, const std::wstring& dest, std::string& outErr)
    {
        std::string body;
        if (!HttpGet(url, body, outErr)) return false;

        HANDLE h = CreateFileW(dest.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            outErr = "Cannot write file";
            return false;
        }
        DWORD written = 0;
        BOOL ok = WriteFile(h, body.data(), (DWORD)body.size(), &written, nullptr);
        CloseHandle(h);
        if (!ok || written != body.size()) {
            outErr = "Write incomplete";
            DeleteFileW(dest.c_str());
            return false;
        }
        return true;
    }

    inline const char* JsonStr(const std::string& json, const char* key, char* out, size_t outSz)
    {
        out[0] = 0;
        std::string needle = std::string("\"") + key + "\":\"";
        size_t p = json.find(needle);
        if (p == std::string::npos) return nullptr;
        p += needle.size();
        size_t e = json.find('"', p);
        if (e == std::string::npos) return nullptr;
        size_t n = e - p;
        if (n >= outSz) n = outSz - 1;
        memcpy(out, json.data() + p, n);
        out[n] = 0;
        return out;
    }

    inline int JsonInt(const std::string& json, const char* key, int fallback)
    {
        std::string needle = std::string("\"") + key + "\":";
        size_t p = json.find(needle);
        if (p == std::string::npos) return fallback;
        p += needle.size();
        while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) p++;
        return atoi(json.c_str() + p);
    }

    inline bool JsonBool(const std::string& json, const char* key, bool fallback)
    {
        std::string needle = std::string("\"") + key + "\":";
        size_t p = json.find(needle);
        if (p == std::string::npos) return fallback;
        p += needle.size();
        while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) p++;
        if (json.compare(p, 4, "true") == 0) return true;
        if (json.compare(p, 5, "false") == 0) return false;
        return fallback;
    }

    inline bool FetchManifest(Manifest& m, std::string& err)
    {
        std::string body;
        if (!HttpGet(LoaderConfig::kManifestUrl, body, err))
            return false;
        m.version = JsonInt(body, "version", 0);
        JsonStr(body, "display", m.display, sizeof(m.display));
        JsonStr(body, "usermode_url", m.usermodeUrl, sizeof(m.usermodeUrl));
        JsonStr(body, "kernel_url", m.kernelUrl, sizeof(m.kernelUrl));
        m.kernelAvailable = JsonBool(body, "kernel_available", false);
        return m.version > 0;
    }

    inline bool ProbeKernelAvailable()
    {
        std::wstring dir = GetLoaderDir();
        std::wstring kexe = PathJoin(PathJoin(dir, L"kernel"), LoaderConfig::kKernelExe);
        return FileExists(kexe);
    }

    inline bool RunPayload(int mode, std::wstring& errMsg)
    {
        std::wstring dir = GetLoaderDir();
        const wchar_t* sub = (mode == 1) ? L"kernel" : L"usermode";
        std::wstring work = PathJoin(dir, sub);
        std::wstring exe = PathJoin(work, LoaderConfig::kUsermodeExe);
        if (mode == 1)
            exe = PathJoin(work, LoaderConfig::kKernelExe);

        if (!FileExists(exe)) {
            if (mode == 1)
                errMsg = L"Kernel mode is not installed.\nPlace kernel\\Frontier.exe or run Update.";
            else
                errMsg = L"Missing usermode\\Frontier.exe.\nBuild Release|x64 or run Update.";
            return false;
        }

        std::wstring cmd = L"\"" + exe + L"\"";
        std::vector<wchar_t> buf(cmd.begin(), cmd.end());
        buf.push_back(L'\0');

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        if (CreateProcessW(exe.c_str(), buf.data(), nullptr, nullptr, FALSE, 0,
                const_cast<LPWSTR>(work.c_str()), nullptr, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return true;
        }

        SHELLEXECUTEINFOW sei{};
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"open";
        sei.lpFile = exe.c_str();
        sei.lpDirectory = work.c_str();
        sei.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&sei))
            return true;

        errMsg = L"Failed to start FRONTIER.";
        return false;
    }

    inline bool ApplyUpdate(const Manifest& m, std::function<void(float, const char*)> progress,
        std::string& err)
    {
        if (!m.usermodeUrl[0]) {
            err = "Manifest missing usermode_url";
            return false;
        }
        std::wstring dir = GetLoaderDir();
        std::wstring um = PathJoin(dir, L"usermode");
        EnsureDir(um);
        std::wstring tmp = PathJoin(um, L"Frontier.new.exe");
        std::wstring final = PathJoin(um, LoaderConfig::kUsermodeExe);

        if (progress) progress(0.15f, "Downloading usermode…");
        if (!HttpDownloadFile(m.usermodeUrl, tmp, err))
            return false;

        if (m.kernelAvailable && m.kernelUrl[0]) {
            if (progress) progress(0.55f, "Downloading kernel…");
            std::wstring km = PathJoin(dir, L"kernel");
            EnsureDir(km);
            std::wstring ktmp = PathJoin(km, L"Frontier.new.exe");
            if (HttpDownloadFile(m.kernelUrl, ktmp, err)) {
                DeleteFileW(PathJoin(km, LoaderConfig::kKernelExe).c_str());
                MoveFileW(ktmp.c_str(), PathJoin(km, LoaderConfig::kKernelExe).c_str());
            }
        }

        if (progress) progress(0.85f, "Installing…");
        DeleteFileW(final.c_str());
        if (!MoveFileW(tmp.c_str(), final.c_str())) {
            if (!CopyFileW(tmp.c_str(), final.c_str(), FALSE)) {
                err = "Could not replace Frontier.exe";
                DeleteFileW(tmp.c_str());
                return false;
            }
            DeleteFileW(tmp.c_str());
        }

        if (progress) progress(1.f, "Up to date");
        return true;
    }
}
