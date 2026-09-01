#pragma once
#include <Windows.h>
#include <urlmon.h>
#include <gdiplus.h>
#include <cstdio>
#include <vector>
#include "loader_config.h"
#include "loader_update.h"
#include "loader_products.h"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "gdiplus.lib")

namespace LoaderAssets {

    inline ULONG_PTR gGdiToken = 0;

    inline bool EnsureGdiplus()
    {
        if (gGdiToken) return true;
        Gdiplus::GdiplusStartupInput input;
        return Gdiplus::GdiplusStartup(&gGdiToken, &input, nullptr) == Gdiplus::Ok;
    }

    inline void ShutdownGdiplus()
    {
        if (gGdiToken) {
            Gdiplus::GdiplusShutdown(gGdiToken);
            gGdiToken = 0;
        }
    }

    inline std::wstring AssetsDir()
    {
        return LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"assets");
    }

    inline std::wstring CachePath(const wchar_t* leaf)
    {
        return LoaderUpdate::PathJoin(AssetsDir(), leaf);
    }

    inline bool DownloadIfMissing(const char* url, const wchar_t* dest)
    {
        if (LoaderUpdate::FileExists(dest)) return true;
        LoaderUpdate::EnsureDir(AssetsDir());
        std::wstring wurl = LoaderUpdate::ToWide(url);
        HRESULT hr = URLDownloadToFileW(nullptr, wurl.c_str(), dest, 0, nullptr);
        return SUCCEEDED(hr) && LoaderUpdate::FileExists(dest);
    }

    inline void EnsureProductArt()
    {
        DownloadIfMissing(LoaderConfig::kTraceCardUrl,
            CachePath(L"trace-card.png").c_str());
        DownloadIfMissing(LoaderConfig::kFrontierCardUrl,
            CachePath(L"frontier-card.png").c_str());
        DownloadIfMissing(LoaderConfig::kTraceMascotUrl,
            CachePath(L"trace-chan.png").c_str());
        DownloadIfMissing(LoaderConfig::kFrontierMascotUrl,
            CachePath(L"frontier-chan.png").c_str());
    }

    inline void DrawCachedImage(HDC hdc, const RECT& dest, const wchar_t* file, BYTE alpha = 255)
    {
        if (!EnsureGdiplus() || !file || !LoaderUpdate::FileExists(file)) return;
        Gdiplus::Bitmap bmp(file);
        if (bmp.GetLastStatus() != Gdiplus::Ok) return;
        Gdiplus::Graphics g(hdc);
        g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        if (alpha < 255) {
            Gdiplus::ColorMatrix matrix = {
                1.f, 0.f, 0.f, 0.f, 0.f,
                0.f, 1.f, 0.f, 0.f, 0.f,
                0.f, 0.f, 1.f, 0.f, 0.f,
                0.f, 0.f, 0.f, alpha / 255.f, 0.f,
                0.f, 0.f, 0.f, 0.f, 1.f,
            };
            Gdiplus::ImageAttributes attrs;
            attrs.SetColorMatrix(&matrix);
            g.DrawImage(&bmp,
                Gdiplus::Rect(dest.left, dest.top, dest.right - dest.left, dest.bottom - dest.top),
                0, 0, bmp.GetWidth(), bmp.GetHeight(), Gdiplus::UnitPixel, &attrs);
        } else {
            g.DrawImage(&bmp, dest.left, dest.top, dest.right - dest.left, dest.bottom - dest.top);
        }
    }

    inline void DrawProductCard(HDC hdc, const RECT& card, LoaderProducts::Product product,
        bool selected, bool locked, float hoverT, HFONT fontBold = nullptr, HFONT fontReg = nullptr)
    {
        hoverT = hoverT < 0.f ? 0.f : (hoverT > 1.f ? 1.f : hoverT);
        const COLORREF base = RGB(17, 17, 17);
        const COLORREF hot = RGB(24, 28, 36);
        const COLORREF fill = RGB(
            (GetRValue(base) * (1 - hoverT) + GetRValue(hot) * hoverT),
            (GetGValue(base) * (1 - hoverT) + GetGValue(hot) * hoverT),
            (GetBValue(base) * (1 - hoverT) + GetBValue(hot) * hoverT));
        HBRUSH b = CreateSolidBrush(fill);
        HPEN p = CreatePen(PS_SOLID, selected ? 2 : 1,
            selected ? LoaderConfig::kAccent : RGB(35, 35, 35));
        HGDIOBJ ob = SelectObject(hdc, b);
        HGDIOBJ op = SelectObject(hdc, p);
        RoundRect(hdc, card.left, card.top, card.right, card.bottom, 14, 14);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(b);
        DeleteObject(p);

        RECT art = card;
        art.top += 10;
        art.bottom -= 52;
        art.left += 10;
        art.right -= 10;

        const wchar_t* cardImg = product == LoaderProducts::ProductTrace
            ? CachePath(L"trace-card.png").c_str()
            : CachePath(L"frontier-card.png").c_str();
        const wchar_t* mascot = product == LoaderProducts::ProductTrace
            ? CachePath(L"trace-chan.png").c_str()
            : CachePath(L"frontier-chan.png").c_str();
        DrawCachedImage(hdc, art, cardImg, locked ? 120 : 255);

        RECT mascotRc = art;
        mascotRc.left = mascotRc.right - 80;
        mascotRc.top = mascotRc.bottom - 80;
        DrawCachedImage(hdc, mascotRc, mascot, locked ? 100 : 255);

        if (locked) {
            HBRUSH overlay = CreateSolidBrush(RGB(0, 0, 0));
            RECT ov = card;
            ov.top += 1; ov.left += 1; ov.right -= 1; ov.bottom -= 1;
            HGDIOBJ oo = SelectObject(hdc, overlay);
            SelectObject(hdc, GetStockObject(NULL_PEN));
            RoundRect(hdc, ov.left, ov.top, ov.right, ov.bottom, 14, 14);
            SelectObject(hdc, oo);
            DeleteObject(overlay);
        }

        SetBkMode(hdc, TRANSPARENT);
        HFONT titleFont = fontBold ? fontBold : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT tagFont = fontReg ? fontReg : titleFont;
        SelectObject(hdc, titleFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        RECT nameRc{ card.left + 14, card.bottom - 44, card.right - 14, card.bottom - 22 };
        DrawTextA(hdc, LoaderProducts::ProductName(product), -1, &nameRc, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, tagFont);
        SetTextColor(hdc, RGB(160, 160, 168));
        RECT tagRc{ card.left + 14, card.bottom - 22, card.right - 14, card.bottom - 6 };
        DrawTextA(hdc, locked ? "Sign in to unlock" : LoaderProducts::ProductTagline(product),
            -1, &tagRc, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

} // namespace LoaderAssets
