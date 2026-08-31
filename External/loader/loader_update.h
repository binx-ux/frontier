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
        bool accessRequired = true;
        char discordInvite[128] = "https://discord.gg/zHGKqd92Pz";
        char driverUrl[512] = "";
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

    inline int LoadLocalVersion()
    {
        std::wstring ini = PathJoin(GetLoaderDir(), L"loader.ini");
        int v = GetPrivateProfileIntW(L"loader", L"localVersion", 0, ini.c_str());
        if (v < 0 || v > 999) {
            v = 0;
            WritePrivateProfileStringW(L"loader", L"localVersion", L"0", ini.c_str());
        }
        return v;
    }

    inline void SaveLocalVersion(int version)
    {
        std::wstring ini = PathJoin(GetLoaderDir(), L"loader.ini");
        wchar_t buf[16];
        swprintf_s(buf, L"%d", version);
        WritePrivateProfileStringW(L"loader", L"localVersion", buf, ini.c_str());
    }

    inline std::wstring UsermodeExePath()
    {
        std::wstring dir = GetLoaderDir();
        return PathJoin(PathJoin(dir, L"usermode"), LoaderConfig::kUsermodeExe);
    }

    inline bool LocalUsermodeExists()
    {
        return FileExists(UsermodeExePath());
    }

    inline bool ResolveUsermodeExe(std::wstring& exeOut, std::wstring& workOut)
    {
        const std::wstring primary = UsermodeExePath();
        if (FileExists(primary)) {
            exeOut = primary;
            workOut = PathJoin(GetLoaderDir(), L"usermode");
            return true;
        }

        std::wstring dir = GetLoaderDir();
        const wchar_t* rel[] = {
            L"..\\x64\\Release\\Frontier.exe",
            L"..\\External\\x64\\Release\\Frontier.exe",
            L"..\\..\\..\\..\\x64\\Release\\Frontier.exe",
            L"..\\..\\..\\x64\\Release\\Frontier.exe",
        };
        for (const wchar_t* r : rel) {
            std::wstring candidate = PathJoin(dir, r);
            wchar_t full[MAX_PATH]{};
            if (GetFullPathNameW(candidate.c_str(), MAX_PATH, full, nullptr) == 0)
                continue;
            if (!FileExists(full))
                continue;
            exeOut = full;
            workOut = full;
            size_t slash = workOut.find_last_of(L"\\/");
            if (slash != std::wstring::npos)
                workOut.resize(slash);
            return true;
        }
        return false;
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

        DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
        WinHttpSetOption(ses, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

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
            outErr = "Network error — check your connection";
            return false;
        }

        DWORD status = 0, statusLen = sizeof(status);
        WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusLen, WINHTTP_NO_HEADER_INDEX);

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

        if (outBody.empty()) {
            outErr = status >= 400 ? "Server rejected request" : "Empty response";
            return false;
        }
        return true;
    }

    inline bool HttpPost(const char* url, const char* contentType, const std::string& body,
        std::string& outBody, std::string& outErr)
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

        DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
        WinHttpSetOption(ses, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

        bool https = uc.nScheme == INTERNET_SCHEME_HTTPS;
        HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0);
        if (!con) { WinHttpCloseHandle(ses); outErr = "Connect failed"; return false; }

        DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET req = WinHttpOpenRequest(con, L"POST", path, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!req) {
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            outErr = "OpenRequest failed";
            return false;
        }

        std::string hdrs = std::string("Content-Type: ") + contentType + "\r\n";
        std::wstring whdrs = ToWide(hdrs.c_str());
        BOOL ok = WinHttpSendRequest(req, whdrs.c_str(), (DWORD)-1L,
            (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
        if (ok) ok = WinHttpReceiveResponse(req, nullptr);
        if (!ok) {
            WinHttpCloseHandle(req);
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            outErr = "Network error — check your connection";
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

        if (outBody.empty()) {
            outErr = "Empty server response";
            return false;
        }
        return true;
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
        m.accessRequired = JsonBool(body, "access_required", false);
        if (!JsonStr(body, "discord_invite", m.discordInvite, sizeof(m.discordInvite)))
            strncpy_s(m.discordInvite, LoaderConfig::kDiscordInvite, _TRUNCATE);
        JsonStr(body, "driver_url", m.driverUrl, sizeof(m.driverUrl));
        return m.version > 0 || m.usermodeUrl[0] != 0;
    }

    inline void ApplyFallbackManifest(Manifest& m)
    {
        if (!m.usermodeUrl[0])
            strncpy_s(m.usermodeUrl, LoaderConfig::kFallbackUsermodeUrl, _TRUNCATE);
        if (!m.display[0])
            strncpy_s(m.display, "FRONTIER v1", _TRUNCATE);
        if (m.version <= 0)
            m.version = 1;
    }

    inline bool ProbeKernelDriverAvailable()
    {
        std::wstring dir = GetLoaderDir();
        std::wstring sys = PathJoin(PathJoin(dir, L"kernel"), L"driver");
        sys = PathJoin(sys, L"FrontierDrv.sys");
        return FileExists(sys);
    }

    inline bool ProbeKernelAvailable()
    {
        std::wstring dir = GetLoaderDir();
        std::wstring kexe = PathJoin(PathJoin(dir, L"kernel"), LoaderConfig::kKernelExe);
        return FileExists(kexe) && ProbeKernelDriverAvailable();
    }

    inline bool RunPayload(int mode, std::wstring& errMsg)
    {
        std::wstring work;
        std::wstring exe;
        if (mode == 0) {
            if (!ResolveUsermodeExe(exe, work)) {
                errMsg = L"Missing usermode\\Frontier.exe.\nBuild External Release|x64 and copy to usermode\\.";
                return false;
            }
        } else {
            std::wstring dir = GetLoaderDir();
            work = PathJoin(dir, L"kernel");
            exe = PathJoin(work, LoaderConfig::kKernelExe);
            if (!FileExists(exe)) {
                errMsg = L"Kernel mode is not installed.\nPlace kernel\\Frontier.exe or run Update.";
                return false;
            }
        }

        if (mode == 1 && !ProbeKernelDriverAvailable()) {
            errMsg = L"Kernel driver missing.\nPlace kernel\\driver\\FrontierDrv.sys next to the loader.";
            return false;
        }

        std::wstring cmd = L"\"" + exe + L"\"";
        std::vector<wchar_t> buf(cmd.begin(), cmd.end());
        buf.push_back(L'\0');

        if (mode == 1) {
            SHELLEXECUTEINFOW sei{};
            sei.cbSize = sizeof(sei);
            sei.lpVerb = L"runas";
            sei.lpFile = exe.c_str();
            sei.lpDirectory = work.c_str();
            sei.nShow = SW_SHOWNORMAL;
            if (ShellExecuteExW(&sei))
                return true;
            errMsg = L"Kernel mode requires Administrator.\nApprove the UAC prompt to load the driver.";
            return false;
        }

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

        if (m.driverUrl[0]) {
            if (progress) progress(0.70f, "Downloading driver…");
            std::wstring driverDir = PathJoin(PathJoin(dir, L"kernel"), L"driver");
            EnsureDir(driverDir);
            std::wstring sysPath = PathJoin(driverDir, L"FrontierDrv.sys");
            std::wstring tmpSys = PathJoin(driverDir, L"FrontierDrv.new.sys");
            if (HttpDownloadFile(m.driverUrl, tmpSys, err)) {
                DeleteFileW(sysPath.c_str());
                MoveFileW(tmpSys.c_str(), sysPath.c_str());
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
        SaveLocalVersion(m.version);
        return true;
    }
}
