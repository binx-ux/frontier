#pragma once
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <cstdio>
#include <thread>
#include "loader_config.h"
#include "loader_update.h"
#include "loader_auth.h"
#include "resource.h"
#include <Shellapi.h>

#pragma comment(lib, "comctl32.lib")

namespace LoaderUI {

    enum CtrlId : int {
        ID_BTN_DISCORD = 1000,
        ID_BTN_UPDATE = 1001,
        ID_BTN_LAUNCH = 1002,
        ID_BTN_RELEASES = 1003,
        ID_EDIT_USER = 1004,
        ID_BTN_VERIFY = 1005,
    };

    struct State {
        int selectedMode = 0;
        bool kernelAvailable = false;
        bool checking = false;
        bool updating = false;
        bool verifying = false;
        bool updateAvailable = false;
        bool accessRequired = true;
        bool verified = false;
        int localVersion = LoaderConfig::kLocalVersion;
        int remoteVersion = 0;
        int robloxUserId = 0;
        char status[256] = "Ready";
        char remoteDisplay[64] = "";
        char robloxUsername[64] = "";
        char discordInvite[128] = "";
        float progress = 0.f;
        LoaderUpdate::Manifest manifest{};
    };

    inline State* gState = nullptr;
    inline HWND gWnd = nullptr;
    inline HFONT gFont = nullptr;
    inline HFONT gFontBold = nullptr;
    inline HBRUSH gBgBrush = nullptr;

    inline RECT ModeCardRect(int index)
    {
        RECT rc{};
        rc.left = 24;
        rc.right = LoaderConfig::kWindowW - 24;
        rc.top = 228 + index * 92;
        rc.bottom = rc.top + 78;
        return rc;
    }

    inline void SetStatus(State* s, const char* msg)
    {
        strncpy_s(s->status, msg, _TRUNCATE);
        if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
    }

    inline void RefreshLaunchButton(State* s)
    {
        HWND launch = GetDlgItem(gWnd, ID_BTN_LAUNCH);
        if (!launch) return;
        bool blocked = s->accessRequired && !s->verified;
        EnableWindow(launch, !blocked && !s->updating);
    }

    inline void Paint(HDC hdc, State* s)
    {
        RECT client{};
        GetClientRect(gWnd, &client);
        FillRect(hdc, &client, gBgBrush);

        SetBkMode(hdc, TRANSPARENT);

        RECT stripe{ 0, 0, 3, client.bottom };
        HBRUSH brand = CreateSolidBrush(LoaderConfig::kBrand);
        FillRect(hdc, &stripe, brand);
        DeleteObject(brand);

        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, LoaderConfig::kBrand);
        TextOutA(hdc, 24, 18, "FRONTIER", 8);
        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        TextOutA(hdc, 108, 20, "Loader", 6);

        char ver[64];
        sprintf_s(ver, "v%d", s->localVersion);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        TextOutA(hdc, client.right - 56, 20, ver, (int)strlen(ver));

        // Discord access card
        if (s->accessRequired) {
            RECT access{ 24, 48, client.right - 24, 210 };
            HBRUSH cardBrush = CreateSolidBrush(LoaderConfig::kCard);
            FillRect(hdc, &access, cardBrush);
            DeleteObject(cardBrush);

            HPEN pen = CreatePen(PS_SOLID, 1, LoaderConfig::kBorder);
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);
            HBRUSH nullBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);
            RoundRect(hdc, access.left, access.top, access.right, access.bottom, 10, 10);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);

            SelectObject(hdc, gFontBold);
            SetTextColor(hdc, LoaderConfig::kText);
            TextOutA(hdc, access.left + 14, access.top + 12, "Discord access required", 23);
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextDim);
            TextOutA(hdc, access.left + 14, access.top + 34,
                "1. Join Discord  2. Run /verify YourRobloxName  3. Verify below", 68);

            if (s->verified) {
                SetTextColor(hdc, RGB(80, 200, 120));
                char ok[96];
                sprintf_s(ok, "Verified — UserId %d", s->robloxUserId);
                TextOutA(hdc, access.left + 14, access.top + 130, ok, (int)strlen(ok));
            } else {
                SetTextColor(hdc, LoaderConfig::kBrand);
                TextOutA(hdc, access.left + 14, access.top + 130,
                    "Not verified — join Discord and run /verify first", 49);
            }
        }

        SetTextColor(hdc, LoaderConfig::kText);
        SelectObject(hdc, gFontBold);
        TextOutA(hdc, 24, s->accessRequired ? 222 : 52, "Select mode", 11);
        SelectObject(hdc, gFont);

        const char* labels[] = { "Usermode", "Kernel" };
        const char* descs[] = {
            "Standard external overlay — no driver required.",
            "Kernel-assisted build — requires driver bundle."
        };

        for (int i = 0; i < 2; i++) {
            RECT card = ModeCardRect(i);
            bool sel = (s->selectedMode == i);
            bool disabled = (i == 1 && !s->kernelAvailable);

            HBRUSH cardBrush = CreateSolidBrush(disabled ? RGB(16, 16, 16) : LoaderConfig::kCard);
            FillRect(hdc, &card, cardBrush);
            DeleteObject(cardBrush);

            HPEN pen = CreatePen(PS_SOLID, sel ? 2 : 1,
                sel ? LoaderConfig::kBrand : LoaderConfig::kBorder);
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);
            HBRUSH nullBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);
            RoundRect(hdc, card.left, card.top, card.right, card.bottom, 10, 10);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);

            SetTextColor(hdc, disabled ? LoaderConfig::kTextDim : (sel ? LoaderConfig::kText : LoaderConfig::kTextDim));
            SelectObject(hdc, gFontBold);
            TextOutA(hdc, card.left + 14, card.top + 12, labels[i], (int)strlen(labels[i]));
            SelectObject(hdc, gFont);
            TextOutA(hdc, card.left + 14, card.top + 34, descs[i], (int)strlen(descs[i]));

            if (i == 1 && !s->kernelAvailable) {
                SetTextColor(hdc, LoaderConfig::kBrand);
                TextOutA(hdc, card.left + 14, card.top + 54, "Not installed — update or add kernel\\Frontier.exe", 47);
            }
        }

        if (s->updating || s->progress > 0.f) {
            RECT barBg{ 24, 420, client.right - 24, 432 };
            FillRect(hdc, &barBg, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
            RECT barFg = barBg;
            barFg.right = barBg.left + (LONG)((barBg.right - barBg.left) * s->progress);
            HBRUSH fg = CreateSolidBrush(LoaderConfig::kBrand);
            FillRect(hdc, &barFg, fg);
            DeleteObject(fg);
        }

        SetTextColor(hdc, LoaderConfig::kTextDim);
        TextOutA(hdc, 24, 444, s->status, (int)strlen(s->status));

        if (s->updateAvailable && s->remoteDisplay[0]) {
            char upd[128];
            sprintf_s(upd, "Update available: %s", s->remoteDisplay);
            SetTextColor(hdc, LoaderConfig::kBrand);
            TextOutA(hdc, 24, 466, upd, (int)strlen(upd));
        }
    }

    inline void CheckAsync(State* s)
    {
        if (s->checking) return;
        s->checking = true;
        SetStatus(s, "Checking for updates…");
        std::thread([s]() {
            LoaderUpdate::Manifest m{};
            std::string err;
            if (LoaderUpdate::FetchManifest(m, err)) {
                s->remoteVersion = m.version;
                strncpy_s(s->remoteDisplay, m.display, _TRUNCATE);
                s->manifest = m;
                s->updateAvailable = m.version > s->localVersion;
                s->accessRequired = m.accessRequired;
                strncpy_s(s->discordInvite, m.discordInvite, _TRUNCATE);
                if (m.kernelAvailable)
                    s->kernelAvailable = true;
                if (s->updateAvailable)
                    SetStatus(s, "Update available — click Update Files");
                else
                    SetStatus(s, "You are on the latest build");
            } else {
                s->updateAvailable = false;
                SetStatus(s, "Offline — using local files");
            }
            if (!s->kernelAvailable)
                s->kernelAvailable = LoaderUpdate::ProbeKernelAvailable();
            if (gWnd) {
                ShowWindow(GetDlgItem(gWnd, ID_BTN_DISCORD), s->accessRequired ? SW_SHOW : SW_HIDE);
                ShowWindow(GetDlgItem(gWnd, ID_EDIT_USER), s->accessRequired ? SW_SHOW : SW_HIDE);
                ShowWindow(GetDlgItem(gWnd, ID_BTN_VERIFY), s->accessRequired ? SW_SHOW : SW_HIDE);
                RefreshLaunchButton(s);
                InvalidateRect(gWnd, nullptr, FALSE);
            }
            s->checking = false;
        }).detach();
    }

    inline void RunVerify(State* s)
    {
        if (s->verifying) return;
        char username[64]{};
        GetWindowTextA(GetDlgItem(gWnd, ID_EDIT_USER), username, 64);
        if (!username[0]) {
            SetStatus(s, "Enter your Roblox username");
            return;
        }
        s->verifying = true;
        SetStatus(s, "Verifying…");
        std::thread([s, username]() {
            int userId = 0;
            std::string err;
            if (!LoaderAuth::ResolveRobloxUserId(username, userId, err)) {
                SetStatus(s, err.c_str());
                s->verifying = false;
                return;
            }
            bool allowed = false;
            bool verifyRequired = true;
            if (!LoaderAuth::CheckAccess(userId, allowed, verifyRequired, err)) {
                SetStatus(s, err.c_str());
                s->verifying = false;
                return;
            }
            s->robloxUserId = userId;
            strncpy_s(s->robloxUsername, username, _TRUNCATE);
            s->verified = allowed || !verifyRequired;
            LoaderAuth::SaveAuth(userId, username, s->verified);
            if (s->verified)
                SetStatus(s, "Access granted — you can launch");
            else
                SetStatus(s, "Not whitelisted — join Discord and run /verify YourRobloxName");
            if (gWnd) {
                RefreshLaunchButton(s);
                InvalidateRect(gWnd, nullptr, FALSE);
            }
            s->verifying = false;
        }).detach();
    }

    inline void RunUpdate(State* s)
    {
        if (s->updating) return;
        if (!s->manifest.usermodeUrl[0]) {
            ShellExecuteA(nullptr, "open", LoaderConfig::kReleasesPage, nullptr, nullptr, SW_SHOWNORMAL);
            SetStatus(s, "Opened GitHub releases — download manually");
            return;
        }
        s->updating = true;
        s->progress = 0.f;
        SetStatus(s, "Updating…");
        RefreshLaunchButton(s);
        LoaderUpdate::Manifest m = s->manifest;
        std::thread([s, m]() {
            std::string err;
            bool ok = LoaderUpdate::ApplyUpdate(m,
                [s](float p, const char* msg) {
                    s->progress = p;
                    SetStatus(s, msg);
                }, err);
            if (ok) {
                s->localVersion = m.version;
                s->updateAvailable = false;
                s->kernelAvailable = LoaderUpdate::ProbeKernelAvailable() || m.kernelAvailable;
                SetStatus(s, "Update complete");
            } else {
                char buf[256];
                sprintf_s(buf, "Update failed: %s", err.c_str());
                SetStatus(s, buf);
            }
            s->progress = 0.f;
            s->updating = false;
            if (gWnd) RefreshLaunchButton(s);
        }).detach();
    }

    inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        State* s = gState;
        switch (msg) {
        case WM_CREATE: {
            CreateWindowW(L"BUTTON", L"Join Discord",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                38, 88, 120, 28, hwnd, (HMENU)ID_BTN_DISCORD, nullptr, nullptr);
            CreateWindowW(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                168, 90, 180, 24, hwnd, (HMENU)ID_EDIT_USER, nullptr, nullptr);
            SendMessageW(GetDlgItem(hwnd, ID_EDIT_USER), EM_SETCUEBANNER, TRUE,
                (LPARAM)L"Roblox username");
            CreateWindowW(L"BUTTON", L"Verify",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                358, 88, 100, 28, hwnd, (HMENU)ID_BTN_VERIFY, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Update Files",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                24, 500, 140, 32, hwnd, (HMENU)ID_BTN_UPDATE, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Launch",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                176, 500, 140, 32, hwnd, (HMENU)ID_BTN_LAUNCH, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Releases",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                328, 500, 140, 32, hwnd, (HMENU)ID_BTN_RELEASES, nullptr, nullptr);
            if (s->robloxUsername[0])
                SetWindowTextA(GetDlgItem(hwnd, ID_EDIT_USER), s->robloxUsername);
            RefreshLaunchButton(s);
            CheckAsync(s);
            return 0;
        }
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
            SetBkMode((HDC)wp, TRANSPARENT);
            SetTextColor((HDC)wp, LoaderConfig::kText);
            return (LRESULT)gBgBrush;
        case WM_CTLCOLOREDIT: {
            SetBkColor((HDC)wp, LoaderConfig::kCard);
            static HBRUSH editBrush = CreateSolidBrush(LoaderConfig::kCard);
            SetTextColor((HDC)wp, LoaderConfig::kText);
            return (LRESULT)editBrush;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            Paint(hdc, s);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (s->updating) break;
            POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            for (int i = 0; i < 2; i++) {
                RECT card = ModeCardRect(i);
                if (PtInRect(&card, pt)) {
                    if (i == 1 && !s->kernelAvailable) break;
                    s->selectedMode = i;
                    LoaderUpdate::SaveMode(i);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;
                }
            }
            return 0;
        }
        case WM_COMMAND: {
            switch (LOWORD(wp)) {
            case ID_BTN_DISCORD: {
                const char* url = s->discordInvite[0] ? s->discordInvite : LoaderConfig::kDiscordInvite;
                ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
                SetStatus(s, "Opened Discord — join, then run /verify YourRobloxName");
                break;
            }
            case ID_BTN_VERIFY:
                RunVerify(s);
                break;
            case ID_BTN_UPDATE:
                if (!s->checking) {
                    if (!s->updateAvailable && !s->manifest.usermodeUrl[0])
                        CheckAsync(s);
                    else
                        RunUpdate(s);
                }
                break;
            case ID_BTN_LAUNCH: {
                if (s->accessRequired && !s->verified) {
                    MessageBoxW(hwnd,
                        L"Join Discord and verify first.\n\n1. Join the server\n2. Run /verify YourRobloxName\n3. Click Verify in the loader",
                        LoaderConfig::kAppName, MB_OK | MB_ICONINFORMATION);
                    break;
                }
                std::wstring err;
                if (!LoaderUpdate::RunPayload(s->selectedMode, err))
                    MessageBoxW(hwnd, err.c_str(), LoaderConfig::kAppName, MB_OK | MB_ICONWARNING);
                else
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
            }
            case ID_BTN_RELEASES:
                ShellExecuteA(nullptr, "open", LoaderConfig::kReleasesPage, nullptr, nullptr, SW_SHOWNORMAL);
                break;
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    inline int Run()
    {
        INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&icc);

        State state{};
        state.selectedMode = LoaderUpdate::LoadSavedMode();
        state.kernelAvailable = LoaderUpdate::ProbeKernelAvailable();
        strncpy_s(state.discordInvite, LoaderConfig::kDiscordInvite, _TRUNCATE);
        LoaderAuth::LoadSavedAuth(state.robloxUserId, state.robloxUsername,
            sizeof(state.robloxUsername), state.verified);
        gState = &state;

        gBgBrush = CreateSolidBrush(LoaderConfig::kBg);
        gFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        gFontBold = CreateFontW(-17, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = gBgBrush;
        wc.lpszClassName = L"FrontierLoaderWnd";
        RegisterClassExW(&wc);

        int sx = GetSystemMetrics(SM_CXSCREEN);
        int sy = GetSystemMetrics(SM_CYSCREEN);
        int x = (sx - LoaderConfig::kWindowW) / 2;
        int y = (sy - LoaderConfig::kWindowH) / 2;

        gWnd = CreateWindowExW(0, wc.lpszClassName, LoaderConfig::kAppName,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            x, y, LoaderConfig::kWindowW, LoaderConfig::kWindowH,
            nullptr, nullptr, wc.hInstance, nullptr);

        if (!gWnd) return 1;
        ShowWindow(gWnd, SW_SHOW);
        UpdateWindow(gWnd);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        DeleteObject(gFont);
        DeleteObject(gFontBold);
        DeleteObject(gBgBrush);
        return (int)msg.wParam;
    }
}
