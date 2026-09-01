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
    inline ImU32 LabelTextDimCol() { return IM_COL32(156, 163, 175, 255); }
    inline ImVec4 LabelTextDimV4() { return ImVec4(0.612f, 0.639f, 0.686f, 1.f); }

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

    inline bool ToggleSwitch(const char* id, bool* v, float w = 36.f, float h = 20.f) {
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
        ImU32 trackOff = IM_COL32(42, 42, 42, 255);
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

        const float pad = 2.f;
        const float thumb = h - pad * 2.f;
        const float tx = p.x + pad + t * (w - thumb - pad * 2.f);
        const float ty = p.y + h * 0.5f;
        if (t > 0.5f)
            dl->AddCircleFilled(ImVec2(tx + thumb * 0.5f, ty), thumb * 0.5f + 3.f, AccentSoftU32(0.35f));
        const ImU32 thumbCol = t > 0.5f
            ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(156, 163, 175, 255);
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
        float rightW = 36.f;
        if (color) rightW += 26.f;
        if (key) rightW += 92.f;
        float leftW = avail - rightW - 4.f;
        if (leftW < 48.f) leftW = 48.f;

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (const char* visible = VisibleLabel(label))
            dl->AddText(ImVec2(p.x, p.y + 1), LabelTextDimCol(), visible);

        float rx = p.x + avail - rightW + 4.f;
        ImGui::SetCursorScreenPos(ImVec2(rx, p.y));
        bool pressed = ToggleSwitch("##sw", v);

        rx += 36.f;
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
        ImGui::Dummy(ImVec2(0, 10));
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
            dl->AddText(ImVec2(rowP.x, rowP.y + 1), LabelTextDimCol(), visible);

        char valBuf[32];
        sprintf_s(valBuf, fmt, *v);
        ImVec2 valSz = ImGui::CalcTextSize(valBuf);
        dl->AddText(
            ImVec2(rowP.x + avail - valSz.x, rowP.y + 1),
            AccentU32(0.95f), valBuf);

        const float labelGap = visible ? 22.f : 4.f;
        const float trackH = 2.f;
        const float trackY = rowP.y + labelGap + 4.f;
        const float trackW = avail;
        float t = (mx > mn) ? ((*v - mn) / (mx - mn)) : 0.f;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;

        const float cy = trackY + 6.f;
        dl->AddLine(ImVec2(rowP.x, cy), ImVec2(rowP.x + trackW, cy), IM_COL32(51, 51, 51, 255), trackH);
        if (t > 0.001f)
            dl->AddLine(ImVec2(rowP.x, cy), ImVec2(rowP.x + trackW * t, cy), AccentU32(0.9f), trackH);

        const float thumbX = rowP.x + trackW * t;
        const float thumbR = 6.f;
        dl->AddCircleFilled(ImVec2(thumbX, cy), thumbR + 4.f, AccentSoftU32(0.35f));
        dl->AddCircleFilled(ImVec2(thumbX, cy), thumbR, AccentU32(1.f));

        ImGui::SetCursorScreenPos(ImVec2(rowP.x, trackY - 2.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0, 0, 0, 0));
        ImGui::SetNextItemWidth(trackW);
        bool ch = ImGui::SliderFloat("##s", v, mn, mx, "");
        ImGui::PopStyleColor(5);
        ImGui::Dummy(ImVec2(0, 20.f));
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

    inline int g_cardDepth = 0;
    inline bool g_cardOpen[16] = {};
    inline bool g_tableOpen = false;

    inline bool BeginCard(const char* title, bool /*defaultOpen*/ = true, bool* headerToggle = nullptr) {
        ImGui::PushID(title ? title : "section");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.05f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));

        const bool visible = ImGui::BeginChild("##sec", ImVec2(0, 0),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

        if (g_cardDepth < 16)
            g_cardOpen[g_cardDepth] = true;
        else
            g_cardOpen[15] = true;
        g_cardDepth++;

        if (visible && title && title[0]) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(
                ImVec2(p.x - 2.f, p.y + 2.f), ImVec2(p.x, p.y + 18.f),
                AccentU32(0.95f), 2.f);

            char upper[64];
            strncpy_s(upper, title, _TRUNCATE);
            for (char* c = upper; *c; ++c) {
                if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');
            }

            ImGui::TextColored(V4(variables::Theme::text), "%s", upper);
            if (headerToggle) {
                ImGui::SameLine(0, 0);
                ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 36.f);
                ToggleSwitch("##hdr", headerToggle);
            }
            ImGui::Dummy(ImVec2(0, 6));
        }

        return visible;
    }

    inline void EndCard() {
        if (g_cardDepth <= 0) return;
        g_cardDepth--;
        const int slot = g_cardDepth < 16 ? g_cardDepth : 15;
        if (!g_cardOpen[slot]) return;
        g_cardOpen[slot] = false;
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 10));
    }

    inline bool BeginSettings(const char* label, bool /*defaultOpen*/ = true) {
        ImGui::PushID(label ? label : "more");
        if (label && label[0]) {
            ImGui::Dummy(ImVec2(0, 6));
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.f, 1.f, 1.f, 0.06f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::TextColored(LabelTextDimV4(), "%s", label);
            ImGui::Dummy(ImVec2(0, 4));
        }
        return true;
    }

    inline void EndSettings() {
        ImGui::PopID();
    }

    inline bool BeginTwoCol(const char* id) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 8));
        g_tableOpen = ImGui::BeginTable(id, 2,
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_PadOuterX);
        if (!g_tableOpen) {
            ImGui::PopStyleVar();
            return false;
        }
        ImGui::TableSetupColumn("L", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("R", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        return true;
    }

    inline void NextCol() { ImGui::TableNextColumn(); }
    inline void EndTwoCol() {
        if (g_tableOpen) {
            ImGui::EndTable();
            g_tableOpen = false;
        }
        ImGui::PopStyleVar();
    }

    inline bool ChipToggle(const char* label, bool* v) {
        ImGui::PushID(label);
        float w = ImGui::GetContentRegionAvail().x;
        if (w < 60.f) w = 60.f;
        const float h = 28.f;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("##chip", ImVec2(w, h));
        bool pressed = ImGui::IsItemClicked();
        if (pressed) *v = !*v;
        bool hov = ImGui::IsItemHovered();

        ImU32 bg = *v ? IM_COL32(42, 42, 42, 255) : IM_COL32(42, 42, 42, 255);
        ImU32 border = *v ? AccentU32(0.9f) : IM_COL32(255, 255, 255, 25);
        if (hov && !*v) border = IM_COL32(255, 255, 255, 60);
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), bg, 6.f);
        dl->AddRect(p, ImVec2(p.x + w, p.y + h), border, 6.f);
        if (*v)
            dl->AddRectFilled(
                ImVec2(p.x + 1.f, p.y + 1.f), ImVec2(p.x + w - 1.f, p.y + h - 1.f),
                AccentU32(0.06f), 5.f);

        ImU32 tc = *v ? AccentU32(1.f) : (hov ? IM_COL32(255, 255, 255, 230) : LabelTextDimCol());
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + (h - ts.y) * 0.5f), tc, label);
        ImGui::Dummy(ImVec2(w, h));
        ImGui::PopID();
        return pressed;
    }

    inline void ChipRow2x2(const char* l0, bool* v0, const char* l1, bool* v1,
        const char* l2, bool* v2, const char* l3, bool* v3) {
        ImGui::PushID(l0 ? l0 : "chips");
        if (ImGui::BeginTable("chips", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ChipToggle(l0, v0);
            ImGui::TableNextColumn(); ChipToggle(l1, v1);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ChipToggle(l2, v2);
            ImGui::TableNextColumn(); ChipToggle(l3, v3);
            ImGui::EndTable();
        }
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 4));
    }

    inline void DrawSubTabCellAt(ImVec2 p, const char* label, bool on, bool hov, float tabW, float barH) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 btnEnd(p.x + tabW, p.y + barH);

        ImU32 bg = IM_COL32(42, 42, 42, 255);
        ImU32 border = on ? AccentU32(0.85f) : IM_COL32(255, 255, 255, 25);
        if (hov && !on) border = IM_COL32(255, 255, 255, 55);
        dl->AddRectFilled(p, btnEnd, bg, 6.f);
        dl->AddRect(p, btnEnd, border, 6.f);
        if (on)
            dl->AddRectFilled(
                ImVec2(p.x + 1.f, p.y + 1.f), ImVec2(btnEnd.x - 1.f, btnEnd.y - 1.f),
                AccentU32(0.08f), 5.f);

        ImVec2 ts = ImGui::CalcTextSize(label);
        ImU32 tc = on ? AccentU32(1.f)
            : (hov ? IM_COL32(255, 255, 255, 230) : LabelTextDimCol());
        dl->AddText(
            ImVec2(p.x + (tabW - ts.x) * 0.5f, p.y + (barH - ts.y) * 0.5f),
            tc, label);
    }

    inline void SubTabs(const char* const names[], int count, int* selected) {
        if (!selected || count <= 0) return;
        ImGui::PushID(names[0]);

        const float barH = 30.f;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.f, 0.f));
        if (ImGui::BeginTable("##subtabs", count,
            ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX)) {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, barH);
            for (int i = 0; i < count; i++) {
                ImGui::TableSetColumnIndex(i);
                const float tabW = ImGui::GetContentRegionAvail().x;
                const bool on = (*selected == i);
                const ImVec2 p = ImGui::GetCursorScreenPos();

                ImGui::PushID(i);
                if (ImGui::InvisibleButton("##sub", ImVec2(tabW, barH)))
                    *selected = i;
                const bool hov = ImGui::IsItemHovered();
                ImGui::PopID();

                DrawSubTabCellAt(p, names[i], on, hov, tabW, barH);
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 10));
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

    inline void RadarPreviewPanel(float height = 220.f) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        if (w < 140.f) w = 140.f;
        ImGui::Dummy(ImVec2(w, height));
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 br(p.x + w, p.y + height);
        dl->AddRectFilled(p, br, IM_COL32(30, 30, 30, 255), 8.f);
        dl->AddRect(p, br, IM_COL32(255, 255, 255, 12), 8.f);
        dl->AddText(ImVec2(p.x + 12.f, p.y + 10.f), IM_COL32(230, 230, 235, 255), "Radar Preview");

        const float cx = p.x + w * 0.5f;
        const float cy = p.y + height * 0.55f;
        const float rad = (w < height ? w : height) * 0.34f;
        dl->AddCircle(ImVec2(cx, cy), rad, IM_COL32(255, 255, 255, 30), 48, 1.f);
        dl->AddLine(ImVec2(cx - rad, cy), ImVec2(cx + rad, cy), IM_COL32(255, 255, 255, 20), 1.f);
        dl->AddLine(ImVec2(cx, cy - rad), ImVec2(cx, cy + rad), IM_COL32(255, 255, 255, 20), 1.f);
        dl->AddCircleFilled(ImVec2(cx, cy), 3.f, IM_COL32(255, 255, 255, 180), 12);

        const ImU32 enemy = IM_COL32(255, 80, 80, 255);
        dl->AddCircleFilled(ImVec2(cx + rad * 0.45f, cy - rad * 0.25f), 4.f, enemy, 12);
        dl->AddCircleFilled(ImVec2(cx - rad * 0.55f, cy + rad * 0.35f), 4.f, enemy, 12);
        dl->AddCircleFilled(ImVec2(cx + rad * 0.15f, cy + rad * 0.62f), 4.f, enemy, 12);
    }
}
