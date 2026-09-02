#pragma once
#include <cstdio>
#include <cmath>
#include <cstring>
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"
#include "../core/updater/updater.h"
#include "brand.h"
#include "brand_assets.h"
#include "frontier_theme.h"
#include "ui_motion.h"
#include "frontier_ui.h"
#include "brand_assets.h"

namespace FrontierShell {

    inline constexpr float kSidebarW = 80.f;
    inline constexpr float kHeaderH = 64.f;
    inline constexpr float kFooterH = 40.f;
    inline constexpr float kNavItemH = 36.f;
    inline constexpr float kNavGap = 10.f;
    inline constexpr float kGhostAreaH = 64.f;

    inline constexpr float kMwPad = 20.f;
    inline constexpr float kMwTitleH = 76.f;
    inline constexpr float kMwFooterH = 30.f;
    inline constexpr float kMwTabH = 34.f;

    inline bool UseTopTabs() { return variables::Theme::layoutMode == 0; }

    inline ImU32 AccentCol(float a = 1.f) {
        return FrontierUI::U32(variables::Theme::brand, a);
    }

    inline ImU32 Gray500() { return IM_COL32(107, 114, 128, 255); }
    inline ImU32 Gray400() { return IM_COL32(156, 163, 175, 255); }

    struct TabItem {
        const char* label;
        int iconKind;
    };

    inline const TabItem* Tabs() {
        static const TabItem kTabs[] = {
            { "Aimbot", 0 }, { "Visuals", 2 }, { "World", 3 }, { "Character", 1 },
            { "Options", 5 }, { "Servers", 7 }, { "Music", 8 }, { "Status", 6 }, { "Configs", 9 },
        };
        return kTabs;
    }

    inline int TabCount() { return 9; }

    inline const char* TabLabel(int tab) {
        const TabItem* tabs = Tabs();
        if (tab < 0 || tab >= TabCount()) return "";
        return tabs[tab].label;
    }

    inline void DrawBrandMark(ImDrawList* dl, ImVec2 c, float size) {
        if (BrandAssets::logoSrv && BrandAssets::logoW > 0 && BrandAssets::logoH > 0) {
            const float aspect = (float)BrandAssets::logoW / (float)BrandAssets::logoH;
            const float h = size;
            const float w = h * aspect;
            dl->AddImage(BrandAssets::LogoTex(),
                ImVec2(c.x - w * 0.5f, c.y - h * 0.5f),
                ImVec2(c.x + w * 0.5f, c.y + h * 0.5f),
                ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 245));
            return;
        }
        ImFont* font = ImGui::GetFont();
        const float fs = size * 1.1f;
        const char* t = "F";
        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, t);
        dl->AddText(font, fs, ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f),
            IM_COL32(235, 45, 45, 255), t);
    }

    inline void DrawNavIcon(ImDrawList* dl, ImVec2 c, int kind, ImU32 col, float s = 1.f) {
        const float r = 5.f * s;
        switch (kind) {
        case 0:
            dl->AddCircle(c, r + 2.f, col, 0, 1.4f);
            dl->AddLine(ImVec2(c.x - r - 3.f, c.y), ImVec2(c.x + r + 3.f, c.y), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y - r - 3.f), ImVec2(c.x, c.y + r + 3.f), col, 1.4f);
            break;
        case 1:
            dl->AddCircleFilled(ImVec2(c.x, c.y - 5.f * s), 3.f * s, col);
            dl->AddLine(ImVec2(c.x, c.y - 2.f * s), ImVec2(c.x, c.y + 5.f * s), col, 1.5f);
            dl->AddLine(ImVec2(c.x - 5.f * s, c.y + 1.f * s), ImVec2(c.x + 5.f * s, c.y + 1.f * s), col, 1.5f);
            break;
        case 2:
            dl->AddBezierCubic(
                ImVec2(c.x - 8.f * s, c.y), ImVec2(c.x - 4.f * s, c.y - 6.f * s),
                ImVec2(c.x + 4.f * s, c.y - 6.f * s), ImVec2(c.x + 8.f * s, c.y), col, 1.3f);
            dl->AddBezierCubic(
                ImVec2(c.x - 8.f * s, c.y), ImVec2(c.x - 4.f * s, c.y + 6.f * s),
                ImVec2(c.x + 4.f * s, c.y + 6.f * s), ImVec2(c.x + 8.f * s, c.y), col, 1.3f);
            dl->AddCircleFilled(c, 2.5f * s, col);
            break;
        case 3:
            dl->AddCircle(c, r + 1.f, col, 0, 1.3f);
            dl->AddBezierCubic(
                ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.3f, c.y - r - 2.f),
                ImVec2(c.x + r * 0.3f, c.y + r + 2.f), ImVec2(c.x + r, c.y), col, 1.2f);
            break;
        case 5:
            dl->AddCircle(c, r, col, 8, 1.3f);
            for (int i = 0; i < 6; i++) {
                float a = (float)i * 3.14159f / 3.f;
                dl->AddLine(
                    ImVec2(c.x + cosf(a) * (r + 1.f), c.y + sinf(a) * (r + 1.f)),
                    ImVec2(c.x + cosf(a) * (r + 4.f), c.y + sinf(a) * (r + 4.f)), col, 1.4f);
            }
            break;
        case 6:
            dl->AddRectFilled(ImVec2(c.x - 5.f * s, c.y - 6.f * s), ImVec2(c.x + 1.f * s, c.y), col, 1.f);
            dl->AddRectFilled(ImVec2(c.x + 2.f * s, c.y - 3.f * s), ImVec2(c.x + 6.f * s, c.y + 3.f * s), col, 1.f);
            break;
        case 7:
            for (int i = 0; i < 3; i++)
                dl->AddRectFilled(
                    ImVec2(c.x - 6.f * s, c.y - 5.f * s + i * 4.f * s),
                    ImVec2(c.x + 6.f * s, c.y - 2.f * s + i * 4.f * s), col, 1.5f);
            break;
        case 8:
            dl->AddCircleFilled(ImVec2(c.x - 2.f * s, c.y + 4.f * s), 2.5f * s, col);
            dl->AddLine(ImVec2(c.x, c.y + 4.f * s), ImVec2(c.x, c.y - 6.f * s), col, 1.6f);
            dl->AddLine(ImVec2(c.x, c.y - 6.f * s), ImVec2(c.x + 5.f * s, c.y - 4.f * s), col, 1.6f);
            break;
        default:
            dl->AddCircle(c, r, col, 0, 1.4f);
            break;
        }
    }

    inline void DrawGridPattern(ImDrawList* dl, ImVec2 min, ImVec2 max, float step = 40.f) {
        const ImU32 gridCol = IM_COL32(255, 255, 255, 8);
        for (float x = min.x; x < max.x; x += step)
            dl->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), gridCol, 1.f);
        for (float y = min.y; y < max.y; y += step)
            dl->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), gridCol, 1.f);
    }

    inline void DrawMwByteChrome(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        const float shellA = 220;
        dl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + wh), IM_COL32(8, 12, 20, shellA), 14.f);
        dl->AddRect(wp, ImVec2(wp.x + ww, wp.y + wh), IM_COL32(255, 255, 255, 20), 14.f, 0, 1.f);
        dl->AddRectFilledMultiColor(
            wp, ImVec2(wp.x + ww, wp.y + wh),
            IM_COL32(255, 255, 255, 12), IM_COL32(255, 255, 255, 8),
            IM_COL32(255, 255, 255, 4), IM_COL32(255, 255, 255, 10));
    }

    inline void DrawMwByteBrand(ImDrawList* dl, ImVec2 wp) {
        const float bx = wp.x + kMwPad;
        const float by = wp.y + 16.f;
        const float logoSize = 34.f;
        ImVec2 logoMin(bx, by);
        ImVec2 logoMax(bx + logoSize, by + logoSize);
        dl->AddRectFilled(logoMin, logoMax, FrontierUI::AccentU32(0.95f), 8.f);
        dl->AddRectFilledMultiColor(logoMin, logoMax,
            IM_COL32(255, 255, 255, 40), IM_COL32(255, 255, 255, 10),
            IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 16));
        if (BrandAssets::logoSrv && BrandAssets::logoW > 0) {
            DrawBrandMark(dl, ImVec2(bx + logoSize * 0.5f, by + logoSize * 0.5f), 22.f);
        } else {
            ImFont* font = ImGui::GetFont();
            const char* t = "A";
            ImVec2 ts = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, t);
            dl->AddText(font, 16.f,
                ImVec2(bx + logoSize * 0.5f - ts.x * 0.5f, by + logoSize * 0.5f - ts.y * 0.5f),
                IM_COL32(4, 16, 24, 255), t);
        }

        const float tx = bx + logoSize + 10.f;
        dl->AddText(ImVec2(tx, by + 2.f), IM_COL32(244, 247, 251, 255), Frontier::kName);
        char verLine[48];
        sprintf_s(verLine, "%s · ahead", Frontier::kVersion);
        dl->AddText(ImVec2(tx, by + 20.f), Gray500(), verLine);
    }

    inline void DrawTopTabBar(ImDrawList* dl, ImVec2 wp, float ww, int* selected) {
        const TabItem* tabs = Tabs();
        const float tabY = wp.y + 14.f;
        const float tabStartX = wp.x + 190.f;
        const float tabEndX = wp.x + ww - kMwPad;
        float x = tabStartX;
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;

        for (int i = 0; i < TabCount(); i++) {
            ImVec2 labelSz = ImGui::CalcTextSize(tabs[i].label);
            const float tabW = labelSz.x + 44.f;
            if (x + tabW > tabEndX)
                break;

            ImVec2 a(x, tabY);
            ImVec2 b(x + tabW, tabY + kMwTabH);
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(300 + i);
            if (ImGui::InvisibleButton("##tab", ImVec2(b.x - a.x, b.y - a.y))) {
                if (*selected != i) {
                    *selected = i;
                    variables::selectedSub = variables::Misc::selectedSubByTab[i];
                    UIMotion::NotifyTabChanged(i);
                }
            }
            bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            const bool on = (*selected == i);
            UIMotion::SetNavHoverTarget(i, hov || on, dt);

            if (on) {
                dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 16), 8.f);
                dl->AddRect(a, b, FrontierUI::AccentU32(0.35f), 8.f, 0, 1.f);
            } else if (hov) {
                dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 8), 8.f);
            }

            DrawNavIcon(dl, ImVec2(a.x + 16.f, (a.y + b.y) * 0.5f), tabs[i].iconKind,
                on ? AccentCol(1.f) : (hov ? IM_COL32(210, 215, 225, 255) : Gray500()), 0.85f);
            dl->AddText(ImVec2(a.x + 28.f, a.y + (kMwTabH - labelSz.y) * 0.5f),
                on ? IM_COL32(244, 247, 251, 255) : (hov ? IM_COL32(220, 225, 235, 255) : Gray500()),
                tabs[i].label);

            x += tabW + 6.f;
        }

        const float sepY = wp.y + kMwTitleH - 1.f;
        dl->AddLine(ImVec2(wp.x + kMwPad, sepY), ImVec2(wp.x + ww - kMwPad, sepY),
            IM_COL32(255, 255, 255, 20));
    }

    inline void DrawMwByteFooter(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        const float y = wp.y + wh - kMwFooterH + 8.f;
        dl->AddCircleFilled(ImVec2(wp.x + kMwPad + 4.f, y + 6.f), 3.f, AccentCol(1.f), 12);
        dl->AddText(ImVec2(wp.x + kMwPad + 14.f, y), AccentCol(0.9f), "UNDETECTED");

        char fps[32];
        sprintf_s(fps, "FPS %d", variables::Perf::currentFps > 0 ? variables::Perf::currentFps : 0);
        ImVec2 fpsSz = ImGui::CalcTextSize(fps);
        dl->AddText(ImVec2(wp.x + ww - fpsSz.x - kMwPad, y), Gray400(), fps);

        char hint[48];
        sprintf_s(hint, "%s hide", FrontierUI::KeyName(variables::Misc::menuVk));
        ImVec2 hintSz = ImGui::CalcTextSize(hint);
        dl->AddText(ImVec2(wp.x + ww - fpsSz.x - hintSz.x - kMwPad - 16.f, y), Gray500(), hint);
    }

    inline void DrawWindowChrome(ImDrawList* dl, ImVec2 wp, float ww, float wh, bool lite = false) {
        if (UseTopTabs() && !lite) {
            DrawMwByteChrome(dl, wp, ww, wh);
            return;
        }
        const bool premium = variables::Theme::preset == 7;
        const ImU32 shellBg = premium ? IM_COL32(8, 12, 20, 252) : IM_COL32(10, 10, 10, 252);
        dl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + wh), shellBg, 12.f);
        dl->AddRect(wp, ImVec2(wp.x + ww, wp.y + wh), IM_COL32(255, 255, 255, premium ? 18 : 12), 12.f, 0, 1.f);

        const float footY = wp.y + wh - kFooterH;
        const ImVec2 railEnd(wp.x + kSidebarW, footY);
        if (premium && !lite) {
            dl->AddRectFilledMultiColor(wp, railEnd,
                IM_COL32(14, 20, 34, 255), IM_COL32(10, 14, 24, 255),
                IM_COL32(8, 12, 20, 255), IM_COL32(12, 18, 30, 255));
            dl->AddRectFilled(ImVec2(wp.x + kSidebarW - 1.f, wp.y + 8.f),
                ImVec2(wp.x + kSidebarW + 1.f, footY - 4.f),
                FrontierUI::AccentU32(0.35f), 1.f);
        } else {
            dl->AddRectFilled(wp, railEnd, IM_COL32(30, 30, 30, 255), 12.f, ImDrawFlags_RoundCornersTopLeft);
        }
        dl->AddLine(
            ImVec2(wp.x + kSidebarW, wp.y + 8.f), ImVec2(wp.x + kSidebarW, footY - 4.f),
            IM_COL32(255, 255, 255, premium ? 18 : 12));

        const ImVec2 mainMin(wp.x + kSidebarW, wp.y);
        const ImVec2 mainMax(wp.x + ww, footY);
        if (lite) {
            dl->AddRectFilled(mainMin, mainMax, IM_COL32(38, 38, 38, 242));
        } else {
            dl->AddRectFilledMultiColor(
                mainMin, mainMax,
                IM_COL32(42, 42, 42, 242), IM_COL32(38, 38, 38, 242),
                IM_COL32(36, 36, 36, 242), IM_COL32(40, 40, 40, 242));
            DrawGridPattern(dl, ImVec2(mainMin.x + 1.f, wp.y + kHeaderH + 1.f), mainMax);
        }

        dl->AddRectFilled(
            ImVec2(wp.x + kSidebarW, wp.y), ImVec2(wp.x + ww, wp.y + kHeaderH),
            IM_COL32(42, 42, 42, 128));
        dl->AddLine(
            ImVec2(wp.x + kSidebarW, wp.y + kHeaderH), ImVec2(wp.x + ww, wp.y + kHeaderH),
            IM_COL32(255, 255, 255, 12));

        dl->AddRectFilled(
            ImVec2(wp.x, footY), ImVec2(wp.x + ww, wp.y + wh),
            IM_COL32(30, 30, 30, 255), 0.f, ImDrawFlags_RoundCornersBottom);
        dl->AddLine(ImVec2(wp.x + kSidebarW, footY), ImVec2(wp.x + ww, footY), IM_COL32(255, 255, 255, 12));
    }

    inline void DrawIconRail(ImDrawList* dl, ImVec2 wp, float wh, int* selected, bool lite = false) {
        const float sbW = kSidebarW;
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;

        DrawBrandMark(dl, ImVec2(wp.x + sbW * 0.5f, wp.y + 32.f), 28.f);

        const TabItem* tabs = Tabs();
        float navY = wp.y + kGhostAreaH;
        const float cx = wp.x + sbW * 0.5f;
        const float navBottom = wp.y + wh - kFooterH - 22.f;

        for (int i = 0; i < TabCount(); i++) {
            if (navY + kNavItemH > navBottom)
                break;

            ImVec2 a(wp.x + 2.f, navY);
            ImVec2 b(wp.x + sbW - 2.f, navY + kNavItemH);
            bool on = (*selected == i);

            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(200 + i);
            if (ImGui::InvisibleButton("##nav", ImVec2(b.x - a.x, b.y - a.y))) {
                if (*selected != i) {
                    *selected = i;
                    variables::selectedSub = variables::Misc::selectedSubByTab[i];
                    UIMotion::NotifyTabChanged(i);
                }
            }
            bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            UIMotion::SetNavHoverTarget(i, hov || on, dt);
            const float hb = lite ? (on ? 1.f : 0.f) : UIMotion::NavHover(i);

            if (on) {
                dl->AddRectFilled(
                    ImVec2(a.x, a.y + 6.f), ImVec2(a.x + 2.f, b.y - 6.f),
                    AccentCol(1.f), 1.f);
                dl->AddRectFilled(
                    ImVec2(a.x - 1.f, a.y + 4.f), ImVec2(a.x + 4.f, b.y - 4.f),
                    AccentCol(0.12f), 2.f);
            }

            ImU32 icCol = on ? AccentCol(1.f)
                : IM_COL32(
                    (int)(107 + hb * 50), (int)(114 + hb * 45), (int)(128 + hb * 40),
                    (int)(200 + hb * 55));
            DrawNavIcon(dl, ImVec2(cx, a.y + kNavItemH * 0.5f), tabs[i].iconKind, icCol, 1.f);
            navY += kNavItemH + kNavGap;
        }

        dl->AddCircleFilled(
            ImVec2(wp.x + sbW * 0.5f, wp.y + wh - kFooterH - 16.f),
            3.f, AccentCol(0.9f), 12);
    }

    inline void DrawTopHeader(ImDrawList* dl, ImVec2 wp, float ww, int /*tab*/) {
        const float x0 = wp.x + kSidebarW + 32.f;
        const float y0 = wp.y + 18.f;
        const bool premium = variables::Theme::preset == 7;

        const char* title = Frontier::kName;
        ImVec2 titlePos(x0, y0);
        if (premium) {
            dl->AddText(ImVec2(titlePos.x + 1.f, titlePos.y + 1.f), FrontierUI::AccentU32(0.18f), title);
            dl->AddText(titlePos, IM_COL32(255, 255, 255, 255), title);
        } else {
            dl->AddText(ImVec2(titlePos.x + 1.f, titlePos.y + 1.f), AccentCol(0.12f), title);
            dl->AddText(titlePos, IM_COL32(255, 255, 255, 255), title);
        }

        ImVec2 nameSz = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(x0 + nameSz.x + 6.f, y0 - 1.f), AccentCol(0.85f), Frontier::kVersion);

        dl->AddText(ImVec2(x0, y0 + 22.f), Gray500(), Frontier::kTagline);

        const char* user = variables::Status::displayName[0] && variables::Status::displayName[0] != '—'
            ? variables::Status::displayName
            : (variables::Status::username[0] && variables::Status::username[0] != '—'
                ? variables::Status::username : "Guest");
        char userLine[80];
        sprintf_s(userLine, "User: %s", user);

        const float badgeW = ImGui::CalcTextSize(userLine).x + 36.f;
        ImVec2 badgeMin(wp.x + ww - badgeW - 32.f, wp.y + 18.f);
        ImVec2 badgeMax(badgeMin.x + badgeW, badgeMin.y + 28.f);
        dl->AddRectFilled(badgeMin, badgeMax, IM_COL32(30, 30, 30, 255), 6.f);
        dl->AddRect(badgeMin, badgeMax, IM_COL32(255, 255, 255, 12), 6.f);
        dl->AddCircleFilled(ImVec2(badgeMin.x + 14.f, (badgeMin.y + badgeMax.y) * 0.5f), 4.f, AccentCol(0.9f), 12);
        dl->AddText(ImVec2(badgeMin.x + 24.f, badgeMin.y + 6.f), IM_COL32(210, 210, 215, 255), userLine);
    }

    inline void DrawStatusFooter(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        const float y = wp.y + wh - kFooterH + 12.f;

        dl->AddCircleFilled(ImVec2(wp.x + kSidebarW + 24.f, y + 5.f), 3.f, AccentCol(1.f), 12);
        dl->AddText(ImVec2(wp.x + kSidebarW + 34.f, y), AccentCol(0.95f), "UNDETECTED");

        char build[64];
        sprintf_s(build, "Build: %s", Frontier::kVersion);
        ImVec2 buildSz = ImGui::CalcTextSize(build);
        dl->AddLine(
            ImVec2(wp.x + kSidebarW + 130.f, y + 2.f),
            ImVec2(wp.x + kSidebarW + 130.f, y + buildSz.y + 2.f),
            IM_COL32(255, 255, 255, 25));
        dl->AddText(ImVec2(wp.x + kSidebarW + 140.f, y), Gray500(), build);

        char hint[48];
        sprintf_s(hint, "%s to hide", FrontierUI::KeyName(variables::Misc::menuVk));
        ImVec2 hintSz = ImGui::CalcTextSize(hint);
        dl->AddText(ImVec2(wp.x + ww - hintSz.x - 24.f, y), Gray500(), hint);

        char fps[32];
        sprintf_s(fps, "FPS: %d", variables::Perf::currentFps > 0 ? variables::Perf::currentFps : 0);
        ImVec2 fpsSz = ImGui::CalcTextSize(fps);
        dl->AddText(ImVec2(wp.x + ww - hintSz.x - fpsSz.x - 40.f, y), Gray400(), fps);

        const char* ping = "Ping: --";
        ImVec2 pingSz = ImGui::CalcTextSize(ping);
        dl->AddText(ImVec2(wp.x + ww - hintSz.x - fpsSz.x - pingSz.x - 56.f, y), Gray500(), ping);
    }

    inline void DrawHeader(ImDrawList* dl, ImVec2 wp, float ww, float wh, int* selected, bool lite = false) {
        DrawWindowChrome(dl, wp, ww, wh, lite);
        if (UseTopTabs() && !lite) {
            DrawMwByteBrand(dl, wp);
            DrawTopTabBar(dl, wp, ww, selected);
            DrawMwByteFooter(dl, wp, ww, wh);
            return;
        }
        DrawIconRail(dl, wp, wh, selected, lite);
        if (!lite) {
            DrawTopHeader(dl, wp, ww, *selected);
            DrawStatusFooter(dl, wp, ww, wh);
        }
    }

    inline ImVec2 ContentOrigin(ImVec2 wp) {
        if (UseTopTabs())
            return ImVec2(wp.x + kMwPad, wp.y + kMwTitleH + 12.f);
        return ImVec2(wp.x + kSidebarW + 32.f, wp.y + kHeaderH + 24.f);
    }

    inline ImVec2 ContentSize(float ww, float wh) {
        if (UseTopTabs())
            return ImVec2(ww - kMwPad * 2.f, wh - kMwTitleH - kMwFooterH - 20.f);
        return ImVec2(ww - kSidebarW - 64.f, wh - kHeaderH - kFooterH - 36.f);
    }

    inline float DragZoneWidth(float ww) {
        return UseTopTabs() ? ww : kSidebarW;
    }

    inline float DragZoneHeight() {
        return UseTopTabs() ? kMwTitleH : kGhostAreaH;
    }
}
