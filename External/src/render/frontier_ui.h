#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"
#include "../core/variables/variables.h"
#include "../core/updater/updater.h"
#include "brand.h"
#include "frontier_theme.h"

namespace FrontierUI {

    inline ImVec4 V4(const float c[4]) { return ImVec4(c[0], c[1], c[2], c[3]); }
    inline ImU32 U32(const float c[4], float a = -1.f) {
        float aa = (a < 0) ? c[3] : a;
        return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(aa * 255));
    }

    inline void KeyNameInto(int vk, char* out, size_t outSz) {
        if (!out || outSz < 2) return;
        switch (vk) {
        case 0: strncpy_s(out, outSz, "none", _TRUNCATE); return;
        case VK_LBUTTON: strncpy_s(out, outSz, "lmb", _TRUNCATE); return;
        case VK_RBUTTON: strncpy_s(out, outSz, "rmb", _TRUNCATE); return;
        case VK_MBUTTON: strncpy_s(out, outSz, "mmb", _TRUNCATE); return;
        case VK_XBUTTON1: strncpy_s(out, outSz, "xbutton1", _TRUNCATE); return;
        case VK_XBUTTON2: strncpy_s(out, outSz, "xbutton2", _TRUNCATE); return;
        case VK_SHIFT: strncpy_s(out, outSz, "shift", _TRUNCATE); return;
        case VK_CONTROL: strncpy_s(out, outSz, "ctrl", _TRUNCATE); return;
        case VK_MENU: strncpy_s(out, outSz, "alt", _TRUNCATE); return;
        case VK_SPACE: strncpy_s(out, outSz, "space", _TRUNCATE); return;
        case VK_TAB: strncpy_s(out, outSz, "tab", _TRUNCATE); return;
        case VK_INSERT: strncpy_s(out, outSz, "ins", _TRUNCATE); return;
        case VK_DELETE: strncpy_s(out, outSz, "del", _TRUNCATE); return;
        case VK_RCONTROL: strncpy_s(out, outSz, "rctrl", _TRUNCATE); return;
        case VK_LCONTROL: strncpy_s(out, outSz, "lctrl", _TRUNCATE); return;
        default:
            if (vk >= 'A' && vk <= 'Z') { out[0] = (char)(vk + 32); out[1] = 0; return; }
            if (vk >= '0' && vk <= '9') { out[0] = (char)vk; out[1] = 0; return; }
            sprintf_s(out, outSz, "%d", vk);
        }
    }

    inline const char* KeyName(int vk) {
        static thread_local char buf[16];
        KeyNameInto(vk, buf, sizeof(buf));
        return buf;
    }

    inline void KeybindChip(const char* id, int* key) {
        ImGui::PushID(id);
        char label[32];
        KeyNameInto(*key, label, sizeof(label));
        static DWORD rebindIgnoreUntil = 0;
        const bool waiting = variables::waitingForKey && variables::keyToRebind == key;
        ImGui::PushStyleColor(ImGuiCol_Button, waiting
            ? ImVec4(0.88f, 0.88f, 0.92f, 1) : ImVec4(0.11f, 0.11f, 0.13f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, waiting
            ? ImVec4(0.95f, 0.95f, 0.98f, 1) : ImVec4(0.17f, 0.17f, 0.20f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.26f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, waiting
            ? ImVec4(0.06f, 0.06f, 0.07f, 1) : V4(variables::Theme::text));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.28f, 0.32f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (waiting) {
            if (ImGui::Button("…", ImVec2(0, 0))) {}
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
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
        ImGui::PopID();
    }

    inline bool ColorSquare(const char* id, float col[4], bool themeSync = false) {
        ImGui::PushID(id);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImVec4 c(col[0], col[1], col[2], col[3]);
        bool changed = false;
        if (ImGui::ColorButton("##swatch", c,
            ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip |
            ImGuiColorEditFlags_NoDragDrop, ImVec2(18, 18))) {
            ImGui::OpenPopup("##colorpopup");
        }
        if (ImGui::BeginPopup("##colorpopup", ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.07f, 0.07f, 0.08f, 1));
            if (ImGui::ColorPicker4("##picker", col,
                ImGuiColorEditFlags_DisplayHex |
                ImGuiColorEditFlags_DisplayRGB |
                ImGuiColorEditFlags_InputRGB |
                ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_AlphaPreviewHalf |
                ImGuiColorEditFlags_PickerHueBar |
                ImGuiColorEditFlags_NoSidePreview |
                ImGuiColorEditFlags_NoSmallPreview))
                changed = true;
            ImGui::PopStyleColor(2);
            ImGui::EndPopup();
        }
        if (changed && themeSync) {
            FrontierTheme::SyncBrandFromAccent();
            FrontierTheme::MarkDirty();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopID();
        return changed;
    }

    inline bool Checkbox(const char* label, bool* v, float* color = nullptr, int* key = nullptr) {
        ImGui::PushID(label);
        ImGui::BeginGroup();

        float avail = ImGui::GetContentRegionAvail().x;
        float rightW = 0.f;
        if (color) rightW += 24.f;
        if (key) rightW += 78.f;
        const float box = 14.f;
        rightW += box + 8.f;
        float leftW = avail - rightW - 4.f;
        if (leftW < 48.f) leftW = 48.f;

        ImVec2 p = ImGui::GetCursorScreenPos();
        bool pressed = ImGui::InvisibleButton("##cb", ImVec2(avail, 22.f));
        if (pressed) *v = !*v;
        bool hovered = ImGui::IsItemHovered();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec4 tc = *v ? V4(variables::Theme::text) : V4(variables::Theme::textDim);
        dl->AddText(ImVec2(p.x, p.y + 2), ImGui::ColorConvertFloat4ToU32(tc), label);

        float bx = p.x + leftW + 8.f;
        float by = p.y + 2.f;
        ImU32 border = *v ? U32(variables::Theme::brand, 0.9f)
            : IM_COL32(hovered ? 70 : 52, hovered ? 70 : 52, hovered ? 76 : 58, 255);
        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + box, by + box), IM_COL32(22, 22, 24, 255), 3.f);
        dl->AddRect(ImVec2(bx, by), ImVec2(bx + box, by + box), border, 3.f);
        if (*v) {
            dl->AddRectFilled(ImVec2(bx + 3, by + 3), ImVec2(bx + box - 3, by + box - 3),
                U32(variables::Theme::brand, 1.f), 2.f);
        }

        float rx = bx + box + 8.f;
        if (color) {
            ImGui::SetCursorScreenPos(ImVec2(rx, by - 1));
            ColorSquare("##col", color);
            rx += 24.f;
        }
        if (key) {
            ImGui::SetCursorScreenPos(ImVec2(rx, by - 3));
            KeybindChip("##kb", key);
        }

        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::PopID();
        return pressed;
    }

    inline bool SliderFloat(const char* label, float* v, float mn, float mx, const char* fmt = "%.2f") {
        ImGui::PushID(label);
        ImGui::BeginGroup();
        ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);
        char buf[32]; sprintf_s(buf, fmt, *v);
        float vw = ImGui::CalcTextSize(buf).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - vw);
        ImGui::TextColored(V4(variables::Theme::text), "%s", buf);
        ImGui::EndGroup();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.11f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.12f, 0.14f, 1));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, V4(variables::Theme::brand));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, V4(variables::Theme::brand));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 5));
        ImGui::SetNextItemWidth(-1);

        ImVec2 sp = ImGui::GetCursorScreenPos();
        float aw = ImGui::CalcItemWidth();
        float t = (mx > mn) ? ((*v - mn) / (mx - mn)) : 0.f;
        if (t < 0) t = 0; if (t > 1) t = 1;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(sp.x, sp.y + 4), ImVec2(sp.x + aw * t, sp.y + 14),
            U32(variables::Theme::brand, 0.55f), 4.f);

        bool ch = ImGui::SliderFloat("##s", v, mn, mx, "");
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);
        ImGui::Spacing();
        ImGui::PopID();
        return ch;
    }

    inline bool SliderInt(const char* label, int* v, int mn, int mx) {
        float fv = (float)*v;
        bool ch = SliderFloat(label, &fv, (float)mn, (float)mx, "%.0f");
        *v = (int)(fv + (fv >= 0 ? 0.5f : -0.5f));
        if (*v < mn) *v = mn;
        if (*v > mx) *v = mx;
        return ch;
    }

    inline bool Combo(const char* label, int* current, const char* const items[], int count) {
        ImGui::PushID(label);
        if (label && label[0])
            ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.11f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.13f, 0.15f, 1));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.07f, 0.07f, 0.08f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.16f, 0.16f, 0.19f, 1));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.20f, 0.24f, 1));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.24f, 0.28f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
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
                        U32(variables::Theme::brand, 1.f), 1.0f);
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(6);
        ImGui::Spacing();
        ImGui::PopID();
        return changed;
    }

    inline int g_dropDepth = 0;
    inline bool g_dropOpen[24] = {};

    inline bool BeginCard(const char* title, bool /*defaultOpen*/ = true) {
        ImGui::PushID(title ? title : "section");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.05f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

        ImGui::BeginChild("##sec", ImVec2(0, 0),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

        if (title && title[0]) {
            ImDrawList* cdl = ImGui::GetWindowDrawList();
            ImVec2 cp = ImGui::GetCursorScreenPos();
            cdl->AddRectFilled(ImVec2(cp.x - 12, cp.y - 2), ImVec2(cp.x - 9, cp.y + 16),
                U32(variables::Theme::brand, 0.7f), 1.f);
            ImGui::TextColored(V4(variables::Theme::text), "%s", title);
            ImGui::Dummy(ImVec2(0, 2));
        }

        if (g_dropDepth < 24)
            g_dropOpen[g_dropDepth] = true;
        g_dropDepth++;
        return true;
    }

    inline void EndCard() {
        if (g_dropDepth <= 0) return;
        g_dropDepth--;
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 8));
    }

    inline bool BeginSettings(const char* label, bool /*defaultOpen*/ = true) {
        ImGui::PushID(label ? label : "more");
        if (label && label[0]) {
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::Separator();
            ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);
            ImGui::Dummy(ImVec2(0, 2));
        }
        if (g_dropDepth < 24)
            g_dropOpen[g_dropDepth] = true;
        g_dropDepth++;
        return true;
    }

    inline void EndSettings() {
        if (g_dropDepth <= 0) return;
        g_dropDepth--;
        ImGui::PopID();
    }

    inline void BeginTwoCol(const char* id) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(9, 0));
        ImGui::BeginTable(id, 2,
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_PadOuterX);
        ImGui::TableSetupColumn("L", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("R", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
    }

    inline void NextCol() { ImGui::TableNextColumn(); }
    inline void EndTwoCol() {
        ImGui::EndTable();
        ImGui::PopStyleVar();
    }

    inline void SubTabs(const char* const names[], int count, int* selected) {
        const bool pills = variables::Theme::subTabStyle != 0;
        for (int i = 0; i < count; i++) {
            if (i) ImGui::SameLine(0, pills ? 6 : 14);
            bool on = (*selected == i);
            if (pills) {
                ImGui::PushStyleColor(ImGuiCol_Button, on ? ImVec4(0.12f, 0.12f, 0.14f, 1) : ImVec4(0.07f, 0.07f, 0.09f, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.14f, 0.17f, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.16f, 0.19f, 1));
                ImGui::PushStyleColor(ImGuiCol_Border, on ? ImVec4(1, 1, 1, 0.22f) : ImVec4(1, 1, 1, 0.08f));
                ImGui::PushStyleColor(ImGuiCol_Text, on ? V4(variables::Theme::brand) : V4(variables::Theme::textDim));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 999.f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 5));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
                if (ImGui::Button(names[i]))
                    *selected = i;
                ImGui::PopStyleVar(3);
                ImGui::PopStyleColor(5);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.04f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.06f));
                ImGui::PushStyleColor(ImGuiCol_Text, on ? V4(variables::Theme::brand) : V4(variables::Theme::textDim));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 6));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
                if (ImGui::Button(names[i]))
                    *selected = i;
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(4);
            }
        }
        ImGui::Dummy(ImVec2(0, 6));
    }

    inline bool ThemeColorRow(const char* label, float col[4], bool syncBrand = false) {
        ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);
        ImGui::SameLine();
        bool ch = ColorSquare(label, col, syncBrand);
        if (ch && syncBrand)
            FrontierTheme::MarkDirty();
        else if (ch)
            FrontierTheme::MarkDirty();
        return ch;
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
        float tw = variables::Toast::warning ? 340.f : 240.f;
        float th = variables::Toast::warning ? 96.f : 76.f;
        ImVec2 p(ds.x - tw - 20, ds.y - th - 20);
        dl->AddRectFilled(p, ImVec2(p.x + tw, p.y + th), IM_COL32(14, 14, 18, (int)(240 * a)), 12.0f);
        ImU32 border = variables::Toast::warning
            ? IM_COL32(220, 90, 70, (int)(220 * a))
            : IM_COL32(70, 70, 82, (int)(210 * a));
        dl->AddRect(p, ImVec2(p.x + tw, p.y + th), border, 12.0f, 0, variables::Toast::warning ? 1.6f : 1.1f);
        dl->AddRectFilled(ImVec2(p.x, p.y + 10), ImVec2(p.x + 3, p.y + th - 10),
            variables::Toast::warning ? IM_COL32(220, 90, 70, (int)(255 * a))
            : U32(variables::Theme::brand, a), 2.f);
        dl->AddText(ImVec2(p.x + 16, p.y + 14), IM_COL32(245, 245, 248, (int)(255 * a)), variables::Toast::title);
        dl->AddText(ImVec2(p.x + 16, p.y + 34), IM_COL32(190, 170, 165, (int)(255 * a)), variables::Toast::body);
        dl->AddText(ImVec2(p.x + 16, p.y + 54), IM_COL32(130, 130, 140, (int)(255 * a)), variables::Toast::footer);
    }

    inline void DrawFooter(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        const float fh = 28.f;
        ImVec2 a(wp.x, wp.y + wh - fh);
        ImVec2 b(wp.x + ww, wp.y + wh);
        dl->AddRectFilled(a, b, IM_COL32(9, 9, 9, 255));
        dl->AddLine(a, ImVec2(b.x, a.y), U32(variables::Theme::border, 0.45f));

        char left[64];
        sprintf_s(left, "%s online", variables::Status::playersOnline);
        dl->AddText(ImVec2(a.x + 14, a.y + 6), U32(variables::Theme::brand, 0.85f), left);

        ImVec2 cs = ImGui::CalcTextSize(Frontier::kFooterLink);
        if (variables::Theme::showFooterLink)
            dl->AddText(ImVec2(a.x + (ww - cs.x) * 0.5f, a.y + 6), IM_COL32(120, 120, 132, 255), Frontier::kFooterLink);

        char buildLbl[32];
        sprintf_s(buildLbl, "Build: ");
        char buildVal[48];
        sprintf_s(buildVal, "%s", Updater::kLocalDisplay);
        ImVec2 bls = ImGui::CalcTextSize(buildLbl);
        ImVec2 bvs = ImGui::CalcTextSize(buildVal);
        float rx = b.x - bls.x - bvs.x - 14;
        dl->AddText(ImVec2(rx, a.y + 6), IM_COL32(130, 130, 142, 255), buildLbl);
        dl->AddText(ImVec2(rx + bls.x, a.y + 6), U32(variables::Theme::brand, 1.f), buildVal);
    }
}
