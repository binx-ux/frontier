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
#include "ui_motion.h"

namespace FrontierUI {

    inline ImVec4 V4(const float c[4]) { return ImVec4(c[0], c[1], c[2], c[3]); }

    inline ImU32 U32(const float c[4], float a = -1.f) {
        float aa = (a < 0) ? c[3] : a;
        return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(aa * 255));
    }
    inline ImU32 AccentU32(float a = 1.f) {
        return U32(variables::Theme::brand, a);
    }

    inline ImU32 AccentSoftU32(float a = 1.f) {
        float aa = a < 0.f ? 0.f : (a > 1.f ? 1.f : a);
        const float* c = variables::Theme::brand;
        return IM_COL32(
            (int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(180 * aa));
    }

    inline const char* VisibleLabel(const char* label) {
        if (!label || !label[0]) return nullptr;
        if (label[0] == '#' && label[1] == '#') return nullptr;
        return label;
    }

    inline ImU32 LabelTextCol() { return IM_COL32(255, 255, 255, 245); }
    inline ImU32 LabelTextDimCol() { return IM_COL32(255, 255, 255, 220); }

    inline void KeyNameInto(int vk, char* out, size_t outSz) {
        if (!out || outSz < 2) return;
        switch (vk) {
        case 0: strncpy_s(out, outSz, "none", _TRUNCATE); return;
        case VK_LBUTTON: strncpy_s(out, outSz, "Mouse 1", _TRUNCATE); return;
        case VK_RBUTTON: strncpy_s(out, outSz, "Mouse 2", _TRUNCATE); return;
        case VK_MBUTTON: strncpy_s(out, outSz, "Mouse 3", _TRUNCATE); return;
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

    inline bool PollKeybindCapture(int* key, DWORD ignoreUntil)
    {
        if (!key) return false;
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            variables::waitingForKey = false;
            variables::keyToRebind = nullptr;
            return true;
        }
        if (GetTickCount() < ignoreUntil)
            return false;

        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            *key = VK_LBUTTON;
            variables::waitingForKey = false;
            variables::keyToRebind = nullptr;
            return true;
        }
        if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
            *key = VK_RBUTTON;
            variables::waitingForKey = false;
            variables::keyToRebind = nullptr;
            return true;
        }
        if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
            *key = VK_MBUTTON;
            variables::waitingForKey = false;
            variables::keyToRebind = nullptr;
            return true;
        }
        if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
            *key = VK_XBUTTON1;
            variables::waitingForKey = false;
            variables::keyToRebind = nullptr;
            return true;
        }
        if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
            *key = VK_XBUTTON2;
            variables::waitingForKey = false;
            variables::keyToRebind = nullptr;
            return true;
        }

        for (int vk = 8; vk < 256; vk++) {
            if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
                vk == VK_XBUTTON1 || vk == VK_XBUTTON2 || vk == VK_ESCAPE)
                continue;
            if (GetAsyncKeyState(vk) & 0x8000) {
                *key = vk;
                variables::waitingForKey = false;
                variables::keyToRebind = nullptr;
                return true;
            }
        }
        return false;
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
            PollKeybindCapture(key, rebindIgnoreUntil);
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

    inline bool ToggleSwitch(const char* id, bool* v, float w = 44.f, float h = 22.f) {
        ImGui::PushID(id);
        ImVec2 p = ImGui::GetCursorScreenPos();
        bool pressed = ImGui::InvisibleButton("##tog", ImVec2(w, h));
        if (pressed) *v = !*v;
        bool hovered = ImGui::IsItemHovered();

        static float thumbBlend[256]{};
        ImGuiID wid = ImGui::GetID("##tog");
        const int slot = (int)(wid % 256);
        const float goal = *v ? 1.f : 0.f;
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;
        thumbBlend[slot] += (goal - thumbBlend[slot]) * (1.f - expf(-20.f * dt));
        const float t = thumbBlend[slot];

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float r = h * 0.5f;
        ImU32 trackOff = IM_COL32(hovered ? 46 : 36, hovered ? 48 : 38, hovered ? 56 : 46, 255);
        ImU32 trackOn = AccentU32(hovered ? 1.f : 0.92f);
        const float invT = 1.f - t;
        ImU32 track = IM_COL32(
            (int)(((trackOff >> IM_COL32_R_SHIFT) & 0xFF) * invT + ((trackOn >> IM_COL32_R_SHIFT) & 0xFF) * t),
            (int)(((trackOff >> IM_COL32_G_SHIFT) & 0xFF) * invT + ((trackOn >> IM_COL32_G_SHIFT) & 0xFF) * t),
            (int)(((trackOff >> IM_COL32_B_SHIFT) & 0xFF) * invT + ((trackOn >> IM_COL32_B_SHIFT) & 0xFF) * t),
            255);
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), track, r);
        if (t > 0.05f) {
            dl->AddRect(p, ImVec2(p.x + w, p.y + h), AccentSoftU32(0.35f + t * 0.2f), r, 0, 1.2f);
            dl->AddRectFilled(
                ImVec2(p.x + 2.f, p.y + 2.f), ImVec2(p.x + w - 2.f, p.y + h - 2.f),
                IM_COL32(255, 255, 255, (int)(18 * t)), r - 2.f);
        } else {
            dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(255, 255, 255, 22), r, 0, 1.f);
        }

        const float pad = 3.f;
        const float thumb = h - pad * 2.f;
        const float tx = p.x + pad + t * (w - thumb - pad * 2.f);
        const float ty = p.y + h * 0.5f;
        if (t > 0.5f)
            dl->AddCircleFilled(ImVec2(tx + thumb * 0.5f, ty), thumb * 0.5f + 1.5f, AccentSoftU32(0.25f + (t - 0.5f) * 0.2f));
        const ImU32 thumbCol = IM_COL32(
            (int)(170 + t * 85), (int)(172 + t * 83), (int)(182 + t * 68), 255);
        dl->AddCircleFilled(ImVec2(tx + thumb * 0.5f, ty), thumb * 0.5f, thumbCol);

        ImGui::PopID();
        return pressed;
    }

    inline bool OptionCheck(const char* label, bool* v) {
        ImGui::PushID(label);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float box = 13.f;

        ImGui::SetCursorScreenPos(ImVec2(p.x + avail - box, p.y));
        bool pressed = ImGui::InvisibleButton("##opt", ImVec2(box, box));
        if (pressed) *v = !*v;
        bool hov = ImGui::IsItemHovered();

        ImU32 fill = *v ? AccentU32(1.f) : IM_COL32(hov ? 34 : 26, hov ? 34 : 26, hov ? 36 : 28, 255);
        dl->AddRectFilled(ImVec2(p.x + avail - box, p.y), ImVec2(p.x + avail, p.y + box), fill, 2.f);
        dl->AddRect(
            ImVec2(p.x + avail - box, p.y), ImVec2(p.x + avail, p.y + box),
            *v ? AccentSoftU32(0.8f) : IM_COL32(255, 255, 255, 28), 2.f);
        if (*v) {
            dl->AddLine(
                ImVec2(p.x + avail - box + 3.f, p.y + 7.f),
                ImVec2(p.x + avail - box + 5.f, p.y + 10.f),
                IM_COL32(255, 255, 255, 240), 1.6f);
            dl->AddLine(
                ImVec2(p.x + avail - box + 5.f, p.y + 10.f),
                ImVec2(p.x + avail - 3.f, p.y + 3.f),
                IM_COL32(255, 255, 255, 240), 1.6f);
        }

        ImU32 tc = *v ? LabelTextCol() : LabelTextDimCol();
        if (const char* visible = VisibleLabel(label))
            dl->AddText(ImVec2(p.x, p.y + 1), tc, visible);

        ImGui::Dummy(ImVec2(0, box + 6.f));
        ImGui::PopID();
        return pressed;
    }

    inline void KeybindRow(const char* label, int* key) {
        ImGui::PushID(label);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (const char* visible = VisibleLabel(label))
            dl->AddText(ImVec2(p.x, p.y + 1), LabelTextCol(), visible);

        static DWORD rebindIgnoreUntil = 0;
        const bool waiting = variables::waitingForKey && variables::keyToRebind == key;
        const char* kn = waiting ? "…" : KeyName(*key);
        ImVec2 ks = ImGui::CalcTextSize(kn);
        dl->AddText(ImVec2(p.x + avail - ks.x, p.y + 1), LabelTextCol(), kn);

        ImGui::SetCursorScreenPos(p);
        if (ImGui::InvisibleButton("##kbr", ImVec2(avail, 18.f))) {
            variables::waitingForKey = true;
            variables::keyToRebind = key;
            rebindIgnoreUntil = GetTickCount() + 250;
        }
        if (waiting) {
            PollKeybindCapture(key, rebindIgnoreUntil);
        }
        ImGui::Dummy(ImVec2(0, 20.f));
        ImGui::PopID();
    }

    inline bool Checkbox(const char* label, bool* v, float* color = nullptr, int* key = nullptr) {
        ImGui::PushID(label);
        ImGui::BeginGroup();

        float avail = ImGui::GetContentRegionAvail().x;
        float rightW = 44.f;
        if (color) rightW += 26.f;
        if (key) rightW += 92.f;
        float leftW = avail - rightW - 4.f;
        if (leftW < 48.f) leftW = 48.f;

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec4 tc = V4(variables::Theme::text);
        if (const char* visible = VisibleLabel(label))
            dl->AddText(ImVec2(p.x, p.y + 1), ImGui::ColorConvertFloat4ToU32(tc), visible);

        float rx = p.x + avail - rightW + 4.f;
        ImGui::SetCursorScreenPos(ImVec2(rx, p.y));
        bool pressed = ToggleSwitch("##sw", v);

        rx += 44.f;
        if (color) {
            ImGui::SetCursorScreenPos(ImVec2(rx, p.y - 1));
            ColorSquare("##col", color);
            rx += 24.f;
        }
        if (key) {
            ImGui::SetCursorScreenPos(ImVec2(rx, p.y - 3));
            KeybindChip("##kb", key);
        }

        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PopID();
        return pressed;
    }

    inline bool SliderFloat(const char* label, float* v, float mn, float mx, const char* fmt = "%.2f") {
        ImGui::PushID(label ? label : "##slider");
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 rowP = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        const char* visible = VisibleLabel(label);
        if (visible)
            dl->AddText(ImVec2(rowP.x, rowP.y + 1), LabelTextCol(), visible);

        char valBuf[32];
        sprintf_s(valBuf, fmt, *v);
        ImVec2 valSz = ImGui::CalcTextSize(valBuf);
        dl->AddText(
            ImVec2(rowP.x + avail - valSz.x, rowP.y + 1),
            LabelTextDimCol(), valBuf);

        const float labelGap = visible ? 24.f : 4.f;
        const float trackH = 8.f;
        const float trackY = rowP.y + labelGap;
        const float trackW = avail;
        float t = (mx > mn) ? ((*v - mn) / (mx - mn)) : 0.f;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;

        ImVec2 tMin(rowP.x, trackY);
        ImVec2 tMax(rowP.x + trackW, trackY + trackH);
        dl->AddRectFilled(tMin, tMax, IM_COL32(18, 19, 24, 255), trackH * 0.5f);
        dl->AddRect(tMin, tMax, IM_COL32(255, 255, 255, 28), trackH * 0.5f);
        if (t > 0.001f) {
            ImVec2 fillMax(tMin.x + trackW * t, tMax.y);
            dl->AddRectFilled(tMin, fillMax, AccentU32(0.95f), trackH * 0.5f);
            dl->AddRectFilled(
                ImVec2(tMin.x, tMin.y + 1.f), ImVec2(fillMax.x, tMin.y + 3.f),
                IM_COL32(255, 255, 255, 35), 2.f);
        }

        const float thumbX = tMin.x + trackW * t;
        const float thumbR = 7.f;
        dl->AddCircleFilled(ImVec2(thumbX, trackY + trackH * 0.5f), thumbR + 2.f, AccentSoftU32(0.28f));
        dl->AddCircleFilled(ImVec2(thumbX, trackY + trackH * 0.5f), thumbR, IM_COL32(245, 246, 252, 255));
        dl->AddCircle(ImVec2(thumbX, trackY + trackH * 0.5f), thumbR, AccentSoftU32(0.85f), 0, 1.4f);

        ImGui::SetCursorScreenPos(ImVec2(rowP.x, trackY - 4.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0, 0, 0, 0));
        ImGui::SetNextItemWidth(trackW);
        bool ch = ImGui::SliderFloat("##s", v, mn, mx, "");
        ImGui::PopStyleColor(5);
        ImGui::Dummy(ImVec2(0, trackH + (visible ? 12.f : 8.f)));
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
        if (const char* visible = VisibleLabel(label))
            ImGui::TextColored(V4(variables::Theme::text), "%s", visible);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.106f, 0.110f, 0.110f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.15f, 1));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.07f, 0.07f, 0.08f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.16f, 0.16f, 0.19f, 1));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.20f, 0.24f, 1));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.24f, 0.28f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
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
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor(6);
        ImGui::Spacing();
        ImGui::PopID();
        return changed;
    }

    inline int g_dropDepth = 0;
    inline bool g_dropOpen[24] = {};

    inline bool BeginCard(const char* title, bool /*defaultOpen*/ = true, bool* headerToggle = nullptr) {
        ImGui::PushID(title ? title : "section");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.058f, 0.065f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.10f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));

        if (!ImGui::BeginChild("##sec", ImVec2(0, 0),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_None)) {
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
            ImGui::PopID();
            return false;
        }

        if (title && title[0]) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(
                ImVec2(p.x - 2.f, p.y + 4.f), ImVec2(p.x, p.y + 20.f),
                AccentU32(0.95f), 2.f);

            const float rowW = ImGui::GetContentRegionAvail().x;
            ImGui::TextColored(V4(variables::Theme::text), "%s", title);
            if (headerToggle) {
                ImGui::SameLine(0, 0);
                ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 44.f);
                ToggleSwitch("##hdr", headerToggle);
            }
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.f, 1.f, 1.f, 0.08f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));
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
            ImGui::TextColored(V4(variables::Theme::text), "%s", label);
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
        if (!selected || count <= 0) return;
        ImGui::PushID(names[0]);

        const float fullW = ImGui::GetContentRegionAvail().x;
        const float barH = 36.f;
        const float gap = 6.f;
        const float pad = 4.f;
        float tabW = (fullW - pad * 2.f - gap * (count - 1)) / (float)count;
        if (tabW < 64.f) tabW = 64.f;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 barOrigin = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(
            ImVec2(barOrigin.x, barOrigin.y + 2.f),
            ImVec2(barOrigin.x + fullW, barOrigin.y + barH),
            IM_COL32(17, 17, 17, 255), 6.f);
        dl->AddRect(
            ImVec2(barOrigin.x, barOrigin.y + 2.f),
            ImVec2(barOrigin.x + fullW, barOrigin.y + barH),
            IM_COL32(255, 255, 255, 14), 6.f);

        ImGui::BeginChild("##subtabbar", ImVec2(fullW, barH + 4.f), ImGuiChildFlags_None,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        for (int i = 0; i < count; i++) {
            if (i) ImGui::SameLine(0, gap);
            bool on = (*selected == i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 btn(p.x, p.y + 4.f);
            ImVec2 btnEnd(p.x + tabW, p.y + barH - 2.f);

            ImGui::PushID(i);
            if (ImGui::InvisibleButton("##sub", ImVec2(tabW, barH - 6.f)))
                *selected = i;
            bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            if (on) {
                dl->AddRectFilled(btn, btnEnd, IM_COL32(48, 104, 194, 220), 5.f);
            } else if (hov) {
                dl->AddRectFilled(btn, btnEnd, IM_COL32(255, 255, 255, 10), 5.f);
            }

            ImVec2 ts = ImGui::CalcTextSize(names[i]);
            ImU32 tc = on ? IM_COL32(255, 255, 255, 255)
                : (hov ? IM_COL32(220, 224, 232, 255) : IM_COL32(140, 144, 156, 255));
            dl->AddText(
                ImVec2(btn.x + (tabW - ts.x) * 0.5f, btn.y + (btnEnd.y - btn.y - ts.y) * 0.5f),
                tc, names[i]);
            ImGui::Dummy(ImVec2(tabW, barH - 6.f));
        }

        ImGui::EndChild();
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 8));
    }

    inline void SubTabList(const char* const names[], int count, int* selected, float width = 132.f, float height = 0.f) {
        if (!selected || count <= 0) return;
        ImGui::PushID(names[0]);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.045f, 0.048f, 0.055f, 1.f));
        ImGui::BeginChild("##sublist", ImVec2(width, height), ImGuiChildFlags_Borders);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (int i = 0; i < count; i++) {
            bool on = (*selected == i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            float rowH = 30.f;
            ImVec2 a(p.x + 4.f, p.y + 1.f);
            ImVec2 b(p.x + width - 8.f, p.y + rowH);

            ImGui::PushID(i);
            if (ImGui::InvisibleButton("##subrow", ImVec2(width - 8.f, rowH)))
                *selected = i;
            bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            if (on) {
                dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 5.f);
                dl->AddRectFilled(
                    ImVec2(a.x - 4.f, a.y + 5.f), ImVec2(a.x - 1.f, b.y - 5.f),
                    AccentU32(1.f), 2.f);
            } else if (hov) {
                dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 6), 5.f);
            }

            ImU32 tc = on ? IM_COL32(255, 255, 255, 255) : IM_COL32(160, 164, 176, 255);
            dl->AddText(ImVec2(a.x + 8.f, a.y + 7.f), tc, names[i]);
            ImGui::Dummy(ImVec2(0, rowH + 2.f));
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    inline bool ThemeColorRow(const char* label, float col[4], bool syncBrand = false) {
        ImGui::TextColored(V4(variables::Theme::text), "%s", label);
        ImGui::SameLine();
        bool ch = ColorSquare(label, col, syncBrand);
        if (ch && syncBrand)
            FrontierTheme::MarkDirty();
        else if (ch)
            FrontierTheme::MarkDirty();
        return ch;
    }

    inline void DrawToast(ImDrawList* dl, ImVec2 ds, float dt) {
        if (variables::Toast::show) {
            variables::Toast::timer -= dt;
            if (variables::Toast::timer <= 0) {
                variables::Toast::show = false;
                variables::Toast::warning = false;
            }
        }

        if (!variables::Toast::show && variables::Misc::toastAnim < 0.02f)
            return;

        const float enter = UIMotion::EaseOutCubic(variables::Misc::toastAnim);
        if (enter < 0.02f)
            return;

        float tw = variables::Toast::warning ? 340.f : 240.f;
        float th = variables::Toast::warning ? 96.f : 76.f;
        const float slideX = (1.f - enter) * 52.f;
        ImVec2 p(ds.x - tw - 20 + slideX, ds.y - th - 20);
        dl->AddRectFilled(p, ImVec2(p.x + tw, p.y + th), IM_COL32(14, 14, 18, (int)(240 * enter)), 12.0f);
        ImU32 border = variables::Toast::warning
            ? IM_COL32(220, 90, 70, (int)(220 * enter))
            : IM_COL32(70, 70, 82, (int)(210 * enter));
        dl->AddRect(p, ImVec2(p.x + tw, p.y + th), border, 12.0f, 0, variables::Toast::warning ? 1.6f : 1.1f);
        dl->AddRectFilled(ImVec2(p.x, p.y + 10), ImVec2(p.x + 3, p.y + th - 10),
            variables::Toast::warning ? IM_COL32(220, 90, 70, (int)(255 * enter))
            : U32(variables::Theme::brand, enter), 2.f);
        dl->AddText(ImVec2(p.x + 16, p.y + 14), IM_COL32(245, 245, 248, (int)(255 * enter)), variables::Toast::title);
        dl->AddText(ImVec2(p.x + 16, p.y + 34), IM_COL32(190, 170, 165, (int)(255 * enter)), variables::Toast::body);
        dl->AddText(ImVec2(p.x + 16, p.y + 54), IM_COL32(130, 130, 140, (int)(255 * enter)), variables::Toast::footer);
    }

    inline void DrawFooter(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        (void)dl; (void)wp; (void)ww; (void)wh;
    }
}
