#pragma once
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include "../../ext/imgui/imgui.h"
#include "../../src/core/variables/variables.h"
#include "frontier_ui.h"
#include "brand_assets.h"
#include "ui_motion.h"

namespace UIFx {

    constexpr float PI = 3.14159265358979323846f;

    inline float Clampf(float v, float lo, float hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    struct Particle {
        float x, y, vx, vy, r, a, life;
    };

    inline Particle particles[96]{};
    inline bool particlesInit = false;
    inline float timeAcc = 0.0f;

    inline void EnsureParticles(float w, float h) {
        if (particlesInit) return;
        for (auto& p : particles) {
            p.x = (float)(rand() % (int)w);
            p.y = (float)(rand() % (int)h);
            p.vx = ((rand() % 100) / 100.0f - 0.5f) * 18.0f;
            p.vy = 12.0f + (rand() % 40);
            p.r = 1.0f + (rand() % 30) / 10.0f;
            p.a = 0.15f + (rand() % 40) / 100.0f;
            p.life = 1.0f;
        }
        particlesInit = true;
    }

    inline void DrawPanelAmbientGlow(ImDrawList* dl, ImVec2 panelPos, ImVec2 panelSize, float intensity = 1.f)
    {
        (void)panelPos;
        (void)panelSize;
        if (intensity <= 0.01f) return;
        dl->AddRectFilledMultiColor(
            ImVec2(0, 0), ImGui::GetIO().DisplaySize,
            IM_COL32(0, 0, 0, (int)(28 * intensity)),
            IM_COL32(0, 0, 0, (int)(28 * intensity)),
            IM_COL32(0, 0, 0, (int)(48 * intensity)),
            IM_COL32(0, 0, 0, (int)(48 * intensity)));
    }

    inline bool IsLiquidGlass() {
        return variables::Theme::preset == 4;
    }

    inline float HashHue01(const char* s, int salt = 0) {
        unsigned h = 2166136261u ^ (unsigned)salt;
        while (s && *s) { h ^= (unsigned char)*s++; h *= 16777619u; }
        return (h % 360) / 360.f;
    }

    inline ImU32 HsvToU32(float h, float s, float v, float a) {
        float r = 0, g = 0, b = 0;
        ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
        return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255));
    }

    inline void DrawLiquidGlassAurora(ImDrawList* dl, ImVec2 size, float t) {
        const ImU32 c1 = HsvToU32(fmodf(t * 0.04f, 1.f), 0.55f, 0.95f, 0.14f);
        const ImU32 c2 = HsvToU32(fmodf(t * 0.03f + 0.35f, 1.f), 0.45f, 0.88f, 0.12f);
        const ImU32 c3 = HsvToU32(fmodf(t * 0.05f + 0.62f, 1.f), 0.50f, 0.92f, 0.10f);
        dl->AddCircleFilled(ImVec2(size.x * 0.18f + sinf(t * 0.35f) * 40.f, size.y * 0.22f + cosf(t * 0.28f) * 30.f), 180.f, c1, 48);
        dl->AddCircleFilled(ImVec2(size.x * 0.78f + cosf(t * 0.31f) * 50.f, size.y * 0.38f + sinf(t * 0.24f) * 35.f), 220.f, c2, 48);
        dl->AddCircleFilled(ImVec2(size.x * 0.52f + sinf(t * 0.22f) * 60.f, size.y * 0.72f + cosf(t * 0.19f) * 25.f), 160.f, c3, 48);
    }

    inline void DrawGlassCardFrame(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float rounding = 10.f) {
        dl->AddRectFilledMultiColor(p0, p1,
            IM_COL32(255, 255, 255, 18), IM_COL32(255, 255, 255, 10),
            IM_COL32(255, 255, 255, 6), IM_COL32(255, 255, 255, 14));
        dl->AddRect(p0, p1, FrontierUI::U32(variables::Theme::border, 0.55f), rounding, 0, 1.2f);
        dl->AddLine(ImVec2(p0.x + 12.f, p0.y + 1.f), ImVec2(p1.x - 12.f, p0.y + 1.f),
            IM_COL32(255, 255, 255, 42), 1.f);
        dl->AddLine(ImVec2(p0.x + 14.f, p1.y - 1.f), ImVec2(p1.x - 14.f, p1.y - 1.f),
            IM_COL32(255, 255, 255, 10), 1.f);
    }

    inline void DrawSpectrumBars(ImDrawList* dl, ImVec2 origin, float width, float height, float t,
        bool active, int bars = 24)
    {
        const float gap = 2.f;
        const float barW = (width - gap * (bars - 1)) / (float)bars;
        for (int i = 0; i < bars; i++) {
            float phase = (float)i * 0.55f + t * (active ? 8.5f : 1.2f);
            float norm = active ? (0.35f + 0.65f * fabsf(sinf(phase))) : 0.18f;
            float h = height * norm;
            float x = origin.x + i * (barW + gap);
            float y0 = origin.y + height - h;
            ImU32 col = FrontierUI::U32(variables::Theme::brand, active ? (0.55f + norm * 0.45f) : 0.25f);
            dl->AddRectFilled(ImVec2(x, y0), ImVec2(x + barW, origin.y + height), col, 2.f);
        }
    }

    inline void DrawAlbumGlowRing(ImDrawList* dl, ImVec2 center, float radius, float t, bool playing,
        const char* moodSeed)
    {
        const float hue = HashHue01(moodSeed ? moodSeed : "frontier", 7);
        const int segments = 64;
        for (int i = 0; i < segments; i++) {
            float a0 = (float)i / (float)segments * 6.2831853f + (playing ? t * 1.6f : 0.f);
            float a1 = (float)(i + 1) / (float)segments * 6.2831853f + (playing ? t * 1.6f : 0.f);
            float pulse = playing ? (0.55f + 0.45f * sinf(t * 3.2f + i * 0.25f)) : 0.35f;
            ImU32 col = HsvToU32(fmodf(hue + i * 0.012f, 1.f), 0.65f, 1.f, 0.22f * pulse);
            dl->PathClear();
            dl->PathArcTo(center, radius, a0, a1, 8);
            dl->PathStroke(col, 0, 2.4f);
        }
    }

    inline void DrawPopulationSparkline(ImDrawList* dl, ImVec2 p0, ImVec2 p1, const int* values,
        int count, float t)
    {
        if (count < 2) return;
        int vmax = 1;
        for (int i = 0; i < count; i++)
            if (values[i] > vmax) vmax = values[i];

        dl->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 8), 8.f);
        dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 18), 8.f);

        const float pad = 8.f;
        const float w = (p1.x - p0.x) - pad * 2.f;
        const float h = (p1.y - p0.y) - pad * 2.f;
        const float step = w / (float)(count - 1);

        ImVec2 prev(p0.x + pad, p1.y - pad - (values[0] / (float)vmax) * h);
        for (int i = 1; i < count; i++) {
            ImVec2 cur(p0.x + pad + step * i, p1.y - pad - (values[i] / (float)vmax) * h);
            float pulse = 0.7f + 0.3f * sinf(t * 2.4f + i * 0.4f);
            dl->AddLine(prev, cur, FrontierUI::U32(variables::Theme::brand, 0.35f + pulse * 0.35f), 2.f);
            dl->AddCircleFilled(cur, 2.2f, FrontierUI::U32(variables::Theme::brand, 0.55f + pulse * 0.25f), 8);
            prev = cur;
        }
    }

    inline void DrawBackgroundFX(ImDrawList* dl, ImVec2 size, float dt, bool menuVisible) {
        if (!menuVisible && !variables::Theme::bgEffect) return;
        EnsureParticles(size.x, size.y);

        float fpsScale = 1.f;
        if (variables::Perf::currentFps > 0)
            fpsScale = 60.f / (float)variables::Perf::currentFps;
        if (fpsScale < 0.5f) fpsScale = 0.5f;
        if (fpsScale > 2.f) fpsScale = 2.f;
        const float step = dt * fpsScale;
        const float t = (float)ImGui::GetTime();

        if (IsLiquidGlass()) {
            dl->AddRectFilledMultiColor(
                ImVec2(0, 0), size,
                IM_COL32(8, 14, 24, menuVisible ? 120 : 90),
                IM_COL32(6, 10, 18, menuVisible ? 100 : 75),
                IM_COL32(10, 16, 28, menuVisible ? 130 : 95),
                IM_COL32(5, 8, 14, menuVisible ? 105 : 80));
            DrawLiquidGlassAurora(dl, size, t);
        } else {
            dl->AddRectFilledMultiColor(
                ImVec2(0, 0), size,
                IM_COL32(12, 12, 16, menuVisible ? 90 : 70),
                IM_COL32(8, 8, 10, menuVisible ? 70 : 50),
                IM_COL32(14, 14, 18, menuVisible ? 100 : 80),
                IM_COL32(6, 6, 8, menuVisible ? 75 : 55));
        }

        const bool drawParticles = menuVisible || variables::Theme::snowEffect;
        if (drawParticles) {
            const ImU32 pc = variables::Theme::snowEffect
                ? IM_COL32(220, 240, 230, 180)
                : IM_COL32((int)(variables::Theme::brand[0] * 255),
                    (int)(variables::Theme::brand[1] * 255),
                    (int)(variables::Theme::brand[2] * 255), 90);
            for (auto& p : particles) {
                p.x += p.vx * step;
                p.y += p.vy * step;
                if (p.y > size.y + 10) { p.y = -10; p.x = (float)(rand() % (int)(size.x + 1)); }
                if (p.x < -10) p.x = size.x + 10;
                if (p.x > size.x + 10) p.x = -10;
                dl->AddCircleFilled(ImVec2(p.x, p.y), p.r, pc, 8);
            }
        }
    }

    inline float EaseOutCubic(float t) {
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        float u = 1.f - t;
        return 1.f - u * u * u;
    }

    inline void DrawLoadingFX(ImDrawList* dl, ImVec2 size, float progress, float slideT) {
        (void)slideT;
        if (progress < 0.f) progress = 0.f;
        if (progress > 1.f) progress = 1.f;

        // Transparent — overlay shows through; logo + bar only

        const float logoY = size.y * 0.42f;
        ImVec2 c(size.x * 0.5f, logoY);

        if (BrandAssets::logoSrv && BrandAssets::logoW > 0 && BrandAssets::logoH > 0) {
            float aspect = (float)BrandAssets::logoW / (float)BrandAssets::logoH;
            float drawH = Clampf(size.y * 0.09f, 52.f, 88.f);
            float drawW = drawH * aspect;
            ImVec2 logoMin(c.x - drawW * 0.5f, c.y - drawH * 0.5f);
            ImVec2 logoMax(c.x + drawW * 0.5f, c.y + drawH * 0.5f);
            dl->AddImage(BrandAssets::LogoTex(), logoMin, logoMax,
                ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 235));
        } else {
            ImFont* font = ImGui::GetFont();
            const float fs = Clampf(size.y * 0.09f, 48.f, 72.f);
            const char* t = "F";
            ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, t);
            dl->AddText(font, fs, ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f),
                IM_COL32(235, 45, 45, 240), t);
        }

        const float barW = Clampf(size.x * 0.28f, 180.f, 320.f);
        const float barH = 2.f;
        const float barY = logoY + (BrandAssets::logoSrv ? 56.f : 34.f);
        ImVec2 barMin(c.x - barW * 0.5f, barY);
        ImVec2 barMax(c.x + barW * 0.5f, barY + barH);
        dl->AddRectFilled(barMin, barMax, IM_COL32(36, 36, 42, 255));
        if (progress > 0.001f) {
            ImVec2 fillMax(barMin.x + barW * progress, barMax.y);
            dl->AddRectFilled(barMin, fillMax, IM_COL32(0, 255, 0, 255));
        }
    }

    // Simple line icons for floating header
    enum class Icon { Combat, Visuals, World, Character, Options, Config, Servers, Music, Status };

    inline void DrawIcon(ImDrawList* dl, ImVec2 c, float s, Icon id, ImU32 col) {
        float h = s * 0.5f;
        switch (id) {
        case Icon::Combat: // crosshair
            dl->AddCircle(c, h * 0.7f, col, 16, 1.4f);
            dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x - h * 0.35f, c.y), col, 1.4f);
            dl->AddLine(ImVec2(c.x + h * 0.35f, c.y), ImVec2(c.x + h, c.y), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y - h), ImVec2(c.x, c.y - h * 0.35f), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y + h * 0.35f), ImVec2(c.x, c.y + h), col, 1.4f);
            break;
        case Icon::Visuals: // eye / brush
            dl->AddCircle(c, h * 0.35f, col, 12, 1.4f);
            dl->PathClear();
            dl->PathArcTo(c, h * 0.85f, 0.4f, 3.14159f - 0.4f, 16);
            dl->PathStroke(col, 0, 1.4f);
            dl->PathClear();
            dl->PathArcTo(c, h * 0.85f, 3.14159f + 0.4f, 3.14159f * 2 - 0.4f, 16);
            dl->PathStroke(col, 0, 1.4f);
            break;
        case Icon::World: // sun
            dl->AddCircle(c, h * 0.4f, col, 16, 1.4f);
            for (int i = 0; i < 8; i++) {
                float a = i * 3.14159f / 4.0f;
                dl->AddLine(
                    ImVec2(c.x + cosf(a) * h * 0.55f, c.y + sinf(a) * h * 0.55f),
                    ImVec2(c.x + cosf(a) * h, c.y + sinf(a) * h), col, 1.3f);
            }
            break;
        case Icon::Character: // person
            dl->AddCircle(ImVec2(c.x, c.y - h * 0.45f), h * 0.32f, col, 12, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y - h * 0.1f), ImVec2(c.x, c.y + h * 0.35f), col, 1.4f);
            dl->AddLine(ImVec2(c.x - h * 0.55f, c.y + h * 0.05f), ImVec2(c.x + h * 0.55f, c.y + h * 0.05f), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y + h * 0.35f), ImVec2(c.x - h * 0.4f, c.y + h), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y + h * 0.35f), ImVec2(c.x + h * 0.4f, c.y + h), col, 1.4f);
            break;
        case Icon::Options: // sliders
            dl->AddLine(ImVec2(c.x - h, c.y - h * 0.45f), ImVec2(c.x + h, c.y - h * 0.45f), col, 1.4f);
            dl->AddCircleFilled(ImVec2(c.x - h * 0.2f, c.y - h * 0.45f), 2.5f, col);
            dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x + h, c.y), col, 1.4f);
            dl->AddCircleFilled(ImVec2(c.x + h * 0.35f, c.y), 2.5f, col);
            dl->AddLine(ImVec2(c.x - h, c.y + h * 0.45f), ImVec2(c.x + h, c.y + h * 0.45f), col, 1.4f);
            dl->AddCircleFilled(ImVec2(c.x - h * 0.45f, c.y + h * 0.45f), 2.5f, col);
            break;
        case Icon::Config: // folder / explorer
            dl->AddRect(ImVec2(c.x - h * 0.7f, c.y - h * 0.15f), ImVec2(c.x + h * 0.7f, c.y + h * 0.75f), col, 2.f, 0, 1.3f);
            dl->AddLine(ImVec2(c.x - h * 0.7f, c.y - h * 0.15f), ImVec2(c.x - h * 0.15f, c.y - h * 0.15f), col, 1.3f);
            dl->AddLine(ImVec2(c.x - h * 0.15f, c.y - h * 0.15f), ImVec2(c.x, c.y - h * 0.45f), col, 1.3f);
            dl->AddLine(ImVec2(c.x, c.y - h * 0.45f), ImVec2(c.x + h * 0.7f, c.y - h * 0.45f), col, 1.3f);
            dl->AddLine(ImVec2(c.x + h * 0.7f, c.y - h * 0.45f), ImVec2(c.x + h * 0.7f, c.y - h * 0.15f), col, 1.3f);
            break;
        case Icon::Servers: // stack
            dl->AddRect(ImVec2(c.x - h * 0.7f, c.y - h * 0.7f), ImVec2(c.x + h * 0.7f, c.y - h * 0.25f), col, 2.f, 0, 1.3f);
            dl->AddRect(ImVec2(c.x - h * 0.7f, c.y - h * 0.15f), ImVec2(c.x + h * 0.7f, c.y + h * 0.3f), col, 2.f, 0, 1.3f);
            dl->AddRect(ImVec2(c.x - h * 0.7f, c.y + h * 0.4f), ImVec2(c.x + h * 0.7f, c.y + h * 0.85f), col, 2.f, 0, 1.3f);
            break;
        case Icon::Music: // note
            dl->AddCircleFilled(ImVec2(c.x - h * 0.25f, c.y + h * 0.45f), h * 0.28f, col, 12);
            dl->AddLine(ImVec2(c.x - h * 0.05f, c.y + h * 0.45f), ImVec2(c.x - h * 0.05f, c.y - h * 0.7f), col, 1.6f);
            dl->AddLine(ImVec2(c.x - h * 0.05f, c.y - h * 0.7f), ImVec2(c.x + h * 0.55f, c.y - h * 0.45f), col, 1.6f);
            break;
        case Icon::Status: // info circle
            dl->AddCircle(c, h * 0.85f, col, 16, 1.4f);
            dl->AddCircleFilled(ImVec2(c.x, c.y - h * 0.35f), 1.8f, col, 8);
            dl->AddLine(ImVec2(c.x, c.y - h * 0.05f), ImVec2(c.x, c.y + h * 0.45f), col, 1.6f);
            break;
        }
    }

    inline bool FloatingHeader(int* selectedTab) {
        const Icon icons[] = {
            Icon::Combat, Icon::Visuals, Icon::World, Icon::Character,
            Icon::Options, Icon::Servers, Icon::Music, Icon::Status, Icon::Config
        };
        const int count = 9;
        const float iconSlot = 40.0f;
        const float barH = 52.0f;
        const float barW = count * iconSlot + 24.0f;
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;

        const float intro = UIMotion::EaseOutCubic(variables::Misc::headerIntro);
        const float introSlide = (1.f - intro) * 14.f;

        float px = (ds.x - barW) * 0.5f;
        float py = variables::Theme::headerY - introSlide;
        variables::Misc::floatX = px;
        variables::Misc::floatY = py;
        variables::Misc::floatW = barW;
        variables::Misc::floatH = barH;

        bool changed = false;
        ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(barW, barH));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, intro);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, UIFx::IsLiquidGlass()
            ? ImVec4(0.06f, 0.08f, 0.12f, 0.72f)
            : ImVec4(0.055f, 0.055f, 0.065f, 0.94f));
        ImGui::PushStyleColor(ImGuiCol_Border, UIFx::IsLiquidGlass()
            ? ImVec4(0.62f, 0.84f, 1.f, 0.28f)
            : ImVec4(1, 1, 1, 0.12f));
        if (!ImGui::Begin("##floathead", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(4);
            return changed;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        // top hairline highlight
        dl->AddLine(ImVec2(wp.x + 14, wp.y + 1), ImVec2(wp.x + ws.x - 14, wp.y + 1),
            IM_COL32(255, 255, 255, 28), 1.f);

        for (int i = 0; i < count; i++) {
            ImGui::PushID(i);
            if (i) ImGui::SameLine(0, 4);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 btn(iconSlot - 4, barH - 16);
            bool pressed = ImGui::InvisibleButton("ic", btn);
            bool hovered = ImGui::IsItemHovered();
            if (pressed) {
                if (*selectedTab == i && variables::Misc::floatingPanelOpen)
                    variables::Misc::floatingPanelOpen = false;
                else {
                    *selectedTab = i;
                    variables::Misc::floatingPanelOpen = true;
                    variables::selectedSub = variables::Misc::selectedSubByTab[i];
                    UIMotion::NotifyTabChanged(i);
                }
                changed = true;
            }

            bool active = (*selectedTab == i && variables::Misc::floatingPanelOpen);
            UIMotion::SetIconHoverTarget(i, hovered || active, dt);
            const float hb = UIMotion::IconHover(i);

            ImVec2 center(p.x + btn.x * 0.5f, p.y + btn.y * 0.42f);
            if (hb > 0.01f) {
                const int bgA = active ? (int)(18 + hb * 8) : (int)(hb * 12);
                dl->AddRectFilled(ImVec2(p.x + 2, p.y + 1), ImVec2(p.x + btn.x - 2, p.y + btn.y - 1),
                    IM_COL32(255, 255, 255, bgA), 10.f);
            }
            ImU32 col = (active || hb > 0.35f)
                ? FrontierUI::U32(variables::Theme::brand, 0.75f + hb * 0.25f)
                : IM_COL32(150, 150, 160, 210);
            DrawIcon(dl, center, 15.0f, icons[i], col);
            if (active) {
                const float indicatorW = 7.f + hb * 3.f;
                dl->AddRectFilled(ImVec2(center.x - indicatorW, p.y + btn.y - 3),
                    ImVec2(center.x + indicatorW, p.y + btn.y - 1),
                    FrontierUI::U32(variables::Theme::brand, 0.85f + hb * 0.1f), 2.f);
            }
            ImGui::PopID();
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
        return changed;
    }

    // --- 3D bacon R6 ESP preview ---
    struct V3 { float x, y, z; };

    inline V3 RotY(V3 v, float a) {
        float c = cosf(a), s = sinf(a);
        return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
    }
    inline V3 RotX(V3 v, float a) {
        float c = cosf(a), s = sinf(a);
        return { v.x, v.y * c - v.z * s, v.y * s + v.z * c };
    }
    inline ImVec2 Proj3(V3 v, ImVec2 c, float scale) {
        float z = v.z + 5.2f;
        if (z < 0.2f) z = 0.2f;
        float f = scale / z;
        return { c.x + v.x * f, c.y - v.y * f };
    }

    inline void DrawBox3D(ImDrawList* dl, V3 center, V3 half, float yaw, float pitch,
        ImVec2 origin, float scale, ImU32 fill, ImU32 edge)
    {
        V3 corners[8] = {
            {-half.x,-half.y,-half.z},{ half.x,-half.y,-half.z},
            { half.x, half.y,-half.z},{-half.x, half.y,-half.z},
            {-half.x,-half.y, half.z},{ half.x,-half.y, half.z},
            { half.x, half.y, half.z},{-half.x, half.y, half.z},
        };
        ImVec2 s[8];
        float depth[8];
        for (int i = 0; i < 8; i++) {
            V3 w = { corners[i].x + center.x, corners[i].y + center.y, corners[i].z + center.z };
            w = RotY(w, yaw);
            w = RotX(w, pitch);
            depth[i] = w.z;
            s[i] = Proj3(w, origin, scale);
        }
        // Faces: each as 4 indices, draw back-to-front by avg depth
        int faces[6][4] = {
            {0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}
        };
        struct FaceOrd { int id; float z; };
        FaceOrd ord[6];
        for (int f = 0; f < 6; f++) {
            float z = 0;
            for (int k = 0; k < 4; k++) z += depth[faces[f][k]];
            ord[f] = { f, z * 0.25f };
        }
        for (int i = 0; i < 5; i++)
            for (int j = i + 1; j < 6; j++)
                if (ord[i].z > ord[j].z) { FaceOrd t = ord[i]; ord[i] = ord[j]; ord[j] = t; }

        for (int i = 0; i < 6; i++) {
            int f = ord[i].id;
            ImVec2 poly[4] = { s[faces[f][0]], s[faces[f][1]], s[faces[f][2]], s[faces[f][3]] };
            // slight shade by face index
            int shade = 18 + (f % 3) * 10;
            ImU32 fc = IM_COL32(
                (int)(((fill >> IM_COL32_R_SHIFT) & 0xFF) * (1.f - shade / 120.f)),
                (int)(((fill >> IM_COL32_G_SHIFT) & 0xFF) * (1.f - shade / 120.f)),
                (int)(((fill >> IM_COL32_B_SHIFT) & 0xFF) * (1.f - shade / 120.f)),
                255);
            dl->AddConvexPolyFilled(poly, 4, fc);
            dl->AddPolyline(poly, 4, edge, ImDrawFlags_Closed, 1.1f);
        }
    }

    inline void DrawEspPreview(ImDrawList* dl, ImVec2 origin, ImVec2 size) {
        const ImVec2 br(origin.x + size.x, origin.y + size.y);
        dl->AddRectFilledMultiColor(origin, br,
            IM_COL32(14, 16, 20, 255), IM_COL32(10, 12, 16, 255),
            IM_COL32(8, 10, 14, 255), IM_COL32(12, 14, 18, 255));
        dl->AddRect(origin, br, FrontierUI::U32(variables::Theme::border, 0.55f), 8.f);

        // subtle floor grid
        const float gridStep = 18.f;
        for (float gx = origin.x + 8.f; gx < br.x - 4.f; gx += gridStep)
            dl->AddLine(ImVec2(gx, origin.y + 8.f), ImVec2(gx, br.y - 8.f), IM_COL32(255, 255, 255, 6));
        for (float gy = origin.y + 8.f; gy < br.y - 4.f; gy += gridStep)
            dl->AddLine(ImVec2(origin.x + 8.f, gy), ImVec2(br.x - 8.f, gy), IM_COL32(255, 255, 255, 6));

        dl->AddText(ImVec2(origin.x + 10.f, origin.y + 8.f), IM_COL32(150, 154, 168, 220), "ESP Preview");

        if (!variables::ESP::enabled) {
            const char* off = "Enable ESP to preview";
            ImVec2 ts = ImGui::CalcTextSize(off);
            dl->AddText(
                ImVec2(origin.x + (size.x - ts.x) * 0.5f, origin.y + (size.y - ts.y) * 0.5f),
                IM_COL32(120, 124, 132, 200), off);
            return;
        }

        const float padX = size.x * 0.18f;
        const float padY = size.y * 0.12f;
        const float minX = origin.x + padX;
        const float maxX = origin.x + size.x - padX;
        const float minY = origin.y + padY + 10.f;
        const float maxY = origin.y + size.y - padY - 8.f;
        const float cx = (minX + maxX) * 0.5f;

        ImU32 bc = IM_COL32(
            (int)(variables::ESP::boxColor[0] * 255),
            (int)(variables::ESP::boxColor[1] * 255),
            (int)(variables::ESP::boxColor[2] * 255), 255);

        if (variables::ESP::fillBox)
            dl->AddRectFilled(ImVec2(minX, minY), ImVec2(maxX, maxY), IM_COL32(255, 255, 255, 14));
        if (variables::ESP::boxes) {
            if (variables::ESP::cornerBox || variables::ESP::boxType == 2) {
                const float w = maxX - minX, h = maxY - minY;
                const float cl = w * 0.28f, ct = h * 0.22f;
                auto corner = [&](float x, float y, float dx, float dy) {
                    dl->AddLine(ImVec2(x, y), ImVec2(x + dx * cl, y), bc, variables::ESP::boxThickness);
                    dl->AddLine(ImVec2(x, y), ImVec2(x, y + dy * ct), bc, variables::ESP::boxThickness);
                };
                corner(minX, minY, 1, 1); corner(maxX, minY, -1, 1);
                corner(minX, maxY, 1, -1); corner(maxX, maxY, -1, -1);
            } else {
                dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), bc, 0.f, 0, variables::ESP::boxThickness);
            }
        }
        if (variables::ESP::healthBar) {
            dl->AddRectFilled(ImVec2(minX - 6.f, minY), ImVec2(minX - 2.f, maxY), IM_COL32(0, 0, 0, 180));
            dl->AddRectFilled(ImVec2(minX - 5.f, minY + (maxY - minY) * 0.22f), ImVec2(minX - 3.f, maxY),
                IM_COL32(
                    (int)(variables::ESP::healthColor[0] * 255),
                    (int)(variables::ESP::healthColor[1] * 255),
                    (int)(variables::ESP::healthColor[2] * 255), 255));
        }
        if (variables::ESP::skeleton) {
            const ImU32 sk = IM_COL32(255, 255, 255, 210);
            const float headY = minY + 12.f;
            dl->AddCircleFilled(ImVec2(cx, headY), 4.f, sk, 12);
            dl->AddLine(ImVec2(cx, headY + 4.f), ImVec2(cx, maxY - 28.f), sk, 1.5f);
            dl->AddLine(ImVec2(cx, minY + 36.f), ImVec2(minX + 8.f, minY + 62.f), sk, 1.5f);
            dl->AddLine(ImVec2(cx, minY + 36.f), ImVec2(maxX - 8.f, minY + 62.f), sk, 1.5f);
            dl->AddLine(ImVec2(cx, maxY - 28.f), ImVec2(minX + 10.f, maxY - 6.f), sk, 1.5f);
            dl->AddLine(ImVec2(cx, maxY - 28.f), ImVec2(maxX - 10.f, maxY - 6.f), sk, 1.5f);
        }
        if (variables::ESP::headDot)
            dl->AddCircleFilled(ImVec2(cx, minY + 12.f), 3.5f, IM_COL32(
                (int)(variables::ESP::headDotColor[0] * 255),
                (int)(variables::ESP::headDotColor[1] * 255),
                (int)(variables::ESP::headDotColor[2] * 255), 255), 12);
        if (variables::ESP::names) {
            const char* n = "Player";
            ImVec2 ts = ImGui::CalcTextSize(n);
            dl->AddText(ImVec2(cx - ts.x * 0.5f, minY - 18.f), IM_COL32(255, 255, 255, 240), n);
        }
        if (variables::ESP::distance) {
            const char* d = "42m";
            ImVec2 ts = ImGui::CalcTextSize(d);
            dl->AddText(ImVec2(cx - ts.x * 0.5f, maxY + 4.f), IM_COL32(200, 204, 214, 230), d);
        }
        if (variables::ESP::equippedItem) {
            const char* w = "Rifle";
            ImVec2 ts = ImGui::CalcTextSize(w);
            dl->AddText(ImVec2(cx - ts.x * 0.5f, maxY + 18.f), IM_COL32(255, 205, 115, 230), w);
        }
        if (variables::ESP::flags)
            dl->AddText(ImVec2(maxX + 4.f, minY), IM_COL32(255, 210, 120, 230), "GUN");
        if (variables::ESP::snaplines)
            dl->AddLine(ImVec2(origin.x + size.x * 0.5f, br.y - 6.f),
                ImVec2(cx, maxY), bc, 1.2f);
    }

    inline void EspPreviewPanel(float height = 220.f) {
        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Live preview");
        ImGui::Dummy(ImVec2(0, 4));
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        if (w < 120.f) w = 120.f;
        ImGui::Dummy(ImVec2(w, height));
        DrawEspPreview(ImGui::GetWindowDrawList(), p, ImVec2(w, height));
    }
}

