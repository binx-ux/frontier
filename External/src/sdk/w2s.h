#pragma once
#include "sdk.h"
#include "offsets.h"
#include "window_manager.h"
#include "../memory/memory.h"

namespace W2S {

    // Cached once per overlay frame — WorldToScreen used to call FindWindow every bone
    inline float cachedSW = 0, cachedSH = 0, cachedOX = 0, cachedOY = 0;
    inline float renderSW = 0, renderSH = 0;
    inline bool viewportReady = false;

    inline void GetViewport(float& screenW, float& screenH, float& originX, float& originY) {
        WindowManager::UpdateRobloxWindowInfo();
        const auto& r = WindowManager::robloxRect;
        int w = r.right - r.left;
        int h = r.bottom - r.top;
        if (w > 50 && h > 50) {
            screenW = static_cast<float>(w);
            screenH = static_cast<float>(h);
            originX = static_cast<float>(r.left);
            originY = static_cast<float>(r.top);
            return;
        }
        screenW = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
        screenH = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
        originX = 0.0f;
        originY = 0.0f;
    }

    inline void BeginFrame() {
        GetViewport(cachedSW, cachedSH, cachedOX, cachedOY);
        renderSW = cachedSW;
        renderSH = cachedSH;
        viewportReady = true;
    }

    inline void SetRenderDimensions(float w, float h) {
        if (w > 50.f && h > 50.f) {
            renderSW = w;
            renderSH = h;
        }
    }

    inline void EnsureViewport(float& screenW, float& screenH, float& originX, float& originY) {
        if (!viewportReady)
            BeginFrame();
        screenW = cachedSW;
        screenH = cachedSH;
        originX = cachedOX;
        originY = cachedOY;
    }

    // Overlay/client space (0..width) — overlay matches Roblox client
    inline RBX::Vec2 WorldToScreen(const RBX::Vec3& worldPos, const RBX::Mat4& viewMatrix) {
        float screenW, screenH, ox, oy;
        EnsureViewport(screenW, screenH, ox, oy);

        float x = (worldPos.X * viewMatrix.data[0]) + (worldPos.Y * viewMatrix.data[1]) + (worldPos.Z * viewMatrix.data[2]) + viewMatrix.data[3];
        float y = (worldPos.X * viewMatrix.data[4]) + (worldPos.Y * viewMatrix.data[5]) + (worldPos.Z * viewMatrix.data[6]) + viewMatrix.data[7];
        float w = (worldPos.X * viewMatrix.data[12]) + (worldPos.Y * viewMatrix.data[13]) + (worldPos.Z * viewMatrix.data[14]) + viewMatrix.data[15];

        RBX::Vec2 screen{};
        if (w < 0.01f)
            return screen;

        float ndcX = x / w;
        float ndcY = y / w;

        // View matrix is built for VisualEngine render resolution — scale to overlay client size.
        float matW = (renderSW > 50.f) ? renderSW : screenW;
        float matH = (renderSH > 50.f) ? renderSH : screenH;
        screen.X = (matW * 0.5f * ndcX) + (matW * 0.5f);
        screen.Y = -(matH * 0.5f * ndcY) + (matH * 0.5f);

        if (matW > 50.f && matH > 50.f &&
            (fabsf(screenW - matW) > 1.f || fabsf(screenH - matH) > 1.f)) {
            screen.X *= screenW / matW;
            screen.Y *= screenH / matH;
        }
        return screen;
    }

    inline RBX::Vec2 WorldToScreenAbsolute(const RBX::Vec3& worldPos, const RBX::Mat4& viewMatrix) {
        float screenW, screenH, ox, oy;
        EnsureViewport(screenW, screenH, ox, oy);
        RBX::Vec2 local = WorldToScreen(worldPos, viewMatrix);
        if (local.X == 0 && local.Y == 0) return local;
        return { local.X + ox, local.Y + oy };
    }

    inline void GetCursorClient(float& cx, float& cy) {
        float screenW, screenH, ox, oy;
        EnsureViewport(screenW, screenH, ox, oy);
        POINT p;
        GetCursorPos(&p);
        cx = static_cast<float>(p.x) - ox;
        cy = static_cast<float>(p.y) - oy;
        if (cx < 0.f) cx = 0.f;
        if (cy < 0.f) cy = 0.f;
        if (screenW > 1.f && cx > screenW) cx = screenW;
        if (screenH > 1.f && cy > screenH) cy = screenH;
    }
}
