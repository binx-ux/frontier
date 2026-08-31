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
        char loaderUrl[512] = "";
        char zipUrl[512] = "";
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

    inline void SaveInstallRecord(int version, const char* display)
    {
        SaveLocalVersion(version);
        if (display && display[0]) {
            std::wstring ini = PathJoin(GetLoaderDir(), L"loader.ini");
            WritePrivateProfileStringW(L"loader", L"installedDisplay", ToWide(display).c_str(), ini.c_str());
        }
    }

    inline bool InstalledDisplayMatches(const char* remoteDisplay)
    {
        if (!remoteDisplay || !remoteDisplay[0]) return true;
        std::wstring ini = PathJoin(GetLoaderDir(), L"loader.ini");
        wchar_t installed[64]{};
        GetPrivateProfileStringW(L"loader", L"installedDisplay", L"", installed, 64, ini.c_str());
        if (!installed[0]) return false;
        char narrow[64]{};
        WideCharToMultiByte(CP_UTF8, 0, installed, -1, narrow, 64, nullptr, nullptr);
        return _stricmp(narrow, remoteDisplay) == 0;
    }

    inline std::wstring LoaderExePath()
    {
        return PathJoin(GetLoaderDir(), L"FrontierLoader.exe");
    }

    inline std::wstring LoaderPendingPath()
    {
        return PathJoin(GetLoaderDir(), L"FrontierLoader.new.exe");
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

    inline bool HasPendingUpdates()
    {
        const std::wstring dir = GetLoaderDir();
        if (FileExists(LoaderPendingPath())) return true;
        if (FileExists(PathJoin(PathJoin(dir, L"usermode"), L"Frontier.new.exe"))) return true;
        if (FileExists(PathJoin(PathJoin(dir, L"kernel"), L"Frontier.new.exe"))) return true;
        if (FileExists(PathJoin(PathJoin(PathJoin(dir, L"kernel"), L"driver"), L"FrontierDrv.new.sys")))
            return true;
        return false;
    }

    inline bool ReplaceFileSafe(const std::wstring& tmp, const std::wstring& final, const wchar_t* pendingKey)
    {
        if (!FileExists(tmp)) return false;

        DeleteFileW(final.c_str());
        if (MoveFileW(tmp.c_str(), final.c_str()))
            return true;

        if (CopyFileW(tmp.c_str(), final.c_str(), FALSE)) {
            DeleteFileW(tmp.c_str());
            return true;
        }

        if (pendingKey) {
            std::wstring ini = PathJoin(GetLoaderDir(), L"loader.ini");
            WritePrivateProfileStringW(L"loader", pendingKey, L"1", ini.c_str());
        }
        return false;
    }

    inline bool ApplyPendingUpdates()
    {
        bool applied = false;
        const std::wstring dir = GetLoaderDir();

        const std::wstring umTmp = PathJoin(PathJoin(dir, L"usermode"), L"Frontier.new.exe");
        if (ReplaceFileSafe(umTmp, UsermodeExePath(), L"pendingUsermode"))
            applied = true;

        const std::wstring kmTmp = PathJoin(PathJoin(dir, L"kernel"), L"Frontier.new.exe");
        const std::wstring kmFinal = PathJoin(PathJoin(dir, L"kernel"), LoaderConfig::kKernelExe);
        if (ReplaceFileSafe(kmTmp, kmFinal, L"pendingKernel"))
            applied = true;

        const std::wstring drvTmp = PathJoin(PathJoin(PathJoin(dir, L"kernel"), L"driver"), L"FrontierDrv.new.sys");
        const std::wstring drvFinal = PathJoin(PathJoin(PathJoin(dir, L"kernel"), L"driver"), L"FrontierDrv.sys");
        if (ReplaceFileSafe(drvTmp, drvFinal, L"pendingDriver"))
            applied = true;

        std::wstring ini = PathJoin(dir, L"loader.ini");
        if (applied) {
            WritePrivateProfileStringW(L"loader", L"pendingUsermode", nullptr, ini.c_str());
            WritePrivateProfileStringW(L"loader", L"pendingKernel", nullptr, ini.c_str());
            WritePrivateProfileStringW(L"loader", L"pendingDriver", nullptr, ini.c_str());
        }
        return applied;
    }

    inline bool NeedsUpdate(const Manifest& m)
    {
        if (!LocalUsermodeExists()) return true;
        if (HasPendingUpdates()) return true;
        if (m.version <= 0) return false;
        if (LoadLocalVersion() < m.version) return true;
        if (!InstalledDisplayMatches(m.display)) return true;
        return false;
    }

    inline bool IsUpToDate(const Manifest& m)
    {
        return !NeedsUpdate(m);
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

    inline bool HttpDownloadFile(const char* url, const std::wstring& dest, std::string& outErr,
        std::function<void(float)> onProgress = nullptr)
    {
        outErr.clear();
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

        const bool https = uc.nScheme == INTERNET_SCHEME_HTTPS;
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
            outErr = "Network error - check your connection";
            return false;
        }

        DWORD status = 0, statusLen = sizeof(status);
        WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusLen, WINHTTP_NO_HEADER_INDEX);
        if (status >= 400) {
            WinHttpCloseHandle(req);
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            outErr = "Server rejected request";
            return false;
        }

        DWORD contentLen = 0;
        DWORD clLen = sizeof(contentLen);
        const bool hasLength = WinHttpQueryHeaders(req,
            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &contentLen, &clLen, WINHTTP_NO_HEADER_INDEX);

        HANDLE h = CreateFileW(dest.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            WinHttpCloseHandle(req);
            WinHttpCloseHandle(con);
            WinHttpCloseHandle(ses);
            outErr = "Cannot write file";
            return false;
        }

        DWORD totalWritten = 0;
        DWORD avail = 0;
        bool success = true;
        do {
            if (!WinHttpQueryDataAvailable(req, &avail)) { success = false; break; }
            if (avail == 0) break;

            std::vector<char> chunk(avail);
            DWORD read = 0;
            if (!WinHttpReadData(req, chunk.data(), avail, &read)) { success = false; break; }
            if (read == 0) break;

            DWORD written = 0;
            if (!WriteFile(h, chunk.data(), read, &written, nullptr) || written != read) {
                success = false;
                break;
            }
            totalWritten += written;
            if (onProgress && hasLength && contentLen > 0)
                onProgress((float)totalWritten / (float)contentLen);
        } while (avail > 0);

        CloseHandle(h);
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);

        if (!success || totalWritten == 0) {
            DeleteFileW(dest.c_str());
            outErr = success ? "Empty download" : "Download interrupted";
            return false;
        }
        if (onProgress) onProgress(1.f);
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
        JsonStr(body, "loader_url", m.loaderUrl, sizeof(m.loaderUrl));
        JsonStr(body, "zip_url", m.zipUrl, sizeof(m.zipUrl));
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

    inline bool PrepareLoaderRelaunch()
    {
        return FileExists(LoaderPendingPath());
    }

    inline bool ScheduleLoaderRelaunch()
    {
        const std::wstring dir = GetLoaderDir();
        const std::wstring loaderNew = LoaderPendingPath();
        if (!FileExists(loaderNew)) return false;

        const std::wstring loaderFinal = LoaderExePath();
        const std::wstring script = PathJoin(dir, L"_frontier_update.cmd");

        FILE* f = nullptr;
        if (_wfopen_s(&f, script.c_str(), L"w") != 0 || !f) return false;
        fwprintf(f, L"@echo off\r\n");
        fwprintf(f, L"timeout /t 2 /nobreak >nul\r\n");
        fwprintf(f, L"move /y \"%s\" \"%s\"\r\n", loaderNew.c_str(), loaderFinal.c_str());
        fwprintf(f, L"start \"\" \"%s\"\r\n", loaderFinal.c_str());
        fwprintf(f, L"del \"%%~f0\"\r\n");
        fclose(f);

        SHELLEXECUTEINFOW sei{};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        sei.lpVerb = L"open";
        sei.lpFile = script.c_str();
        sei.lpDirectory = dir.c_str();
        sei.nShow = SW_HIDE;
        if (!ShellExecuteExW(&sei)) return false;
        if (sei.hProcess) CloseHandle(sei.hProcess);
        return true;
    }

    inline bool ApplyUpdate(const Manifest& m, std::function<void(float, const char*)> progress,
        std::string& err)
    {
        ApplyPendingUpdates();

        if (IsUpToDate(m)) {
            if (progress) progress(1.f, "Already up to date");
            return true;
        }

        if (!m.usermodeUrl[0]) {
            err = "Manifest missing usermode_url";
            return false;
        }

        const std::wstring dir = GetLoaderDir();
        const std::wstring um = PathJoin(dir, L"usermode");
        EnsureDir(um);
        const std::wstring tmp = PathJoin(um, L"Frontier.new.exe");
        const std::wstring final = PathJoin(um, LoaderConfig::kUsermodeExe);

        if (progress) progress(0.12f, "Downloading usermode...");
        if (!HttpDownloadFile(m.usermodeUrl, tmp, err, [&](float p) {
            if (progress) progress(0.12f + p * 0.48f, "Downloading usermode...");
        }))
            return false;

        if (m.kernelAvailable && m.kernelUrl[0]) {
            if (progress) progress(0.62f, "Downloading kernel...");
            std::wstring km = PathJoin(dir, L"kernel");
            EnsureDir(km);
            std::wstring ktmp = PathJoin(km, L"Frontier.new.exe");
            if (HttpDownloadFile(m.kernelUrl, ktmp, err, [&](float p) {
                if (progress) progress(0.62f + p * 0.12f, "Downloading kernel...");
            })) {
                ReplaceFileSafe(ktmp, PathJoin(km, LoaderConfig::kKernelExe), L"pendingKernel");
            }
        }

        if (m.driverUrl[0]) {
            if (progress) progress(0.76f, "Downloading driver...");
            std::wstring driverDir = PathJoin(PathJoin(dir, L"kernel"), L"driver");
            EnsureDir(driverDir);
            std::wstring sysPath = PathJoin(driverDir, L"FrontierDrv.sys");
            std::wstring tmpSys = PathJoin(driverDir, L"FrontierDrv.new.sys");
            if (HttpDownloadFile(m.driverUrl, tmpSys, err, [&](float p) {
                if (progress) progress(0.76f + p * 0.08f, "Downloading driver...");
            })) {
                ReplaceFileSafe(tmpSys, sysPath, L"pendingDriver");
            }
        }

        if (progress) progress(0.86f, "Installing usermode...");
        if (!ReplaceFileSafe(tmp, final, L"pendingUsermode")) {
            err = "Frontier.exe is in use - will update on next loader start";
            SaveInstallRecord(m.version, m.display);
            ApplyPendingUpdates();
        } else {
            if (progress) progress(0.90f, "Installed usermode");
        }

        if (m.loaderUrl[0]) {
            if (progress) progress(0.93f, "Updating loader...");
            std::wstring loaderNew = LoaderPendingPath();
            std::string loaderErr;
            if (HttpDownloadFile(m.loaderUrl, loaderNew, loaderErr, [&](float p) {
                if (progress) progress(0.93f + p * 0.05f, "Updating loader...");
            })) {
                if (progress) progress(0.98f, "Loader update ready");
            }
        }

        if (progress) progress(1.f, "Up to date");
        SaveInstallRecord(m.version, m.display);
        ApplyPendingUpdates();
        return true;
    }
}
