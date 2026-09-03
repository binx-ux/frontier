#pragma once
#include <Windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <CommCtrl.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <thread>
#include "loader_config.h"
#include "loader_update.h"
#include "loader_oauth.h"
#include "loader_license.h"
#include "loader_products.h"
#include "loader_assets.h"
#include "resource.h"
#include <Shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

namespace LoaderUI {

    enum Screen : int {
        ScreenDownloading = 0,
        ScreenReady = 1,
        ScreenAuth = 2,
        ScreenLauncher = 3,
        ScreenProducts = 4,
    };

    enum HoverTarget : int {
        HoverNone = 0,
        HoverPrimary = 1,
        HoverMode0 = 2,
        HoverMode1 = 3,
        HoverMin = 4,
        HoverClose = 5,
        HoverDiscord = 6,
        HoverActivate = 7,
        HoverSignOut = 8,
        HoverSupport = 9,
        HoverProductTrace = 10,
        HoverProductFrontier = 11,
        HoverGuest = 12,
        HoverChangeKey = 13,
    };

    struct State {
        Screen screen = ScreenDownloading;
        int selectedMode = 0;
        bool kernelAvailable = false;
        bool checking = false;
        bool updating = false;
        bool updateAvailable = false;
        int localVersion = LoaderConfig::kLocalVersion;
        int remoteVersion = 0;
        char status[256] = "Checking files...";
        char remoteDisplay[64] = "";
        float progress = 0.f;
        bool indeterminate = false;
        bool filesReady = false;
        bool authenticated = false;
        bool licenseActive = false;
        bool authBusy = false;
        int authStatusTone = 0; // 0=hint, 1=busy, 2=error, 3=success
        char licenseToken[768] = "";
        char licenseHwid[64] = "";
        char authStatus[256] = "";
        char signedInAs[64] = "";
        LoaderUpdate::Manifest manifest{};
        int selectedProduct = LoaderProducts::ProductNone;
        bool launchAfterUpdate = false;
    };

    inline State* gState = nullptr;
    inline HWND gWnd = nullptr;
    inline std::atomic<bool> gUiAlive{ false };
    inline HFONT gFont = nullptr;
    inline HFONT gFontBold = nullptr;
    inline HFONT gFontLogo = nullptr;
    inline HBRUSH gBgBrush = nullptr;
    inline HBRUSH gEditBrush = nullptr;
    inline HICON gAppIcon = nullptr;
    inline HICON gAppIconSm = nullptr;
    inline int gHover = HoverNone;
    inline bool gTrackingMouse = false;
    inline HWND gKeyEdit = nullptr;
    inline HWND gPassEdit = nullptr;

    inline const UINT WM_AUTH_DONE = WM_USER + 10;
    inline const UINT WM_RELAUNCH_LOADER = WM_USER + 12;
    inline const UINT WM_CHECK_DONE = WM_USER + 22;
    inline const UINT WM_UPDATE_TICK = WM_USER + 23;
    inline const UINT WM_UPDATE_DONE = WM_USER + 24;
    inline const UINT WM_BOOT_DONE = WM_USER + 25;
    inline const UINT WM_SHOW_MSG = WM_USER + 26;
    inline const UINT WM_RUN_TRACE = WM_USER + 27;

    struct CheckDoneMsg {
        LoaderUpdate::Manifest manifest{};
        bool fetchOk = false;
        bool needsUpdate = false;
        bool hasLocal = false;
        bool kernelAvailable = false;
        int localVersion = 0;
        int remoteVersion = 0;
        char remoteDisplay[64]{};
        char status[256]{};
    };

    struct UpdateTickMsg {
        float progress = 0.f;
        char status[256]{};
    };

    struct UpdateDoneMsg {
        bool ok = false;
        bool launchAfter = false;
        bool relaunch = false;
        char status[256]{};
        LoaderUpdate::Manifest manifest{};
    };

    struct AuthDonePayload {
        bool ok = false;
        bool licensed = false;
        char status[256]{};
        char token[768]{};
        char signedInAs[64]{};
    };

    struct BootDoneMsg {
        bool authenticated = false;
        bool licenseActive = false;
        char authStatus[256]{};
        char licenseToken[768]{};
        char licenseHwid[64]{};
        char signedInAs[64]{};
    };

    struct ShowMsgPayload {
        wchar_t text[512]{};
        UINT type = MB_OK | MB_ICONINFORMATION;
    };

    inline bool PostToUi(UINT msg, WPARAM wp, LPARAM lp)
    {
        if (!gUiAlive.load(std::memory_order_acquire)) return false;
        HWND hwnd = gWnd;
        if (!hwnd || !IsWindow(hwnd)) return false;
        return PostMessageW(hwnd, msg, wp, lp) != 0;
    }

    inline void RequestRedraw()
    {
        HWND hwnd = gWnd;
        if (hwnd && IsWindow(hwnd))
            InvalidateRect(hwnd, nullptr, FALSE);
    }

    inline bool ShowKernelMode(const State* s)
    {
        return LoaderConfig::kKernelModeOffered || (s && s->kernelAvailable);
    }

    inline bool LocalUsermodeExists()
    {
        return LoaderUpdate::LocalUsermodeExists() || [&]() {
            std::wstring exe, work;
            return LoaderUpdate::ResolveUsermodeExe(exe, work);
        }();
    }

    inline float HoverOn(int target)
    {
        return gHover == target ? 1.f : 0.f;
    }

    inline COLORREF BlendRgb(COLORREF a, COLORREF b, float t)
    {
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        return RGB(
            GetRValue(a) + (int)((GetRValue(b) - GetRValue(a)) * t),
            GetGValue(a) + (int)((GetGValue(b) - GetGValue(a)) * t),
            GetBValue(a) + (int)((GetBValue(b) - GetBValue(a)) * t));
    }

    inline void SetStatus(State* s, const char* msg)
    {
        if (!s || !msg) return;
        strncpy_s(s->status, msg, _TRUNCATE);
        RequestRedraw();
    }

    inline void SetAuthStatus(State* s, const char* msg, int tone = 0)
    {
        if (!s || !msg) return;
        strncpy_s(s->authStatus, msg, _TRUNCATE);
        s->authStatusTone = tone;
        RequestRedraw();
    }

    inline void SetProgress(State* s, float value, bool indeterminate = false)
    {
        if (!s) return;
        if (value < 0.f) value = 0.f;
        if (value > 1.f) value = 1.f;
        s->progress = value;
        s->indeterminate = indeterminate;
    }

    // ---- layout ----

    inline RECT BrandPanelRect()
    {
        return RECT{ 0, 0, LoaderConfig::kBrandPanelW, LoaderConfig::kWindowH };
    }

    inline RECT CloseButtonRect()
    {
        return RECT{ LoaderConfig::kWindowW - 40, 8, LoaderConfig::kWindowW - 12, 32 };
    }

    inline RECT MinButtonRect()
    {
        return RECT{ LoaderConfig::kWindowW - 74, 8, LoaderConfig::kWindowW - 46, 32 };
    }

    inline RECT SupportLinkRect()
    {
        return RECT{ LoaderConfig::kBrandPanelW + 16, LoaderConfig::kWindowH - 24,
            LoaderConfig::kBrandPanelW + 90, LoaderConfig::kWindowH - 6 };
    }

    inline RECT ContentRect()
    {
        return RECT{
            LoaderConfig::kBrandPanelW + LoaderConfig::kMarginX, 40,
            LoaderConfig::kWindowW - LoaderConfig::kMarginX, LoaderConfig::kWindowH - 28
        };
    }

    inline RECT HelloTextRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top, r.right, r.top + 20 };
    }

    inline RECT SubtitleTextRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 22, r.right, r.top + 50 };
    }

    inline RECT KeyFieldRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 52, r.right, r.top + 78 };
    }

    inline RECT PasswordFieldRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 86, r.right, r.top + 112 };
    }

    inline RECT AuthStatusRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 86, r.right, r.top + 110 };
    }

    inline RECT PrimaryButtonRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 118, r.right, r.top + 146 };
    }

    inline RECT SignOutButtonRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 88, r.right, r.top + 116 };
    }

    inline RECT GuestButtonRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 154, r.right, r.top + 182 };
    }

    inline RECT AuthContinueRect(const State* s)
    {
        RECT r = ContentRect();
        if (s && s->authenticated)
            return RECT{ r.left, r.top + 52, r.right, r.top + 80 };
        return PrimaryButtonRect();
    }

    inline RECT ProgressBarRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.bottom - 8, r.right, r.bottom - 2 };
    }

    inline RECT TraceProductRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 58, r.right, r.top + 58 + 72 };
    }

    inline RECT FrontierProductRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 138, r.right, r.top + 138 + 72 };
    }

    inline RECT ChangeKeyButtonRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 218, r.right, r.top + 246 };
    }

    inline bool ShowChangeKeyButton(const State* s)
    {
        (void)s;
        return false; // open-source: no key UI
    }

    inline RECT GamePanelRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left, r.top + 24, r.left + 140, r.bottom };
    }

    inline RECT InfoPanelRect()
    {
        RECT r = ContentRect();
        return RECT{ r.left + 152, r.top + 24, r.right, r.bottom };
    }

    inline RECT LaunchButtonRect()
    {
        RECT ip = InfoPanelRect();
        return RECT{ ip.left + 10, ip.bottom - 38, ip.right - 10, ip.bottom - 8 };
    }

    inline RECT ModeCardRect(const State* s, int displayIndex)
    {
        RECT gp = GamePanelRect();
        const int top = gp.top + 26 + displayIndex * 38;
        return RECT{ gp.left + 8, top, gp.right - 8, top + 32 };
    }

    inline bool HeaderDragArea(POINT pt)
    {
        if (pt.y < 0 || pt.y > 36) return false;
        if (PtInRect(&CloseButtonRect(), pt)) return false;
        if (PtInRect(&MinButtonRect(), pt)) return false;
        return true;
    }

    // ---- edits ----

    inline void RepositionKeyEdit()
    {
        if (!gKeyEdit || !IsWindow(gKeyEdit)) return;
        RECT field = KeyFieldRect();
        const int pad = 10;
        SetWindowPos(gKeyEdit, nullptr,
            field.left + pad, field.top + 5,
            field.right - field.left - pad * 2,
            field.bottom - field.top - 10,
            SWP_NOZORDER | SWP_NOACTIVATE);
        if (gPassEdit && IsWindow(gPassEdit)) {
            ShowWindow(gPassEdit, SW_HIDE);
            EnableWindow(gPassEdit, FALSE);
        }
    }

    inline void SetKeyEditVisible(bool visible)
    {
        if (!gKeyEdit || !IsWindow(gKeyEdit)) return;
        RepositionKeyEdit();
        ShowWindow(gKeyEdit, visible ? SW_SHOW : SW_HIDE);
        EnableWindow(gKeyEdit, visible && !(gState && gState->authBusy));
    }

    inline void SetPassEditVisible(bool visible)
    {
        (void)visible;
        if (gPassEdit && IsWindow(gPassEdit)) {
            ShowWindow(gPassEdit, SW_HIDE);
            EnableWindow(gPassEdit, FALSE);
        }
    }

    inline void SyncAuthFields(State* s)
    {
        const bool show = s && s->screen == ScreenAuth && !s->authenticated;
        SetKeyEditVisible(show);
        SetPassEditVisible(false);
    }

    inline void TransitionToScreen(State* s, Screen next)
    {
        if (!s || s->screen == next) return;
        s->screen = next;
        SyncAuthFields(s);
        RequestRedraw();
    }

    inline void GoToLauncher(State* s);
    inline void GoToProducts(State* s);
    inline void RunLaunch(State* s);
    inline void RunUpdate(State* s);
    inline void CheckAsync(State* s);

    // ---- drawing ----

    inline void FillRoundRect(HDC hdc, const RECT& rc, COLORREF fill, COLORREF border, int radius = 8)
    {
        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HGDIOBJ ob = SelectObject(hdc, brush);
        HGDIOBJ op = SelectObject(hdc, pen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    inline void FillVerticalGradient(HDC hdc, const RECT& rc, COLORREF top, COLORREF bottom)
    {
        if (rc.bottom <= rc.top) return;
        TRIVERTEX vert[2]{};
        vert[0].x = rc.left; vert[0].y = rc.top;
        vert[0].Red = (COLOR16)(GetRValue(top) << 8);
        vert[0].Green = (COLOR16)(GetGValue(top) << 8);
        vert[0].Blue = (COLOR16)(GetBValue(top) << 8);
        vert[1].x = rc.right; vert[1].y = rc.bottom;
        vert[1].Red = (COLOR16)(GetRValue(bottom) << 8);
        vert[1].Green = (COLOR16)(GetGValue(bottom) << 8);
        vert[1].Blue = (COLOR16)(GetBValue(bottom) << 8);
        GRADIENT_RECT gRect{ 0, 1 };
        GradientFill(hdc, vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
    }

    inline void DrawBackground(HWND hwnd, HDC hdc)
    {
        RECT client{};
        GetClientRect(hwnd, &client);
        FillVerticalGradient(hdc, client, LoaderConfig::kBgTop, LoaderConfig::kBgBottom);
        RECT brand = BrandPanelRect();
        FillVerticalGradient(hdc, brand, RGB(14, 15, 20), RGB(9, 10, 14));
        RECT content = ContentRect();
        InflateRect(&content, 8, 4);
        FillRoundRect(hdc, content, RGB(12, 16, 24), RGB(24, 30, 42), 12);
        RECT sep{ brand.right - 1, brand.top, brand.right, brand.bottom };
        HBRUSH b = CreateSolidBrush(LoaderConfig::kBorder);
        FillRect(hdc, &sep, b);
        DeleteObject(b);
    }

    inline void DrawBrandPanel(HDC hdc)
    {
        RECT panel = BrandPanelRect();
        const int cy = (panel.top + panel.bottom) / 2;

        RECT logo{ panel.left + 28, cy - 42, panel.left + 62, cy - 8 };
        FillRoundRect(hdc, logo, LoaderConfig::kAccent, LoaderConfig::kAccentHover, 8);
        SelectObject(hdc, gFontBold);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(4, 16, 24));
        DrawTextA(hdc, "A", -1, &logo, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, gFontLogo ? gFontLogo : gFontBold);
        SetTextColor(hdc, LoaderConfig::kText);
        RECT mark{ panel.left + 28, cy - 2, panel.right - 16, cy + 22 };
        DrawTextA(hdc, "FRONTIER", -1, &mark, DT_LEFT | DT_SINGLELINE);

        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        char sub[64];
        sprintf_s(sub, "%s - ahead.best", LoaderConfig::kDisplayVersion);
        RECT subRc{ panel.left + 28, cy + 22, panel.right - 16, cy + 42 };
        DrawTextA(hdc, sub, -1, &subRc, DT_LEFT | DT_SINGLELINE);

        RECT glow{ panel.left + 28, cy + 50, panel.right - 28, cy + 51 };
        HBRUSH accentBrush = CreateSolidBrush(LoaderConfig::kAccent);
        FillRect(hdc, &glow, accentBrush);
        DeleteObject(accentBrush);
    }

    inline void DrawChromeBtn(HDC hdc, const RECT& rc, const char* label, bool danger, bool hot)
    {
        COLORREF fill = hot
            ? (danger ? RGB(140, 48, 56) : LoaderConfig::kCardHover)
            : (danger ? RGB(32, 18, 22) : LoaderConfig::kCard);
        FillRoundRect(hdc, rc, fill, LoaderConfig::kBorder, 6);
        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, danger ? RGB(240, 150, 150) : LoaderConfig::kTextDim);
        DrawTextA(hdc, label, -1, const_cast<RECT*>(&rc), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    inline void DrawWindowControls(HDC hdc)
    {
        DrawChromeBtn(hdc, MinButtonRect(), "-", false, gHover == HoverMin);
        DrawChromeBtn(hdc, CloseButtonRect(), "x", true, gHover == HoverClose);
    }

    inline void DrawSectionTitle(HDC hdc, const char* title, const char* sub)
    {
        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        RECT t = HelloTextRect();
        char upper[96];
        strncpy_s(upper, title ? title : "", _TRUNCATE);
        for (char* c = upper; *c; ++c) {
            if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');
        }
        DrawTextA(hdc, upper, -1, &t, DT_LEFT | DT_SINGLELINE);
        if (sub && sub[0]) {
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kText);
            RECT subRc = SubtitleTextRect();
            const int saved = SaveDC(hdc);
            IntersectClipRect(hdc, subRc.left, subRc.top, subRc.right, subRc.bottom);
            DrawTextA(hdc, sub, -1, &subRc, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
            RestoreDC(hdc, saved);
        }
    }

    inline void DrawInputChrome(HDC hdc)
    {
        const bool keyFocus = gKeyEdit && GetFocus() == gKeyEdit;
        FillRoundRect(hdc, KeyFieldRect(), LoaderConfig::kInputBg,
            keyFocus ? LoaderConfig::kBorderActive : LoaderConfig::kInputBorder, 6);
        if (!gKeyEdit || !IsWindowVisible(gKeyEdit) || !GetWindowTextLengthA(gKeyEdit)) {
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextPlaceholder);
            RECT hint = KeyFieldRect();
            hint.left += 10;
            DrawTextA(hdc, "License key", -1, &hint, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
    }

    inline void DrawButton(HDC hdc, const RECT& btn, const char* label, bool enabled, bool hot)
    {
        COLORREF fill = enabled
            ? (hot ? LoaderConfig::kAccentHover : LoaderConfig::kAccent)
            : LoaderConfig::kAccentDim;
        FillRoundRect(hdc, btn, fill, hot ? LoaderConfig::kAccentLight : fill, 8);
        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextA(hdc, label, -1, const_cast<RECT*>(&btn), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    inline void DrawGhostButton(HDC hdc, const RECT& btn, const char* label, bool hot)
    {
        COLORREF fill = hot ? LoaderConfig::kCardHover : LoaderConfig::kCard;
        FillRoundRect(hdc, btn, fill, LoaderConfig::kBorder, 6);
        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        DrawTextA(hdc, label, -1, const_cast<RECT*>(&btn), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    inline void DrawProgressBar(HDC hdc, const State* s, bool success)
    {
        if (!s) return;
        RECT barBg = ProgressBarRect();
        FillRoundRect(hdc, barBg, LoaderConfig::kBarTrack, LoaderConfig::kBarTrack, 3);
        float pct = s->indeterminate ? 0.35f : s->progress;
        if (pct < 0.f) pct = 0.f;
        if (pct > 1.f) pct = 1.f;
        const int w = barBg.right - barBg.left;
        if (w > 0 && pct > 0.01f) {
            RECT fg = barBg;
            fg.right = barBg.left + (LONG)(w * pct);
            if (fg.right < fg.left + 4) fg.right = fg.left + 4;
            FillRoundRect(hdc, fg, success ? LoaderConfig::kSuccess : LoaderConfig::kAccent,
                success ? LoaderConfig::kSuccess : LoaderConfig::kAccent, 3);
        }
    }

    inline void DrawProductRow(HDC hdc, const RECT& rc, const char* name,
        const char* line, const char* hint, COLORREF accent, bool hot, bool locked)
    {
        COLORREF border = hot ? accent : LoaderConfig::kBorder;
        FillRoundRect(hdc, rc, hot ? LoaderConfig::kCardHover : LoaderConfig::kCard, border, 10);
        if (hot) {
            RECT glow = rc;
            InflateRect(&glow, -1, -1);
            FillRoundRect(hdc, glow, RGB(20, 28, 40), accent, 10);
        }

        HBRUSH dot = CreateSolidBrush(accent);
        HGDIOBJ ob = SelectObject(hdc, dot);
        const int x = rc.left + 16;
        Ellipse(hdc, x, rc.top + 18, x + 10, rc.top + 28);
        SelectObject(hdc, ob);
        DeleteObject(dot);

        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, LoaderConfig::kText);
        RECT nameRc{ x + 18, rc.top + 12, rc.right - 28, rc.top + 30 };
        DrawTextA(hdc, name, -1, &nameRc, DT_LEFT | DT_SINGLELINE);

        SelectObject(hdc, gFont);
        SetTextColor(hdc, locked ? LoaderConfig::kWarning : LoaderConfig::kTextDim);
        RECT lineRc{ x + 18, rc.top + 30, rc.right - 28, rc.top + 48 };
        DrawTextA(hdc, line, -1, &lineRc, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

        SetTextColor(hdc, LoaderConfig::kTextMuted);
        RECT hintRc{ x + 18, rc.top + 48, rc.right - 28, rc.bottom - 8 };
        DrawTextA(hdc, hint, -1, &hintRc, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);

        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, hot ? accent : LoaderConfig::kTextMuted);
        RECT arrow{ rc.right - 24, rc.top, rc.right - 6, rc.bottom };
        DrawTextA(hdc, ">", -1, &arrow, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    inline void DrawProducts(HDC hdc, const State* s)
    {
        const bool hasKey = s && s->authenticated;
        const bool licensed = s && s->licenseActive;
        const char* sub = !hasKey
            ? "TRACE is free. Activate your key for FRONTIER."
            : (licensed
                ? "Ready to launch."
                : "Key saved — purchase at ahead.best to unlock.");
        DrawSectionTitle(hdc, "Products", sub);
        const bool frontierDown = LoaderConfig::kFrontierMaintenance;
        const bool frontierLocked = !hasKey || !licensed;
        const char* frontierHint = frontierDown
            ? LoaderConfig::kFrontierMaintenanceMsg
            : (!hasKey
                ? "Activate your key first"
                : (!licensed
                    ? "Click to enter your key"
                    : "Usermode or Kernel mode"));
        DrawProductRow(hdc, TraceProductRect(), "TRACE",
            LoaderProducts::ProductTagline(LoaderProducts::ProductTrace),
            "Copies loadstring to clipboard",
            LoaderConfig::kTraceAccent, gHover == HoverProductTrace, false);
        DrawProductRow(hdc, FrontierProductRect(), "FRONTIER",
            LoaderProducts::ProductTagline(LoaderProducts::ProductFrontier),
            frontierHint,
            LoaderConfig::kFrontierAccent, gHover == HoverProductFrontier,
            frontierLocked || frontierDown);
        if (ShowChangeKeyButton(s))
            DrawGhostButton(hdc, ChangeKeyButtonRect(), "Enter license key", gHover == HoverChangeKey);
    }

    inline void DrawModeCard(HDC hdc, const State* s, int idx, int mode, const char* label, bool hot)
    {
        RECT card = ModeCardRect(s, idx);
        const bool sel = s && s->selectedMode == mode;
        COLORREF fill = sel ? RGB(28, 30, 44) : (hot ? LoaderConfig::kCardHover : LoaderConfig::kCardInner);
        FillRoundRect(hdc, card, fill, sel ? LoaderConfig::kAccent : LoaderConfig::kBorder, 6);
        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, sel ? LoaderConfig::kText : LoaderConfig::kTextDim);
        RECT t = card; t.left += 10;
        DrawTextA(hdc, label, -1, &t, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    inline void DrawFooter(HDC hdc)
    {
        SelectObject(hdc, gFont);
        SetBkMode(hdc, TRANSPARENT);
        const bool supportHot = gHover == HoverSupport;
        SetTextColor(hdc, supportHot ? LoaderConfig::kAccentLight : LoaderConfig::kTextMuted);
        TextOutA(hdc, LoaderConfig::kBrandPanelW + 16, LoaderConfig::kWindowH - 20,
            "support", 7);
        const char* ver = LoaderConfig::kDisplayVersion;
        SIZE vs{};
        GetTextExtentPoint32A(hdc, ver, (int)strlen(ver), &vs);
        SetTextColor(hdc, LoaderConfig::kTextMuted);
        TextOutA(hdc, LoaderConfig::kWindowW - vs.cx - 14, LoaderConfig::kWindowH - 20, ver, (int)strlen(ver));
    }

    inline void DrawDownloading(HDC hdc, const State* s)
    {
        const char* line = (s && s->status[0]) ? s->status : "Fetching build...";
        DrawSectionTitle(hdc, "Updating", line);
        DrawProgressBar(hdc, s, false);
    }

    inline void DrawReady(HDC hdc, const State* s)
    {
        if (s && s->filesReady) {
            DrawSectionTitle(hdc, "Ready", "Files are up to date.");
            DrawProgressBar(hdc, s, true);
            DrawButton(hdc, PrimaryButtonRect(), "Continue", true, gHover == HoverPrimary);
        } else {
            const char* line = (s && s->status[0]) ? s->status : "Download required files.";
            DrawSectionTitle(hdc, "Setup", line);
            DrawProgressBar(hdc, s, false);
            DrawButton(hdc, PrimaryButtonRect(), "Download", true, gHover == HoverPrimary);
        }
    }

    inline void DrawAuth(HDC hdc, const State* s)
    {
        if (!s) return;
        if (s->authenticated) {
            char sub[96];
            if (s->signedInAs[0])
                sprintf_s(sub, "Key %s", s->signedInAs);
            else
                strncpy_s(sub, "License active on this PC.", _TRUNCATE);
            DrawSectionTitle(hdc, "Activated", sub);
            DrawButton(hdc, AuthContinueRect(s), "Continue", true, gHover == HoverPrimary);
            DrawGhostButton(hdc, SignOutButtonRect(), "Clear key", gHover == HoverSignOut);
        } else {
            DrawSectionTitle(hdc, "Activate", "Paste your FRONTIER license key.");
            DrawInputChrome(hdc);
            if (s->authStatus[0]) {
                SelectObject(hdc, gFont);
                COLORREF tone = LoaderConfig::kTextMuted;
                if (s->authStatusTone == 1) tone = LoaderConfig::kAccentLight;
                else if (s->authStatusTone == 2) tone = RGB(255, 140, 150);
                else if (s->authStatusTone == 3) tone = LoaderConfig::kSuccess;
                SetTextColor(hdc, tone);
                DrawTextA(hdc, s->authStatus, -1, &AuthStatusRect(), DT_LEFT | DT_WORDBREAK);
            }
            const char* label = s->authBusy ? "Please wait..." : "Activate";
            DrawButton(hdc, PrimaryButtonRect(), label, !s->authBusy, gHover == HoverActivate);
            DrawGhostButton(hdc, GuestButtonRect(), "Browse free products", gHover == HoverGuest);
        }
    }

    inline void DrawLauncher(HDC hdc, const State* s)
    {
        if (!s) return;
        DrawSectionTitle(hdc, "Launch", "Pick mode and start.");
        FillRoundRect(hdc, GamePanelRect(), LoaderConfig::kPanel, LoaderConfig::kBorder, 8);
        FillRoundRect(hdc, InfoPanelRect(), LoaderConfig::kPanel, LoaderConfig::kBorder, 8);
        int idx = 0;
        if (ShowKernelMode(s))
            DrawModeCard(hdc, s, idx++, 1, "Kernel", gHover == HoverMode0);
        DrawModeCard(hdc, s, idx, 0, "Usermode", gHover == HoverMode1);
        RECT ip = InfoPanelRect();
        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextMuted);
        const char* mode = s->selectedMode == 1 ? "Kernel" : "Usermode";
        char buf[96];
        sprintf_s(buf, "Mode: %s  |  %s", mode, LoaderConfig::kDisplayVersion);
        TextOutA(hdc, ip.left + 12, ip.top + 14, buf, (int)strlen(buf));
        if (s->authenticated) {
            SetTextColor(hdc, s->licenseActive ? LoaderConfig::kSuccess : LoaderConfig::kWarning);
            const char* lic = s->licenseActive ? "License: active" : "License: not active";
            TextOutA(hdc, ip.left + 12, ip.top + 34, lic, (int)strlen(lic));
        }
        DrawButton(hdc, LaunchButtonRect(), "Start", true, gHover == HoverPrimary);
    }

    inline void Paint(HWND hwnd, HDC hdc, State* s)
    {
        if (!hwnd || !hdc || !s) return;
        RECT client{};
        GetClientRect(hwnd, &client);
        const int w = client.right - client.left;
        const int h = client.bottom - client.top;
        if (w <= 0 || h <= 0) return;

        HDC mem = CreateCompatibleDC(hdc);
        if (!mem) return;
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        if (!bmp) { DeleteDC(mem); return; }
        HGDIOBJ old = SelectObject(mem, bmp);

        SetBkMode(mem, TRANSPARENT);
        DrawBackground(hwnd, mem);
        DrawBrandPanel(mem);
        DrawWindowControls(mem);
        switch (s->screen) {
        case ScreenDownloading: DrawDownloading(mem, s); break;
        case ScreenReady: DrawReady(mem, s); break;
        case ScreenAuth: DrawAuth(mem, s); break;
        case ScreenProducts: DrawProducts(mem, s); break;
        case ScreenLauncher: DrawLauncher(mem, s); break;
        }
        DrawFooter(mem);
        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);

        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
    }

    // ---- logic ----

    inline void GoReady(State* s, bool ready)
    {
        if (!s) return;
        TransitionToScreen(s, ScreenReady);
        s->filesReady = ready;
        s->updating = false;
        s->checking = false;
        s->indeterminate = false;
        s->progress = ready ? 1.f : 0.f;
        s->selectedMode = 0;
        if (ready)
            strncpy_s(s->status, "All files are up to date.", _TRUNCATE);
        RequestRedraw();
    }

    inline void ShowMessage(const wchar_t* text, UINT type = MB_OK | MB_ICONINFORMATION)
    {
        if (!text) return;
        auto* p = new ShowMsgPayload{};
        wcsncpy_s(p->text, text, _TRUNCATE);
        p->type = type;
        if (!PostToUi(WM_SHOW_MSG, 0, (LPARAM)p))
            delete p;
    }

    inline void RunTraceProduct(State* s)
    {
        (void)s;
        std::wstring msg;
        LoaderProducts::RunTraceLaunch(msg);
        ShowMessage(msg.c_str());
    }

    inline void HandleCheckDone(State* s, CheckDoneMsg* msg)
    {
        if (!s || !msg) return;
        s->manifest = msg->manifest;
        s->localVersion = msg->localVersion;
        s->remoteVersion = msg->remoteVersion;
        strncpy_s(s->remoteDisplay, msg->remoteDisplay, _TRUNCATE);
        s->updateAvailable = msg->needsUpdate;
        if (msg->kernelAvailable) s->kernelAvailable = true;
        s->selectedMode = 0;
        s->checking = false;
        if (msg->needsUpdate) RunUpdate(s);
        else {
            GoReady(s, msg->hasLocal);
            SetStatus(s, msg->status);
        }
    }

    inline void HandleUpdateTick(State* s, UpdateTickMsg* tick)
    {
        if (!s || !tick) return;
        SetProgress(s, tick->progress, false);
        SetStatus(s, tick->status);
    }

    inline void HandleUpdateDone(State* s, UpdateDoneMsg* done)
    {
        if (!s || !done) return;
        s->manifest = done->manifest;
        if (done->ok) {
            s->localVersion = done->manifest.version;
            s->updateAvailable = false;
            s->kernelAvailable = LoaderUpdate::ProbeKernelAvailable() || done->manifest.kernelAvailable;
            s->updating = false;
            const int launchMode = s->selectedMode;
            if (!done->launchAfter)
                s->selectedMode = 0;
            if (done->launchAfter) {
                s->launchAfterUpdate = false;
                s->selectedMode = launchMode;
                const bool ready = launchMode == 1
                    ? LoaderUpdate::ProbeKernelAvailable()
                    : LoaderUpdate::LocalUsermodeExists();
                if (ready && !LoaderUpdate::NeedsUpdate(s->manifest))
                    RunLaunch(s);
                else
                    GoToLauncher(s);
            } else {
                GoReady(s, LoaderUpdate::LocalUsermodeExists() && !LoaderUpdate::NeedsUpdate(s->manifest));
                if (done->status[0]) SetStatus(s, done->status);
            }
            if (done->relaunch) PostToUi(WM_RELAUNCH_LOADER, 0, 0);
        } else {
            s->launchAfterUpdate = false;
            s->updating = false;
            GoReady(s, LoaderUpdate::LocalUsermodeExists());
            if (done->status[0]) SetStatus(s, done->status);
        }
    }

    inline void CheckAsync(State* s)
    {
        if (!s || s->checking) return;
        s->checking = true;
        TransitionToScreen(s, ScreenDownloading);
        SetProgress(s, 0.35f, true);
        SetStatus(s, "Checking for updates...");
        LoaderUpdate::ApplyPendingUpdates();
        if (LoaderUpdate::LocalUsermodeExists()) {
            const int saved = LoaderUpdate::LoadLocalVersion();
            if (saved < LoaderConfig::kLocalVersion)
                LoaderUpdate::SaveInstallRecord(LoaderConfig::kLocalVersion, LoaderConfig::kDisplayVersion);
        }
        std::thread([]() {
            auto* msg = new CheckDoneMsg{};
            LoaderUpdate::Manifest m{};
            std::string err;
            msg->fetchOk = LoaderUpdate::FetchManifest(m, err);
            LoaderUpdate::ApplyFallbackManifest(m);
            msg->manifest = m;
            msg->localVersion = LoaderUpdate::LoadLocalVersion();
            msg->remoteVersion = m.version;
            strncpy_s(msg->remoteDisplay, m.display, _TRUNCATE);
            msg->needsUpdate = LoaderUpdate::NeedsUpdate(m) && m.usermodeUrl[0] != 0;
            msg->hasLocal = LoaderUpdate::LocalUsermodeExists();
            msg->kernelAvailable = m.kernelAvailable || LoaderUpdate::ProbeKernelAvailable();
            if (!msg->hasLocal)
                strncpy_s(msg->status, msg->fetchOk ? "Download required — click Download." : "Offline - check connection.", _TRUNCATE);
            else if (!msg->fetchOk)
                strncpy_s(msg->status, "Offline mode - using local files", _TRUNCATE);
            else
                strncpy_s(msg->status, "All files are up to date.", _TRUNCATE);
            if (!PostToUi(WM_CHECK_DONE, 0, (LPARAM)msg))
                delete msg;
        }).detach();
    }

    inline void RunUpdate(State* s)
    {
        if (!s || s->updating) return;
        if (!s->manifest.usermodeUrl[0]) {
            ShellExecuteA(nullptr, "open", LoaderConfig::kReleasesPage, nullptr, nullptr, SW_SHOWNORMAL);
            GoReady(s, LocalUsermodeExists());
            SetStatus(s, "Opened GitHub releases");
            return;
        }
        s->updating = true;
        TransitionToScreen(s, ScreenDownloading);
        SetProgress(s, 0.f, false);
        SetStatus(s, "Downloading...");
        const LoaderUpdate::Manifest m = s->manifest;
        const bool launchAfter = s->launchAfterUpdate;
        std::thread([m, launchAfter]() {
            std::string err;
            const bool ok = LoaderUpdate::ApplyUpdate(m,
                [](float p, const char* statusMsg) {
                    auto* tick = new UpdateTickMsg{};
                    tick->progress = p;
                    strncpy_s(tick->status, statusMsg, _TRUNCATE);
                    if (!PostToUi(WM_UPDATE_TICK, 0, (LPARAM)tick))
                        delete tick;
                }, err);
            auto* done = new UpdateDoneMsg{};
            done->manifest = m;
            done->ok = ok;
            done->launchAfter = launchAfter;
            done->relaunch = ok && LoaderUpdate::PrepareLoaderRelaunch();
            if (ok) {
                if (!launchAfter)
                    strncpy_s(done->status, "All files are up to date.", _TRUNCATE);
            } else {
                char buf[256];
                sprintf_s(buf, "Update failed: %s", err.c_str());
                strncpy_s(done->status, buf, _TRUNCATE);
            }
            if (!PostToUi(WM_UPDATE_DONE, 0, (LPARAM)done))
                delete done;
        }).detach();
    }

    inline void GoToProducts(State* s)
    {
        if (!s) return;
        TransitionToScreen(s, ScreenProducts);
        gHover = HoverNone;
        RequestRedraw();
    }

    inline void GoToActivateKey(State* s, const char* msg, int tone = 2)
    {
        if (!s) return;
        LoaderLicense::ClearLicense();
        LoaderOAuth::ClearSession();
        s->licenseToken[0] = 0;
        s->signedInAs[0] = 0;
        s->authenticated = false;
        s->licenseActive = false;
        s->authBusy = false;
        if (gKeyEdit && IsWindow(gKeyEdit))
            SetWindowTextA(gKeyEdit, "");
        SetAuthStatus(s, msg && msg[0] ? msg : "Paste your license key from ahead.best.", tone);
        TransitionToScreen(s, ScreenAuth);
    }

    inline void GoToLauncher(State* s)
    {
        if (!s) return;
        s->selectedMode = 0;
        LoaderUpdate::SaveMode(0);
        TransitionToScreen(s, ScreenLauncher);
        gHover = HoverNone;
        RequestRedraw();
    }

    inline void RunLaunch(State* s)
    {
        if (!s) return;
        // Open-source: no license gate.
        if (!s->licenseToken[0])
            strncpy_s(s->licenseToken, LoaderLicense::kOpenToken, _TRUNCATE);
        if (!s->licenseHwid[0])
            strncpy_s(s->licenseHwid, "opensource", _TRUNCATE);
        s->authenticated = true;
        s->licenseActive = true;
        if (s->selectedMode == 1 && LoaderUpdate::NeedsKernelBundle(s->manifest)) {
            s->launchAfterUpdate = true;
            RunUpdate(s);
            return;
        }
        if (s->selectedMode == 1 && !LoaderUpdate::ProbeKernelAvailable()) {
            ShowMessage(L"Kernel mode is not installed on this PC.\nRun Update or use Usermode.",
                MB_OK | MB_ICONINFORMATION);
            return;
        }
        std::wstring work, exe;
        if (s->selectedMode == 0) {
            if (LoaderUpdate::NeedsUpdate(s->manifest) && s->manifest.usermodeUrl[0]) {
                s->launchAfterUpdate = true;
                RunUpdate(s);
                return;
            }
            if (!LoaderUpdate::ResolveUsermodeExe(exe, work)) {
                ShowMessage(L"Missing usermode\\Frontier.exe.", MB_OK | MB_ICONWARNING);
                return;
            }
        } else {
            std::wstring dir = LoaderUpdate::GetLoaderDir();
            work = LoaderUpdate::PathJoin(dir, L"kernel");
            exe = LoaderUpdate::PathJoin(work, LoaderConfig::kKernelExe);
        }
        if (!LoaderLicense::WriteSessionFile(work, s->licenseToken, s->licenseHwid)) {
            ShowMessage(L"Could not write session file.", MB_OK | MB_ICONWARNING);
            return;
        }
        std::wstring err;
        if (!LoaderUpdate::RunPayload(s->selectedMode, err))
            ShowMessage(err.c_str(), MB_OK | MB_ICONWARNING);
        else if (gWnd && IsWindow(gWnd))
            DestroyWindow(gWnd);
    }

    inline void RefreshAuthUi(State* s)
    {
        if (!s) return;
        s->authenticated = s->licenseToken[0] != 0;
        SyncAuthFields(s);
        RequestRedraw();
    }

    inline void FinishAuthUi(State* s, AuthDonePayload* p)
    {
        if (!s || !p) return;
        if (p->ok) {
            strncpy_s(s->licenseToken, p->token, _TRUNCATE);
            s->licenseActive = p->licensed;
            SetAuthStatus(s, p->status, p->licensed ? 3 : 0);
            if (p->signedInAs[0])
                strncpy_s(s->signedInAs, p->signedInAs, _TRUNCATE);
            s->authenticated = true;
        } else {
            SetAuthStatus(s, p->status, 2);
            s->authenticated = false;
            s->licenseActive = false;
        }
        s->authBusy = false;
        RefreshAuthUi(s);
        if (p->ok && s->screen == ScreenAuth) {
            s->selectedMode = 0;
            LoaderUpdate::SaveMode(0);
            GoToProducts(s);
        }
    }

    inline void MaskKeyDisplay(const char* key, char* out, size_t outSz)
    {
        if (!out || outSz == 0) return;
        out[0] = 0;
        if (!key || !key[0]) return;
        const size_t len = strlen(key);
        if (len <= 8) {
            strncpy_s(out, outSz, key, _TRUNCATE);
            return;
        }
        char head[6]{}, tail[6]{};
        strncpy_s(head, key, 4);
        strncpy_s(tail, key + len - 4, 4);
        sprintf_s(out, outSz, "%s...%s", head, tail);
    }

    inline void MaskEmailDisplay(const char* email, char* out, size_t outSz)
    {
        if (!out || outSz == 0) return;
        out[0] = 0;
        if (!email || !email[0]) return;
        const char* at = strchr(email, '@');
        if (!at) {
            strncpy_s(out, outSz, email, _TRUNCATE);
            return;
        }
        const size_t headLen = (size_t)(at - email);
        if (headLen <= 2) {
            strncpy_s(out, outSz, email, _TRUNCATE);
            return;
        }
        char head[8]{};
        strncpy_s(head, email, headLen > 2 ? 2 : headLen);
        sprintf_s(out, outSz, "%s***%s", head, at);
    }

    inline bool IsHardLicenseError(const std::string& err)
    {
        if (err.empty()) return false;
        const char* needles[] = {
            "revoked", "invalid_token", "invalid_key", "invalid_email", "hwid_mismatch",
            "another PC", "mismatch", "not allowed", "invalid_password",
            "password_required", "key_revoked"
        };
        for (const char* n : needles) {
            if (err.find(n) != std::string::npos)
                return true;
        }
        return false;
    }

    inline void RunActivateKey(State* s)
    {
        if (!s || s->authBusy) return;
        if (s->authenticated) { GoToProducts(s); return; }
        if (!gKeyEdit || !IsWindow(gKeyEdit)) return;
        char raw[128]{}, key[128]{};
        GetWindowTextA(gKeyEdit, raw, sizeof(raw));
        LoaderLicense::NormalizeKeyInput(raw, key, sizeof(key));
        if (!key[0]) {
            SetAuthStatus(s, "Enter your license key.", 2);
            RefreshAuthUi(s);
            return;
        }
        SetWindowTextA(gKeyEdit, key);
        s->authBusy = true;
        SetAuthStatus(s, "Activating...", 1);
        RefreshAuthUi(s);
        const std::string hw = s->licenseHwid[0] ? s->licenseHwid : LoaderLicense::GetHwid();
        strncpy_s(s->licenseHwid, hw.c_str(), _TRUNCATE);
        const std::string keyCopy = key;
        std::thread([keyCopy, hw]() {
            auto* payload = new AuthDonePayload{};
            char token[768]{}, msg[256]{};
            std::string err;
            bool licensed = false;
            const bool ok = LoaderLicense::ActivateKey(keyCopy.c_str(), "", hw.c_str(),
                token, sizeof(token), msg, sizeof(msg), &licensed, err);
            payload->ok = ok;
            payload->licensed = licensed;
            if (ok) {
                strncpy_s(payload->token, token, _TRUNCATE);
                strncpy_s(payload->status, msg[0] ? msg : "License activated.", _TRUNCATE);
                MaskKeyDisplay(keyCopy.c_str(), payload->signedInAs, sizeof(payload->signedInAs));
            } else {
                strncpy_s(payload->status, err.c_str(), _TRUNCATE);
            }
            if (!PostToUi(WM_AUTH_DONE, 0, (LPARAM)payload))
                delete payload;
        }).detach();
    }

    inline void SignOutLicense(State* s)
    {
        if (!s) return;
        LoaderLicense::ClearLicense();
        LoaderOAuth::ClearSession();
        s->licenseToken[0] = 0;
        s->signedInAs[0] = 0;
        s->authenticated = false;
        s->licenseActive = false;
        SetAuthStatus(s, "Key cleared.", 0);
        RefreshAuthUi(s);
    }

    inline void BootAsync(State* s)
    {
        (void)s;
        std::thread([]() {
            auto* msg = new BootDoneMsg{};
            strncpy_s(msg->licenseToken, LoaderLicense::kOpenToken, _TRUNCATE);
            strncpy_s(msg->licenseHwid, "opensource", _TRUNCATE);
            strncpy_s(msg->signedInAs, "open source", _TRUNCATE);
            strncpy_s(msg->authStatus, "Open source — no key required.", _TRUNCATE);
            msg->authenticated = true;
            msg->licenseActive = true;
            if (!PostToUi(WM_BOOT_DONE, 0, (LPARAM)msg))
                delete msg;
        }).detach();
    }

    inline int HitTestHover(const State* s, POINT pt)
    {
        if (!s) return HoverNone;
        if (PtInRect(&CloseButtonRect(), pt)) return HoverClose;
        if (PtInRect(&MinButtonRect(), pt)) return HoverMin;
        if (PtInRect(&SupportLinkRect(), pt)) return HoverSupport;
        if (s->updating) return HoverNone;
        if (s->screen == ScreenReady && PtInRect(&PrimaryButtonRect(), pt)) return HoverPrimary;
        if (s->screen == ScreenAuth) {
            if (s->authenticated && PtInRect(&AuthContinueRect(s), pt)) return HoverPrimary;
            if (!s->authenticated && PtInRect(&PrimaryButtonRect(), pt)) return HoverActivate;
            if (s->authenticated && PtInRect(&SignOutButtonRect(), pt)) return HoverSignOut;
            if (!s->authenticated && PtInRect(&GuestButtonRect(), pt)) return HoverGuest;
        }
        if (s->screen == ScreenProducts) {
            if (PtInRect(&TraceProductRect(), pt)) return HoverProductTrace;
            if (PtInRect(&FrontierProductRect(), pt)) return HoverProductFrontier;
            if (ShowChangeKeyButton(s) && PtInRect(&ChangeKeyButtonRect(), pt)) return HoverChangeKey;
        }
        if (s->screen == ScreenLauncher) {
            if (PtInRect(&LaunchButtonRect(), pt)) return HoverPrimary;
            if (ShowKernelMode(s) && PtInRect(&ModeCardRect(s, 0), pt)) return HoverMode0;
            const int u = ShowKernelMode(s) ? 1 : 0;
            if (PtInRect(&ModeCardRect(s, u), pt)) return HoverMode1;
        }
        return HoverNone;
    }

    inline void HandleProductsClick(State* s, POINT pt)
    {
        if (!s) return;
        if (PtInRect(&TraceProductRect(), pt)) {
            s->selectedProduct = LoaderProducts::ProductTrace;
            PostToUi(WM_RUN_TRACE, 0, 0);
            return;
        }
        if (PtInRect(&FrontierProductRect(), pt)) {
            if (LoaderConfig::kFrontierMaintenance) {
                ShowMessage(L"FRONTIER is temporarily down.\nETA ~2 weeks.", MB_OK | MB_ICONINFORMATION);
                return;
            }
            s->authenticated = true;
            s->licenseActive = true;
            if (!s->licenseToken[0])
                strncpy_s(s->licenseToken, LoaderLicense::kOpenToken, _TRUNCATE);
            s->selectedProduct = LoaderProducts::ProductFrontier;
            GoToLauncher(s);
        }
    }

    inline void HandleLauncherClick(State* s, POINT pt)
    {
        if (!s) return;
        if (ShowKernelMode(s) && PtInRect(&ModeCardRect(s, 0), pt)) {
            s->selectedMode = 1;
            LoaderUpdate::SaveMode(1);
            RequestRedraw();
            return;
        }
        const int u = ShowKernelMode(s) ? 1 : 0;
        if (PtInRect(&ModeCardRect(s, u), pt)) {
            s->selectedMode = 0;
            LoaderUpdate::SaveMode(0);
            RequestRedraw();
            return;
        }
        if (PtInRect(&LaunchButtonRect(), pt))
            RunLaunch(s);
    }

    inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        State* s = gState;
        switch (msg) {
        case WM_CREATE: {
            HINSTANCE inst = ((LPCREATESTRUCTW)lp)->hInstance;
            gKeyEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL,
                0, 0, 100, 24, hwnd, (HMENU)1001, inst, nullptr);
            gPassEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
                0, 0, 100, 24, hwnd, (HMENU)1002, inst, nullptr);
            if (gKeyEdit && gFont) SendMessageW(gKeyEdit, WM_SETFONT, (WPARAM)gFont, TRUE);
            if (gPassEdit && gFont) SendMessageW(gPassEdit, WM_SETFONT, (WPARAM)gFont, TRUE);
            SetKeyEditVisible(false);
            SetPassEditVisible(false);
            BootAsync(s);
            CheckAsync(s);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProcW(hwnd, msg, wp, lp);
            if (hit == HTCLIENT) {
                POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                ScreenToClient(hwnd, &pt);
                if (HeaderDragArea(pt)) return HTCAPTION;
            }
            return hit;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            Paint(hwnd, hdc, s);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (!s) return 0;
            POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            const int next = HitTestHover(s, pt);
            if (next != gHover) {
                gHover = next;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (!gTrackingMouse) {
                TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                gTrackingMouse = true;
            }
            return 0;
        }
        case WM_MOUSELEAVE:
            gTrackingMouse = false;
            if (gHover != HoverNone) {
                gHover = HoverNone;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONDOWN: {
            if (!s) return 0;
            POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            if (PtInRect(&CloseButtonRect(), pt)) { DestroyWindow(hwnd); return 0; }
            if (PtInRect(&MinButtonRect(), pt)) { ShowWindow(hwnd, SW_MINIMIZE); return 0; }
            if (PtInRect(&SupportLinkRect(), pt)) {
                ShellExecuteA(nullptr, "open", LoaderConfig::kDiscordInvite, nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }
            if (s->updating) return 0;
            if (s->screen == ScreenReady && PtInRect(&PrimaryButtonRect(), pt)) {
                if (s->filesReady) {
                    s->selectedMode = 0;
                    GoToProducts(s);
                } else {
                    LoaderUpdate::ApplyFallbackManifest(s->manifest);
                    RunUpdate(s);
                }
                return 0;
            }
            if (s->screen == ScreenAuth) {
                if (s->authenticated && PtInRect(&SignOutButtonRect(), pt)) { SignOutLicense(s); return 0; }
                if (s->authenticated && PtInRect(&AuthContinueRect(s), pt)) { GoToProducts(s); return 0; }
                if (!s->authenticated && PtInRect(&GuestButtonRect(), pt)) { GoToProducts(s); return 0; }
                if (!s->authenticated && PtInRect(&PrimaryButtonRect(), pt)) { RunActivateKey(s); return 0; }
                return 0;
            }
            if (s->screen == ScreenProducts) {
                if (ShowChangeKeyButton(s) && PtInRect(&ChangeKeyButtonRect(), pt)) {
                    GoToActivateKey(s, "Paste your license key from ahead.best.");
                    return 0;
                }
                HandleProductsClick(s, pt);
                return 0;
            }
            if (s->screen == ScreenLauncher) HandleLauncherClick(s, pt);
            return 0;
        }
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT && s && !s->updating) {
                SetCursor(LoadCursor(nullptr, gHover != HoverNone ? IDC_HAND : IDC_ARROW));
                return TRUE;
            }
            break;
        case WM_CTLCOLOREDIT:
            if (gEditBrush) {
                SetTextColor((HDC)wp, LoaderConfig::kText);
                SetBkColor((HDC)wp, LoaderConfig::kInputBg);
                return (INT_PTR)gEditBrush;
            }
            break;
        case WM_COMMAND:
            if ((LOWORD(wp) == 1001 || LOWORD(wp) == 1002) && HIWORD(wp) == EN_CHANGE)
                InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case WM_KEYDOWN:
            if (wp == VK_RETURN && s && s->screen == ScreenAuth && GetFocus() == gKeyEdit) {
                RunActivateKey(s);
                return 0;
            }
            break;
        case WM_BOOT_DONE: {
            auto* boot = (BootDoneMsg*)lp;
            if (s && boot) {
                if (boot->licenseHwid[0])
                    strncpy_s(s->licenseHwid, boot->licenseHwid, _TRUNCATE);
                if (boot->licenseToken[0])
                    strncpy_s(s->licenseToken, boot->licenseToken, _TRUNCATE);
                s->authenticated = boot->authenticated;
                s->licenseActive = boot->licenseActive;
                if (boot->authStatus[0]) {
                    strncpy_s(s->authStatus, boot->authStatus, _TRUNCATE);
                    s->authStatusTone = boot->licenseActive ? 3 : 0;
                }
                if (boot->signedInAs[0]) strncpy_s(s->signedInAs, boot->signedInAs, _TRUNCATE);
                RefreshAuthUi(s);
            }
            delete boot;
            return 0;
        }
        case WM_AUTH_DONE: {
            auto* p = (AuthDonePayload*)lp;
            FinishAuthUi(s, p);
            delete p;
            return 0;
        }
        case WM_CHECK_DONE: {
            auto* p = (CheckDoneMsg*)lp;
            HandleCheckDone(s, p);
            delete p;
            return 0;
        }
        case WM_UPDATE_TICK: {
            auto* p = (UpdateTickMsg*)lp;
            HandleUpdateTick(s, p);
            delete p;
            return 0;
        }
        case WM_UPDATE_DONE: {
            auto* p = (UpdateDoneMsg*)lp;
            HandleUpdateDone(s, p);
            delete p;
            return 0;
        }
        case WM_SHOW_MSG: {
            auto* p = (ShowMsgPayload*)lp;
            if (p) {
                MessageBoxW(hwnd, p->text, LoaderConfig::kAppName, p->type);
                delete p;
            }
            return 0;
        }
        case WM_RUN_TRACE:
            if (s) RunTraceProduct(s);
            return 0;
        case WM_RELAUNCH_LOADER:
            if (LoaderUpdate::ScheduleLoaderRelaunch())
                PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            gUiAlive.store(false, std::memory_order_release);
            gWnd = nullptr;
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    inline void ApplyWindowChrome(HWND hwnd)
    {
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        int round = DWMWCP_ROUND;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &round, sizeof(round));
        COLORREF border = RGB(0, 0, 0);
        DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border, sizeof(border));
    }

    inline int Run()
    {
        INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&icc);

        State state{};
        state.kernelAvailable = LoaderUpdate::ProbeKernelAvailable();
        LoaderUpdate::ApplyPendingUpdates();
        gState = &state;

        gBgBrush = CreateSolidBrush(LoaderConfig::kBg);
        gEditBrush = CreateSolidBrush(LoaderConfig::kInputBg);
        gFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        gFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        gFontLogo = CreateFontW(-20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");

        HINSTANCE inst = GetModuleHandleW(nullptr);
        gAppIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        gAppIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 20, 20, LR_DEFAULTCOLOR);

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = inst;
        wc.hIcon = gAppIcon ? gAppIcon : LoadIconW(inst, MAKEINTRESOURCEW(IDI_ICON1));
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = gBgBrush;
        wc.lpszClassName = L"FrontierLoaderWnd2";
        RegisterClassExW(&wc);

        const int w = LoaderConfig::kWindowW;
        const int h = LoaderConfig::kWindowH;
        const int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        const int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

        wchar_t title[64];
        swprintf_s(title, L"FRONTIER %hs", LoaderConfig::kDisplayVersion);
        gWnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, title,
            WS_POPUP | WS_CLIPCHILDREN,
            x, y, w, h, nullptr, nullptr, inst, nullptr);
        if (!gWnd) return 1;

        gUiAlive.store(true, std::memory_order_release);
        ApplyWindowChrome(gWnd);
        ShowWindow(gWnd, SW_SHOW);
        UpdateWindow(gWnd);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (gAppIcon) DestroyIcon(gAppIcon);
        if (gAppIconSm) DestroyIcon(gAppIconSm);
        if (gFont) DeleteObject(gFont);
        if (gFontBold) DeleteObject(gFontBold);
        if (gFontLogo) DeleteObject(gFontLogo);
        if (gEditBrush) DeleteObject(gEditBrush);
        if (gBgBrush) DeleteObject(gBgBrush);
        LoaderAssets::ShutdownGdiplus();
        gState = nullptr;
        return (int)msg.wParam;
    }

} // namespace LoaderUI
