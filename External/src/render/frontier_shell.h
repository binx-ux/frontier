#pragma once
#include <cctype>
#include <cstdio>
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"
#include "../core/updater/updater.h"
#include "brand.h"
#include "frontier_theme.h"
#include "frontier_ui.h"

namespace FrontierShell {

    inline constexpr float kSidebarW = 196.f;
    inline constexpr float kBrandH = 72.f;
    inline constexpr float kUserH = 58.f;
    inline constexpr float kFootH = 0.f;
    inline constexpr float kTopH = 0.f;
    inline constexpr float kNavItemH = 38.f;

    struct TabItem {
        const char* label;
        int iconKind; // 0 crosshair 1 move 2 sliders 3 world 4 char 5 gear 6 explore 7 servers 8 music 9 status
    };

    inline const TabItem* Tabs() {
        static const TabItem kTabs[] = {
            { "Combat", 0 }, { "Visuals", 2 }, { "World", 3 }, { "Character", 1 },
            { "Options", 5 }, { "Explorer", 6 }, { "Servers", 7 }, { "Music", 8 }, { "Status", 9 },
        };
        return kTabs;
    }

    inline int TabCount() { return 9; }

    inline void DrawNavIcon(ImDrawList* dl, ImVec2 c, int kind, ImU32 col, float s = 1.f) {
        const float r = 5.f * s;
        switch (kind) {
        case 0: // crosshair
            dl->AddCircle(c, r + 2.f, col, 0, 1.4f);
            dl->AddLine(ImVec2(c.x - r - 3.f, c.y), ImVec2(c.x + r + 3.f, c.y), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y - r - 3.f), ImVec2(c.x, c.y + r + 3.f), col, 1.4f);
            break;
        case 1: // move / character
            dl->AddCircleFilled(ImVec2(c.x, c.y - 5.f * s), 3.f * s, col);
            dl->AddLine(ImVec2(c.x, c.y - 2.f * s), ImVec2(c.x, c.y + 5.f * s), col, 1.5f);
            dl->AddLine(ImVec2(c.x - 5.f * s, c.y + 1.f * s), ImVec2(c.x + 5.f * s, c.y + 1.f * s), col, 1.5f);
            break;
        case 2: // sliders / visuals
            for (int i = 0; i < 3; i++) {
                float y = c.y - 4.f * s + i * 4.f * s;
                dl->AddLine(ImVec2(c.x - 6.f * s, y), ImVec2(c.x + 6.f * s, y), col, 1.2f);
                dl->AddCircleFilled(ImVec2(c.x + (i - 1) * 2.5f * s, y), 2.f * s, col);
            }
            break;
        case 3: // globe / world
            dl->AddCircle(c, r + 1.f, col, 0, 1.3f);
            dl->AddBezierCubic(
                ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.3f, c.y - r - 2.f),
                ImVec2(c.x + r * 0.3f, c.y + r + 2.f), ImVec2(c.x + r, c.y), col, 1.2f);
            dl->AddLine(ImVec2(c.x, c.y - r - 1.f), ImVec2(c.x, c.y + r + 1.f), col, 1.2f);
            break;
        case 5: // gear / options
            dl->AddCircle(c, r, col, 8, 1.3f);
            for (int i = 0; i < 6; i++) {
                float a = (float)i * 3.14159f / 3.f;
                dl->AddLine(
                    ImVec2(c.x + cosf(a) * (r + 1.f), c.y + sinf(a) * (r + 1.f)),
                    ImVec2(c.x + cosf(a) * (r + 4.f), c.y + sinf(a) * (r + 4.f)), col, 1.4f);
            }
            break;
        case 6: // explorer / tree
            dl->AddRectFilled(ImVec2(c.x - 5.f * s, c.y - 6.f * s), ImVec2(c.x + 1.f * s, c.y), col, 1.f);
            dl->AddRectFilled(ImVec2(c.x + 2.f * s, c.y - 3.f * s), ImVec2(c.x + 6.f * s, c.y + 3.f * s), col, 1.f);
            dl->AddLine(ImVec2(c.x - 2.f * s, c.y - 3.f * s), ImVec2(c.x + 2.f * s, c.y), col, 1.2f);
            break;
        case 7: // servers / list
            for (int i = 0; i < 3; i++)
                dl->AddRectFilled(
                    ImVec2(c.x - 6.f * s, c.y - 5.f * s + i * 4.f * s),
                    ImVec2(c.x + 6.f * s, c.y - 2.f * s + i * 4.f * s), col, 1.5f);
            break;
        case 8: // music / note
            dl->AddCircleFilled(ImVec2(c.x - 2.f * s, c.y + 4.f * s), 2.5f * s, col);
            dl->AddLine(ImVec2(c.x, c.y + 4.f * s), ImVec2(c.x, c.y - 6.f * s), col, 1.6f);
            dl->AddLine(ImVec2(c.x, c.y - 6.f * s), ImVec2(c.x + 5.f * s, c.y - 4.f * s), col, 1.6f);
            break;
        default: // status / info
            dl->AddCircle(c, r, col, 0, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y - 2.f * s), ImVec2(c.x, c.y + 1.f * s), col, 1.6f);
            dl->AddCircleFilled(ImVec2(c.x, c.y + 4.f * s), 1.5f * s, col);
            break;
        }
    }

    inline void DrawSidebar(ImDrawList* dl, ImVec2 wp, float wh, int* selected) {
        const float sbW = kSidebarW;
        const float sbH = wh;
        ImVec2 sbEnd(wp.x + sbW, wp.y + sbH);

        dl->AddRectFilled(wp, sbEnd, IM_COL32(14, 14, 16, 255), 0.f);
        dl->AddLine(ImVec2(sbEnd.x, wp.y), ImVec2(sbEnd.x, sbEnd.y),
            FrontierUI::U32(variables::Theme::border, 0.35f));

        // Brand block
        dl->AddText(ImVec2(wp.x + 18, wp.y + 16), IM_COL32(245, 245, 248, 255), Frontier::kName);
        char verLine[64];
        sprintf_s(verLine, "%s  ·  %s", Frontier::kVersion, Frontier::kBuildTag);
        dl->AddText(ImVec2(wp.x + 18, wp.y + 34), IM_COL32(120, 120, 132, 255), verLine);

        // Nav items
        const TabItem* tabs = Tabs();
        int count = TabCount();
        float navTop = wp.y + kBrandH + 8.f;
        float navBottom = wp.y + wh - kUserH - 8.f;

        for (int i = 0; i < count; i++) {
            float iy = navTop + i * kNavItemH;
            if (iy + kNavItemH > navBottom) break;

            ImVec2 a(wp.x + 10.f, iy);
            ImVec2 b(wp.x + sbW - 10.f, iy + kNavItemH - 2.f);
            bool on = (*selected == i);

            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(200 + i);
            if (ImGui::InvisibleButton("##nav", ImVec2(b.x - a.x, b.y - a.y))) {
                *selected = i;
                variables::selectedSub = variables::Misc::selectedSubByTab[i];
            }
            bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            if (on)
                dl->AddRectFilled(a, b, IM_COL32(32, 32, 36, 255), 8.f);
            else if (hov)
                dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 8), 8.f);

            ImU32 icCol = on ? IM_COL32(245, 245, 248, 255)
                : (hov ? IM_COL32(190, 190, 198, 255) : IM_COL32(110, 110, 122, 255));
            DrawNavIcon(dl, ImVec2(a.x + 18.f, a.y + (b.y - a.y) * 0.5f), tabs[i].iconKind, icCol);

            dl->AddText(ImVec2(a.x + 36.f, a.y + (b.y - a.y) * 0.5f - 7.f),
                icCol, tabs[i].label);
        }

        // User footer
        float userY = wp.y + wh - kUserH;
        dl->AddLine(ImVec2(wp.x + 14, userY), ImVec2(wp.x + sbW - 14, userY),
            FrontierUI::U32(variables::Theme::border, 0.25f));

        const char* user = variables::Status::username[0] ? variables::Status::username : "Player";
        const char* handle = variables::Status::displayName[0] ? variables::Status::displayName : user;

        dl->AddCircleFilled(ImVec2(wp.x + 28, userY + 30), 14.f, IM_COL32(38, 38, 44, 255));
        dl->AddCircle(ImVec2(wp.x + 28, userY + 30), 14.f, IM_COL32(70, 70, 80, 255), 0, 1.f);
        char initial[2] = { user[0] ? (char)toupper(user[0]) : '?', 0 };
        ImVec2 isz = ImGui::CalcTextSize(initial);
        dl->AddText(ImVec2(wp.x + 28 - isz.x * 0.5f, userY + 30 - isz.y * 0.5f),
            IM_COL32(200, 200, 210, 255), initial);

        dl->AddText(ImVec2(wp.x + 50, userY + 18), IM_COL32(230, 230, 238, 255), user);
        char handleLine[80];
        sprintf_s(handleLine, "@%s", handle);
        dl->AddText(ImVec2(wp.x + 50, userY + 34), IM_COL32(110, 110, 122, 255), handleLine);
    }

    inline void DrawHeader(ImDrawList* dl, ImVec2 wp, float ww, float wh, int* selected) {
        (void)ww;
        DrawSidebar(dl, wp, wh, selected);
    }

    inline ImVec2 ContentOrigin(ImVec2 wp) {
        return ImVec2(wp.x + kSidebarW + 14.f, wp.y + 12.f);
    }

    inline ImVec2 ContentSize(float ww, float wh) {
        return ImVec2(ww - kSidebarW - 28.f, wh - 36.f);
    }
}
