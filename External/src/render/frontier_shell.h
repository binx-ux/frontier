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

namespace FrontierShell {

    inline constexpr float kSidebarW = 80.f;
    inline constexpr float kHeaderH = 64.f;
    inline constexpr float kFooterH = 40.f;
    inline constexpr float kNavItemH = 36.f;
    inline constexpr float kNavGap = 10.f;
    inline constexpr float kGhostAreaH = 64.f;

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
            { "Aimbot", 0 }, { "Visuals", 2 }, { "World", 3 }, { "Player", 1 },
            { "Miscellaneous", 5 }, { "Servers", 7 }, { "Music", 8 }, { "Status", 6 }, { "Configs", 9 },
        };
        return kTabs;
    }

    inline int TabCount() { return 9; }

    inline const char* TabLabel(int tab) {
        const TabItem* tabs = Tabs();
        if (tab < 0 || tab >= TabCount()) return "";
        return tabs[tab].label;
    }

    inline void DrawGhostIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        const float r = 10.f * s;
        dl->AddCircleFilled(ImVec2(c.x - 4.f * s, c.y - 2.f * s), 2.2f * s, col);
        dl->AddCircleFilled(ImVec2(c.x + 4.f * s, c.y - 2.f * s), 2.2f * s, col);
        dl->AddBezierCubic(
            ImVec2(c.x - r, c.y - 2.f * s), ImVec2(c.x - r * 0.5f, c.y - r - 4.f * s),
            ImVec2(c.x + r * 0.5f, c.y - r - 4.f * s), ImVec2(c.x + r, c.y - 2.f * s),
            col, 1.6f * s);
        dl->AddLine(ImVec2(c.x - r + 2.f * s, c.y + r * 0.2f), ImVec2(c.x - r + 2.f * s, c.y + r + 2.f * s), col, 1.4f * s);
        dl->AddLine(ImVec2(c.x, c.y + r * 0.35f), ImVec2(c.x, c.y + r + 2.f * s), col, 1.4f * s);
        dl->AddLine(ImVec2(c.x + r - 2.f * s, c.y + r * 0.2f), ImVec2(c.x + r - 2.f * s, c.y + r + 2.f * s), col, 1.4f * s);
        dl->AddCircleFilled(c, r + 6.f * s, AccentCol(0.08f), 24);
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

    inline void DrawWindowChrome(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        dl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + wh), IM_COL32(10, 10, 10, 252), 12.f);
        dl->AddRect(wp, ImVec2(wp.x + ww, wp.y + wh), IM_COL32(255, 255, 255, 12), 12.f, 0, 1.f);

        const float footY = wp.y + wh - kFooterH;
        const ImVec2 railEnd(wp.x + kSidebarW, footY);
        dl->AddRectFilled(wp, railEnd, IM_COL32(30, 30, 30, 255), 12.f, ImDrawFlags_RoundCornersTopLeft);
        dl->AddLine(
            ImVec2(wp.x + kSidebarW, wp.y + 8.f), ImVec2(wp.x + kSidebarW, footY - 4.f),
            IM_COL32(255, 255, 255, 12));

        const ImVec2 mainMin(wp.x + kSidebarW, wp.y);
        const ImVec2 mainMax(wp.x + ww, footY);
        dl->AddRectFilledMultiColor(
            mainMin, mainMax,
            IM_COL32(42, 42, 42, 242), IM_COL32(38, 38, 38, 242),
            IM_COL32(36, 36, 36, 242), IM_COL32(40, 40, 40, 242));
        dl->AddCircleFilled(ImVec2(wp.x + ww - 60.f, wp.y + 30.f), 100.f, AccentCol(0.05f), 48);
        DrawGridPattern(dl, ImVec2(mainMin.x + 1.f, wp.y + kHeaderH + 1.f), mainMax);

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

    inline void DrawIconRail(ImDrawList* dl, ImVec2 wp, float wh, int* selected) {
        const float sbW = kSidebarW;
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;

        DrawGhostIcon(dl, ImVec2(wp.x + sbW * 0.5f, wp.y + 32.f), 1.05f, AccentCol(0.95f));

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
            const float hb = UIMotion::NavHover(i);

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
            4.f, AccentCol(0.9f), 16);
        dl->AddCircleFilled(
            ImVec2(wp.x + sbW * 0.5f, wp.y + wh - kFooterH - 16.f),
            8.f, AccentCol(0.15f), 16);
    }

    inline void DrawTopHeader(ImDrawList* dl, ImVec2 wp, float ww, int /*tab*/) {
        const float x0 = wp.x + kSidebarW + 32.f;
        const float y0 = wp.y + 18.f;

        const char* title = Frontier::kName;
        ImVec2 titlePos(x0, y0);
        dl->AddText(ImVec2(titlePos.x + 1.f, titlePos.y + 1.f), AccentCol(0.12f), title);
        dl->AddText(titlePos, IM_COL32(255, 255, 255, 255), title);

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
        dl->AddCircleFilled(ImVec2(wp.x + kSidebarW + 24.f, y + 5.f), 6.f, AccentCol(0.2f), 12);
        dl->AddText(ImVec2(wp.x + kSidebarW + 34.f, y), AccentCol(0.95f), "UNDETECTED");

        char build[64];
        sprintf_s(build, "Build: %s", Frontier::kVersion);
        ImVec2 buildSz = ImGui::CalcTextSize(build);
        dl->AddLine(
            ImVec2(wp.x + kSidebarW + 130.f, y + 2.f),
            ImVec2(wp.x + kSidebarW + 130.f, y + buildSz.y + 2.f),
            IM_COL32(255, 255, 255, 25));
        dl->AddText(ImVec2(wp.x + kSidebarW + 140.f, y), Gray500(), build);

        const char* hint = "INSERT to hide";
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

    inline void DrawHeader(ImDrawList* dl, ImVec2 wp, float ww, float wh, int* selected) {
        DrawWindowChrome(dl, wp, ww, wh);
        DrawIconRail(dl, wp, wh, selected);
        DrawTopHeader(dl, wp, ww, *selected);
        DrawStatusFooter(dl, wp, ww, wh);
    }

    inline ImVec2 ContentOrigin(ImVec2 wp) {
        return ImVec2(wp.x + kSidebarW + 32.f, wp.y + kHeaderH + 24.f);
    }

    inline ImVec2 ContentSize(float ww, float wh) {
        return ImVec2(ww - kSidebarW - 64.f, wh - kHeaderH - kFooterH - 36.f);
    }
}
