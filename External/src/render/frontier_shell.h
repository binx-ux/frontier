#pragma once
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"
#include "brand.h"
#include "frontier_theme.h"
#include "frontier_ui.h"

namespace FrontierShell {

    inline constexpr float kBrandH = 34.f;
    inline constexpr float kTabH = 34.f;
    inline constexpr float kTopH = kBrandH + kTabH;
    inline constexpr float kFootH = 28.f;
    inline constexpr float kRailW = 56.f;

    struct TabItem {
        const char* label;
        char icon;
    };

    inline const TabItem* Tabs() {
        static const TabItem kTabs[] = {
            { "Combat", 'C' }, { "Visuals", 'V' }, { "World", 'W' }, { "Move", 'M' },
            { "Setup", 'S' }, { "Explore", 'E' }, { "Servers", 'R' }, { "Audio", 'A' }, { "Status", 'I' },
        };
        return kTabs;
    }

    inline int TabCount() { return 9; }

    inline void DrawBrandRow(ImDrawList* dl, ImVec2 wp, float ww) {
        dl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + kBrandH), IM_COL32(12, 12, 12, 255));
        dl->AddRectFilled(ImVec2(wp.x, wp.y + 8), ImVec2(wp.x + 2.5f, wp.y + kBrandH - 8),
            FrontierUI::U32(variables::Theme::brand, 0.75f), 2.f);
        dl->AddLine(ImVec2(wp.x + 14, wp.y + 1), ImVec2(wp.x + ww - 14, wp.y + 1),
            IM_COL32(255, 255, 255, 18), 1.f);

        ImVec2 nameSz = ImGui::CalcTextSize(Frontier::kName);
        ImVec2 midSz = ImGui::CalcTextSize("· Interface");

        ImGui::SetCursorScreenPos(ImVec2(wp.x + 14, wp.y + 8));
        ImGui::TextColored(FrontierUI::V4(variables::Theme::brand), "%s", Frontier::kName);
        ImGui::SameLine(0, 6);
        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "· Interface");

        const char* modeLbl = FrontierTheme::LayoutLabel();
        ImVec2 modeSz = ImGui::CalcTextSize(modeLbl);
        float pillW = modeSz.x + 22.f;
        ImVec2 pillPos(wp.x + 14 + nameSz.x + midSz.x + 16, wp.y + 9);
        ImVec2 pillEnd(pillPos.x + pillW, pillPos.y + 18);
        dl->AddRect(pillPos, pillEnd, FrontierUI::U32(variables::Theme::brand, 0.85f), 9.f, 0, 1.f);
        dl->AddText(ImVec2(pillPos.x + 11, pillPos.y + 2),
            FrontierUI::U32(variables::Theme::brand, 1.f), modeLbl);

        ImGui::SetCursorScreenPos(pillPos);
        ImGui::InvisibleButton("##layoutpill", ImVec2(pillW, 18));
        if (ImGui::IsItemClicked())
            variables::Theme::layoutMode = variables::Theme::layoutMode ? 0 : 1;

        const char* user = variables::Status::username[0] ? variables::Status::username : "—";
        ImVec2 us = ImGui::CalcTextSize(user);
        dl->AddText(ImVec2(wp.x + ww - us.x - 14, wp.y + 10),
            IM_COL32(170, 170, 180, 255), user);

        if (variables::Theme::useFloatingHeader) {
            const char* fl = "Float";
            ImVec2 fs = ImGui::CalcTextSize(fl);
            float fx = wp.x + ww - us.x - fs.x - 28;
            ImGui::SetCursorScreenPos(ImVec2(fx, wp.y + 10));
            ImGui::PushStyleColor(ImGuiCol_Text, FrontierUI::V4(variables::Theme::textDim));
            if (ImGui::SmallButton(fl))
                variables::Theme::useFloatingHeader = false;
            ImGui::PopStyleColor();
        }

        dl->AddLine(ImVec2(wp.x, wp.y + kBrandH - 1), ImVec2(wp.x + ww, wp.y + kBrandH - 1),
            FrontierUI::U32(variables::Theme::border, 0.35f));
    }

    inline void DrawTabBar(ImDrawList* dl, ImVec2 wp, float ww, int* selected) {
        float y0 = wp.y + kBrandH;
        dl->AddRectFilled(ImVec2(wp.x, y0), ImVec2(wp.x + ww, y0 + kTabH), IM_COL32(10, 10, 10, 255));

        const TabItem* tabs = Tabs();
        int count = TabCount();
        ImGui::SetCursorScreenPos(ImVec2(wp.x + 10, y0 + 4));

        for (int i = 0; i < count; i++) {
            if (i) ImGui::SameLine(0, 4);
            bool on = (*selected == i);
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, on ? ImVec4(0.14f, 0.14f, 0.16f, 1) : ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.16f, 0.18f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.18f, 0.20f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, on ? FrontierUI::V4(variables::Theme::brand)
                : FrontierUI::V4(variables::Theme::textDim));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));
            if (ImGui::Button(tabs[i].label)) {
                *selected = i;
                variables::selectedSub = variables::Misc::selectedSubByTab[i];
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
            ImGui::PopID();
        }

        dl->AddLine(ImVec2(wp.x, y0 + kTabH - 1), ImVec2(wp.x + ww, y0 + kTabH - 1),
            FrontierUI::U32(variables::Theme::border, 0.35f));
    }

    inline void DrawRail(ImDrawList* dl, ImVec2 wp, float wh, int* selected) {
        float y0 = wp.y + kBrandH;
        float y1 = wp.y + wh - kFootH;
        dl->AddRectFilled(ImVec2(wp.x, y0), ImVec2(wp.x + kRailW, y1), IM_COL32(10, 10, 10, 255));
        dl->AddLine(ImVec2(wp.x + kRailW, y0), ImVec2(wp.x + kRailW, y1),
            FrontierUI::U32(variables::Theme::border, 0.45f));

        const TabItem* tabs = Tabs();
        int count = TabCount();
        float itemH = 46.f;
        float startY = y0 + 6.f;

        for (int i = 0; i < count; i++) {
            float iy = startY + i * itemH;
            if (iy + itemH > y1 - 4.f) break;

            ImVec2 a(wp.x + 4.f, iy);
            ImVec2 b(wp.x + kRailW - 4.f, iy + itemH - 4.f);
            bool on = (*selected == i);

            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(100 + i);
            if (ImGui::InvisibleButton("##rail", ImVec2(b.x - a.x, b.y - a.y))) {
                *selected = i;
                variables::selectedSub = variables::Misc::selectedSubByTab[i];
            }
            bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            if (on) {
                dl->AddRectFilled(a, b, IM_COL32(28, 28, 32, 220), 8.f);
                dl->AddRectFilled(ImVec2(a.x, a.y + 8), ImVec2(a.x + 2.f, b.y - 8),
                    FrontierUI::U32(variables::Theme::brand, 1.f), 1.f);
            } else if (hov) {
                dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 8.f);
            }

            char ic[2] = { tabs[i].icon, 0 };
            ImVec2 icSz = ImGui::CalcTextSize(ic);
            dl->AddText(ImVec2(a.x + (b.x - a.x - icSz.x) * 0.5f, a.y + 8),
                on ? FrontierUI::U32(variables::Theme::brand, 1.f)
                : IM_COL32(120, 120, 132, 255), ic);

            const char* shortLabel = tabs[i].label;
            ImVec2 ls = ImGui::CalcTextSize(shortLabel);
            float lx = a.x + (b.x - a.x - ls.x) * 0.5f;
            if (lx < a.x + 2.f) lx = a.x + 2.f;
            dl->AddText(ImVec2(lx, a.y + 26),
                on ? IM_COL32(230, 230, 238, 255) : IM_COL32(110, 110, 122, 255), shortLabel);
        }
    }

    inline void DrawHeader(ImDrawList* dl, ImVec2 wp, float ww, float wh, int* selected) {
        DrawBrandRow(dl, wp, ww);
        if (variables::Theme::layoutMode == 1)
            DrawRail(dl, wp, wh, selected);
        else
            DrawTabBar(dl, wp, ww, selected);
    }

    inline ImVec2 ContentOrigin(ImVec2 wp) {
        if (variables::Theme::layoutMode == 1)
            return ImVec2(wp.x + kRailW + 10.f, wp.y + kBrandH + 8.f);
        return ImVec2(wp.x + 12.f, wp.y + kTopH + 8.f);
    }

    inline ImVec2 ContentSize(float ww, float wh) {
        if (variables::Theme::layoutMode == 1)
            return ImVec2(ww - kRailW - 20.f, wh - kBrandH - kFootH - 16.f);
        return ImVec2(ww - 24.f, wh - kTopH - kFootH - 16.f);
    }
}
