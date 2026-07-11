#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"
#include "../core/variables/variables.h"
#include <Shellapi.h>

namespace MatchaUI {

    inline ImVec4 V4(const float c[4]) { return ImVec4(c[0], c[1], c[2], c[3]); }
    inline ImU32 U32(const float c[4], float a = -1.f) {
        float aa = (a < 0) ? c[3] : a;
        return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(aa * 255));
    }

    inline const char* KeyName(int vk) {
        switch (vk) {
        case 0: return "none";
        case VK_LBUTTON: return "lmb";
        case VK_RBUTTON: return "rmb";
        case VK_MBUTTON: return "mmb";
        case VK_XBUTTON1: return "xbutton1";
        case VK_XBUTTON2: return "xbutton2";
        case VK_SHIFT: return "shift";
        case VK_CONTROL: return "ctrl";
        case VK_MENU: return "alt";
        case VK_SPACE: return "space";
        case VK_TAB: return "tab";
        case VK_INSERT: return "ins";
        case VK_DELETE: return "del";
        case VK_RCONTROL: return "rctrl";
        case VK_LCONTROL: return "lctrl";
        default:
            if (vk >= 'A' && vk <= 'Z') { static char l[2]; l[0] = (char)(vk + 32); l[1] = 0; return l; }
            if (vk >= '0' && vk <= '9') { static char d[2]; d[0] = (char)vk; d[1] = 0; return d; }
            static char buf[16]; sprintf_s(buf, "%d", vk); return buf;
        }
    }

    inline void KeybindChip(const char* id, int* key) {
        ImGui::PushID(id);
        char label[32];
        sprintf_s(label, "%s", KeyName(*key));
        static DWORD rebindIgnoreUntil = 0;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.14f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.20f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.24f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));
        if (variables::waitingForKey && variables::keyToRebind == key) {
            if (ImGui::Button("...", ImVec2(0, 0))) {}
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                variables::waitingForKey = false; variables::keyToRebind = nullptr;
            }
            else if (GetTickCount() >= rebindIgnoreUntil) {
                for (int vk = 8; vk < 255; vk++) {
                    if (vk == VK_ESCAPE || vk == VK_LBUTTON || vk == VK_RBUTTON) continue;
                    if (GetAsyncKeyState(vk) & 0x8000) {
                        *key = vk; variables::waitingForKey = false; variables::keyToRebind = nullptr; break;
                    }
                }
                if (variables::waitingForKey) {
                    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
                        *key = VK_XBUTTON1; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                    else if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
                        *key = VK_XBUTTON2; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                }
            }
        }
        else {
            if (ImGui::Button(label, ImVec2(0, 0))) {
                variables::waitingForKey = true;
                variables::keyToRebind = key;
                rebindIgnoreUntil = GetTickCount() + 250;
            }
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }

    inline void ColorSquare(const char* id, float col[4]) {
        ImGui::PushID(id);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImVec4 c(col[0], col[1], col[2], col[3]);
        // ColorButton opens a real picker popup (hex + RGB + alpha) — ColorEdit4 NoInputs was click-stolen by row hitboxes
        if (ImGui::ColorButton("##swatch", c,
            ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip |
            ImGuiColorEditFlags_NoDragDrop, ImVec2(18, 18))) {
            ImGui::OpenPopup("##colorpopup");
        }
        if (ImGui::BeginPopup("##colorpopup", ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.08f, 0.09f, 1));
            ImGui::ColorPicker4("##picker", col,
                ImGuiColorEditFlags_DisplayHex |
                ImGuiColorEditFlags_DisplayRGB |
                ImGuiColorEditFlags_InputRGB |
                ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_AlphaPreviewHalf |
                ImGuiColorEditFlags_PickerHueBar |
                ImGuiColorEditFlags_NoSidePreview |
                ImGuiColorEditFlags_NoSmallPreview);
            ImGui::PopStyleColor(2);
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopID();
    }

    // Matcha square checkbox — left hitbox only so color/keybind stay clickable
    inline bool Checkbox(const char* label, bool* v, float* color = nullptr, int* key = nullptr) {
        ImGui::PushID(label);
        ImGui::BeginGroup();

        float avail = ImGui::GetContentRegionAvail().x;
        float rightW = 0.0f;
        if (color) rightW += 24.0f;
        if (key) rightW += 74.0f;
        float leftW = avail - rightW - 6.0f;
        if (leftW < 48.0f) leftW = 48.0f;

        ImVec2 p = ImGui::GetCursorScreenPos();
        float box = 14.0f;
        bool pressed = ImGui::InvisibleButton("##cb", ImVec2(leftW, 22.0f));
        if (pressed) *v = !*v;
        bool hovered = ImGui::IsItemHovered();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 border = IM_COL32(70, 70, 78, 255);
        ImU32 fill = *v ? IM_COL32(235, 235, 240, 255) : IM_COL32(28, 28, 32, 255);
        if (hovered && !*v) fill = IM_COL32(36, 36, 42, 255);
        dl->AddRectFilled(ImVec2(p.x, p.y + 4), ImVec2(p.x + box, p.y + 4 + box), fill, 3.0f);
        dl->AddRect(ImVec2(p.x, p.y + 4), ImVec2(p.x + box, p.y + 4 + box), border, 3.0f, 0, 1.0f);

        ImVec4 tc = *v ? V4(variables::Theme::text) : V4(variables::Theme::textDim);
        ImVec2 labelPos(p.x + box + 8, p.y + 3);
        dl->PushClipRect(labelPos, ImVec2(p.x + leftW - 2, p.y + 22), true);
        dl->AddText(labelPos, ImGui::ColorConvertFloat4ToU32(tc), label);
        dl->PopClipRect();

        if (color) {
            ImGui::SameLine(0, 4);
            ColorSquare("##col", color);
        }
        if (key) {
            ImGui::SameLine(0, 4);
            KeybindChip("##kb", key);
        }

        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::PopID();
        return pressed;
    }

    inline bool SliderFloat(const char* label, float* v, float mn, float mx, const char* fmt = "%.2f") {
        ImGui::PushID(label);
        ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x > 40 ? ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 48 : 0);
        char buf[32]; sprintf_s(buf, fmt, *v);
        ImGui::TextColored(V4(variables::Theme::text), "%s", buf);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.90f, 0.90f, 0.92f, 1));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 100.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::SetNextItemWidth(-1);
        bool ch = ImGui::SliderFloat("##s", v, mn, mx, "");
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
        ImGui::Spacing();
        ImGui::PopID();
        return ch;
    }

    inline bool SliderInt(const char* label, int* v, int mn, int mx) {
        ImGui::PushID(label);
        ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x > 40 ? ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 48 : 0);
        char buf[32]; sprintf_s(buf, "%d", *v);
        ImGui::TextColored(V4(variables::Theme::text), "%s", buf);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.90f, 0.90f, 0.92f, 1));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 100.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::SetNextItemWidth(-1);
        bool ch = ImGui::SliderInt("##si", v, mn, mx, "");
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
        ImGui::Spacing();
        ImGui::PopID();
        return ch;
    }

    inline bool Combo(const char* label, int* current, const char* const items[], int count) {
        ImGui::PushID(label);
        if (label && label[0])
            ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.09f, 0.09f, 0.10f, 1));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.16f, 0.16f, 0.18f, 1));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.18f, 0.20f, 1));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.22f, 0.25f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
        ImGui::SetNextItemWidth(-1);

        const char* preview = (*current >= 0 && *current < count) ? items[*current] : "";
        bool changed = false;
        if (ImGui::BeginCombo("##cb", preview, ImGuiComboFlags_None)) {
            for (int i = 0; i < count; i++) {
                bool sel = (i == *current);
                ImVec2 rp = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(items[i], sel)) {
                    *current = i;
                    changed = true;
                }
                if (sel) {
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(rp.x, rp.y), ImVec2(rp.x + 3, rp.y + ImGui::GetTextLineHeight() + 4),
                        IM_COL32(255, 255, 255, 255), 1.0f);
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(6);
        ImGui::Spacing();
        ImGui::PopID();
        return changed;
    }

    inline void BeginCard(const char* title) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, V4(variables::Theme::border));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
        ImGui::BeginChild(title, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
        if (title && title[0]) {
            ImGui::TextColored(V4(variables::Theme::textDim), "%s", title);
            ImGui::Dummy(ImVec2(0, 2));
            ImVec2 a = ImGui::GetCursorScreenPos();
            float lw = ImGui::GetContentRegionAvail().x;
            ImGui::GetWindowDrawList()->AddLine(a, ImVec2(a.x + lw, a.y), U32(variables::Theme::border, 0.7f));
            ImGui::Dummy(ImVec2(0, 6));
        }
    }

    inline void EndCard() {
        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);
        ImGui::Spacing();
    }

    // Stretch-equal table columns — Columns API was clipping left cards
    inline void BeginTwoCol(const char* id) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(5, 0));
        ImGui::BeginTable(id, 2,
            ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_PadOuterX);
        ImGui::TableNextColumn();
    }

    inline void NextCol() { ImGui::TableNextColumn(); }
    inline void EndTwoCol() {
        ImGui::EndTable();
        ImGui::PopStyleVar();
    }

    inline void SubTabs(const char* const names[], int count, int* selected) {
        for (int i = 0; i < count; i++) {
            if (i) ImGui::SameLine(0, 6);
            bool on = (*selected == i);
            ImGui::PushStyleColor(ImGuiCol_Button, on ? ImVec4(0.16f, 0.16f, 0.18f, 1) : ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.14f, 0.16f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.18f, 0.20f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, on ? V4(variables::Theme::text) : V4(variables::Theme::textDim));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            if (ImGui::Button(names[i], ImVec2(0, 26)))
                *selected = i;
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
        }
        ImGui::Spacing();
    }

    inline void DrawToast(ImDrawList* dl, ImVec2 ds, float dt) {
        if (!variables::Toast::show) return;
        variables::Toast::timer -= dt;
        if (variables::Toast::timer <= 0) {
            variables::Toast::show = false;
            variables::Toast::warning = false;
            return;
        }
        float a = variables::Toast::timer > 0.5f ? 1.f : variables::Toast::timer / 0.5f;
        float tw = variables::Toast::warning ? 320.f : 220.f;
        float th = variables::Toast::warning ? 92.f : 72.f;
        ImVec2 p(ds.x - tw - 18, ds.y - th - 18);
        dl->AddRectFilled(p, ImVec2(p.x + tw, p.y + th), IM_COL32(18, 18, 22, (int)(235 * a)), 10.0f);
        ImU32 border = variables::Toast::warning
            ? IM_COL32(220, 90, 70, (int)(220 * a))
            : IM_COL32(50, 50, 58, (int)(200 * a));
        dl->AddRect(p, ImVec2(p.x + tw, p.y + th), border, 10.0f, 0, variables::Toast::warning ? 1.6f : 1.0f);
        ImU32 dot = variables::Toast::warning
            ? IM_COL32(240, 90, 70, (int)(255 * a))
            : IM_COL32(240, 240, 245, (int)(255 * a));
        dl->AddCircleFilled(ImVec2(p.x + 22, p.y + 28), 10, dot, 16);
        dl->AddText(ImVec2(p.x + 42, p.y + 12), IM_COL32(240, 240, 245, (int)(255 * a)), variables::Toast::title);
        dl->AddText(ImVec2(p.x + 42, p.y + 30), IM_COL32(200, 170, 160, (int)(255 * a)), variables::Toast::body);
        dl->AddText(ImVec2(p.x + 42, p.y + 52), IM_COL32(140, 140, 150, (int)(255 * a)), variables::Toast::footer);
    }

    inline void DrawFooter(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        float fh = 28.0f;
        ImVec2 a(wp.x, wp.y + wh - fh);
        ImVec2 b(wp.x + ww, wp.y + wh);
        dl->AddRectFilled(a, b, IM_COL32(10, 10, 12, 255));
        dl->AddLine(a, ImVec2(b.x, a.y), U32(variables::Theme::border, 0.6f));
        dl->AddCircleFilled(ImVec2(a.x + 16, a.y + fh * 0.5f), 4, IM_COL32(60, 220, 110, 255), 12);
        char online[48];
        sprintf_s(online, "%d online", variables::Misc::onlineCount);
        dl->AddText(ImVec2(a.x + 26, a.y + 6), IM_COL32(180, 180, 190, 255), online);

        const char* disc = "discord.gg/rUXya4U5qM";
        ImVec2 ts = ImGui::CalcTextSize(disc);
        ImVec2 discPos(wp.x + (ww - ts.x) * 0.5f, a.y + 6);
        dl->AddText(discPos, IM_COL32(160, 160, 175, 255), disc);
        ImGui::SetCursorScreenPos(discPos);
        if (ImGui::InvisibleButton("##discordlink", ts))
            ShellExecuteA(nullptr, "open", "https://discord.gg/rUXya4U5qM", nullptr, nullptr, SW_SHOWNORMAL);
        if (ImGui::IsItemHovered())
            dl->AddText(discPos, IM_COL32(220, 220, 230, 255), disc);

        const char* build = "Build: Jul 11 2026";
        ImVec2 bs = ImGui::CalcTextSize(build);
        dl->AddText(ImVec2(b.x - bs.x - 12, a.y + 6), IM_COL32(140, 140, 150, 255), build);
    }
}
