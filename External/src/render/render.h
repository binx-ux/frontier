#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <chrono>
#include <string>
#include <cstdio>
#include <cmath>
#include <cstring>
#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_impl_win32.h"
#include "../../ext/imgui/imgui_impl_dx11.h"
#include "../../src/core/variables/variables.h"
#include "../../src/core/globals/globals.h"
#include "../../src/core/cache/cache.h"
#include "../../src/memory/memory.h"
#include "../../src/sdk/offsets.h"
#include "../../src/sdk/structs.h"
#include "../../src/sdk/window_manager.h"
#include "../../src/sdk/w2s.h"
#include "../../src/render/avatar_cache.h"
#include "../../src/render/ui_fx.h"
#include "../../src/render/spotify_player.h"
#include "../../src/render/matcha_menu.h"
#include "../../src/core/telemetry/telemetry.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    if (msg == WM_CLOSE) {
        Globals::running = false;
        DestroyWindow(hWnd);
        return 0;
    }
    if (msg == WM_DESTROY) {
        Globals::running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

namespace UI {
    inline ImVec4 V4(const float c[4]) { return ImVec4(c[0], c[1], c[2], c[3]); }
    inline ImU32 U32(const float c[4], float a = -1.f) {
        float aa = (a < 0) ? c[3] : a;
        return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(aa * 255));
    }

    inline void ApplyStyle() {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 16.0f;
        s.ChildRounding = 12.0f;
        s.FrameRounding = 10.0f;
        s.GrabRounding = 10.0f;
        s.PopupRounding = 10.0f;
        s.ScrollbarRounding = 10.0f;
        s.TabRounding = 10.0f;
        s.WindowPadding = ImVec2(14, 14);
        s.FramePadding = ImVec2(11, 7);
        s.ItemSpacing = ImVec2(10, 9);
        s.ItemInnerSpacing = ImVec2(8, 6);
        s.ScrollbarSize = 9.0f;
        s.GrabMinSize = 11.0f;
        s.WindowBorderSize = 0.0f;
        s.ChildBorderSize = 1.0f;
        s.FrameBorderSize = 0.0f;
        s.AntiAliasedLines = true;
        s.AntiAliasedFill = true;
        s.CurveTessellationTol = 0.8f;

        auto& c = s.Colors;
        c[ImGuiCol_WindowBg] = V4(variables::Theme::bg);
        c[ImGuiCol_ChildBg] = V4(variables::Theme::card);
        c[ImGuiCol_Border] = V4(variables::Theme::border);
        c[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.11f, 0.13f, 1);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.15f, 0.18f, 1);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.18f, 0.20f, 1);
        c[ImGuiCol_CheckMark] = V4(variables::Theme::accent);
        c[ImGuiCol_SliderGrab] = V4(variables::Theme::accent);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.94f, 0.94f, 0.96f, 1);
        c[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.17f, 1);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.23f, 1);
        c[ImGuiCol_ButtonActive] = ImVec4(0.82f, 0.82f, 0.86f, 1);
        c[ImGuiCol_Header] = ImVec4(0.14f, 0.15f, 0.17f, 1);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.20f, 0.22f, 1);
        c[ImGuiCol_HeaderActive] = V4(variables::Theme::accent);
        c[ImGuiCol_Text] = V4(variables::Theme::text);
        c[ImGuiCol_TextDisabled] = V4(variables::Theme::textDim);
        c[ImGuiCol_Separator] = V4(variables::Theme::border);
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.06f, 0.5f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.27f, 0.30f, 1);
    }

    inline const char* KeyName(int vk) {
        switch (vk) {
        case 0: return "None";
        case VK_LBUTTON: return "lmb";
        case VK_RBUTTON: return "rmb";
        case VK_MBUTTON: return "mmb";
        case VK_XBUTTON1: return "x1";
        case VK_XBUTTON2: return "x2";
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

    inline void Keybind(const char* id, int* key) {
        ImGui::PushID(id);
        char label[32];
        sprintf_s(label, "%s", KeyName(*key));
        static DWORD rebindIgnoreUntil = 0;
        if (variables::waitingForKey && variables::keyToRebind == key) {
            if (ImGui::Button("...", ImVec2(52, 0))) {}
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                variables::waitingForKey = false; variables::keyToRebind = nullptr;
            }
            else if (GetTickCount() >= rebindIgnoreUntil) {
                // Prefer keyboard; allow mouse side buttons after ignore window
                for (int vk = 8; vk < 255; vk++) { // skip mouse 1-7 during rebind spam
                    if (vk == VK_ESCAPE || vk == VK_LBUTTON || vk == VK_RBUTTON) continue;
                    if (GetAsyncKeyState(vk) & 0x8000) {
                        *key = vk; variables::waitingForKey = false; variables::keyToRebind = nullptr; break;
                    }
                }
                // Allow X1/X2 mouse after delay
                if (variables::waitingForKey) {
                    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
                        *key = VK_XBUTTON1; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                    else if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
                        *key = VK_XBUTTON2; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                    else if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                        *key = VK_MBUTTON; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                }
            }
        }
        else {
            if (ImGui::Button(label, ImVec2(52, 0))) {
                variables::waitingForKey = true;
                variables::keyToRebind = key;
                rebindIgnoreUntil = GetTickCount() + 250; // ignore click that opened rebind
            }
        }
        ImGui::PopID();
    }

    inline bool ToggleRow(const char* label, bool* v, int* key = nullptr) {
        bool changed = ImGui::Checkbox(label, v);
        if (key) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x > 60 ? ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 60 : 0);
            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - 56);
            Keybind(label, key);
        }
        return changed;
    }

    inline void BeginCard(const char* title) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, V4(variables::Theme::border));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild(title, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetCursorScreenPos();
        float cw = ImGui::GetContentRegionAvail().x;
        dl->AddRectFilled(wp, ImVec2(wp.x + cw, wp.y + 28), IM_COL32(18, 18, 22, 255), 10.0f, ImDrawFlags_RoundCornersTop);
        dl->AddRectFilled(ImVec2(wp.x, wp.y + 4), ImVec2(wp.x + 3, wp.y + 24), U32(variables::Theme::accent), 1.0f);
        dl->AddLine(ImVec2(wp.x, wp.y + 27), ImVec2(wp.x + cw, wp.y + 27), U32(variables::Theme::border, 0.65f));
        dl->PushClipRect(ImVec2(wp.x + 14, wp.y), ImVec2(wp.x + cw - 8, wp.y + 28), true);
        ImGui::SetCursorScreenPos(ImVec2(wp.x + 14, wp.y + 6));
        ImGui::TextColored(V4(variables::Theme::accent), "%s", title);
        dl->PopClipRect();
        ImGui::SetCursorScreenPos(ImVec2(wp.x, wp.y + 28));
        ImGui::Dummy(ImVec2(cw, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 6));
        ImGui::BeginChild("##body", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);
    }

    inline void EndCard() {
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::Spacing();
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::Spacing();
    }

    // Right-aligned animated pill toggle
    inline bool PillToggle(const char* label, bool* v) {
        ImGui::PushID(label);
        const float pillW = 42.0f, pillH = 22.0f;
        const float gap = 10.0f;
        float rowW = ImGui::GetContentRegionAvail().x;
        float labelMax = rowW - pillW - gap;
        if (labelMax < 20.0f) labelMax = 20.0f;
        float dt = ImGui::GetIO().DeltaTime;

        // Ellipsis so labels never clip mid-glyph
        char shown[96];
        strncpy_s(shown, label, _TRUNCATE);
        if (ImGui::CalcTextSize(shown).x > labelMax) {
            size_t n = strlen(shown);
            while (n > 1) {
                n--;
                shown[n] = 0;
                char test[104];
                sprintf_s(test, "%s...", shown);
                if (ImGui::CalcTextSize(test).x <= labelMax) {
                    strcat_s(shown, "...");
                    break;
                }
            }
        }

        ImVec2 start = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(shown[0] ? shown : label);

        ImGui::SameLine(0, 0);
        ImGui::SetCursorScreenPos(ImVec2(start.x + rowW - pillW, start.y + 1.0f));
        ImVec2 p = ImGui::GetCursorScreenPos();

        // Smooth 0→1 per-toggle via ImGui storage
        ImGuiID sid = ImGui::GetID("##pillanim");
        ImGuiStorage* st = ImGui::GetStateStorage();
        float anim = st->GetFloat(sid, *v ? 1.0f : 0.0f);
        float target = *v ? 1.0f : 0.0f;
        float speed = 14.0f;
        if (anim < target) { anim += dt * speed; if (anim > target) anim = target; }
        else if (anim > target) { anim -= dt * speed; if (anim < target) anim = target; }
        st->SetFloat(sid, anim);

        float ar = variables::Theme::accent[0], ag = variables::Theme::accent[1], ab = variables::Theme::accent[2];
        ImU32 bgOff = IM_COL32(22, 22, 28, 255);
        ImU32 bgOn = IM_COL32((int)(ar * 255), (int)(ag * 255), (int)(ab * 255), 255);
        auto lerpU8 = [](int a, int b, float t) -> int {
            return (int)(a + (b - a) * t);
        };
        ImU32 bg = IM_COL32(
            lerpU8(22, (int)(ar * 255), anim),
            lerpU8(22, (int)(ag * 255), anim),
            lerpU8(28, (int)(ab * 255), anim),
            255);
        (void)bgOff; (void)bgOn;
        dl->AddRectFilled(p, ImVec2(p.x + pillW, p.y + pillH), bg, 100.0f);
        if (anim > 0.05f)
            dl->AddRect(p, ImVec2(p.x + pillW, p.y + pillH),
                IM_COL32((int)(ar * 255), (int)(ag * 255), (int)(ab * 255), (int)(140 * anim)), 100.0f, 0, 1.2f);
        float knob = pillH - 4.0f;
        float kxOff = p.x + 2.0f;
        float kxOn = p.x + pillW - knob - 2.0f;
        float kx = kxOff + (kxOn - kxOff) * anim;
        dl->AddCircleFilled(ImVec2(kx + knob * 0.5f, p.y + pillH * 0.5f), knob * 0.5f, IM_COL32(245, 245, 248, 255));
        dl->AddCircleFilled(ImVec2(kx + knob * 0.5f, p.y + pillH * 0.5f), knob * 0.32f, IM_COL32(12, 12, 14, 255));
        bool pressed = ImGui::InvisibleButton("##pill", ImVec2(pillW, pillH));
        if (pressed) *v = !*v;
        ImGui::PopID();
        return pressed;
    }

    inline bool PillTab(const char* label, bool active) {
        ImGui::PushID(label);
        ImGuiID sid = ImGui::GetID("##tabanim");
        ImGuiStorage* st = ImGui::GetStateStorage();
        float anim = st->GetFloat(sid, active ? 1.0f : 0.0f);
        float target = active ? 1.0f : 0.0f;
        float dt = ImGui::GetIO().DeltaTime;
        float speed = 12.0f;
        if (anim < target) { anim += dt * speed; if (anim > target) anim = target; }
        else if (anim > target) { anim -= dt * speed; if (anim < target) anim = target; }
        st->SetFloat(sid, anim);

        ImVec4 bg = ImVec4(0.15f * anim, 0.15f * anim, 0.17f * anim, anim > 0.01f ? 1.f : 0.f);
        ImVec4 tx = ImVec4(
            0.55f + 0.39f * anim,
            0.55f + 0.39f * anim,
            0.58f + 0.38f * anim, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Button, bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.16f, 0.18f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, tx);
        bool clicked = ImGui::Button(label, ImVec2(0, 32));
        ImGui::PopStyleColor(4);
        if (anim > 0.02f) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            float mid = (min.x + max.x) * 0.5f;
            float half = (max.x - min.x) * 0.5f * anim;
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(mid - half, max.y - 2.0f),
                ImVec2(mid + half, max.y),
                U32(variables::Theme::accent, 0.85f * anim), 2.0f);
        }
        ImGui::PopID();
        return clicked;
    }
}

class OverlayWindow {
private:
    HWND windowHandle;
    WNDCLASSEXW windowClass;
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dContext;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* renderTarget;

    void SetupD3D11(HWND hwnd) {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL obtained;
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd, &swapChain, &d3dDevice, &obtained, &d3dContext);

        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (backBuffer) {
            d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
            backBuffer->Release();
        }
    }

    void CleanupD3D11() {
        if (renderTarget) { renderTarget->Release(); renderTarget = nullptr; }
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
        if (d3dContext) { d3dContext->Release(); d3dContext = nullptr; }
        if (d3dDevice) { d3dDevice->Release(); d3dDevice = nullptr; }
    }

public:
    OverlayWindow() : windowHandle(nullptr), d3dDevice(nullptr), d3dContext(nullptr),
        swapChain(nullptr), renderTarget(nullptr) {
        ZeroMemory(&windowClass, sizeof(windowClass));
    }

    ID3D11Device* GetDevice() const { return d3dDevice; }

    bool Initialize() {
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = OverlayWndProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"MatchWareExternal";
        if (!RegisterClassExW(&windowClass)) return false;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        windowHandle = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            windowClass.lpszClassName, L"Match-Ware",
            WS_POPUP, 0, 0, screenW, screenH, nullptr, nullptr, windowClass.hInstance, nullptr);
        if (!windowHandle) return false;

        SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), 255, LWA_ALPHA);
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(windowHandle, &margins);
        ShowWindow(windowHandle, SW_SHOW);
        UpdateWindow(windowHandle);
        SetupD3D11(windowHandle);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        UI::ApplyStyle();
        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplDX11_Init(d3dDevice, d3dContext);
        AvatarCache::Init(d3dDevice);
        return true;
    }

    void BeginFrame() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                Globals::running = false;
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        WindowManager::UpdateRobloxWindowInfo();
        WindowManager::AdjustOverlayPosition(windowHandle);
        W2S::BeginFrame(); // keep viewport cache in sync with overlay size every frame

        // Click-through unless cursor is over menu / floating header / spotify / consent
        bool overMenu = false;
        if (!variables::Loading::active && !Telemetry::consentPending.load()) {
            float cx, cy;
            W2S::GetCursorClient(cx, cy);
            if (variables::menuOpen) {
                overMenu = cx >= variables::Misc::menuX && cy >= variables::Misc::menuY &&
                    cx <= variables::Misc::menuX + variables::Misc::menuW &&
                    cy <= variables::Misc::menuY + variables::Misc::menuH;
            }
            // Spotify mini must stay clickable even when the main menu is closed
            bool overSpot = variables::Audio::spotifyMini && variables::Misc::spotW > 0 &&
                cx >= variables::Misc::spotX && cy >= variables::Misc::spotY &&
                cx <= variables::Misc::spotX + variables::Misc::spotW &&
                cy <= variables::Misc::spotY + variables::Misc::spotH;
            overMenu = overMenu || overSpot;
            if (variables::Theme::useFloatingHeader && variables::Misc::floatW > 0) {
                bool overFloat = cx >= variables::Misc::floatX && cy >= variables::Misc::floatY &&
                    cx <= variables::Misc::floatX + variables::Misc::floatW &&
                    cy <= variables::Misc::floatY + variables::Misc::floatH;
                overMenu = overMenu || overFloat;
            }
        }
        bool consentOpen = Telemetry::consentPending.load();
        bool clickable = consentOpen || variables::Loading::active || overMenu || variables::waitingForKey;
        LONG style = WS_EX_TOPMOST | WS_EX_LAYERED;
        if (!consentOpen)
            style |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        if (!clickable) style |= WS_EX_TRANSPARENT;
        static LONG lastStyle = 0;
        if (style != lastStyle) {
            SetWindowLong(windowHandle, GWL_EXSTYLE, style);
            SetWindowPos(windowHandle, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
            lastStyle = style;
        }
        if (consentOpen) {
            SetForegroundWindow(windowHandle);
            SetFocus(windowHandle);
            SetActiveWindow(windowHandle);
        }

        bool hideCapture = variables::Misc::streamProof || variables::Misc::streamerModePlus;
        SetWindowDisplayAffinity(windowHandle, hideCapture ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void RenderLoading() {
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        float p = variables::Loading::progress;
        if (p < 0) p = 0; if (p > 1) p = 1;
        UIFx::DrawLoadingFX(dl, ds, p);

        float t = UIFx::timeAcc;
        float fade = UIFx::Clampf(t / 0.35f, 0.f, 1.f);
        char loadLine[96];
        if (variables::Loading::failed)
            sprintf_s(loadLine, "%s", variables::Loading::error[0] ? variables::Loading::error : "Session failed");
        else {
            int dots = ((int)(t * 2.5f) % 3) + 1;
            char dbuf[4] = "...";
            dbuf[dots] = 0;
            const char* base = variables::Loading::status[0] ? variables::Loading::status : "Loading session";
            sprintf_s(loadLine, "%s%s", base, dbuf);
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ds);
        ImGui::Begin("##loading", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

        float cx = ds.x * 0.5f;
        float cy = ds.y * 0.40f;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);

        // Percent inside ring
        if (!variables::Loading::failed) {
            char pct[16];
            sprintf_s(pct, "%d%%", (int)(p * 100.f + 0.5f));
            ImGui::SetWindowFontScale(1.35f);
            ImVec2 ps = ImGui::CalcTextSize(pct);
            ImGui::SetCursorPos(ImVec2(cx - ps.x * 0.5f, cy - ps.y * 0.5f));
            ImGui::TextColored(ImVec4(0.94f, 0.94f, 0.96f, 1), "%s", pct);
            ImGui::SetWindowFontScale(1.0f);
        }

        // Brand — hero signal
        {
            const char* a = "Match-";
            const char* b = "Ware";
            ImGui::SetWindowFontScale(2.15f);
            ImVec2 sa = ImGui::CalcTextSize(a);
            ImVec2 sb = ImGui::CalcTextSize(b);
            float tw = sa.x + sb.x;
            float brandY = cy + 108.f;
            ImGui::SetCursorPos(ImVec2(cx - tw * 0.5f, brandY));
            ImGui::TextColored(ImVec4(0.94f, 0.94f, 0.96f, 1), "%s", a);
            ImGui::SameLine(0, 0);
            ImGui::TextColored(UI::V4(variables::Theme::accent), "%s", b);
            ImGui::SetWindowFontScale(1.0f);

            const char* sub = "EXTERNAL";
            ImVec2 ss = ImGui::CalcTextSize(sub);
            ImGui::SetCursorPos(ImVec2(cx - ss.x * 0.5f, brandY + 42.f));
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.60f, 1), "%s", sub);
        }

        // Status
        {
            ImVec2 ts = ImGui::CalcTextSize(loadLine);
            float statusY = cy + 172.f;
            ImGui::SetCursorPos(ImVec2(cx - ts.x * 0.5f, statusY));
            if (variables::Loading::failed)
                ImGui::TextColored(ImVec4(1.f, 0.42f, 0.40f, 1), "%s", loadLine);
            else
                ImGui::TextColored(UI::V4(variables::Theme::accent), "%s", loadLine);
        }

        // Progress bar + shimmer
        if (!variables::Loading::failed) {
            float barW = 280.0f;
            float barH = 4.0f;
            float barX = cx - barW * 0.5f;
            float barY = cy + 202.f;
            ImVec2 a(barX, barY), b(barX + barW, barY + barH);
            dl->AddRectFilled(a, b, IM_COL32(28, 28, 34, (int)(230 * fade)), 3.0f);
            float fill = barW * p;
            if (fill > 0.5f) {
                dl->AddRectFilled(a, ImVec2(barX + fill, barY + barH), UI::U32(variables::Theme::accent, fade), 3.0f);
                float shim = fmodf(t * 1.6f, 1.2f) - 0.1f;
                float sx = barX + fill * UIFx::Clampf(shim, 0.f, 1.f);
                dl->AddRectFilled(
                    ImVec2(sx - 18.f, barY), ImVec2(sx + 8.f, barY + barH),
                    IM_COL32(255, 255, 255, (int)(70 * fade)), 3.0f);
            }

            // Step dots
            const char* steps[] = { "Attach", "Offsets", "Game", "Ready" };
            float thresholds[] = { 0.20f, 0.45f, 0.70f, 0.95f };
            float dotsY = barY + 28.f;
            float span = 220.f;
            float startX = cx - span * 0.5f;
            for (int i = 0; i < 4; i++) {
                float dx = startX + (span / 3.f) * i;
                bool on = p >= thresholds[i];
                bool cur = !on && (i == 0 || p >= thresholds[i - 1]);
                ImU32 dc = on ? UI::U32(variables::Theme::accent, fade)
                    : (cur ? IM_COL32(180, 180, 190, (int)(220 * fade)) : IM_COL32(60, 60, 68, (int)(200 * fade)));
                dl->AddCircleFilled(ImVec2(dx, dotsY), on || cur ? 4.2f : 3.2f, dc, 12);
                if (i < 3) {
                    dl->AddLine(ImVec2(dx + 8.f, dotsY), ImVec2(dx + (span / 3.f) - 8.f, dotsY),
                        IM_COL32(50, 50, 56, (int)(180 * fade)), 1.2f);
                }
                ImVec2 ls = ImGui::CalcTextSize(steps[i]);
                ImGui::SetCursorPos(ImVec2(dx - ls.x * 0.5f, dotsY + 10.f));
                ImGui::TextColored(on ? ImVec4(0.85f, 0.85f, 0.88f, 1) : ImVec4(0.45f, 0.45f, 0.48f, 1), "%s", steps[i]);
            }
        }

        // Supported games footer
        if (!variables::Loading::failed) {
            const char* games = "Supports  Arsenal  ·  MiscGunTest:X";
            ImVec2 gs = ImGui::CalcTextSize(games);
            ImGui::SetCursorPos(ImVec2(cx - gs.x * 0.5f, ds.y - 64.f));
            ImGui::TextColored(ImVec4(0.42f, 0.42f, 0.46f, 1), "%s", games);
            // Extra MGT tip while finishing load
            if (strstr(variables::Loading::status, "MiscGunTest") != nullptr) {
                const char* tip = "MiscGunTest:X — do not use Hitbox Extender (ban risk)";
                ImVec2 tipS = ImGui::CalcTextSize(tip);
                ImGui::SetCursorPos(ImVec2(cx - tipS.x * 0.5f, ds.y - 42.f));
                ImGui::TextColored(ImVec4(1.f, 0.45f, 0.35f, 1), "%s", tip);
            }
        }
        else {
            const char* hint = "Press Esc to exit";
            ImVec2 hs = ImGui::CalcTextSize(hint);
            ImGui::SetCursorPos(ImVec2(cx - hs.x * 0.5f, cy + 230.f));
            ImGui::TextColored(UI::V4(variables::Theme::textDim), "%s", hint);
        }

        ImGui::PopStyleVar();
        ImGui::End();
    }

    void ApplySmoothProfile() {
        switch (variables::Aimbot::smoothProfile) {
        case 1: // Legit
            variables::Aimbot::smoothing = 18.0f;
            variables::Aimbot::predictionX = 0.08f;
            variables::Aimbot::predictionY = 0.08f;
            variables::Aimbot::fovRadius = 80.0f;
            break;
        case 2: // Semi
            variables::Aimbot::smoothing = 12.0f;
            variables::Aimbot::predictionX = 0.12f;
            variables::Aimbot::predictionY = 0.12f;
            variables::Aimbot::fovRadius = 130.0f;
            break;
        case 3: // Rage
            variables::Aimbot::smoothing = 5.0f;
            variables::Aimbot::predictionX = 0.2f;
            variables::Aimbot::predictionY = 0.2f;
            variables::Aimbot::fovRadius = 220.0f;
            break;
        default: break;
        }
        // Keep easy 0–100 UI knobs in sync
        variables::Aimbot::uiSmoothness = (variables::Aimbot::smoothing - 4.f) / 26.f * 100.f;
        variables::Aimbot::uiFov = (variables::Aimbot::fovRadius - 20.f) / 480.f * 100.f;
        if (variables::Aimbot::uiSmoothness < 0.f) variables::Aimbot::uiSmoothness = 0.f;
        if (variables::Aimbot::uiSmoothness > 100.f) variables::Aimbot::uiSmoothness = 100.f;
        if (variables::Aimbot::uiFov < 0.f) variables::Aimbot::uiFov = 0.f;
        if (variables::Aimbot::uiFov > 100.f) variables::Aimbot::uiFov = 100.f;
    }

    void RenderMenu() {
        if (variables::Loading::active) return;

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;

        // Floating icon bar always drawn & clickable (opens menu)
        if (variables::Theme::useFloatingHeader && !Telemetry::consentPending.load()) {
            int prevTab = variables::selectedTab;
            UIFx::FloatingHeader(&variables::selectedTab);
            if (variables::selectedTab != prevTab)
                variables::selectedSub = 0;
        }

        // Smooth open / close (alpha + scale + slight rise)
        float target = variables::menuOpen ? 1.f : 0.f;
        float speed = variables::Misc::menuAnimSpeed;
        if (speed < 4.f) speed = 4.f;
        float t = 1.f - expf(-speed * dt);
        variables::Misc::menuAnim += (target - variables::Misc::menuAnim) * t;
        if (fabsf(variables::Misc::menuAnim - target) < 0.001f)
            variables::Misc::menuAnim = target;

        if (variables::Misc::menuAnim < 0.01f) return;

        if (variables::Theme::bgEffect)
            UIFx::DrawBackgroundFX(bg, ds, dt);

        float anim = variables::Misc::menuAnim;
        // Ease-out cubic for scale feel
        float ease = 1.f - powf(1.f - anim, 3.f);
        float scale = 0.88f + 0.12f * ease;
        float baseW = 600.f, baseH = 780.f;
        float winW = baseW * scale;
        float winH = baseH * scale;
        float rise = (1.f - ease) * 28.f;

        static int layoutVer = 4;
        if (layoutVer == 4) {
            ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
            layoutVer = 5;
        }
        ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(520, 560), ImVec2(720, 1000));
        ImGui::SetNextWindowPos(
            ImVec2(ds.x * 0.5f - winW * 0.5f, ds.y * 0.5f - winH * 0.5f + rise),
            ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ease);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, MatchaUI::V4(variables::Theme::bg));
        ImGui::PushStyleColor(ImGuiCol_Border, MatchaUI::V4(variables::Theme::border));
        ImGui::Begin("##mw", &variables::menuOpen,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float ww = ImGui::GetWindowSize().x;
        float wh = ImGui::GetWindowSize().y;
        const float TOP_H = 48.0f;
        const float FOOT_H = 28.0f;

        wdl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + TOP_H), IM_COL32(14, 14, 16, 255));
        wdl->AddLine(ImVec2(wp.x, wp.y + TOP_H - 1), ImVec2(wp.x + ww, wp.y + TOP_H - 1),
            MatchaUI::U32(variables::Theme::border, 0.7f));

        ImGui::SetCursorPos(ImVec2(16, 10));
        ImGui::TextColored(ImVec4(0.94f, 0.94f, 0.96f, 1), "Match-");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::accent), "Ware");
        ImGui::SetCursorPos(ImVec2(16, 28));
        static const char* tabNames[] = { "Combat", "Visuals", "World", "Character", "Options", "Configs", "Servers", "Music", "Status" };
        int ti = variables::selectedTab;
        if (ti < 0 || ti > 8) ti = 0;
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", tabNames[ti]);

        ImGui::SetCursorPos(ImVec2(10, TOP_H + 4));
        float bodyH = wh - TOP_H - FOOT_H - 8;
        if (bodyH < 80) bodyH = 80;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
        ImGui::BeginChild("##navbody", ImVec2(ww - 20, bodyH), false);

        // 3 rows x 3 equal tabs
        float avail = ImGui::GetContentRegionAvail().x;
        float gap = 4.0f;
        float tabW = (avail - gap * 2.0f) / 3.0f;
        if (tabW < 72.f) tabW = 72.f;
        for (int i = 0; i < 9; i++) {
            if (i % 3) ImGui::SameLine(0, gap);
            bool on = variables::selectedTab == i;
            ImGui::PushStyleColor(ImGuiCol_Button, on ? ImVec4(0.16f, 0.16f, 0.18f, 1) : ImVec4(0.08f, 0.08f, 0.09f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.14f, 0.16f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.18f, 0.20f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, on ? MatchaUI::V4(variables::Theme::text) : MatchaUI::V4(variables::Theme::textDim));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            if (ImGui::Button(tabNames[i], ImVec2(tabW, 24))) {
                variables::selectedTab = i;
                variables::selectedSub = 0;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
        }
        ImGui::Spacing();
        ImGui::BeginChild("##body", ImVec2(0, -2), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        MatchaMenu::RenderBody();
        ImGui::EndChild();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        MatchaUI::DrawFooter(wdl, wp, ww, wh);

        variables::Misc::menuX = wp.x;
        variables::Misc::menuY = wp.y;
        variables::Misc::menuW = ww;
        variables::Misc::menuH = wh;
        variables::Misc::menuHovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
            ImGui::IsAnyItemActive() ||
            variables::waitingForKey;

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        MatchaUI::DrawToast(ImGui::GetForegroundDrawList(), ds, ImGui::GetIO().DeltaTime);

        if (variables::Misc::showKeybinds) {
            ImGui::SetNextWindowPos(ImVec2(16, ds.y * 0.35f), ImGuiCond_FirstUseEver);
            ImGui::Begin("##binds", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextDisabled("Keybinds");
            ImGui::Text("Menu  %s", MatchaUI::KeyName(variables::Misc::menuVk));
            ImGui::Text("Aim   %s", MatchaUI::KeyName(variables::Aimbot::aimbotKey));
            ImGui::Text("Trig  %s", MatchaUI::KeyName(variables::Trigger::key));
            ImGui::Text("Fly   %s", MatchaUI::KeyName(variables::Local::flyKey));
            ImGui::End();
        }
    }

    static std::string TruncateForSpotify(const std::string& s, float maxW) {
        if (ImGui::CalcTextSize(s.c_str()).x <= maxW) return s;
        std::string t = s;
        while (t.size() > 3 && ImGui::CalcTextSize((t + "...").c_str()).x > maxW)
            t.pop_back();
        return t + "...";
    }

    void RenderConsentGate() {
        if (!Telemetry::consentPending.load()) return;

        Telemetry::animT += ImGui::GetIO().DeltaTime;
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        bg->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(6, 6, 8, 210));

        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ds.x * 0.5f - 210, ds.y * 0.5f - 130), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22, 20));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.09f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.1f));
        ImGui::Begin("##telemetry_consent", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        ImGui::TextColored(ImVec4(0.96f, 0.96f, 0.98f, 1), "Anonymous usage notice");
        ImGui::Spacing();
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 376);
        ImGui::TextColored(ImVec4(0.62f, 0.62f, 0.66f, 1),
            "Match-Ware can send a one-line launch ping so we know the app started. "
            "It does not include your name, key, HWID, PC name, or any personal info.");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1),
            "Agree to allow the ping. Decline and nothing is sent.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.55f, 0.38f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.65f, 0.45f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.48f, 0.32f, 1));
        if (ImGui::Button("Agree", ImVec2(170, 40)))
            Telemetry::Agree();
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, 16);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.2f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.28f, 0.3f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.14f, 0.16f, 1));
        if (ImGui::Button("Decline", ImVec2(170, 40)))
            Telemetry::Deny();
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    void RenderSpotifyMini() {
        if (variables::Loading::active || Telemetry::consentPending.load() ||
            !variables::Audio::spotifyMini) {
            variables::Misc::spotW = variables::Misc::spotH = 0;
            return;
        }

        SpotifyPlayer::Refresh();

        ImGui::SetNextWindowSize(ImVec2(300, 118), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(14, 70), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.07f, 0.94f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.08f));
        ImGui::Begin("##spotify_mini", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();

        // Album placeholder / play state
        ImU32 artCol = SpotifyPlayer::playing ? IM_COL32(30, 215, 96, 255) : IM_COL32(40, 40, 48, 255);
        dl->AddRectFilled(ImVec2(wp.x + 12, wp.y + 14), ImVec2(wp.x + 56, wp.y + 58), artCol, 6.0f);
        dl->AddText(ImVec2(wp.x + 24, wp.y + 28), IM_COL32(8, 8, 10, 255), SpotifyPlayer::playing ? "||" : ">");

        ImGui::SetCursorPos(ImVec2(66, 12));
        ImGui::PushTextWrapPos(286);
        ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.97f, 1), "%s",
            TruncateForSpotify(SpotifyPlayer::trackTitle, 210).c_str());
        ImGui::SetCursorPos(ImVec2(66, 30));
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1), "%s",
            TruncateForSpotify(SpotifyPlayer::trackArtist, 210).c_str());
        ImGui::PopTextWrapPos();

        // Transport controls — visible + clickable
        ImGui::SetCursorPos(ImVec2(66, 58));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.25f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.12f, 1));

        if (ImGui::Button("|<", ImVec2(44, 28))) SpotifyPlayer::Prev();
        ImGui::SameLine(0, 8);
        if (ImGui::Button(SpotifyPlayer::playing ? "Pause" : "Play", ImVec2(70, 28)))
            SpotifyPlayer::PlayPause();
        ImGui::SameLine(0, 8);
        if (ImGui::Button(">|", ImVec2(44, 28))) SpotifyPlayer::Next();
        ImGui::SameLine(0, 8);
        if (ImGui::Button("-", ImVec2(28, 28))) SpotifyPlayer::VolDown();
        ImGui::SameLine(0, 4);
        if (ImGui::Button("+", ImVec2(28, 28))) SpotifyPlayer::VolUp();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        if (!SpotifyPlayer::connected) {
            ImGui::SetCursorPos(ImVec2(66, 92));
            ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.5f, 1), "Waiting for Spotify.exe");
        }

        variables::Misc::spotX = ImGui::GetWindowPos().x;
        variables::Misc::spotY = ImGui::GetWindowPos().y;
        variables::Misc::spotW = ImGui::GetWindowSize().x;
        variables::Misc::spotH = ImGui::GetWindowSize().y;

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    void render(ImDrawList* drawList) {
        static auto lastTime = std::chrono::high_resolution_clock::now();
        static int frameCount = 0;
        static int fps = 0;
        frameCount++;
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() >= 1000) {
            fps = frameCount;
            variables::Perf::currentFps = fps;
            frameCount = 0;
            lastTime = now;
        }

        if (variables::Misc::showFps && !variables::Loading::active) {
            char buf[128];
            if (variables::Misc::streamerModePlus)
                sprintf_s(buf, "Private  |  %d fps", fps);
            else if (variables::Misc::streamerMode)
                sprintf_s(buf, "Match-Ware  |  %d fps", fps);
            else
                sprintf_s(buf, "Match-Ware  |  %s  |  %d fps", Offsets::ClientVersion.c_str(), fps);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float x = ImGui::GetIO().DisplaySize.x - ts.x - 14;
            drawList->AddText(ImVec2(x + 1, 11), IM_COL32(0, 0, 0, 200), buf);
            drawList->AddText(ImVec2(x, 10), UI::U32(variables::Theme::accent), buf);
        }

        if (variables::Aimbot::enabled && variables::Aimbot::showFOV && !variables::Loading::active) {
            float screenW, screenH, ox, oy;
            W2S::GetViewport(screenW, screenH, ox, oy);
            // Always center — must match aimbot FOV check
            ImVec2 c(screenW * 0.5f, screenH * 0.5f);
            int a = (int)(variables::Aimbot::fovOpacity * 255);
            float fr = variables::Aimbot::fovColor[0];
            float fg = variables::Aimbot::fovColor[1];
            float fb = variables::Aimbot::fovColor[2];
            if (variables::Extra::fovRainbow) {
                float hue = fmodf((float)ImGui::GetTime() * 0.45f, 1.f);
                ImGui::ColorConvertHSVtoRGB(hue, 0.9f, 1.f, fr, fg, fb);
            }
            ImU32 col = IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a);
            if (variables::Aimbot::fovFilled)
                drawList->AddCircleFilled(c, variables::Aimbot::fovRadius,
                    IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a / 4), 64);
            if (variables::Aimbot::fovGlow)
                drawList->AddCircle(c, variables::Aimbot::fovRadius,
                    IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a / 3), 64, 4.0f);
            drawList->AddCircle(c, variables::Aimbot::fovRadius, col, 64, 1.5f);
        }
    }

    void EndFrame() {
        ImGui::Render();
        float clearColor[4] = { 0, 0, 0, 0 };
        d3dContext->OMSetRenderTargets(1, &renderTarget, nullptr);
        d3dContext->ClearRenderTargetView(renderTarget, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        static LARGE_INTEGER freq{};
        static LARGE_INTEGER lastPresent{};
        static bool qpcInit = false;
        if (!qpcInit) {
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(&lastPresent);
            qpcInit = true;
        }

        if (variables::Perf::vsync) {
            swapChain->Present(1, 0);
        } else {
            swapChain->Present(0, 0);
            int target = variables::Perf::targetFps;
            if (target > 0 && target <= 480 && freq.QuadPart > 0) {
                const LONGLONG ticksPerFrame = freq.QuadPart / target;
                LARGE_INTEGER now{};
                QueryPerformanceCounter(&now);
                LONGLONG elapsed = now.QuadPart - lastPresent.QuadPart;
                if (elapsed < ticksPerFrame) {
                    LONGLONG remain = ticksPerFrame - elapsed;
                    // Sleep most of the wait, spin the last ~1ms for smoother pacing
                    double remainMs = (1000.0 * (double)remain) / (double)freq.QuadPart;
                    if (remainMs > 1.5) {
                        Sleep((DWORD)(remainMs - 1.0));
                        QueryPerformanceCounter(&now);
                        remain = ticksPerFrame - (now.QuadPart - lastPresent.QuadPart);
                    }
                    while (remain > 0) {
                        QueryPerformanceCounter(&now);
                        remain = ticksPerFrame - (now.QuadPart - lastPresent.QuadPart);
                    }
                }
            }
        }
        QueryPerformanceCounter(&lastPresent);
    }

    void Cleanup() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupD3D11();
        if (windowHandle) { DestroyWindow(windowHandle); windowHandle = nullptr; }
        UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
    }

    HWND GetWindowHandle() const { return windowHandle; }
};
