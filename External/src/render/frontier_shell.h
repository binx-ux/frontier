#pragma once
#include <cctype>
#include <cstdio>
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"
#include "../core/updater/updater.h"
#include "brand.h"
#include "brand_assets.h"
#include "frontier_theme.h"
#include "frontier_ui.h"

namespace FrontierShell {

    inline constexpr float kSidebarW = 270.f;
    inline constexpr float kBrandH = 58.f;
    inline constexpr float kFooterH = 23.f;
    inline constexpr float kTopH = 34.f;
    inline constexpr float kNavItemH = 36.f;

    struct TabItem {
        const char* label;
        int iconKind;
    };

    struct NavSection {
        const char* title;
        int tabs[8];
        int count;
    };

    inline const TabItem* Tabs() {
        static const TabItem kTabs[] = {
            { "Aimbot", 0 }, { "Players", 2 }, { "World", 3 }, { "Player", 1 },
            { "Miscellaneous", 5 }, { "Lists", 7 }, { "Music", 8 }, { "Status", 6 }, { "Configs", 9 },
        };
        return kTabs;
    }

    inline int TabCount() { return 9; }

    inline const NavSection* Sections(int* outCount) {
        static const NavSection kSections[] = {
            { "Player", { 0, 3 }, 2 },
            { "Visuals", { 1, 2 }, 2 },
            { "Miscellaneous", { 5, 4, 6, 7, 8 }, 5 },
        };
        *outCount = 3;
        return kSections;
    }

    inline const char* TabLabel(int tab) {
        const TabItem* tabs = Tabs();
        if (tab < 0 || tab >= TabCount()) return "";
        return tabs[tab].label;
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
            for (int i = 0; i < 3; i++) {
                float y = c.y - 4.f * s + i * 4.f * s;
                dl->AddLine(ImVec2(c.x - 6.f * s, y), ImVec2(c.x + 6.f * s, y), col, 1.2f);
                dl->AddCircleFilled(ImVec2(c.x + (i - 1) * 2.5f * s, y), 2.f * s, col);
            }
            break;
        case 3:
            dl->AddCircle(c, r + 1.f, col, 0, 1.3f);
            dl->AddBezierCubic(
                ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.3f, c.y - r - 2.f),
                ImVec2(c.x + r * 0.3f, c.y + r + 2.f), ImVec2(c.x + r, c.y), col, 1.2f);
            dl->AddLine(ImVec2(c.x, c.y - r - 1.f), ImVec2(c.x, c.y + r + 1.f), col, 1.2f);
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
            dl->AddLine(ImVec2(c.x - 2.f * s, c.y - 3.f * s), ImVec2(c.x + 2.f * s, c.y), col, 1.2f);
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
            dl->AddLine(ImVec2(c.x, c.y - 2.f * s), ImVec2(c.x, c.y + 1.f * s), col, 1.6f);
            dl->AddCircleFilled(ImVec2(c.x, c.y + 4.f * s), 1.5f * s, col);
            break;
        }
    }

    inline void DrawWindowChrome(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        dl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + wh), IM_COL32(8, 8, 10, 252), 8.f);
        dl->AddRectFilledMultiColor(
            wp, ImVec2(wp.x + ww, wp.y + wh),
            IM_COL32(14, 14, 17, 255), IM_COL32(10, 10, 12, 255),
            IM_COL32(11, 11, 14, 255), IM_COL32(8, 8, 10, 255));

        dl->AddCircleFilled(
            ImVec2(wp.x + kSidebarW + 120.f, wp.y + 40.f), 80.f,
            FrontierUI::U32(variables::Theme::brand, 0.06f), 48);

        dl->AddLine(
            ImVec2(wp.x + kSidebarW, wp.y + 6.f),
            ImVec2(wp.x + kSidebarW, wp.y + wh - kFooterH - 4.f),
            IM_COL32(255, 255, 255, 22));

        float footY = wp.y + wh - kFooterH;
        dl->AddRectFilled(
            ImVec2(wp.x, footY), ImVec2(wp.x + ww, wp.y + wh),
            IM_COL32(12, 12, 14, 255), 0.f, ImDrawFlags_RoundCornersBottom);
        ImVec2 brandSz = ImGui::CalcTextSize(Frontier::kName);
        dl->AddText(
            ImVec2(wp.x + ww * 0.5f - brandSz.x * 0.5f, footY + 4.f),
            IM_COL32(255, 255, 255, 120), Frontier::kName);
    }

    inline void DrawSidebar(ImDrawList* dl, ImVec2 wp, float wh, int* selected) {
        const float sbW = kSidebarW;
        ImVec2 sbEnd(wp.x + sbW, wp.y + wh - kFooterH);

        dl->AddRectFilled(wp, sbEnd, IM_COL32(10, 10, 11, 240), 7.f, ImDrawFlags_RoundCornersTopLeft);

        if (BrandAssets::logoSrv && BrandAssets::logoW > 0 && BrandAssets::logoH > 0) {
            float aspect = (float)BrandAssets::logoW / (float)BrandAssets::logoH;
            float drawH = 34.f;
            float drawW = drawH * aspect;
            ImVec2 logoMin(wp.x + 22.f, wp.y + 12.f);
            dl->AddImage(BrandAssets::LogoTex(), logoMin, ImVec2(logoMin.x + drawW, logoMin.y + drawH));
        } else {
            dl->AddText(ImVec2(wp.x + 22.f, wp.y + 16.f), FrontierUI::AccentU32(0.95f), Frontier::kName);
        }

        const TabItem* tabs = Tabs();
        int secCount = 0;
        const NavSection* sections = Sections(&secCount);
        float navY = wp.y + kBrandH + 4.f;
        const float navLeft = wp.x + 14.f;
        const float navRight = wp.x + sbW - 10.f;

        for (int s = 0; s < secCount; s++) {
            dl->AddText(ImVec2(navLeft + 4.f, navY + 2.f), IM_COL32(255, 255, 255, 210), sections[s].title);
            navY += 22.f;

            for (int ti = 0; ti < sections[s].count; ti++) {
                int i = sections[s].tabs[ti];
                if (i < 0 || i >= TabCount()) continue;

                ImVec2 a(navLeft, navY);
                ImVec2 b(navRight, navY + kNavItemH);
                bool on = (*selected == i);

                ImGui::SetCursorScreenPos(a);
                ImGui::PushID(200 + i);
                if (ImGui::InvisibleButton("##nav", ImVec2(b.x - a.x, b.y - a.y)))
                    *selected = i;
                bool hov = ImGui::IsItemHovered();
                ImGui::PopID();

                if (on) {
                    dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 8.f);
                    dl->AddRectFilled(
                        ImVec2(a.x - 8.f, a.y + 6.f), ImVec2(a.x - 4.f, b.y - 6.f),
                        FrontierUI::AccentU32(1.f), 2.f);
                    dl->AddRect(a, b, FrontierUI::AccentSoftU32(0.35f), 8.f, 0, 1.f);
                } else if (hov) {
                    dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 8), 8.f);
                }

                ImU32 icCol = on ? FrontierUI::AccentU32(1.f)
                    : (hov ? IM_COL32(230, 230, 235, 230) : IM_COL32(255, 255, 255, 190));
                DrawNavIcon(dl, ImVec2(a.x + 16.f, a.y + kNavItemH * 0.5f), tabs[i].iconKind, icCol, 0.95f);

                ImU32 txCol = on ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 220);
                dl->AddText(ImVec2(a.x + 34.f, a.y + kNavItemH * 0.5f - 7.f), txCol, tabs[i].label);

                navY += kNavItemH;
            }
            navY += 6.f;
        }
    }

    inline void DrawHeader(ImDrawList* dl, ImVec2 wp, float ww, float wh, int* selected) {
        DrawWindowChrome(dl, wp, ww, wh);
        DrawSidebar(dl, wp, wh, selected);
    }

    inline ImVec2 ContentOrigin(ImVec2 wp) {
        return ImVec2(wp.x + kSidebarW + 14.f, wp.y + 12.f);
    }

    inline ImVec2 ContentSize(float ww, float wh) {
        return ImVec2(ww - kSidebarW - 28.f, wh - kFooterH - 20.f);
    }
}
