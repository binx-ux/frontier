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
#pragma comment(lib, "gdi32.lib")

namespace LoaderUI {

    enum CtrlId : int {
        ID_BTN_SIGNIN = 1000,
        ID_BTN_SIGNOUT = 1001,
        ID_BTN_UPDATE = 1002,
        ID_BTN_LAUNCH = 1003,
        ID_BTN_RELEASES = 1004,
        ID_BTN_JOIN = 1005,
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
        bool signedIn = false;
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
    inline HFONT gFontTitle = nullptr;
    inline HBRUSH gBgBrush = nullptr;

    inline RECT AuthCardRect()
    {
        return RECT{ 20, 52, LoaderConfig::kWindowW - 20, 228 };
    }

    inline RECT ModeCardRect(int index)
    {
        RECT rc{};
        rc.left = 20;
        rc.right = LoaderConfig::kWindowW - 20;
        rc.top = 248 + index * 88;
        rc.bottom = rc.top + 76;
        return rc;
    }

    inline void SetStatus(State* s, const char* msg)
    {
        strncpy_s(s->status, msg, _TRUNCATE);
        if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
    }

    inline void SyncVerified(State* s)
    {
        s->signedIn = s->session.token[0] != 0;
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
        HWND join = GetDlgItem(gWnd, ID_BTN_JOIN);
        HWND launch = GetDlgItem(gWnd, ID_BTN_LAUNCH);
        if (signIn) ShowWindow(signIn, s->accessRequired && !s->signedIn ? SW_SHOW : SW_HIDE);
        if (signOut) ShowWindow(signOut, s->accessRequired && s->signedIn ? SW_SHOW : SW_HIDE);
        if (join) ShowWindow(join, s->accessRequired && !s->verified ? SW_SHOW : SW_HIDE);
        if (launch) EnableWindow(launch, (!s->accessRequired || s->verified) && !s->updating && !s->signingIn);
        if (signIn) EnableWindow(signIn, !s->signingIn);
    }

    inline void FillRoundRect(HDC hdc, const RECT& rc, COLORREF fill, COLORREF border, int radius = 10)
    {
        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    inline void DrawStep(HDC hdc, int x, int y, int num, const char* label, bool done, bool active)
    {
        COLORREF dot = done ? RGB(80, 200, 120) : (active ? LoaderConfig::kBrand : RGB(56, 56, 64));
        HBRUSH b = CreateSolidBrush(dot);
        HGDIOBJ ob = SelectObject(hdc, b);
        Ellipse(hdc, x, y, x + 18, y + 18);
        SelectObject(hdc, ob);
        DeleteObject(b);

        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, LoaderConfig::kText);
        char numBuf[4];
        sprintf_s(numBuf, "%d", num);
        SIZE ns{};
        GetTextExtentPoint32A(hdc, numBuf, (int)strlen(numBuf), &ns);
        TextOutA(hdc, x + (18 - ns.cx) / 2, y + 2, numBuf, (int)strlen(numBuf));

        SelectObject(hdc, gFont);
        SetTextColor(hdc, active ? LoaderConfig::kText : LoaderConfig::kTextDim);
        TextOutA(hdc, x + 26, y + 2, label, (int)strlen(label));
    }

    inline void Paint(HDC hdc, State* s)
    {
        RECT client{};
        GetClientRect(gWnd, &client);
        FillRect(hdc, &client, gBgBrush);

        SetBkMode(hdc, TRANSPARENT);

        RECT header{ 0, 0, client.right, 44 };
        HBRUSH headerBrush = CreateSolidBrush(RGB(16, 16, 18));
        FillRect(hdc, &header, headerBrush);
        DeleteObject(headerBrush);

        RECT stripe{ 0, 0, 3, client.bottom };
        HBRUSH brand = CreateSolidBrush(LoaderConfig::kBrand);
        FillRect(hdc, &stripe, brand);
        DeleteObject(brand);

        SelectObject(hdc, gFontTitle);
        SetTextColor(hdc, LoaderConfig::kText);
        TextOutA(hdc, 18, 10, "FRONTIER", 8);
        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        TextOutA(hdc, 118, 14, "Loader", 6);

        char ver[32];
        sprintf_s(ver, "v%d", s->localVersion);
        SIZE vs{};
        GetTextExtentPoint32A(hdc, ver, (int)strlen(ver), &vs);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        TextOutA(hdc, client.right - vs.cx - 18, 14, ver, (int)strlen(ver));

        if (s->accessRequired) {
            RECT access = AuthCardRect();
            FillRoundRect(hdc, access, LoaderConfig::kCard, LoaderConfig::kBorder);

            SelectObject(hdc, gFontBold);
            SetTextColor(hdc, LoaderConfig::kText);
            TextOutA(hdc, access.left + 16, access.top + 14, "Discord access", 14);

            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextDim);
            TextOutA(hdc, access.left + 16, access.top + 36,
                "Join the server, verify with /verify, then sign in.", 48);

            bool step1 = true;
            bool step2 = s->signedIn || s->verified;
            bool step3 = s->verified;
            DrawStep(hdc, access.left + 16, access.top + 62, 1, "Join Discord", true, !s->signedIn);
            DrawStep(hdc, access.left + 16, access.top + 86, 2, "Run /verify YourRobloxName", step2, s->signedIn && !s->verified);
            DrawStep(hdc, access.left + 16, access.top + 110, 3, "Sign in below", step3, s->signedIn);

            if (s->signingIn) {
                SetTextColor(hdc, LoaderConfig::kDiscord);
                TextOutA(hdc, access.left + 16, access.top + 140,
                    "Waiting for browser — complete Discord sign-in…", 47);
            } else if (s->verified && s->discordName[0]) {
                SetTextColor(hdc, RGB(80, 200, 120));
                char ok[160];
                if (s->session.robloxUserId > 0)
                    sprintf_s(ok, "Ready — %s  ·  Roblox %d", s->discordName, s->session.robloxUserId);
                else
                    sprintf_s(ok, "Ready — %s", s->discordName);
                TextOutA(hdc, access.left + 16, access.top + 140, ok, (int)strlen(ok));
            } else if (s->signedIn && s->discordName[0]) {
                SetTextColor(hdc, RGB(255, 190, 90));
                char pending[160];
                sprintf_s(pending, "Signed in as %s — finish /verify to launch", s->discordName);
                TextOutA(hdc, access.left + 16, access.top + 140, pending, (int)strlen(pending));
            } else {
                SetTextColor(hdc, LoaderConfig::kTextDim);
                TextOutA(hdc, access.left + 16, access.top + 140, "Not signed in", 13);
            }
        }

        SetTextColor(hdc, LoaderConfig::kText);
        SelectObject(hdc, gFontBold);
        TextOutA(hdc, 20, s->accessRequired ? 238 : 52, "Launch mode", 11);
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

            FillRoundRect(hdc, card,
                disabled ? RGB(14, 14, 16) : LoaderConfig::kCard,
                sel ? LoaderConfig::kBrand : LoaderConfig::kBorder);

            SetTextColor(hdc, disabled ? LoaderConfig::kTextDim : LoaderConfig::kText);
            SelectObject(hdc, gFontBold);
            TextOutA(hdc, card.left + 14, card.top + 12, labels[i], (int)strlen(labels[i]));
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextDim);
            TextOutA(hdc, card.left + 14, card.top + 34, descs[i], (int)strlen(descs[i]));

            if (i == 1 && !s->kernelAvailable) {
                SetTextColor(hdc, LoaderConfig::kBrand);
                TextOutA(hdc, card.left + 14, card.top + 54, "Not installed", 13);
            }
        }

        if (s->updating || s->progress > 0.f) {
            RECT barBg{ 20, 430, client.right - 20, 438 };
            FillRoundRect(hdc, barBg, RGB(28, 28, 34), RGB(28, 28, 34), 4);
            RECT barFg = barBg;
            barFg.right = barBg.left + (LONG)((barBg.right - barBg.left) * s->progress);
            if (barFg.right > barFg.left) {
                HBRUSH fg = CreateSolidBrush(LoaderConfig::kBrand);
                FillRect(hdc, &barFg, fg);
                DeleteObject(fg);
            }
        }

        SetTextColor(hdc, LoaderConfig::kTextDim);
        TextOutA(hdc, 20, 452, s->status, (int)strlen(s->status));

        if (s->updateAvailable && s->remoteDisplay[0]) {
            char upd[128];
            sprintf_s(upd, "Update available: %s", s->remoteDisplay);
            SetTextColor(hdc, LoaderConfig::kBrand);
            TextOutA(hdc, 20, 472, upd, (int)strlen(upd));
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
                SetStatus(s, s->verified ? "Signed in — you can launch" : "Complete /verify in Discord");
            } else {
                if (!s->session.token[0]) {
                    s->session.verified = false;
                    s->session.allowed = false;
                }
                SyncVerified(s);
                if (err[0]) SetStatus(s, err.c_str());
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
            LoaderOAuth::Session session = s->session;
            std::string err;
            bool ok = LoaderOAuth::SignIn(session, err);
            s->session = session;
            SyncVerified(s);
            if (ok && s->verified)
                SetStatus(s, "Signed in — you can launch");
            else if (ok && s->signedIn)
                SetStatus(s, err.empty() ? "Discord linked — run /verify in the server" : err.c_str());
            else
                SetStatus(s, err.empty() ? "Discord sign-in failed" : err.c_str());
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
                36, 178, 210, 34, hwnd, (HMENU)ID_BTN_SIGNIN, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Sign out",
                WS_CHILD | BS_PUSHBUTTON,
                256, 178, 90, 34, hwnd, (HMENU)ID_BTN_SIGNOUT, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Join Discord",
                WS_CHILD | BS_PUSHBUTTON,
                36, 178, 120, 34, hwnd, (HMENU)ID_BTN_JOIN, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Update Files",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 500, 130, 34, hwnd, (HMENU)ID_BTN_UPDATE, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Launch",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                162, 500, 130, 34, hwnd, (HMENU)ID_BTN_LAUNCH, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Releases",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                304, 500, 130, 34, hwnd, (HMENU)ID_BTN_RELEASES, nullptr, nullptr);
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
            case ID_BTN_JOIN:
                ShellExecuteA(nullptr, "open",
                    s->discordInvite[0] ? s->discordInvite : LoaderConfig::kDiscordInvite,
                    nullptr, nullptr, SW_SHOWNORMAL);
                SetStatus(s, "Opened Discord invite");
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
                        L"Complete Discord access first:\n\n1. Join the Discord server\n2. Run /verify YourRobloxName\n3. Sign in with Discord",
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
        gFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        gFontBold = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        gFontTitle = CreateFontW(-22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
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

        EnumChildWindows(gWnd, [](HWND child, LPARAM font) {
            SendMessageW(child, WM_SETFONT, font, TRUE);
            return TRUE;
        }, (LPARAM)gFont);

        ShowWindow(gWnd, SW_SHOW);
        UpdateWindow(gWnd);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        DeleteObject(gFont);
        DeleteObject(gFontBold);
        DeleteObject(gFontTitle);
        DeleteObject(gBgBrush);
        return (int)msg.wParam;
    }
}
