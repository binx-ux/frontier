#pragma once
#include <Windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <CommCtrl.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <thread>
#include "loader_config.h"
#include "loader_update.h"
#include "loader_oauth.h"
#include "loader_license.h"
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
        float progressAnim = 0.f;
        float animTime = 0.f;
        bool progressActive = false;
        bool indeterminate = false;
        bool filesReady = false;
        bool authenticated = false;
        bool authBusy = false;
        char licenseToken[768] = "";
        char licenseHwid[64] = "";
        char authStatus[256] = "Enter your lifetime license key to activate.";
        char signedInAs[64] = "";
        LoaderUpdate::Manifest manifest{};
    };

    inline bool ShowKernelMode(const State* s)
    {
        return LoaderConfig::kKernelModeOffered || (s && s->kernelAvailable);
    }

    inline State* gState = nullptr;
    inline HWND gWnd = nullptr;
    inline HFONT gFont = nullptr;
    inline HFONT gFontBold = nullptr;
    inline HFONT gFontBrand = nullptr;
    inline HFONT gFontHero = nullptr;
    inline HFONT gFontLogo = nullptr;
    inline HBRUSH gBgBrush = nullptr;
    inline HICON gAppIcon = nullptr;
    inline HICON gAppIconSm = nullptr;
    inline int gHover = HoverNone;
    inline bool gTrackingMouse = false;
    inline HWND gKeyEdit = nullptr;
    inline HBRUSH gEditBrush = nullptr;
    inline const UINT_PTR kAnimTimerId = 1;
    inline const UINT WM_AUTH_DONE = WM_USER + 10;

    struct AuthDonePayload {
        bool ok;
        char status[256];
        char token[768];
    };

    inline void SetProgressTarget(State* s, float target)
    {
        if (target < 0.f) target = 0.f;
        if (target > 1.f) target = 1.f;
        s->progress = target;
        s->indeterminate = false;
        s->progressActive = true;
    }

    inline void SetIndeterminateProgress(State* s, bool on)
    {
        s->indeterminate = on;
        s->progressActive = on;
        if (on)
            s->progressAnim = 0.08f;
    }

    inline void TickAnimation(State* s, float dt)
    {
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;
        s->animTime += dt;

        if (s->indeterminate) {
            const float wave = 0.5f + 0.5f * sinf(s->animTime * 3.2f);
            s->progressAnim = 0.06f + wave * 0.38f;
            return;
        }

        const float diff = s->progress - s->progressAnim;
        if (fabsf(diff) < 0.001f) {
            s->progressAnim = s->progress;
            return;
        }
        s->progressAnim += diff * (1.f - expf(-dt * 10.f));
    }

    inline bool NeedsAnimation(const State* s)
    {
        if (!s) return false;
        if (s->indeterminate || s->progressActive) return true;
        if (s->screen == ScreenDownloading) return true;
        if (s->screen == ScreenReady && s->filesReady)
            return fabsf(s->progressAnim - 1.f) > 0.002f;
        return false;
    }

    inline void StartAnimTimer(HWND hwnd)
    {
        SetTimer(hwnd, kAnimTimerId, 16, nullptr);
    }

    inline void StopAnimTimer(HWND hwnd)
    {
        KillTimer(hwnd, kAnimTimerId);
    }

    inline void SetStatus(State* s, const char* msg)
    {
        strncpy_s(s->status, msg, _TRUNCATE);
        if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
    }

    inline RECT HeaderRect()
    {
        return RECT{ 0, 0, LoaderConfig::kWindowW, 44 };
    }

    inline RECT CloseButtonRect()
    {
        return RECT{ LoaderConfig::kWindowW - 36, 14, LoaderConfig::kWindowW - 16, 34 };
    }

    inline RECT MinButtonRect()
    {
        return RECT{ LoaderConfig::kWindowW - 66, 14, LoaderConfig::kWindowW - 46, 34 };
    }

    inline RECT SupportLinkRect()
    {
        return RECT{ 22, LoaderConfig::kWindowH - 34, 140, LoaderConfig::kWindowH - 12 };
    }

    inline RECT RightContentRect()
    {
        return RECT{ LoaderConfig::kSplitX + 36, 52, LoaderConfig::kWindowW - 36, LoaderConfig::kWindowH - 48 };
    }

    inline RECT ContentPanelRect()
    {
        RECT r = RightContentRect();
        r.top += 48;
        r.bottom = r.top + 120;
        return r;
    }

    inline RECT ProgressBarRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.top + 120, r.right, r.top + 132 };
    }

    inline RECT SecondaryButtonRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.bottom - 52, r.right, r.bottom - 8 };
    }

    inline RECT KeyFieldRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.top + 88, r.right, r.top + 128 };
    }

    inline RECT AuthStatusRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.top + 138, r.right, r.top + 168 };
    }

    inline RECT DiscordButtonRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.top + 176, r.right, r.top + 214 };
    }

    inline RECT PrimaryButtonRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.top + 140, r.right, r.top + 180 };
    }

    inline RECT ActivateButtonRect()
    {
        return PrimaryButtonRect();
    }

    inline RECT LaunchButtonRect()
    {
        return SecondaryButtonRect();
    }

    inline RECT AuthContinueRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.top + 176, r.right, r.top + 220 };
    }

    inline RECT SignOutButtonRect()
    {
        RECT r = RightContentRect();
        return RECT{ r.left, r.top + 228, r.right, r.top + 266 };
    }

    inline int ModeCardTop(State* s, int displayIndex)
    {
        (void)s;
        RECT r = RightContentRect();
        return r.top + 68 + displayIndex * 62;
    }

    inline RECT ModeCardRect(State* s, int displayIndex)
    {
        RECT r = RightContentRect();
        const int top = ModeCardTop(s, displayIndex);
        return RECT{ r.left, top, r.right, top + 54 };
    }

    inline bool HeaderDragArea(POINT pt)
    {
        if (pt.y < 0 || pt.y > 48) return false;
        if (PtInRect(&CloseButtonRect(), pt)) return false;
        if (PtInRect(&MinButtonRect(), pt)) return false;
        if (PtInRect(&SupportLinkRect(), pt)) return false;
        if (PtInRect(&SignOutButtonRect(), pt)) return false;
        return true;
    }

    inline void RepositionKeyEdit()
    {
        if (!gKeyEdit) return;
        RECT field = KeyFieldRect();
        const int padL = 36;
        const int padV = 8;
        SetWindowPos(gKeyEdit, nullptr,
            field.left + padL, field.top + padV,
            field.right - field.left - padL - 8,
            field.bottom - field.top - padV * 2,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    inline void SetKeyEditVisible(bool visible)
    {
        if (!gKeyEdit) return;
        RepositionKeyEdit();
        ShowWindow(gKeyEdit, visible ? SW_SHOW : SW_HIDE);
        EnableWindow(gKeyEdit, visible && !(gState && gState->authBusy));
    }

    inline void RefreshAuthUi(State* s)
    {
        s->authenticated = s->licenseToken[0] != 0;
        SetKeyEditVisible(s->screen == ScreenAuth);
        if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
    }

    inline void TryRestoreLicense(State* s)
    {
        LoaderLicense::LoadSaved(s->licenseToken, sizeof(s->licenseToken), s->licenseHwid, sizeof(s->licenseHwid));
        if (!s->licenseHwid[0]) {
            std::string hw = LoaderLicense::GetHwid();
            strncpy_s(s->licenseHwid, hw.c_str(), _TRUNCATE);
        }
        if (s->licenseToken[0]) {
            std::string err;
            if (LoaderLicense::ValidateTokenRemote(s->licenseToken, s->licenseHwid, err)) {
                s->authenticated = true;
                strncpy_s(s->authStatus, "License active on this PC.", _TRUNCATE);
            } else {
                // Keep saved license on network/API blips — only clear on explicit revocation
                const bool revoked = err.find("revoked") != std::string::npos
                    || err.find("invalid") != std::string::npos
                    || err.find("not allowed") != std::string::npos;
                if (revoked) {
                    LoaderLicense::ClearLicense();
                    s->licenseToken[0] = 0;
                } else if (s->licenseToken[0]) {
                    s->authenticated = true;
                    strncpy_s(s->authStatus, "Offline mode — saved license kept.", _TRUNCATE);
                } else {
                    strncpy_s(s->authStatus, err.c_str(), _TRUNCATE);
                }
            }
        }
    }

    inline bool LocalUsermodeExists()
    {
        return LoaderUpdate::LocalUsermodeExists() || [&]() {
            std::wstring exe, work;
            return LoaderUpdate::ResolveUsermodeExe(exe, work);
        }();
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

    inline void FillRoundRectSolid(HDC hdc, const RECT& rc, COLORREF fill, int radius = 10)
    {
        FillRoundRect(hdc, rc, fill, fill, radius);
    }

    inline void DrawCenteredTextA(HDC hdc, HFONT font, COLORREF color, int top, int height, const char* text)
    {
        RECT client{};
        GetClientRect(gWnd, &client);
        SelectObject(hdc, font);
        SetTextColor(hdc, color);
        RECT rc{ client.left + 36, top, client.right - 36, top + height };
        DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_WORDBREAK | DT_VCENTER);
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

    inline void DrawSoftGlow(HDC hdc, int cx, int cy, int radius, COLORREF core, COLORREF edge, int steps = 16)
    {
        HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);
        HGDIOBJ oldPen = SelectObject(hdc, nullPen);
        for (int step = 0; step < steps; ++step) {
            const float t = (float)step / (float)(steps - 1);
            const int r = radius - (int)(t * radius * 0.95f);
            if (r <= 1) break;
            COLORREF c = BlendRgb(core, edge, t);
            HBRUSH brush = CreateSolidBrush(c);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
            SelectObject(hdc, oldBrush);
            DeleteObject(brush);
        }
        SelectObject(hdc, oldPen);
    }

    inline void FillVerticalGradient(HDC hdc, const RECT& rc, COLORREF top, COLORREF bottom)
    {
        const int h = rc.bottom - rc.top;
        if (h <= 0) return;
        TRIVERTEX vert[2]{};
        vert[0].x = rc.left;
        vert[0].y = rc.top;
        vert[0].Red = (COLOR16)(GetRValue(top) << 8);
        vert[0].Green = (COLOR16)(GetGValue(top) << 8);
        vert[0].Blue = (COLOR16)(GetBValue(top) << 8);
        vert[0].Alpha = 0;
        vert[1].x = rc.right;
        vert[1].y = rc.bottom;
        vert[1].Red = (COLOR16)(GetRValue(bottom) << 8);
        vert[1].Green = (COLOR16)(GetGValue(bottom) << 8);
        vert[1].Blue = (COLOR16)(GetBValue(bottom) << 8);
        vert[1].Alpha = 0;
        GRADIENT_RECT gRect{ 0, 1 };
        GradientFill(hdc, vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
    }

    inline void DrawLeftBrandPanel(HDC hdc, float animTime)
    {
        RECT client{};
        GetClientRect(gWnd, &client);
        const int h = client.bottom - client.top;

        RECT left{ 0, 0, LoaderConfig::kSplitX, h };
        FillVerticalGradient(hdc, left, RGB(12, 12, 14), LoaderConfig::kPanelLeft);

        const int cx = LoaderConfig::kSplitX / 2;
        const int cy = h / 2 - 12;
        const float pulse = 0.5f + 0.5f * sinf(animTime * 0.7f);

        DrawSoftGlow(hdc, cx, cy, 96,
            BlendRgb(LoaderConfig::kAccentGlow, LoaderConfig::kAccent, 0.18f + pulse * 0.12f),
            LoaderConfig::kPanelLeft, 10);

        HPEN accentPen = CreatePen(PS_SOLID, 2, LoaderConfig::kAccent);
        HGDIOBJ op = SelectObject(hdc, accentPen);
        MoveToEx(hdc, cx - 42, cy - 36, nullptr);
        LineTo(hdc, cx + 42, cy - 36);
        SelectObject(hdc, op);
        DeleteObject(accentPen);

        SelectObject(hdc, gFontLogo);
        SetTextColor(hdc, LoaderConfig::kText);
        RECT logoRc{ 0, cy - 24, LoaderConfig::kSplitX, cy + 16 };
        DrawTextA(hdc, "FRONTIER", -1, &logoRc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextMuted);
        RECT tagRc{ 0, cy + 18, LoaderConfig::kSplitX, cy + 40 };
        DrawTextA(hdc, "Premium External", -1, &tagRc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(34, 34, 38));
        op = SelectObject(hdc, pen);
        MoveToEx(hdc, LoaderConfig::kSplitX - 1, 0, nullptr);
        LineTo(hdc, LoaderConfig::kSplitX - 1, h);
        SelectObject(hdc, op);
        DeleteObject(pen);
    }

    inline void DrawRightBackdrop(HDC hdc, float animTime)
    {
        RECT client{};
        GetClientRect(gWnd, &client);
        const int w = client.right - client.left;
        const int h = client.bottom - client.top;
        const int rx = LoaderConfig::kSplitX;

        RECT right{ rx, 0, w, h };
        FillVerticalGradient(hdc, right, LoaderConfig::kBgTop, LoaderConfig::kBgBottom);

        const float drift = sinf(animTime * 0.45f) * 8.f;
        DrawSoftGlow(hdc, rx + 160 + (int)drift, 80, 100,
            RGB(72, 10, 6), LoaderConfig::kBgBottom, 8);
    }

    inline void DrawContentCard(HDC hdc)
    {
        RECT r = RightContentRect();
        RECT card{
            r.left - 4,
            r.top - 12,
            r.right + 4,
            r.bottom + 8
        };

        FillRoundRect(hdc, card, LoaderConfig::kCard, LoaderConfig::kBorder, 16);
        RECT inner = card;
        inner.left += 1; inner.top += 1; inner.right -= 1; inner.bottom -= 1;
        FillRoundRect(hdc, inner, LoaderConfig::kCardInner, LoaderConfig::kCardInner, 15);

        HPEN accentLine = CreatePen(PS_SOLID, 2, LoaderConfig::kAccent);
        HGDIOBJ oldPen = SelectObject(hdc, accentLine);
        MoveToEx(hdc, inner.left + 20, inner.top + 2, nullptr);
        LineTo(hdc, inner.left + 56, inner.top + 2);
        SelectObject(hdc, oldPen);
        DeleteObject(accentLine);
    }

    inline void DrawBackground(HDC hdc, float animTime)
    {
        RECT client{};
        GetClientRect(gWnd, &client);
        FillRect(hdc, &client, gBgBrush);
        DrawRightBackdrop(hdc, animTime);
        DrawLeftBrandPanel(hdc, animTime);
        DrawContentCard(hdc);
    }

    inline void DrawWindowControls(HDC hdc)
    {
        RECT minRc = MinButtonRect();
        RECT closeRc = CloseButtonRect();
        FillRoundRectSolid(hdc, minRc, gHover == HoverMin ? RGB(36, 36, 40) : RGB(24, 24, 28), 6);
        FillRoundRectSolid(hdc, closeRc, gHover == HoverClose ? RGB(196, 52, 52) : RGB(24, 24, 28), 6);
        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, RGB(210, 214, 222));
        DrawTextA(hdc, "-", -1, &minRc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        SetTextColor(hdc, RGB(240, 242, 246));
        DrawTextA(hdc, "x", -1, &closeRc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    inline void DrawBrandPanel(HDC hdc, float animTime)
    {
        (void)hdc;
        (void)animTime;
    }

    inline void DrawRightTitle(HDC hdc, const char* title, const char* subtitle = nullptr)
    {
        RECT r = RightContentRect();
        SelectObject(hdc, gFontBrand);
        SetTextColor(hdc, LoaderConfig::kText);
        RECT titleRc{ r.left, r.top, r.right, r.top + 28 };
        DrawTextA(hdc, title, -1, &titleRc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        if (subtitle && subtitle[0]) {
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextDim);
            RECT subRc{ r.left, r.top + 30, r.right, r.top + 52 };
            DrawTextA(hdc, subtitle, -1, &subRc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }
    }

    inline void DrawInputChrome(HDC hdc)
    {
        RECT field = KeyFieldRect();
        FillRoundRect(hdc, field, LoaderConfig::kInputBg, LoaderConfig::kInputBorder, 10);
        RECT inner = field;
        inner.left += 1; inner.top += 1; inner.right -= 1; inner.bottom -= 1;
        FillRoundRect(hdc, inner, LoaderConfig::kInputBg, LoaderConfig::kInputBg, 9);

        // key icon
        const int ix = field.left + 16;
        const int iy = (field.top + field.bottom) / 2;
        HPEN pen = CreatePen(PS_SOLID, 2, LoaderConfig::kTextDim);
        HGDIOBJ op = SelectObject(hdc, pen);
        Arc(hdc, ix - 6, iy - 10, ix + 8, iy + 4, ix + 8, iy - 2, ix - 2, iy - 10);
        MoveToEx(hdc, ix + 6, iy, nullptr);
        LineTo(hdc, ix + 12, iy + 8);
        SelectObject(hdc, op);
        DeleteObject(pen);
    }

    inline void DrawGradientButton(HDC hdc, const RECT& btn, const char* label, bool enabled, bool hover)
    {
        COLORREF fill = enabled ? (hover ? LoaderConfig::kAccentHover : LoaderConfig::kAccent) : LoaderConfig::kAccentDim;
        COLORREF border = BlendRgb(fill, RGB(255, 255, 255), hover ? 0.18f : 0.10f);
        FillRoundRect(hdc, btn, fill, border, 12);

        HPEN hi = CreatePen(PS_SOLID, 1, LoaderConfig::kAccentLight);
        HGDIOBJ op = SelectObject(hdc, hi);
        MoveToEx(hdc, btn.left + 14, btn.top + 1, nullptr);
        LineTo(hdc, btn.right - 14, btn.top + 1);
        SelectObject(hdc, op);
        DeleteObject(hi);

        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextA(hdc, label, -1, const_cast<RECT*>(&btn), DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    inline void DrawProgressBar(HDC hdc, State* s, bool successTone, bool showPercent = true)
    {
        RECT barBg = ProgressBarRect();
        FillRoundRectSolid(hdc, barBg, LoaderConfig::kBarTrack, 6);

        float pct = s->progressAnim;
        if (s->screen == ScreenReady && s->filesReady && !s->indeterminate)
            pct = s->progressAnim;
        if (pct < 0.f) pct = 0.f;
        if (pct > 1.f) pct = 1.f;

        const int barW = barBg.right - barBg.left;
        if (pct > 0.005f && barW > 0) {
            RECT barFg = barBg;
            barFg.right = barBg.left + (LONG)(barW * pct);
            if (barFg.right <= barFg.left + 4)
                barFg.right = barFg.left + 4;

            COLORREF fill = successTone ? LoaderConfig::kSuccess : LoaderConfig::kAccent;
            FillRoundRectSolid(hdc, barFg, fill, 6);
        }

        if (showPercent && !successTone) {
            char pctBuf[8];
            sprintf_s(pctBuf, "%d%%", (int)(pct * 100.f + 0.5f));
            RECT r = RightContentRect();
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextMuted);
            DrawTextA(hdc, pctBuf, -1, &RECT{ r.left, barBg.bottom + 6, r.right, barBg.bottom + 24 },
                DT_LEFT | DT_SINGLELINE);
        }
    }

    inline void DrawButtonAt(HDC hdc, const RECT& btn, const char* label, bool enabled, bool hover)
    {
        DrawGradientButton(hdc, btn, label, enabled, hover);
    }

    inline void DrawPrimaryButton(HDC hdc, const char* label, bool enabled, bool hover)
    {
        DrawGradientButton(hdc, PrimaryButtonRect(), label, enabled, hover);
    }

    inline void DrawGhostButton(HDC hdc, const RECT& btn, const char* label, bool hover)
    {
        COLORREF fill = hover ? LoaderConfig::kCardHover : LoaderConfig::kCard;
        FillRoundRect(hdc, btn, fill, LoaderConfig::kBorder, 10);
        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        DrawTextA(hdc, label, -1, const_cast<RECT*>(&btn), DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    inline void DrawLoginHint(HDC hdc)
    {
        RECT field = KeyFieldRect();
        SelectObject(hdc, gFont);
        SetTextColor(hdc, RGB(90, 94, 104));
        RECT hint{ field.left + 38, field.top, field.right - 8, field.bottom };
        if (!gKeyEdit || !IsWindowVisible(gKeyEdit) || !GetWindowTextLengthA(gKeyEdit)) {
            DrawTextA(hdc, "FRTR-XXXX-XXXX-XXXX", -1, &hint, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }
    }

    inline void DrawModeCard(HDC hdc, State* s, int displayIndex, int modeValue,
        const char* label, const char* tag, const char* subtitle = nullptr)
    {
        RECT card = ModeCardRect(s, displayIndex);
        bool sel = (s->selectedMode == modeValue);
        bool hover = false;
        if (displayIndex == 0)
            hover = (gHover == HoverMode0);
        else
            hover = (gHover == HoverMode1);

        COLORREF fill = sel ? BlendRgb(LoaderConfig::kCard, LoaderConfig::kAccentGlow, 0.35f)
            : (hover ? LoaderConfig::kCardHover : LoaderConfig::kCard);
        COLORREF border = sel ? LoaderConfig::kBorderActive : LoaderConfig::kBorder;
        FillRoundRect(hdc, card, fill, border, 12);

        if (sel) {
            HPEN p = CreatePen(PS_SOLID, 2, LoaderConfig::kAccent);
            HGDIOBJ op = SelectObject(hdc, p);
            MoveToEx(hdc, card.left + 14, card.top + 10, nullptr);
            LineTo(hdc, card.left + 14, card.bottom - 10);
            SelectObject(hdc, op);
            DeleteObject(p);
        }

        SelectObject(hdc, gFontBold);
        SetTextColor(hdc, LoaderConfig::kText);
        TextOutA(hdc, card.left + 18, card.top + 12, label, (int)strlen(label));

        if (subtitle && subtitle[0]) {
            SelectObject(hdc, gFont);
            SetTextColor(hdc, LoaderConfig::kTextMuted);
            TextOutA(hdc, card.left + 18, card.top + 30, subtitle, (int)strlen(subtitle));
        }

        if (tag && tag[0]) {
            SelectObject(hdc, gFont);
            const bool testingTag = (strcmp(tag, LoaderConfig::kKernelModeTag) == 0);
            SetTextColor(hdc, testingTag ? LoaderConfig::kWarning : LoaderConfig::kAccent);
            SIZE ts{};
            GetTextExtentPoint32A(hdc, tag, (int)strlen(tag), &ts);
            TextOutA(hdc, card.right - ts.cx - 18, card.top + 14, tag, (int)strlen(tag));
        }
    }

    inline void DrawDownloadingScreen(HDC hdc, State* s)
    {
        DrawWindowControls(hdc);
        DrawBrandPanel(hdc, s->animTime);
        DrawRightTitle(hdc, "Updating");
        RECT r = RightContentRect();
        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        RECT statusRc{ r.left, r.top + 40, r.right, r.top + 64 };
        DrawTextA(hdc, s->status, -1, &statusRc, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
        DrawProgressBar(hdc, s, false);
    }

    inline void DrawReadyScreen(HDC hdc, State* s)
    {
        DrawWindowControls(hdc);
        DrawBrandPanel(hdc, s->animTime);
        DrawRightTitle(hdc, s->filesReady ? "Ready" : "Setup");
        RECT r = RightContentRect();
        const char* sub = s->filesReady ? "All files are up to date." : s->status;
        SelectObject(hdc, gFont);
        SetTextColor(hdc, s->filesReady ? LoaderConfig::kSuccess : LoaderConfig::kTextDim);
        RECT subRc{ r.left, r.top + 40, r.right, r.top + 90 };
        DrawTextA(hdc, sub, -1, &subRc, DT_LEFT | DT_WORDBREAK);
        if (!s->filesReady)
            DrawProgressBar(hdc, s, false, true);
        else
            DrawProgressBar(hdc, s, true, false);
        DrawPrimaryButton(hdc, s->filesReady ? "Continue" : "Download",
            true, gHover == HoverPrimary);
    }

    inline void DrawAuthStatusBanner(HDC hdc, State* s)
    {
        if (!s->authStatus[0]) return;
        RECT banner = AuthStatusRect();
        SelectObject(hdc, gFont);
        SetTextColor(hdc, s->authenticated ? LoaderConfig::kSuccess : RGB(255, 140, 150));
        DrawTextA(hdc, s->authStatus, -1, &banner, DT_LEFT | DT_WORDBREAK);
    }

    inline void DrawAuthScreen(HDC hdc, State* s)
    {
        DrawWindowControls(hdc);
        DrawBrandPanel(hdc, s->animTime);
        DrawRightTitle(hdc, "Sign in", "Enter your lifetime license key");
        DrawInputChrome(hdc);
        DrawLoginHint(hdc);
        DrawAuthStatusBanner(hdc, s);
        if (s->authenticated) {
            DrawGradientButton(hdc, AuthContinueRect(), "Continue", true, gHover == HoverPrimary);
            DrawGhostButton(hdc, SignOutButtonRect(), "Sign out", gHover == HoverSignOut);
        } else {
            DrawGradientButton(hdc, ActivateButtonRect(),
                s->authBusy ? "Please wait..." : "Activate License",
                !s->authBusy, gHover == HoverActivate);
            if (LoaderConfig::kDiscordOAuthEnabled) {
                DrawGhostButton(hdc, DiscordButtonRect(),
                    s->authBusy ? "Discord..." : "Sign in with Discord",
                    gHover == HoverDiscord);
            }
        }
    }

    inline void DrawLauncherScreen(HDC hdc, State* s)
    {
        DrawWindowControls(hdc);
        DrawBrandPanel(hdc, s->animTime);
        DrawRightTitle(hdc, "Launch");
        RECT r = RightContentRect();
        SelectObject(hdc, gFont);
        SetTextColor(hdc, LoaderConfig::kTextDim);
        TextOutA(hdc, r.left, r.top + 40, "Select how FRONTIER attaches to Roblox", 38);

        int display = 0;
        if (ShowKernelMode(s)) {
            DrawModeCard(hdc, s, display++, 1, "Kernel", LoaderConfig::kKernelModeTag,
                "Driver attach — testing only");
        }
        DrawModeCard(hdc, s, display, 0, "Usermode", "Recommended",
            "Standard external — best for most games");
        DrawGradientButton(hdc, LaunchButtonRect(), "Launch", true, gHover == HoverPrimary);
    }

    inline void DrawFooter(HDC hdc, State* s)
    {
        RECT support = SupportLinkRect();
        SelectObject(hdc, gFont);
        SetTextColor(hdc, gHover == HoverSupport ? LoaderConfig::kAccentHover : LoaderConfig::kAccent);
        DrawTextA(hdc, "support", -1, &support, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        char ver[64];
        if (s->remoteDisplay[0]) {
            strncpy_s(ver, s->remoteDisplay, _TRUNCATE);
        } else if (s->remoteVersion > 0) {
            sprintf_s(ver, "v%d", s->remoteVersion);
        } else if (s->localVersion > 0) {
            sprintf_s(ver, "v%d", s->localVersion);
        } else {
            strcpy_s(ver, "v1.2.3");
        }

        SetTextColor(hdc, LoaderConfig::kTextMuted);
        SIZE vs{};
        GetTextExtentPoint32A(hdc, ver, (int)strlen(ver), &vs);
        TextOutA(hdc, LoaderConfig::kWindowW - vs.cx - 22, LoaderConfig::kWindowH - 24, ver, (int)strlen(ver));
    }

    inline void PaintContent(HDC hdc, State* s)
    {
        SetBkMode(hdc, TRANSPARENT);
        switch (s->screen) {
        case ScreenDownloading: DrawDownloadingScreen(hdc, s); break;
        case ScreenReady: DrawReadyScreen(hdc, s); break;
        case ScreenAuth: DrawAuthScreen(hdc, s); break;
        case ScreenLauncher: DrawLauncherScreen(hdc, s); break;
        }
        DrawFooter(hdc, s);
    }

    inline void Paint(HDC hdc, State* s)
    {
        RECT client{};
        GetClientRect(gWnd, &client);
        const int w = client.right - client.left;
        const int h = client.bottom - client.top;

        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(mem, bmp);

        FillRect(mem, &client, gBgBrush);
        DrawBackground(mem, s ? s->animTime : 0.f);

        PaintContent(mem, s);
        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);

        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);
    }

    inline void GoReady(State* s, bool ready = true)
    {
        s->screen = ScreenReady;
        s->filesReady = ready;
        s->updating = false;
        s->checking = false;
        s->indeterminate = false;
        if (ready) {
            SetProgressTarget(s, 1.f);
            strncpy_s(s->status, "All files are up to date.", _TRUNCATE);
        } else {
            s->progress = 0.f;
            s->progressAnim = 0.f;
            s->progressActive = false;
        }
        s->selectedMode = 0;
        if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
    }

    inline void RunUpdate(State* s);

    inline void CheckAsync(State* s)
    {
        if (s->checking) return;
        s->checking = true;
        s->screen = ScreenDownloading;
        SetIndeterminateProgress(s, true);
        SetStatus(s, "Checking files...");
        std::thread([s]() {
            LoaderUpdate::Manifest m{};
            std::string err;
            bool ok = LoaderUpdate::FetchManifest(m, err);
            LoaderUpdate::ApplyFallbackManifest(m);
            s->manifest = m;
            s->localVersion = LoaderUpdate::LoadLocalVersion();
            s->remoteVersion = m.version;
            strncpy_s(s->remoteDisplay, m.display, _TRUNCATE);
            s->updateAvailable = ok && m.version > s->localVersion;
            if (s->localVersion <= 0 || s->localVersion > 999)
                LoaderUpdate::SaveLocalVersion(m.version > 0 ? m.version : 1);
            s->localVersion = LoaderUpdate::LoadLocalVersion();
            if (m.kernelAvailable)
                s->kernelAvailable = true;

            if (!s->kernelAvailable)
                s->kernelAvailable = LoaderUpdate::ProbeKernelAvailable();
            s->selectedMode = 0;

            const bool missingLocal = !LocalUsermodeExists();
            if (missingLocal && s->manifest.usermodeUrl[0]) {
                s->checking = false;
                if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
                RunUpdate(s);
                return;
            }

            if (missingLocal) {
                GoReady(s, false);
                SetStatus(s, ok ? "Could not download — click Download below." : "Offline — click Download to fetch FRONTIER.");
            } else {
                if (s->localVersion <= 0)
                    LoaderUpdate::SaveLocalVersion(1);
                s->localVersion = LoaderUpdate::LoadLocalVersion();
                GoReady(s, true);
                if (!ok)
                    SetStatus(s, "Offline mode - using local files");
                else if (s->updateAvailable)
                    SetStatus(s, "Local build ready (remote update available on GitHub)");
            }

            s->checking = false;
            if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
        }).detach();
    }

    inline void RunUpdate(State* s)
    {
        if (s->updating) return;
        if (!s->manifest.usermodeUrl[0]) {
            ShellExecuteA(nullptr, "open", LoaderConfig::kReleasesPage, nullptr, nullptr, SW_SHOWNORMAL);
            GoReady(s, LocalUsermodeExists());
            SetStatus(s, "Opened GitHub releases");
            return;
        }

        s->updating = true;
        s->screen = ScreenDownloading;
        SetProgressTarget(s, 0.f);
        SetStatus(s, "Downloading usermode...");
        LoaderUpdate::Manifest m = s->manifest;
        std::thread([s, m]() {
            std::string err;
            bool ok = LoaderUpdate::ApplyUpdate(m,
                [s](float p, const char* msg) {
                    SetProgressTarget(s, p);
                    SetStatus(s, msg);
                }, err);
            if (ok) {
                s->localVersion = m.version;
                s->updateAvailable = false;
                s->kernelAvailable = LoaderUpdate::ProbeKernelAvailable() || m.kernelAvailable;
                s->selectedMode = 0;
                GoReady(s, true);
            } else {
                char buf[256];
                sprintf_s(buf, "Update failed: %s", err.c_str());
                GoReady(s, LocalUsermodeExists());
                SetStatus(s, buf);
            }
            if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
        }).detach();
    }

    inline void RunLaunch(State* s)
    {
        if (!s->authenticated || !s->licenseToken[0]) {
            MessageBoxW(gWnd, L"Activate your license before launching.", LoaderConfig::kAppName, MB_OK | MB_ICONWARNING);
            return;
        }
        if (s->selectedMode == 1 && !LoaderUpdate::ProbeKernelAvailable()) {
            MessageBoxW(gWnd,
                L"Kernel mode is in testing and is not installed on this PC.\n"
                L"Use Usermode (recommended), or run Update to fetch the kernel bundle.",
                LoaderConfig::kAppName, MB_OK | MB_ICONINFORMATION);
            return;
        }
        std::wstring work;
        std::wstring exe;
        if (s->selectedMode == 0) {
            if (!LoaderUpdate::ResolveUsermodeExe(exe, work)) {
                MessageBoxW(gWnd, L"Missing usermode\\Frontier.exe.", LoaderConfig::kAppName, MB_OK | MB_ICONWARNING);
                return;
            }
        } else {
            std::wstring dir = LoaderUpdate::GetLoaderDir();
            work = LoaderUpdate::PathJoin(dir, L"kernel");
            exe = LoaderUpdate::PathJoin(work, LoaderConfig::kKernelExe);
        }
        if (!LoaderLicense::WriteSessionFile(work, s->licenseToken, s->licenseHwid)) {
            MessageBoxW(gWnd, L"Could not write session file.", LoaderConfig::kAppName, MB_OK | MB_ICONWARNING);
            return;
        }
        std::wstring err;
        if (!LoaderUpdate::RunPayload(s->selectedMode, err))
            MessageBoxW(gWnd, err.c_str(), LoaderConfig::kAppName, MB_OK | MB_ICONWARNING);
        else
            PostMessage(gWnd, WM_CLOSE, 0, 0);
    }

    inline void FinishAuthUi(State* s, AuthDonePayload* payload)
    {
        if (!s || !payload) return;
        if (payload->ok) {
            strncpy_s(s->licenseToken, payload->token, _TRUNCATE);
            strncpy_s(s->authStatus, payload->status, _TRUNCATE);
            s->authenticated = true;
        } else {
            strncpy_s(s->authStatus, payload->status, _TRUNCATE);
            s->authenticated = false;
        }
        s->authBusy = false;
        RefreshAuthUi(s);
        if (gWnd) InvalidateRect(gWnd, nullptr, FALSE);
    }

    inline void RunActivateKey(State* s)
    {
        if (s->authBusy || !gKeyEdit) return;
        char raw[80]{};
        GetWindowTextA(gKeyEdit, raw, sizeof(raw));
        char key[64]{};
        LoaderLicense::NormalizeKeyInput(raw, key, sizeof(key));
        if (!key[0]) {
            strncpy_s(s->authStatus, "Paste your FRTR license key first.", _TRUNCATE);
            s->authenticated = false;
            InvalidateRect(gWnd, nullptr, FALSE);
            return;
        }
        SetWindowTextA(gKeyEdit, key);
        s->authBusy = true;
        strncpy_s(s->authStatus, "Activating license...", _TRUNCATE);
        RefreshAuthUi(s);
        std::string hw = s->licenseHwid[0] ? s->licenseHwid : LoaderLicense::GetHwid();
        strncpy_s(s->licenseHwid, hw.c_str(), _TRUNCATE);
        std::string keyCopy = key;
        std::thread([s, keyCopy, hw]() {
            auto* payload = new AuthDonePayload{};
            char token[768]{}, msg[256]{};
            std::string err;
            bool ok = LoaderLicense::ActivateKey(keyCopy.c_str(), hw.c_str(), token, sizeof(token), msg, sizeof(msg), err);
            payload->ok = ok;
            if (ok) {
                strncpy_s(payload->token, token, _TRUNCATE);
                strncpy_s(payload->status, msg[0] ? msg : "License activated on this PC.", _TRUNCATE);
            } else {
                strncpy_s(payload->status, err.c_str(), _TRUNCATE);
            }
            if (gWnd)
                PostMessage(gWnd, WM_AUTH_DONE, 0, (LPARAM)payload);
            else
                delete payload;
        }).detach();
    }

    inline void RunDiscordSignIn(State* s)
    {
        if (s->authBusy) return;
        s->authBusy = true;
        strncpy_s(s->authStatus, "Opening Discord sign-in...", _TRUNCATE);
        RefreshAuthUi(s);
        std::string hw = s->licenseHwid[0] ? s->licenseHwid : LoaderLicense::GetHwid();
        strncpy_s(s->licenseHwid, hw.c_str(), _TRUNCATE);
        std::thread([s, hw]() {
            auto* payload = new AuthDonePayload{};
            LoaderOAuth::Session session{};
            std::string err;
            bool oauthOk = LoaderOAuth::SignIn(session, err);
            if (!oauthOk) {
                payload->ok = false;
                strncpy_s(payload->status, err.c_str(), _TRUNCATE);
                if (gWnd) PostMessage(gWnd, WM_AUTH_DONE, 0, (LPARAM)payload);
                else delete payload;
                return;
            }
            strncpy_s(s->signedInAs, session.globalName[0] ? session.globalName : session.username, _TRUNCATE);
            char token[768]{}, msg[256]{};
            bool ok = LoaderLicense::ActivateDiscord(session.discordId, hw.c_str(), token, sizeof(token), msg, sizeof(msg), err);
            payload->ok = ok;
            if (ok) {
                strncpy_s(payload->token, token, _TRUNCATE);
                strncpy_s(payload->status, msg[0] ? msg : "Signed in with Discord.", _TRUNCATE);
            } else {
                char combined[320];
                sprintf_s(combined, "%s Use your FRTR key if you have not redeemed on Discord yet.", err.c_str());
                strncpy_s(payload->status, combined, _TRUNCATE);
            }
            if (gWnd) PostMessage(gWnd, WM_AUTH_DONE, 0, (LPARAM)payload);
            else delete payload;
        }).detach();
    }

    inline void SignOutLicense(State* s)
    {
        LoaderLicense::ClearLicense();
        LoaderOAuth::ClearSession();
        s->licenseToken[0] = 0;
        s->signedInAs[0] = 0;
        s->authenticated = false;
        strncpy_s(s->authStatus, "Signed out. Enter your license key.", _TRUNCATE);
        RefreshAuthUi(s);
    }

    inline int HitTestHover(State* s, POINT pt)
    {
        if (PtInRect(&CloseButtonRect(), pt)) return HoverClose;
        if (PtInRect(&MinButtonRect(), pt)) return HoverMin;
        if (PtInRect(&SupportLinkRect(), pt)) return HoverSupport;
        if (s->updating) return HoverNone;

        if (s->screen == ScreenReady && s->filesReady && PtInRect(&PrimaryButtonRect(), pt))
            return HoverPrimary;
        if (s->screen == ScreenReady && !s->filesReady && PtInRect(&PrimaryButtonRect(), pt))
            return HoverPrimary;

        if (s->screen == ScreenAuth) {
            if (s->authenticated && PtInRect(&AuthContinueRect(), pt))
                return HoverPrimary;
            if (LoaderConfig::kDiscordOAuthEnabled && PtInRect(&DiscordButtonRect(), pt))
                return HoverDiscord;
            if (PtInRect(&ActivateButtonRect(), pt))
                return HoverActivate;
            if (s->authenticated && PtInRect(&SignOutButtonRect(), pt))
                return HoverSignOut;
        }

        if (s->screen == ScreenLauncher) {
            if (PtInRect(&LaunchButtonRect(), pt))
                return HoverPrimary;
            if (ShowKernelMode(s) && PtInRect(&ModeCardRect(s, 0), pt))
                return HoverMode0;
            const int userIdx = ShowKernelMode(s) ? 1 : 0;
            if (PtInRect(&ModeCardRect(s, userIdx), pt))
                return HoverMode1;
        }
        return HoverNone;
    }

    inline void UpdateHover(HWND hwnd, State* s, POINT pt)
    {
        int next = HitTestHover(s, pt);
        if (next != gHover) {
            gHover = next;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    inline void HandleLauncherClick(State* s, POINT pt)
    {
        if (ShowKernelMode(s) && PtInRect(&ModeCardRect(s, 0), pt)) {
            s->selectedMode = 1;
            LoaderUpdate::SaveMode(1);
            InvalidateRect(gWnd, nullptr, FALSE);
            return;
        }
        const int userIdx = ShowKernelMode(s) ? 1 : 0;
        if (PtInRect(&ModeCardRect(s, userIdx), pt)) {
            s->selectedMode = 0;
            LoaderUpdate::SaveMode(0);
            InvalidateRect(gWnd, nullptr, FALSE);
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
            if (s) {
                TryRestoreLicense(s);
                s->authenticated = s->licenseToken[0] != 0;
            }
            HINSTANCE inst = ((LPCREATESTRUCTW)lp)->hInstance;
            gKeyEdit = CreateWindowExW(
                0, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | ES_UPPERCASE,
                KeyFieldRect().left + 36, KeyFieldRect().top + 8,
                KeyFieldRect().right - KeyFieldRect().left - 44,
                KeyFieldRect().bottom - KeyFieldRect().top - 16,
                hwnd, (HMENU)1001, inst, nullptr);
            if (gKeyEdit && gFont)
                SendMessageW(gKeyEdit, WM_SETFONT, (WPARAM)gFont, TRUE);
            SetKeyEditVisible(false);
            CheckAsync(s);
            StartAnimTimer(hwnd);
            return 0;
        }
        case WM_TIMER:
            if (wp == kAnimTimerId && s) {
                static DWORD lastTick = 0;
                DWORD now = GetTickCount();
                if (!lastTick) lastTick = now;
                float dt = (now - lastTick) / 1000.f;
                lastTick = now;
                TickAnimation(s, dt);
                if (NeedsAnimation(s))
                    InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProcW(hwnd, msg, wp, lp);
            if (hit == HTCLIENT) {
                POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                ScreenToClient(hwnd, &pt);
                if (HeaderDragArea(pt))
                    return HTCAPTION;
            }
            return hit;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            Paint(hdc, s);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE: {
            POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            UpdateHover(hwnd, s, pt);
            if (!gTrackingMouse) {
                TRACKMOUSEEVENT tme{};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
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
            POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            if (PtInRect(&CloseButtonRect(), pt)) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
            if (PtInRect(&MinButtonRect(), pt)) {
                ShowWindow(hwnd, SW_MINIMIZE);
                return 0;
            }
            if (PtInRect(&SupportLinkRect(), pt)) {
                ShellExecuteA(nullptr, "open", LoaderConfig::kDiscordInvite, nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }
            if (s->updating) return 0;

            if (s->screen == ScreenReady && PtInRect(&PrimaryButtonRect(), pt)) {
                if (s->filesReady) {
                    s->selectedMode = 0;
                    s->screen = ScreenAuth;
                    SetKeyEditVisible(true);
                    gHover = HoverNone;
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else {
                    LoaderUpdate::ApplyFallbackManifest(s->manifest);
                    RunUpdate(s);
                }
                return 0;
            }
            if (s->screen == ScreenAuth) {
                if (s->authenticated && PtInRect(&SignOutButtonRect(), pt)) {
                    SignOutLicense(s);
                    return 0;
                }
                if (LoaderConfig::kDiscordOAuthEnabled && PtInRect(&DiscordButtonRect(), pt)) {
                    RunDiscordSignIn(s);
                    return 0;
                }
                if (PtInRect(&ActivateButtonRect(), pt)) {
                    RunActivateKey(s);
                    return 0;
                }
                if (s->authenticated && PtInRect(&AuthContinueRect(), pt)) {
                    s->screen = ScreenLauncher;
                    s->selectedMode = 0;
                    LoaderUpdate::SaveMode(0);
                    SetKeyEditVisible(false);
                    gHover = HoverNone;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                return 0;
            }
            if (s->screen == ScreenLauncher)
                HandleLauncherClick(s, pt);
            return 0;
        }
        case WM_SETCURSOR: {
            if (LOWORD(lp) == HTCLIENT && gState && !gState->updating) {
                SetCursor(LoadCursor(nullptr, gHover != HoverNone ? IDC_HAND : IDC_ARROW));
                return TRUE;
            }
            break;
        }
        case WM_AUTH_DONE: {
            auto* payload = (AuthDonePayload*)lp;
            if (s && payload) {
                FinishAuthUi(s, payload);
            }
            delete payload;
            return 0;
        }
        case WM_CTLCOLOREDIT:
            if (gEditBrush) {
                SetTextColor((HDC)wp, LoaderConfig::kText);
                SetBkColor((HDC)wp, LoaderConfig::kInputBg);
                return (INT_PTR)gEditBrush;
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wp) == 1001 && HIWORD(wp) == EN_CHANGE && s && s->screen == ScreenAuth)
                InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case WM_KEYDOWN:
            if (wp == VK_RETURN && s && s->screen == ScreenAuth && GetFocus() == gKeyEdit) {
                RunActivateKey(s);
                return 0;
            }
            break;
        case WM_DESTROY:
            StopAnimTimer(hwnd);
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
        state.selectedMode = 0;
        state.localVersion = LoaderUpdate::LoadLocalVersion();
        state.kernelAvailable = LoaderUpdate::ProbeKernelAvailable();
        gState = &state;

        gBgBrush = CreateSolidBrush(LoaderConfig::kBg);
        gEditBrush = CreateSolidBrush(LoaderConfig::kInputBg);
        gFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        gFontBold = CreateFontW(-16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        gFontBrand = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        gFontHero = CreateFontW(-42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        gFontLogo = CreateFontW(-34, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Semibold");
        if (!gFontLogo)
            gFontLogo = gFontHero;

        HINSTANCE inst = GetModuleHandleW(nullptr);
        gAppIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        gAppIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 20, 20, LR_DEFAULTCOLOR);

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = inst;
        wc.hIcon = gAppIcon ? gAppIcon : LoadIconW(inst, MAKEINTRESOURCEW(IDI_ICON1));
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = gBgBrush;
        wc.lpszClassName = L"FrontierLoaderWnd";
        RegisterClassExW(&wc);

        const int w = LoaderConfig::kWindowW;
        const int h = LoaderConfig::kWindowH;
        int sx = GetSystemMetrics(SM_CXSCREEN);
        int sy = GetSystemMetrics(SM_CYSCREEN);
        int x = (sx - w) / 2;
        int y = (sy - h) / 2;

        gWnd = CreateWindowExW(
            WS_EX_APPWINDOW,
            wc.lpszClassName,
            LoaderConfig::kAppName,
            WS_POPUP,
            x, y, w, h,
            nullptr, nullptr, inst, nullptr);

        if (!gWnd) return 1;
        ApplyWindowChrome(gWnd);
        SendMessageW(gWnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
        SendMessageW(gWnd, WM_SETICON, ICON_SMALL, (LPARAM)(gAppIconSm ? gAppIconSm : wc.hIcon));

        ShowWindow(gWnd, SW_SHOW);
        UpdateWindow(gWnd);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (gAppIcon) DestroyIcon(gAppIcon);
        if (gAppIconSm) DestroyIcon(gAppIconSm);
        DeleteObject(gFont);
        DeleteObject(gFontBold);
        DeleteObject(gFontBrand);
        DeleteObject(gFontHero);
        if (gFontLogo && gFontLogo != gFontHero)
            DeleteObject(gFontLogo);
        DeleteObject(gEditBrush);
        DeleteObject(gBgBrush);
        return (int)msg.wParam;
    }
}
