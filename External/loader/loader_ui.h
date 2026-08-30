#pragma once
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <cstdio>
#include <thread>
#include "loader_config.h"
#include "loader_update.h"
#include "loader_oauth.h"
#include "resource.h"
#include <Shellapi.h>

#pragma comment(lib, "comctl32.lib")

namespace LoaderUI {

    enum CtrlId : int {
        ID_BTN_SIGNIN = 1000,
        ID_BTN_SIGNOUT = 1001,
        ID_BTN_UPDATE = 1002,
        ID_BTN_LAUNCH = 1003,
        ID_BTN_RELEASES = 1004,
    };

    struct State {
        int selectedMode = 0;
        bool kernelAvailable = false;
        bool checking = false;
        bool updating = false;
        bool signingIn = false;
        bool updateAvailable = false;
        bool accessRequired = true;
        bool verified = false;
        int localVersion = LoaderConfig::kLocalVersion;
        int remoteVersion = 0;
        char status[256] = "Ready";
        char remoteDisplay[64] = "";
        char discordInvite[128] = "";
        char discordName[64] = "";
        float progress = 0.f;
        LoaderUpdate::Manifest manifest{};
        LoaderOAuth::Session session{};
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

    inline void SyncVerified(State* s)
    {
        s->verified = s->session.verified && s->session.allowed;
        if (s->session.globalName[0])
            strncpy_s(s->discordName, s->session.globalName, _TRUNCATE);
        else
            strncpy_s(s->discordName, s->session.username, _TRUNCATE);
    }

    inline void RefreshAuthButtons(State* s)
    {
        HWND signIn = GetDlgItem(gWnd, ID_BTN_SIGNIN);
        HWND signOut = GetDlgItem(gWnd, ID_BTN_SIGNOUT);
        HWND launch = GetDlgItem(gWnd, ID_BTN_LAUNCH);
        if (signIn) ShowWindow(signIn, s->accessRequired && !s->verified ? SW_SHOW : SW_HIDE);
        if (signOut) ShowWindow(signOut, s->accessRequired && s->verified ? SW_SHOW : SW_HIDE);
        if (launch) EnableWindow(launch, (!s->accessRequired || s->verified) && !s->updating && !s->signingIn);
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
        TextOutA(hdc, client.right - 56, 20, ver, (int)strlen(ver));

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
            TextOutA(hdc, access.left + 14, access.top + 12, "Sign in with Discord", 20);
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextDim);
            TextOutA(hdc, access.left + 14, access.top + 34,
                "Join the server, run /verify YourRobloxName, then sign in below.", 63);

            if (s->verified && s->discordName[0]) {
                SetTextColor(hdc, RGB(80, 200, 120));
                char ok[128];
                if (s->session.robloxUserId > 0)
                    sprintf_s(ok, "Signed in as %s  (Roblox %d)", s->discordName, s->session.robloxUserId);
                else
                    sprintf_s(ok, "Signed in as %s", s->discordName);
                TextOutA(hdc, access.left + 14, access.top + 130, ok, (int)strlen(ok));
            } else if (s->signingIn) {
                SetTextColor(hdc, LoaderConfig::kBrand);
                TextOutA(hdc, access.left + 14, access.top + 130,
                    "Waiting for Discord — complete sign-in in your browser…", 54);
            } else {
                SetTextColor(hdc, LoaderConfig::kBrand);
                TextOutA(hdc, access.left + 14, access.top + 130,
                    "Not signed in", 13);
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
                RefreshAuthButtons(s);
                InvalidateRect(gWnd, nullptr, FALSE);
            }
            s->checking = false;
        }).detach();
    }

    inline void RefreshSessionAsync(State* s)
    {
        if (!s->session.token[0]) return;
        std::thread([s]() {
            std::string err;
            if (LoaderOAuth::RefreshSession(s->session, err)) {
                SyncVerified(s);
                SetStatus(s, "Signed in with Discord");
            } else {
                s->session.verified = false;
                s->session.allowed = false;
                SyncVerified(s);
            }
            if (gWnd) {
                RefreshAuthButtons(s);
                InvalidateRect(gWnd, nullptr, FALSE);
            }
        }).detach();
    }

    inline void RunSignIn(State* s)
    {
        if (s->signingIn) return;
        s->signingIn = true;
        SetStatus(s, "Opening Discord sign-in…");
        RefreshAuthButtons(s);
        InvalidateRect(gWnd, nullptr, FALSE);
        std::thread([s]() {
            LoaderOAuth::Session session{};
            std::string err;
            bool ok = LoaderOAuth::SignIn(session, err);
            s->session = session;
            SyncVerified(s);
            if (ok) {
                SetStatus(s, "Signed in — you can launch");
            } else {
                s->session.verified = false;
                s->session.allowed = false;
                SyncVerified(s);
                SetStatus(s, err.c_str());
            }
            s->signingIn = false;
            if (gWnd) {
                RefreshAuthButtons(s);
                InvalidateRect(gWnd, nullptr, FALSE);
            }
        }).detach();
    }

    inline void RunSignOut(State* s)
    {
        LoaderOAuth::ClearSession();
        memset(&s->session, 0, sizeof(s->session));
        SyncVerified(s);
        SetStatus(s, "Signed out");
        RefreshAuthButtons(s);
        InvalidateRect(gWnd, nullptr, FALSE);
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
        RefreshAuthButtons(s);
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
            if (gWnd) RefreshAuthButtons(s);
        }).detach();
    }

    inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        State* s = gState;
        switch (msg) {
        case WM_CREATE: {
            CreateWindowW(L"BUTTON", L"Sign in with Discord",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                38, 88, 200, 32, hwnd, (HMENU)ID_BTN_SIGNIN, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Sign out",
                WS_CHILD | BS_PUSHBUTTON,
                248, 88, 100, 32, hwnd, (HMENU)ID_BTN_SIGNOUT, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Update Files",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                24, 500, 140, 32, hwnd, (HMENU)ID_BTN_UPDATE, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Launch",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                176, 500, 140, 32, hwnd, (HMENU)ID_BTN_LAUNCH, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Releases",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                328, 500, 140, 32, hwnd, (HMENU)ID_BTN_RELEASES, nullptr, nullptr);
            SyncVerified(s);
            RefreshAuthButtons(s);
            CheckAsync(s);
            if (s->session.token[0])
                RefreshSessionAsync(s);
            return 0;
        }
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
            SetBkMode((HDC)wp, TRANSPARENT);
            SetTextColor((HDC)wp, LoaderConfig::kText);
            return (LRESULT)gBgBrush;
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
            case ID_BTN_SIGNIN:
                RunSignIn(s);
                break;
            case ID_BTN_SIGNOUT:
                RunSignOut(s);
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
                        L"Sign in with Discord first.\n\n1. Join the server\n2. Run /verify YourRobloxName\n3. Click Sign in with Discord",
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
        LoaderOAuth::LoadSession(state.session);
        SyncVerified(&state);
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
