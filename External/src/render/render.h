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
#include "../../src/render/embedded_fonts.h"
#include "../../src/render/brand_assets.h"
#include "../../src/render/brand.h"
#include "../../src/render/frontier_shell.h"
#include "../../src/render/frontier_menu.h"
#include "../../src/core/features/aimbot/aimbot.h"
#include "../../src/core/telemetry/telemetry.h"
#include "../../src/core/updater/updater.h"
#include "../../src/core/servers/server_browser.h"

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

    inline void ApplyStyle() {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 12.0f;
        s.ChildRounding = 10.0f;
        s.FrameRounding = 8.0f;
        s.GrabRounding = 8.0f;
        s.PopupRounding = 10.0f;
        s.ScrollbarRounding = 6.0f;
        s.TabRounding = 8.0f;
        s.WindowPadding = ImVec2(12, 10);
        s.FramePadding = ImVec2(10, 6);
        s.ItemSpacing = ImVec2(8, 6);
        s.ItemInnerSpacing = ImVec2(8, 4);
        s.ScrollbarSize = 6.0f;
        s.GrabMinSize = 12.0f;
        s.WindowBorderSize = 1.0f;
        s.ChildBorderSize = 1.0f;
        s.FrameBorderSize = 0.0f;
        s.PopupBorderSize = 1.0f;
        s.AntiAliasedLines = true;
        s.AntiAliasedFill = true;
        s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

        auto& c = s.Colors;
        c[ImGuiCol_WindowBg] = V4(variables::Theme::bg);
        c[ImGuiCol_ChildBg] = V4(variables::Theme::card);
        c[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.08f, 0.98f);
        c[ImGuiCol_Border] = V4(variables::Theme::border);
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.10f, 1);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.12f, 0.14f, 1);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.18f, 1);
        c[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.07f, 1);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1);
        c[ImGuiCol_CheckMark] = V4(variables::Theme::brand);
        c[ImGuiCol_SliderGrab] = V4(variables::Theme::accent);
        c[ImGuiCol_SliderGrabActive] = ImVec4(1.f, 1.f, 1.f, 1);
        c[ImGuiCol_Button] = ImVec4(0.13f, 0.13f, 0.15f, 1);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.21f, 1);
        c[ImGuiCol_ButtonActive] = ImVec4(0.88f, 0.88f, 0.92f, 1);
        c[ImGuiCol_Header] = ImVec4(0.13f, 0.13f, 0.15f, 1);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.17f, 0.20f, 1);
        c[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.22f, 0.26f, 1);
        c[ImGuiCol_Text] = V4(variables::Theme::text);
        c[ImGuiCol_TextDisabled] = V4(variables::Theme::textDim);
        c[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.24f, 0.7f);
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.04f, 0.05f, 0.35f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.32f, 1);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.45f, 1);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.60f, 1);
        c[ImGuiCol_ResizeGrip] = ImVec4(0.3f, 0.3f, 0.34f, 0.4f);
        c[ImGuiCol_ResizeGripHovered] = ImVec4(0.7f, 0.7f, 0.75f, 0.6f);
    }

    inline void ApplyIfDirty() {
        if (!variables::Theme::styleDirty) return;
        ApplyStyle();
        variables::Theme::styleDirty = false;
    }
}

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

namespace CaptureExclusion {
    inline void Apply(HWND hwnd, bool hide)
    {
        if (!hwnd || !IsWindow(hwnd)) return;

        const DWORD affinity = hide ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        if (!SetWindowDisplayAffinity(hwnd, affinity)) {
            if (hide)
                SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
            else
                SetWindowDisplayAffinity(hwnd, WDA_NONE);
        }

        BOOL dwmHide = hide ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, 12, &dwmHide, sizeof(dwmHide)); // DWMWA_EXCLUDED_FROM_PEEK
        DwmSetWindowAttribute(hwnd, 11, &dwmHide, sizeof(dwmHide)); // DWMWA_DISALLOW_PEEK
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
        windowClass.lpszClassName = L"FrontierExternal";
        if (!RegisterClassExW(&windowClass)) return false;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        windowHandle = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            windowClass.lpszClassName, L"FRONTIER",
            WS_POPUP, 0, 0, screenW, screenH, nullptr, nullptr, windowClass.hInstance, nullptr);
        if (!windowHandle) return false;

        SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), 255, LWA_ALPHA);
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(windowHandle, &margins);
        ShowWindow(windowHandle, SW_SHOW);
        UpdateWindow(windowHandle);
        variables::Misc::overlayHwnd = windowHandle;
        SetupD3D11(windowHandle);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        // Persist layout in <exe_dir>\frontier\imgui.ini
        {
            static char iniPath[MAX_PATH]{};
            char exeDir[MAX_PATH]{};
            if (GetModuleFileNameA(nullptr, exeDir, MAX_PATH) > 0) {
                char* slash = strrchr(exeDir, '\\');
                if (slash) *slash = 0;
                char cfgDir[MAX_PATH]{};
                sprintf_s(cfgDir, "%s\\%s", exeDir, Frontier::kIniFolder);
                CreateDirectoryA(cfgDir, nullptr);
                sprintf_s(iniPath, "%s\\imgui.ini", cfgDir);
                io.IniFilename = iniPath;
            } else {
                io.IniFilename = nullptr;
            }
        }
        EmbeddedFonts::Apply(io);
        UI::ApplyStyle();
        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplDX11_Init(d3dDevice, d3dContext);
        AvatarCache::Init(d3dDevice);
        BrandAssets::LoadLogo(d3dDevice);
        CaptureExclusion::Apply(windowHandle, variables::Misc::streamProof || variables::Misc::streamerModePlus);
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
        AvatarCache::ProcessPending();

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
            if (variables::Theme::useFloatingHeader && variables::Misc::floatingPanelOpen &&
                variables::Misc::panelW > 0) {
                bool overPanel = cx >= variables::Misc::panelX && cy >= variables::Misc::panelY &&
                    cx <= variables::Misc::panelX + variables::Misc::panelW &&
                    cy <= variables::Misc::panelY + variables::Misc::panelH;
                overMenu = overMenu || overPanel;
            }
        }
        bool consentOpen = Telemetry::consentPending.load();
        bool clickable = consentOpen || variables::Loading::active || overMenu ||
            variables::waitingForKey || variables::Servers::redirecting ||
            Updater::outOfDate.load();
        LONG style = WS_EX_TOPMOST | WS_EX_LAYERED;
        if (!consentOpen)
            style |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        if (!clickable) style |= WS_EX_TRANSPARENT;
        static LONG lastStyle = 0;
        const bool styleChanged = (style != lastStyle);
        if (styleChanged) {
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

        const bool hideCapture = variables::Misc::streamProof || variables::Misc::streamerModePlus;
        static bool lastHideCapture = false;
        static RECT lastOverlayRect{};
        RECT overlayRect{};
        if (GetWindowRect(windowHandle, &overlayRect)) {
            const bool rectChanged =
                overlayRect.left != lastOverlayRect.left ||
                overlayRect.top != lastOverlayRect.top ||
                overlayRect.right != lastOverlayRect.right ||
                overlayRect.bottom != lastOverlayRect.bottom;
            if (rectChanged)
                lastOverlayRect = overlayRect;
            if (hideCapture || hideCapture != lastHideCapture || styleChanged || rectChanged)
                CaptureExclusion::Apply(windowHandle, hideCapture);
        } else if (hideCapture || hideCapture != lastHideCapture || styleChanged) {
            CaptureExclusion::Apply(windowHandle, hideCapture);
        }
        lastHideCapture = hideCapture;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // When the menu is closed, don't let ImGui hold the OS cursor.
        if (!clickable) {
            ImGuiIO& io = ImGui::GetIO();
            io.MouseDrawCursor = false;
            io.WantCaptureMouse = false;
            io.WantCaptureKeyboard = false;
            for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); ++i)
                io.MouseDown[i] = false;
        }
    }

    void RenderLoading() {
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();

        static float displayProgress = 0.f;
        float target = variables::Loading::failed ? 0.f : variables::Loading::progress;
        float dt = ImGui::GetIO().DeltaTime;
        displayProgress += (target - displayProgress) * (dt * 6.f);
        if (target >= 0.995f || target <= displayProgress + 0.002f)
            displayProgress = target;

        UIFx::DrawLoadingFX(dl, ds, displayProgress, 1.f);

        char loadLine[128];
        if (variables::Loading::failed)
            sprintf_s(loadLine, "%s", variables::Loading::error[0] ? variables::Loading::error : "Session failed");
        else {
            const char* base = variables::Loading::status[0] ? variables::Loading::status : "Initializing";
            sprintf_s(loadLine, "%s", base);
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ds);
        ImGui::Begin("##loading", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

        const float textY = ds.y * 0.42f + 72.f;
        ImVec2 ls = ImGui::CalcTextSize(loadLine);
        ImGui::SetCursorPos(ImVec2((ds.x - ls.x) * 0.5f, textY));
        if (variables::Loading::failed)
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.42f, 0.95f), "%s", loadLine);
        else
            ImGui::TextColored(ImVec4(0.52f, 0.52f, 0.58f, 0.9f), "%s", loadLine);

        if (variables::Loading::failed) {
            const char* hint = "Closing automatically…";
            ImVec2 hs = ImGui::CalcTextSize(hint);
            ImGui::SetCursorPos(ImVec2((ds.x - hs.x) * 0.5f, textY + 22.f));
            ImGui::TextColored(ImVec4(0.48f, 0.48f, 0.54f, 0.85f), "%s", hint);
        }

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
        // Keep easy 0?100 UI knobs in sync
        variables::Aimbot::uiSmoothness = (variables::Aimbot::smoothing - 4.f) / 26.f * 100.f;
        variables::Aimbot::uiFov = (variables::Aimbot::fovRadius - 20.f) / 480.f * 100.f;
        if (variables::Aimbot::uiSmoothness < 0.f) variables::Aimbot::uiSmoothness = 0.f;
        if (variables::Aimbot::uiSmoothness > 100.f) variables::Aimbot::uiSmoothness = 100.f;
        if (variables::Aimbot::uiFov < 0.f) variables::Aimbot::uiFov = 0.f;
        if (variables::Aimbot::uiFov > 100.f) variables::Aimbot::uiFov = 100.f;
    }

    static ImVec2 FloatingPanelSize(int tab, int sub, ImVec2 ds, float barBottom)
    {
        float maxH = ds.y - barBottom - 12.f;
        if (maxH < 360.f) maxH = 360.f;

        float w = 520.f, h = 480.f;
        switch (tab) {
        case 0: // Combat — tall content; use most of the screen + scroll
            w = 860.f;
            switch (sub) {
            case 0: h = maxH; break;
            case 1: h = 400.f; w = 540.f; break;
            case 2: h = 280.f; w = 480.f; break;
            default: h = 560.f; w = 720.f; break;
            }
            break;
        case 1: // Visuals
            switch (sub) {
            case 0: w = 860.f; h = maxH; break;
            case 1: w = 480.f; h = 400.f; break;
            case 2: w = 480.f; h = 420.f; break;
            default: w = 520.f; h = 500.f; break;
            }
            break;
        case 2: w = 700.f; h = 520.f; break;
        case 3: w = 760.f; h = maxH; break;
        case 4: w = 660.f; h = 600.f; break;
        case 5: w = 660.f; h = 600.f; break;
        case 6: w = 500.f; h = maxH < 700.f ? maxH : 700.f; break;
        case 7: w = 640.f; h = maxH < 720.f ? maxH : 720.f; break;
        default: break;
        }

        if (w > ds.x - 24.f) w = ds.x - 24.f;
        if (h > maxH) h = maxH;
        if (h < 220.f) h = (maxH < 220.f) ? maxH : 220.f;
        return ImVec2(w, h);
    }

    void RenderFloatingPanel() {
        if (!variables::Misc::floatingPanelOpen) {
            variables::Misc::panelW = 0;
            variables::Misc::panelH = 0;
            return;
        }

        int tab = variables::selectedTab;
        if (tab < 0 || tab > 8) tab = 0;
        int sub = variables::Misc::selectedSubByTab[tab];
        if (sub < 0) sub = 0;

        static const char* tabNames[] = {
            "Combat", "Visuals", "World", "Character", "Options",
            "Servers", "Music", "Status", "Configs"
        };

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        float barBottom = variables::Theme::headerY + variables::Misc::floatH + 10.f;
        ImVec2 sz = FloatingPanelSize(tab, sub, ds, barBottom);

        float px = (ds.x - sz.x) * 0.5f;
        float py = barBottom;

        ImGui::SetNextWindowSize(sz, ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(380.f, 200.f), ImVec2(ds.x - 16.f, ds.y - 8.f));
        {
            static bool placed[9] = {};
            if (!placed[tab]) {
                ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Always);
                placed[tab] = true;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, FrontierUI::V4(variables::Theme::bg));
        ImGui::PushStyleColor(ImGuiCol_Border, FrontierUI::V4(variables::Theme::border));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.06f, 0.06f, 0.07f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.09f, 0.09f, 0.11f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.06f, 0.06f, 0.07f, 1.f));

        char winId[48];
        sprintf_s(winId, "%s##mwpanel%d", tabNames[tab], tab);
        bool open = variables::Misc::floatingPanelOpen;
        // Outer window does not scroll — body child does (fits all content)
        ImGuiWindowFlags wflags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::Begin(winId, &open, wflags);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        variables::Misc::panelX = wp.x;
        variables::Misc::panelY = wp.y;
        variables::Misc::panelW = ws.x;
        variables::Misc::panelH = ws.y;

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        UIFx::DrawPanelAmbientGlow(ImGui::GetBackgroundDrawList(), wp, ws, 1.f);
        wdl->AddLine(ImVec2(wp.x + 12, wp.y + 1), ImVec2(wp.x + ws.x - 12, wp.y + 1),
            IM_COL32(255, 255, 255, 22), 1.f);
        wdl->AddRectFilled(ImVec2(wp.x, wp.y + 8), ImVec2(wp.x + 2.5f, wp.y + 34),
            FrontierUI::U32(variables::Theme::brand, 0.65f), 2.f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGui::BeginChild("##mwscroll", ImVec2(0, 0), ImGuiChildFlags_NavFlattened,
            ImGuiWindowFlags_AlwaysVerticalScrollbar);
        FrontierMenu::RenderTab(tab);
        ImGui::Dummy(ImVec2(0, 8)); // bottom pad so last controls aren't flush
        ImGui::EndChild();
        ImGui::PopStyleColor();

        variables::Misc::floatingPanelOpen = open;
        variables::Misc::menuHovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
            ImGui::IsAnyItemActive() ||
            variables::waitingForKey;

        ImGui::End();
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);
    }

    void RenderRedirectKick() {
        if (!variables::Servers::redirecting) return;

        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;
        variables::Servers::redirectTimer -= dt;
        if (variables::Servers::redirectTimer <= 0.f) {
            ServerBrowser::DismissDisconnectScreen();
            return;
        }

        const bool banTroll = variables::Servers::disconnectKind == 1;
        const float t = (float)ImGui::GetTime();

        // Progress animation — redirect pulses; ban troll crawls then stalls near the end
        float& prog = variables::Servers::redirectProgress;
        if (banTroll) {
            const float target = 0.87f + 0.03f * sinf(t * 0.7f);
            prog += (target - prog) * (1.f - expf(-dt * 1.4f));
            if (prog > 0.895f) prog = 0.895f + 0.004f * sinf(t * 2.2f);

            const int phase = (int)(t * 0.22f) % 3;
            const char* phases[] = {
                "Verifying account status...",
                "Contacting moderation services...",
                "Processing moderation action..."
            };
            strcpy_s(variables::Servers::redirectStatus, phases[phase]);
        } else {
            const float pulse = 0.42f + 0.38f * (0.5f + 0.5f * sinf(t * 3.2f));
            prog += (pulse - prog) * (1.f - expf(-dt * 8.f));
        }

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 185));

        const float boxW = 492.f;
        const float padX = 26.f;
        const float padY = 22.f;
        const float iconCol = 58.f;

        ImFont* bold = EmbeddedFonts::gBold ? EmbeddedFonts::gBold : ImGui::GetFont();
        ImFont* body = EmbeddedFonts::gBody ? EmbeddedFonts::gBody : ImGui::GetFont();
        ImFont* fontSmall = EmbeddedFonts::gSmall ? EmbeddedFonts::gSmall : ImGui::GetFont();
        const float boldSize = 20.f;
        const float bodySize = 16.f;
        const float smallSize = 13.f;

        const char* title = "Disconnected";
        const char* message = nullptr;
        if (variables::Servers::redirectMsg[0])
            message = variables::Servers::redirectMsg;
        else if (banTroll)
            message = "You have been banned from this experience.\nModeration message: Unauthorized third-party software detected.";
        else
            message = "Match is redirecting you, please wait";

        const char* status = nullptr;
        if (variables::Servers::redirectStatus[0])
            status = variables::Servers::redirectStatus;
        else if (banTroll)
            status = "Verifying account status...";
        else
            status = "Joining server...";

        const float wrapW = boxW - padX * 2.f - iconCol - 8.f;
        ImVec2 msgSize = body->CalcTextSizeA(bodySize, FLT_MAX, wrapW, message);

        const float barH = 7.f;
        const float barGap = 14.f;
        const float statusGap = 10.f;
        const float errGap = banTroll ? 16.f : 0.f;
        const float boxH = padY + 34.f + msgSize.y + barGap + barH + statusGap + 18.f + errGap + padY;
        ImVec2 box(ds.x * 0.5f - boxW * 0.5f, ds.y * 0.5f - (boxH + 54.f) * 0.5f);
        ImVec2 boxMax(box.x + boxW, box.y + boxH);

        // Roblox native dialog shell
        dl->AddRectFilled(box, boxMax, IM_COL32(35, 37, 39, 252), 10.f);
        dl->AddRect(box, boxMax, IM_COL32(255, 255, 255, 28), 10.f, 0, 1.f);
        dl->AddRectFilled(ImVec2(box.x, box.y + 10.f), ImVec2(box.x + 4.f, box.y + boxH - 10.f),
            IM_COL32(248, 217, 109, 255), 2.f);

        // Warning triangle (Roblox yellow)
        ImVec2 tip(box.x + padX + 22.f, box.y + padY + 22.f);
        ImVec2 t1(tip.x, tip.y - 17.f), t2(tip.x - 19.f, tip.y + 15.f), t3(tip.x + 19.f, tip.y + 15.f);
        dl->AddTriangle(t1, t2, t3, IM_COL32(0, 0, 0, 180), 1.4f);
        dl->AddTriangleFilled(t1, t2, t3, IM_COL32(248, 217, 109, 255));
        if (bold)
            dl->AddText(bold, boldSize * 0.72f, ImVec2(tip.x - 4.f, tip.y - 9.f),
                IM_COL32(35, 37, 39, 255), "!");

        const float textX = box.x + padX + iconCol;
        if (bold)
            dl->AddText(bold, boldSize, ImVec2(textX, box.y + padY), IM_COL32(255, 255, 255, 255), title);
        dl->AddText(body, bodySize, ImVec2(textX, box.y + padY + 30.f), IM_COL32(200, 203, 208, 255),
            message, nullptr, wrapW);

        float barX = box.x + padX;
        float barY = box.y + padY + 34.f + msgSize.y + barGap;
        float barW = boxW - padX * 2.f;
        dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(57, 59, 63, 255), 3.f);

        float fillW = barW * prog;
        if (fillW > 2.f) {
            dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + fillW, barY + barH),
                IM_COL32(235, 237, 240, 245), 3.f);
            if (!banTroll) {
                const float shimmer = barX + 6.f + fmodf(t * 120.f, (fillW > 24.f ? fillW - 24.f : 1.f));
                dl->AddRectFilled(ImVec2(shimmer, barY + 1.f), ImVec2(shimmer + 16.f, barY + barH - 1.f),
                    IM_COL32(255, 255, 255, 70), 2.f);
            }
        }

        dl->AddText(fontSmall, smallSize, ImVec2(barX, barY + barH + statusGap),
            IM_COL32(151, 155, 162, 255), status);

        if (banTroll) {
            dl->AddText(fontSmall, smallSize, ImVec2(barX, boxMax.y - padY - smallSize),
                IM_COL32(120, 124, 132, 255), "Error Code: 600");
        }

        // Cancel button (native Roblox outline style)
        const float btnW = 132.f;
        const float btnH = 34.f;
        ImVec2 btnMin(ds.x * 0.5f - btnW * 0.5f, boxMax.y + 16.f);
        ImVec2 btnMax(btnMin.x + btnW, btnMin.y + btnH);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ds);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##redirectkick", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        ImGui::PopStyleVar();

        ImGui::SetCursorScreenPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##redirectblock", ds);

        ImGui::SetCursorScreenPos(btnMin);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.06f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.10f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.92f, 0.94f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        if (ImGui::Button("Cancel", ImVec2(btnW, btnH)))
            ServerBrowser::DismissDisconnectScreen();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
        ImGui::End();
    }

    void RenderMenu() {
        if (variables::Loading::active) return;

        variables::Misc::menuHovered = false;
        UI::ApplyIfDirty();

        RenderUpdateBanner();
        RenderRedirectKick();
        if (variables::Servers::redirecting) return;

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;

        // Floating icon bar — each icon opens its own compact panel
        if (variables::Theme::useFloatingHeader && !Telemetry::consentPending.load()) {
            int prevTab = variables::selectedTab;
            UIFx::FloatingHeader(&variables::selectedTab);
            if (variables::selectedTab != prevTab)
                variables::selectedSub = variables::Misc::selectedSubByTab[variables::selectedTab];

            if (variables::Theme::bgEffect)
                UIFx::DrawBackgroundFX(bg, ds, dt);

            RenderFloatingPanel();

            ImVec2 ds2 = ImGui::GetIO().DisplaySize;
            FrontierUI::DrawToast(ImGui::GetForegroundDrawList(), ds2, ImGui::GetIO().DeltaTime);

            if (variables::Misc::showKeybinds) {
                ImGui::SetNextWindowPos(ImVec2(16, ds2.y * 0.35f), ImGuiCond_FirstUseEver);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.055f, 0.065f, 0.94f));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.10f));
                ImGui::Begin("##binds", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::TextColored(FrontierUI::V4(variables::Theme::text), "Keybinds");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Menu");
                ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Misc::menuVk));
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Aim");
                ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Aimbot::aimbotKey));
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Trig");
                ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Trigger::key));
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Fly");
                ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Local::flyKey));
                ImGui::End();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(3);
            }
            return;
        }

        // Classic full menu (floating header disabled)
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
        float ease = 1.f - powf(1.f - anim, 3.f);
        float uiScale = variables::Theme::menuScale;
        if (uiScale < 0.85f) uiScale = 0.85f;
        if (uiScale > 1.15f) uiScale = 1.15f;
        float baseW = 849.f * uiScale;
        float baseH = 538.f * uiScale;

        ImGui::SetNextWindowSize(ImVec2(baseW, baseH), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(720.f, 460.f), ImVec2(ds.x - 16.f, ds.y - 8.f));
        {
            static bool haveDragPos = false;
            float px = haveDragPos ? variables::Misc::menuX : (ds.x * 0.5f - baseW * 0.5f);
            float py = haveDragPos ? variables::Misc::menuY : (ds.y * 0.5f - baseH * 0.5f);
            ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Appearing);
            if (variables::Misc::menuW > 1.f && variables::Misc::menuH > 1.f)
                haveDragPos = true;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ease);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::V4(variables::Theme::bg));
        ImGui::PushStyleColor(ImGuiCol_Border, UI::V4(variables::Theme::border));
        ImGui::Begin("##mw", &variables::menuOpen,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float ww = ImGui::GetWindowSize().x;
        float wh = ImGui::GetWindowSize().y;

        FrontierShell::DrawHeader(wdl, wp, ww, wh, &variables::selectedTab);

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##mwdrag", ImVec2(FrontierShell::kSidebarW, FrontierShell::kBrandH));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 d = ImGui::GetIO().MouseDelta;
            ImGui::SetWindowPos(ImVec2(wp.x + d.x, wp.y + d.y));
            wp = ImGui::GetWindowPos();
        }

        ImVec2 contentOrigin = FrontierShell::ContentOrigin(wp);
        ImVec2 contentSize = FrontierShell::ContentSize(ww, wh);
        ImGui::SetCursorScreenPos(contentOrigin);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("##content", contentSize, ImGuiChildFlags_None,
            ImGuiWindowFlags_AlwaysVerticalScrollbar);
        FrontierMenu::RenderBody();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        FrontierMenu::RefreshStatusInfo();
        FrontierUI::DrawFooter(wdl, wp, ww, wh);

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

        FrontierUI::DrawToast(ImGui::GetForegroundDrawList(), ds, ImGui::GetIO().DeltaTime);

        if (variables::Misc::showKeybinds) {
            ImGui::SetNextWindowPos(ImVec2(16, ds.y * 0.35f), ImGuiCond_FirstUseEver);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.055f, 0.065f, 0.94f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.10f));
            ImGui::Begin("##binds", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::text), "Keybinds");
            ImGui::Dummy(ImVec2(0, 2));
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Menu");
            ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Misc::menuVk));
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Aim");
            ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Aimbot::aimbotKey));
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Trig");
            ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Trigger::key));
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Fly");
            ImGui::SameLine(72); ImGui::Text("%s", FrontierUI::KeyName(variables::Local::flyKey));
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(3);
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
        bg->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(5, 5, 7, 220));
        bg->AddRectFilledMultiColor(ImVec2(0, 0), ds,
            IM_COL32(18, 18, 24, 50), IM_COL32(8, 8, 10, 20),
            IM_COL32(16, 16, 22, 60), IM_COL32(6, 6, 8, 25));

        ImGui::SetNextWindowSize(ImVec2(440, 280), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ds.x * 0.5f - 220, ds.y * 0.5f - 140), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 22));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.075f, 0.085f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.12f));
        ImGui::Begin("##telemetry_consent", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        wdl->AddRectFilled(ImVec2(wp.x, wp.y + 10), ImVec2(wp.x + 3, wp.y + ws.y - 10),
            FrontierUI::U32(variables::Theme::brand, 0.85f), 2.f);
        wdl->AddLine(ImVec2(wp.x + 16, wp.y + 1), ImVec2(wp.x + ws.x - 16, wp.y + 1),
            IM_COL32(255, 255, 255, 24), 1.f);

        ImGui::TextColored(FrontierUI::V4(variables::Theme::text), "%s", Frontier::kName);
        ImGui::SameLine();
        ImGui::TextColored(FrontierUI::V4(variables::Theme::brand), "· %s", Frontier::kTagline);
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 390);
        ImGui::TextColored(ImVec4(0.62f, 0.62f, 0.68f, 1),
            "This build does not collect telemetry or send launch data.");
        ImGui::PopTextWrapPos();
        ImGui::Dummy(ImVec2(0, 14));

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.90f, 0.94f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.82f, 0.82f, 0.86f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.08f, 0.08f, 0.10f, 1));
        if (ImGui::Button("Agree", ImVec2(180, 42)))
            Telemetry::Agree();
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, 14);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.24f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.12f, 0.14f, 1));
        if (ImGui::Button("Decline", ImVec2(180, 42)))
            Telemetry::Deny();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    void RenderUpdateBanner() {
        Updater::CheckAsync();
        if (!Updater::outOfDate.load()) return;

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const float barH = 42.f;
        ImVec2 a(ds.x * 0.5f - 220.f, 10.f);
        ImVec2 b(a.x + 440.f, a.y + barH);
        dl->AddRectFilled(a, b, IM_COL32(40, 12, 12, 240), 10.f);
        dl->AddRect(a, b, IM_COL32(230, 70, 70, 220), 10.f, 0, 1.4f);
        // red out-of-date dot
        dl->AddCircleFilled(ImVec2(a.x + 22.f, a.y + barH * 0.5f), 7.f, IM_COL32(240, 60, 60, 255), 16);
        dl->AddCircle(ImVec2(a.x + 22.f, a.y + barH * 0.5f), 7.f, IM_COL32(255, 180, 180, 200), 16, 1.5f);

        char remote[32]{};
        {
            std::lock_guard<std::mutex> lock(Updater::gMutex);
            strncpy_s(remote, Updater::remoteDisplay, _TRUNCATE);
        }
        char line[160]{};
        sprintf_s(line, "OUT OF DATE  ·  v%s → v%s  ·  Please update",
            Updater::kLocalDisplay, remote[0] ? remote : "?");
        dl->AddText(ImVec2(a.x + 40.f, a.y + 13.f), IM_COL32(255, 220, 220, 255), line);

        ImGui::SetNextWindowPos(a);
        ImGui::SetNextWindowSize(ImVec2(440.f, barH));
        ImGui::Begin("##updatebanner", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        if (ImGui::InvisibleButton("##updateclick", ImVec2(440.f, barH)))
            Updater::OpenDownload();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Open FRONTIER releases on GitHub");
        ImGui::End();
    }

    void RenderSpotifyMini() {
        if (variables::Loading::active || Telemetry::consentPending.load() ||
            !variables::Audio::spotifyMini) {
            variables::Misc::spotW = variables::Misc::spotH = 0;
            return;
        }

        SpotifyPlayer::Refresh();
        SpotifyPlayer::TickAlbumArt();

        ImGui::SetNextWindowSize(ImVec2(328, 132), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(14, 70), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.055f, 0.065f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.11f));
        ImGui::Begin("##spotify_mini", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        float t = (float)ImGui::GetTime();
        dl->AddLine(ImVec2(wp.x + 12, wp.y + 1), ImVec2(wp.x + ws.x - 12, wp.y + 1),
            IM_COL32(255, 255, 255, 22), 1.f);

        // Soft silver bloom behind art (brand mono, not Spotify green flood)
        dl->AddCircleFilled(ImVec2(wp.x + 40, wp.y + 48), 36.f, IM_COL32(210, 210, 220, 18), 32);

        ImVec2 artMin(wp.x + 12, wp.y + 14);
        ImVec2 artMax(wp.x + 68, wp.y + 70);
        ID3D11ShaderResourceView* art = SpotifyPlayer::ArtSrvForDraw();
        if (art && SpotifyPlayer::ArtStateForDraw() == 2) {
            dl->AddImageRounded(ImTextureRef((void*)art), artMin, artMax,
                ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 9.f);
        } else {
            unsigned h = SpotifyPlayer::HashStr(SpotifyPlayer::trackTitle);
            int r = 36 + (h & 60), g = 36 + ((h >> 8) & 60), b = 42 + ((h >> 16) & 70);
            dl->AddRectFilled(artMin, artMax, IM_COL32(r, g, b, 255), 9.f);
            for (int i = 0; i < 4; i++) {
                float bh = SpotifyPlayer::playing
                    ? (8.f + 18.f * (0.5f + 0.5f * sinf(t * 6.f + i * 1.3f)))
                    : 6.f;
                float bx = artMin.x + 14.f + i * 10.f;
                dl->AddRectFilled(ImVec2(bx, artMax.y - 12.f - bh), ImVec2(bx + 6.f, artMax.y - 12.f),
                    IM_COL32(240, 240, 245, 200), 2.f);
            }
        }
        dl->AddRect(artMin, artMax, IM_COL32(255, 255, 255, 36), 9.f);

        ImGui::SetCursorPos(ImVec2(78, 12));
        ImGui::PushTextWrapPos(300);
        ImGui::TextColored(ImVec4(0.96f, 0.96f, 0.98f, 1), "%s",
            TruncateForSpotify(SpotifyPlayer::trackTitle, 210).c_str());
        ImGui::SetCursorPos(ImVec2(78, 30));
        ImGui::TextColored(ImVec4(0.52f, 0.52f, 0.58f, 1), "%s",
            TruncateForSpotify(SpotifyPlayer::trackArtist, 210).c_str());
        ImGui::PopTextWrapPos();

        float shimmer = fmodf(t * 0.35f, 1.f);
        dl->AddRectFilled(ImVec2(wp.x + 78, wp.y + 52), ImVec2(wp.x + ws.x - 14, wp.y + 56),
            IM_COL32(255, 255, 255, 16), 2.f);
        if (SpotifyPlayer::playing) {
            dl->AddRectFilled(ImVec2(wp.x + 78, wp.y + 52),
                ImVec2(wp.x + 78 + (ws.x - 92) * shimmer, wp.y + 56),
                IM_COL32(230, 230, 240, 160), 2.f);
        }

        ImGui::SetCursorPos(ImVec2(78, 64));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.14f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.21f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.12f, 1));

        if (ImGui::Button("|<", ImVec2(40, 30))) SpotifyPlayer::Prev();
        ImGui::SameLine(0, 6);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.90f, 0.94f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.08f, 0.08f, 0.10f, 1));
        if (ImGui::Button(SpotifyPlayer::playing ? "Pause" : "Play", ImVec2(72, 30)))
            SpotifyPlayer::PlayPause();
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 6);
        if (ImGui::Button(">|", ImVec2(40, 30))) SpotifyPlayer::Next();
        ImGui::SameLine(0, 6);
        if (ImGui::Button("-", ImVec2(28, 30))) SpotifyPlayer::VolDown();
        ImGui::SameLine(0, 4);
        if (ImGui::Button("+", ImVec2(28, 30))) SpotifyPlayer::VolUp();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        if (!SpotifyPlayer::connected) {
            ImGui::SetCursorPos(ImVec2(78, 100));
            ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.52f, 1), "Waiting for Spotify.exe");
        }

        variables::Misc::spotX = ImGui::GetWindowPos().x;
        variables::Misc::spotY = ImGui::GetWindowPos().y;
        variables::Misc::spotW = ImGui::GetWindowSize().x;
        variables::Misc::spotH = ImGui::GetWindowSize().y;

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
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
                sprintf_s(buf, "FRONTIER  |  %d fps", fps);
            else
                sprintf_s(buf, "FRONTIER %s  |  %s  |  %d fps", Frontier::kVersion, Offsets::ClientVersion.c_str(), fps);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float x = ImGui::GetIO().DisplaySize.x - ts.x - 14;
            drawList->AddText(ImVec2(x + 1, 11), IM_COL32(0, 0, 0, 200), buf);
            drawList->AddText(ImVec2(x, 10), FrontierUI::U32(variables::Theme::text), buf);
        }

        if (variables::Aimbot::showFOV && !variables::Loading::active) {
            Aimbot::RefreshFovCenter();
            ImVec2 c(Aimbot::fovCenterX, Aimbot::fovCenterY);
            int a = (int)(variables::Aimbot::fovOpacity * 255);
            Aimbot::SyncUiSliders();
            const bool silentActive = variables::Aimbot::silentAim || variables::Aimbot::aimType == 1;
            float fr = silentActive ? variables::Theme::brand[0] : variables::Aimbot::fovColor[0];
            float fg = silentActive ? variables::Theme::brand[1] : variables::Aimbot::fovColor[1];
            float fb = silentActive ? variables::Theme::brand[2] : variables::Aimbot::fovColor[2];
            if (!silentActive && variables::Extra::fovRainbow) {
                float hue = fmodf((float)ImGui::GetTime() * 0.45f, 1.f);
                ImGui::ColorConvertHSVtoRGB(hue, 0.9f, 1.f, fr, fg, fb);
            }
            ImU32 col = IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a);
            float fovDraw = silentActive ? variables::Aimbot::silentFovRadius : variables::Aimbot::fovRadius;
            if (variables::Aimbot::fovStyle == 1) {
                const int dots = 56;
                const float spin = (float)ImGui::GetTime() * 1.35f;
                const float dotR = 2.4f;
                constexpr float kPi = 3.14159265358979323846f;
                for (int i = 0; i < dots; i++) {
                    float ang = spin + (i / (float)dots) * kPi * 2.f;
                    float x = c.x + cosf(ang) * fovDraw;
                    float y = c.y + sinf(ang) * fovDraw;
                    drawList->AddCircleFilled(ImVec2(x, y), dotR, col, 8);
                }
            } else {
                if (variables::Aimbot::fovFilled)
                    drawList->AddCircleFilled(c, fovDraw,
                        IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a / 4), 64);
                if (variables::Aimbot::fovGlow)
                    drawList->AddCircle(c, fovDraw,
                        IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a / 3), 64, 4.0f);
                drawList->AddCircle(c, fovDraw, col, 64, silentActive ? 1.8f : 1.5f);
            }
            drawList->AddCircleFilled(c, 2.2f, IM_COL32(255, 255, 255, 180), 12);
        }

        if (variables::MagicBullet::showFov && variables::MagicBullet::enabled && !variables::Loading::active) {
            Aimbot::RefreshFovCenter();
            ImVec2 c(Aimbot::fovCenterX, Aimbot::fovCenterY);
            float fr = variables::MagicBullet::fovColor[0];
            float fg = variables::MagicBullet::fovColor[1];
            float fb = variables::MagicBullet::fovColor[2];
            int a = (int)(variables::MagicBullet::fovColor[3] * 255);
            if (a < 40) a = 200;
            ImU32 col = IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a);
            float fovDraw = variables::MagicBullet::fovRadius;
            if (fovDraw < 20.f) fovDraw = 20.f;
            drawList->AddCircle(c, fovDraw, col, 64, 1.6f);
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
        Aimbot::OnAimReleased();
        SpotifyPlayer::Shutdown();
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupD3D11();
        if (windowHandle) { DestroyWindow(windowHandle); windowHandle = nullptr; }
        UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
    }

    HWND GetWindowHandle() const { return windowHandle; }
};
