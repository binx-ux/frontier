#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <cstdio>
#include <cstring>

#ifndef APPCOMMAND_MEDIA_PLAY_PAUSE
#define APPCOMMAND_MEDIA_PLAY_PAUSE 14
#endif
#ifndef APPCOMMAND_MEDIA_NEXTTRACK
#define APPCOMMAND_MEDIA_NEXTTRACK 11
#endif
#ifndef APPCOMMAND_MEDIA_PREVIOUSTRACK
#define APPCOMMAND_MEDIA_PREVIOUSTRACK 12
#endif
#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND 0x0319
#endif

namespace SpotifyPlayer {

    inline char trackTitle[256] = "Spotify not detected";
    inline char trackArtist[128] = "Spotify";
    inline bool connected = false;
    inline bool playing = false;
    inline HWND spotifyHwnd = nullptr;
    inline DWORD lastRefreshMs = 0;
    inline DWORD spotifyPid = 0;
    inline DWORD lastPidScanMs = 0;

    inline DWORD FindSpotifyPid()
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        DWORD pid = 0;
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"Spotify.exe") == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return pid;
    }

    inline BOOL CALLBACK EnumSpotifyWnd(HWND hwnd, LPARAM lParam) {
        if (!IsWindowVisible(hwnd)) return TRUE;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid || pid != spotifyPid) return TRUE;

        wchar_t title[512]{};
        GetWindowTextW(hwnd, title, 512);
        if (title[0] == 0) return TRUE;

        RECT r{};
        GetWindowRect(hwnd, &r);
        if ((r.right - r.left) < 100 || (r.bottom - r.top) < 100) return TRUE;

        *reinterpret_cast<HWND*>(lParam) = hwnd;
        return FALSE;
    }

    inline void Refresh() {
        DWORD now = GetTickCount();
        if (now - lastRefreshMs < 500) return;
        lastRefreshMs = now;

        if (!spotifyPid || (now - lastPidScanMs) > 4000) {
            spotifyPid = FindSpotifyPid();
            lastPidScanMs = now;
        }

        spotifyHwnd = nullptr;
        if (spotifyPid)
            EnumWindows(EnumSpotifyWnd, reinterpret_cast<LPARAM>(&spotifyHwnd));
        connected = spotifyHwnd != nullptr;

        if (!connected) {
            strcpy_s(trackTitle, "Spotify not detected");
            strcpy_s(trackArtist, "Open Spotify to control");
            playing = false;
            return;
        }

        wchar_t title[512]{};
        GetWindowTextW(spotifyHwnd, title, 512);
        char utf8[512]{};
        WideCharToMultiByte(CP_UTF8, 0, title, -1, utf8, sizeof(utf8), nullptr, nullptr);

        if (_stricmp(utf8, "Spotify") == 0 || _stricmp(utf8, "Spotify Premium") == 0 ||
            _stricmp(utf8, "Spotify Free") == 0 || _stricmp(utf8, "Advertisement") == 0) {
            strcpy_s(trackTitle, "Paused / Idle");
            strcpy_s(trackArtist, "Spotify");
            playing = false;
            return;
        }

        std::string s(utf8);
        const char* suffix = " - Spotify";
        size_t pos = s.rfind(suffix);
        if (pos != std::string::npos)
            s = s.substr(0, pos);

        // Spotify desktop title is usually "Artist - Song"
        size_t dash = s.find(" - ");
        if (dash != std::string::npos) {
            strcpy_s(trackArtist, s.substr(0, dash).c_str());
            strcpy_s(trackTitle, s.substr(dash + 3).c_str());
        }
        else {
            if (s.empty()) s = "Now Playing";
            if (s.size() > 80) s = s.substr(0, 77) + "...";
            strcpy_s(trackTitle, s.c_str());
            strcpy_s(trackArtist, "Spotify");
        }
        playing = true;
    }

    inline void SendMediaKey(WORD vk) {
        INPUT in[2]{};
        in[0].type = INPUT_KEYBOARD;
        in[0].ki.wVk = vk;
        in[1].type = INPUT_KEYBOARD;
        in[1].ki.wVk = vk;
        in[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, in, sizeof(INPUT));
    }

    inline void SendAppCommand(int cmd) {
        if (spotifyHwnd && IsWindow(spotifyHwnd)) {
            // Prefer Spotify window — more reliable than global media keys alone
            SendMessageW(spotifyHwnd, WM_APPCOMMAND, (WPARAM)spotifyHwnd, MAKELONG(0, cmd));
            PostMessageW(spotifyHwnd, WM_APPCOMMAND, (WPARAM)spotifyHwnd, MAKELONG(0, cmd));
        }
        // Also hit the foreground / system media session as fallback
        HWND fg = GetForegroundWindow();
        if (fg)
            SendMessageW(fg, WM_APPCOMMAND, (WPARAM)fg, MAKELONG(0, cmd));
    }

    inline void PlayPause() {
        SendAppCommand(APPCOMMAND_MEDIA_PLAY_PAUSE);
        SendMediaKey(VK_MEDIA_PLAY_PAUSE);
        playing = !playing;
    }

    inline void Next() {
        SendAppCommand(APPCOMMAND_MEDIA_NEXTTRACK);
        SendMediaKey(VK_MEDIA_NEXT_TRACK);
    }

    inline void Prev() {
        SendAppCommand(APPCOMMAND_MEDIA_PREVIOUSTRACK);
        SendMediaKey(VK_MEDIA_PREV_TRACK);
    }

    inline void VolUp() { SendMediaKey(VK_VOLUME_UP); }
    inline void VolDown() { SendMediaKey(VK_VOLUME_DOWN); }
}
