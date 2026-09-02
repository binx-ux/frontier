#pragma once
#include <Windows.h>
#include <ShlObj.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "loader_config.h"
#include "loader_update.h"

namespace LoaderProducts {

    enum Product : int {
        ProductNone = 0,
        ProductTrace = 1,
        ProductFrontier = 2,
    };

    struct ExecutorHit {
        char name[32]{};
        wchar_t path[MAX_PATH]{};
    };

    inline bool FileExistsW(const wchar_t* path)
    {
        if (!path || !path[0]) return false;
        DWORD a = GetFileAttributesW(path);
        return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
    }

    inline bool ExpandPath(const wchar_t* in, wchar_t* out, DWORD outChars)
    {
        if (!in || !out || outChars < 2) return false;
        DWORD n = ExpandEnvironmentStringsW(in, out, outChars);
        return n > 0 && n < outChars;
    }

    inline void PushIfExists(std::vector<ExecutorHit>& out, const char* name, const wchar_t* path)
    {
        if (!FileExistsW(path)) return;
        for (const auto& e : out)
            if (_wcsicmp(e.path, path) == 0) return;
        ExecutorHit hit{};
        strncpy_s(hit.name, name, _TRUNCATE);
        wcsncpy_s(hit.path, path, _TRUNCATE);
        out.push_back(hit);
    }

    inline void ScanDirForExe(const wchar_t* dir, const wchar_t* exeName, const char* label,
        std::vector<ExecutorHit>& out)
    {
        if (!dir || !dir[0] || !exeName) return;
        wchar_t path[MAX_PATH]{};
        swprintf_s(path, L"%s\\%s", dir, exeName);
        PushIfExists(out, label, path);
    }

    inline std::vector<ExecutorHit> ScanExecutors()
    {
        std::vector<ExecutorHit> hits;
        wchar_t localAppData[MAX_PATH]{};
        wchar_t appData[MAX_PATH]{};
        SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData);
        SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData);

        struct Candidate { const char* label; const wchar_t* exe; const wchar_t* subfolder; };
        const Candidate list[] = {
            { "Wave", L"Wave.exe", L"Wave" },
            { "Solara", L"Solara.exe", L"Solara" },
            { "Volt", L"Volt.exe", L"Volt" },
            { "Velocity", L"Velocity.exe", L"Velocity" },
            { "Xeno", L"Xeno.exe", L"Xeno" },
            { "Real", L"Real.exe", L"Real" },
            { "Potassium", L"Potassium.exe", L"Potassium" },
            { "Seliware", L"Seliware.exe", L"Seliware" },
            { "Madium", L"Madium.exe", L"Madium" },
            { "Cosmic", L"Cosmic.exe", L"Cosmic" },
        };

        for (const auto& c : list) {
            wchar_t dir[MAX_PATH]{};
            swprintf_s(dir, L"%s\\%s", localAppData, c.subfolder);
            ScanDirForExe(dir, c.exe, c.label, hits);
            swprintf_s(dir, L"%s\\%s", appData, c.subfolder);
            ScanDirForExe(dir, c.exe, c.label, hits);
        }

        wchar_t expanded[MAX_PATH]{};
        const wchar_t* envPaths[] = {
            L"%LOCALAPPDATA%\\Wave\\Wave.exe",
            L"%LOCALAPPDATA%\\Solara\\Solara.exe",
            L"%LOCALAPPDATA%\\Volt\\Volt.exe",
            L"%LOCALAPPDATA%\\Velocity\\Velocity.exe",
            L"%LOCALAPPDATA%\\Xeno\\Xeno.exe",
            L"%LOCALAPPDATA%\\Wave\\bin\\Wave.exe",
        };
        const char* envLabels[] = { "Wave", "Solara", "Volt", "Velocity", "Xeno", "Wave" };
        for (int i = 0; i < (int)(sizeof(envPaths) / sizeof(envPaths[0])); i++) {
            if (ExpandPath(envPaths[i], expanded, MAX_PATH))
                PushIfExists(hits, envLabels[i], expanded);
        }

        return hits;
    }

    inline std::string BuildTraceLoadstring()
    {
        char buf[512];
        sprintf_s(buf, "loadstring(game:HttpGet(\"%s?v=%llu\"))()",
            LoaderConfig::kTraceLoaderUrl, (unsigned long long)GetTickCount64());
        return std::string(buf);
    }

    inline bool CopyUtf8ToClipboard(const char* text)
    {
        if (!text || !text[0]) return false;
        if (!OpenClipboard(nullptr)) return false;
        EmptyClipboard();
        const size_t bytes = strlen(text) + 1;
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (!h) { CloseClipboard(); return false; }
        memcpy(GlobalLock(h), text, bytes);
        GlobalUnlock(h);
        SetClipboardData(CF_TEXT, h);
        CloseClipboard();
        return true;
    }

    inline bool LaunchExecutable(const wchar_t* path)
    {
        if (!path || !path[0]) return false;
        HINSTANCE r = ShellExecuteW(nullptr, L"open", path, nullptr, nullptr, SW_SHOWNORMAL);
        return (INT_PTR)r > 32;
    }

    inline bool RunTraceLaunch(std::wstring& statusOut)
    {
        const std::string loadstring = BuildTraceLoadstring();
        if (!CopyUtf8ToClipboard(loadstring.c_str())) {
            statusOut = L"Could not copy loadstring to clipboard.";
            return false;
        }

        auto executors = ScanExecutors();
        if (executors.empty()) {
            statusOut = L"Loadstring copied!\nNo executor found - open yours manually and paste (Ctrl+V).";
            return true;
        }

        const wchar_t* best = executors[0].path;
        for (const auto& e : executors) {
            if (_stricmp(e.name, "Wave") == 0) { best = e.path; break; }
        }

        if (!LaunchExecutable(best)) {
            statusOut = L"Loadstring copied, but could not open executor.\nPaste into your executor manually.";
            return true;
        }

        char msg[256];
        sprintf_s(msg, "Opened %s and copied TRACE loadstring to clipboard.\nInject in Roblox, then paste (Ctrl+V).",
            executors[0].name);
        statusOut = LoaderUpdate::ToWide(msg);
        return true;
    }

    inline const char* ProductName(Product p)
    {
        switch (p) {
        case ProductTrace: return "TRACE";
        case ProductFrontier: return "FRONTIER";
        default: return "Product";
        }
    }

    inline const char* ProductTagline(Product p)
    {
        switch (p) {
        case ProductTrace: return "Free - Arsenal internal";
        case ProductFrontier: return "Lifetime - Windows external";
        default: return "";
        }
    }

} // namespace LoaderProducts
