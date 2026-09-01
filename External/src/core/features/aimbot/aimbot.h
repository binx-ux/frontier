#pragma once
#include "../../../sdk/w2s.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/window_manager.h"
#include "../../../memory/memory.h"
#include "../../../core/cache/cache.h"
#include "../../../core/variables/variables.h"
#include "../../../sdk/sdk.h"
#include "visibility.h"
#include <windows.h>
#include <cmath>
#include <chrono>
#include <utility>
#include <vector>

// Ground-up aimbot: FOV gate + wall check + PD mouse control + AFK always-on.
// Patterns drawn from common external aim designs (FOV hysteresis, P+D damping, deadzone).
namespace Aimbot {

    inline uintptr_t lockedPlayerAddr = 0;
    inline float lockScreenX = 0.f;
    inline float lockScreenY = 0.f;
    inline bool hasLockScreen = false;
    inline bool aimToggleLatched = false;
    inline float accumX = 0.0f;
    inline float accumY = 0.0f;
    inline float prevErrX = 0.0f;
    inline float prevErrY = 0.0f;
    inline auto lastAimTick = std::chrono::steady_clock::now();
    inline auto lockAcquiredAt = std::chrono::steady_clock::now();
    inline bool wasAiming = false;
    inline bool silentSpoofActive = false;
    inline bool cursorClipped = false;
    inline float fovCenterX = 0.f;
    inline float fovCenterY = 0.f;
    inline int lockMissFrames = 0;

    inline void GetAimCenter(float& ax, float& ay)
    {
        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);

        ax = sw * 0.5f;
        ay = sh * 0.5f;

        if (ax < 0.f) ax = 0.f;
        if (ay < 0.f) ay = 0.f;
        if (ax > sw) ax = sw;
        if (ay > sh) ay = sh;
        fovCenterX = ax;
        fovCenterY = ay;
    }

    inline bool CombatBlocked()
    {
        if (variables::waitingForKey) return true;
        // Block only while the menu is capturing mouse/input — not merely because it is open
        if (variables::Misc::menuHovered) return true;
        if (!variables::Theme::useFloatingHeader && variables::Misc::menuAnim > 0.05f)
            return true;
        return false;
    }

    inline void RefreshFovCenter()
    {
        GetAimCenter(fovCenterX, fovCenterY);
    }

    inline void ClipCursorToGame()
    {
        WindowManager::UpdateRobloxWindowInfo();
        const RECT& r = WindowManager::robloxRect;
        if ((r.right - r.left) < 50 || (r.bottom - r.top) < 50) return;
        ClipCursor(&r);
        cursorClipped = true;
    }

    inline void ReleaseCursorClip()
    {
        if (cursorClipped) {
            ClipCursor(nullptr);
            cursorClipped = false;
        }
    }

    inline void ClearMoveState()
    {
        prevErrX = prevErrY = 0.f;
        accumX = accumY = 0.f;
    }

    inline void ClampCursorToGameWindow()
    {
        POINT pt{};
        if (!GetCursorPos(&pt)) return;
        WindowManager::UpdateRobloxWindowInfo();
        const RECT& r = WindowManager::robloxRect;
        const int rw = r.right - r.left;
        const int rh = r.bottom - r.top;
        if (rw < 50 || rh < 50) return;

        const LONG margin = 6;
        const LONG minX = r.left + margin;
        const LONG maxX = r.right - margin;
        const LONG minY = r.top + margin;
        const LONG maxY = r.bottom - margin;
        const LONG cx = (pt.x < minX) ? minX : (pt.x > maxX ? maxX : pt.x);
        const LONG cy = (pt.y < minY) ? minY : (pt.y > maxY ? maxY : pt.y);
        if (cx != pt.x || cy != pt.y)
            SetCursorPos(cx, cy);
    }

    inline void MoveMouse(float x, float y)
    {
        accumX += x;
        accumY += y;
        LONG ix = static_cast<LONG>(accumX);
        LONG iy = static_cast<LONG>(accumY);
        if (ix == 0 && iy == 0) return;
        accumX -= (float)ix;
        accumY -= (float)iy;

        POINT pt{};
        if (GetCursorPos(&pt)) {
            WindowManager::UpdateRobloxWindowInfo();
            const RECT& r = WindowManager::robloxRect;
            const int rw = r.right - r.left;
            const int rh = r.bottom - r.top;
            if (rw > 50 && rh > 50) {
                const LONG margin = 6;
                const LONG minX = r.left + margin;
                const LONG maxX = r.right - margin;
                const LONG minY = r.top + margin;
                const LONG maxY = r.bottom - margin;
                LONG tx = pt.x + ix;
                LONG ty = pt.y + iy;
                if (tx < minX) ix -= (tx - minX);
                else if (tx > maxX) ix -= (tx - maxX);
                if (ty < minY) iy -= (ty - minY);
                else if (ty > maxY) iy -= (ty - maxY);
                if (ix == 0 && iy == 0) return;
            }
        }

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = ix;
        input.mi.dy = iy;
        SendInput(1, &input, sizeof(INPUT));
    }

    inline void ClickMouse()
    {
        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, inputs, sizeof(INPUT));
    }

    // Non-blocking hold/release for trigger (avoids Sleep on render thread)
    inline bool clickHeld = false;
    inline auto clickDownAt = std::chrono::steady_clock::now();

    inline void PulseClickStart()
    {
        if (clickHeld) return;
        INPUT down = {};
        down.type = INPUT_MOUSE;
        down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &down, sizeof(INPUT));
        clickHeld = true;
        clickDownAt = std::chrono::steady_clock::now();
    }

    inline void PulseClickTick()
    {
        if (!clickHeld) return;
        float hold = variables::Trigger::releaseMs;
        if (variables::Rage::enabled && variables::Rage::shoot && hold > 10.f)
            hold = 8.f;
        if (hold < 1.f) hold = 1.f;
        if (std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - clickDownAt).count() < hold)
            return;
        INPUT up = {};
        up.type = INPUT_MOUSE;
        up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &up, sizeof(INPUT));
        clickHeld = false;
    }

    inline float Dist2(float ax, float ay, float bx, float by) {
        float dx = ax - bx, dy = ay - by;
        return sqrtf(dx * dx + dy * dy);
    }

    inline float Clamp(float v, float lo, float hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    inline bool IsKeyHeld(int vk)
    {
        if (vk == 1 || vk == VK_LBUTTON) return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (vk == 2 || vk == VK_RBUTTON) return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (vk == 4 || vk == VK_MBUTTON) return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        if (vk == 5 || vk == VK_XBUTTON1) return (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
        if (vk == 6 || vk == VK_XBUTTON2) return (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
        if (vk > 0) return (GetAsyncKeyState(vk) & 0x8000) != 0;
        return false;
    }

    inline bool IsAimKeyHeld()
    {
        if (!WindowManager::IsRobloxFocused()) return false;
        return IsKeyHeld(variables::Aimbot::aimbotKey);
    }

    inline bool IsSilentAimKeyHeld()
    {
        if (!WindowManager::IsRobloxFocused()) return false;
        int vk = variables::Aimbot::silentAimKey;
        if (vk <= 0) return IsAimKeyHeld();
        return IsKeyHeld(vk);
    }

    inline bool ShouldAim()
    {
        if (CombatBlocked()) return false;

        // Never aim from key/hold paths while Roblox is in the background
        const bool focused = WindowManager::IsRobloxFocused();

        // Aim only while shooting (optional)
        if (variables::Extra::aimOnShot && !(focused && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)))
            return false;

        // AFK / leave-it-running: aim without holding a key (still require focus so it
        // doesn't steal mouse while you're in another app)
        if (variables::Aimbot::alwaysOn || variables::Misc::afkAssist)
            return focused && (variables::Aimbot::enabled || variables::Rage::enabled || variables::Misc::afkAssist);

        if (variables::Rage::enabled)
            return focused; // key toggles enabled in main; no hold required
        const bool silentPath = variables::Aimbot::silentAim || variables::Aimbot::aimType == 1;
        if (!variables::Aimbot::enabled && !silentPath) return false;
        if (!focused) return false;
        if (variables::Aimbot::toggleMode) {
            bool held = silentPath ? IsSilentAimKeyHeld() : IsAimKeyHeld();
            if (held && !aimToggleLatched) {
                variables::Aimbot::toggledOn = !variables::Aimbot::toggledOn;
                aimToggleLatched = true;
            }
            else if (!held) aimToggleLatched = false;
            return variables::Aimbot::toggledOn;
        }
        return silentPath ? IsSilentAimKeyHeld() : IsAimKeyHeld();
    }

    inline RBX::Vec3 EyePos()
    {
        if (Globals::camera.Addr) {
            auto cf = Globals::camera.GetCameraCFrame();
            RBX::Vec3 p = cf.GetPosition();
            RBX::Vec3 look = cf.GetLookVector();
            // Nudge origin forward so wall rays don't self-hit near the camera
            p.X += look.X * 0.45f;
            p.Y += look.Y * 0.45f;
            p.Z += look.Z * 0.45f;
            return p;
        }
        RBX::Vec3 p = PlayerCache::localPlayerPos;
        p.Y += 1.5f;
        return p;
    }

    inline void LiveRefresh(PlayerCache::CachedPlayer& plr)
    {
        if (plr.headAddr) {
            plr.bones.head = RBX::RbxInstance(plr.headAddr).GetPos();
            plr.bones.hasHead = true;
        }
        if (plr.rootPartAddr) {
            plr.bones.hrp = RBX::RbxInstance(plr.rootPartAddr).GetPos();
            plr.bones.hasHrp = true;
            plr.position = plr.bones.hrp;
            auto prim = RBX::RbxInstance(plr.rootPartAddr).GetPrimitivePtr();
            if (prim)
                plr.velocity = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
        }
    }

    inline RBX::Vec3 AimPoint(const PlayerCache::CachedPlayer& plr, const RBX::Mat4* viewMatrix = nullptr)
    {
        int bone = variables::Aimbot::aimTarget;
        if (variables::Extra::randomBone) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            bone = (int)((ms / 350) + (plr.userId % 7)) % 6;
        }
        RBX::Vec3 pos = plr.bones.hasHead ? plr.bones.head : plr.position;
        switch (bone) {
        case 1: pos = plr.bones.hasHrp ? plr.bones.hrp : plr.position; break;
        case 2: pos = plr.bones.isR6 ? plr.bones.leftLeg : plr.bones.leftUpperLeg; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 3: pos = plr.bones.isR6 ? plr.bones.rightLeg : plr.bones.rightUpperLeg; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 4: pos = plr.bones.isR6 ? plr.bones.leftArm : plr.bones.leftUpperArm; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 5: pos = plr.bones.isR6 ? plr.bones.rightArm : plr.bones.rightUpperArm; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 6: {
            if (!viewMatrix) {
                pos = plr.bones.hasHead ? plr.bones.head : plr.position;
                break;
            }
            float sw, sh, ox, oy;
            W2S::GetViewport(sw, sh, ox, oy);
            float cx = 0.f, cy = 0.f;
            GetAimCenter(cx, cy);
            float best = 1e12f;
            RBX::Vec3 bestPos = pos;
            auto tryBone = [&](const RBX::Vec3& b) {
                if (b.X == 0.f && b.Y == 0.f && b.Z == 0.f) return;
                RBX::Vec2 s = W2S::WorldToScreen(b, *viewMatrix);
                if (s.X == 0.f && s.Y == 0.f) return;
                float d = Dist2(cx, cy, s.X, s.Y);
                if (d < best) { best = d; bestPos = b; }
            };
            tryBone(plr.bones.head);
            tryBone(plr.bones.hrp);
            tryBone(plr.bones.isR6 ? plr.bones.leftLeg : plr.bones.leftUpperLeg);
            tryBone(plr.bones.isR6 ? plr.bones.rightLeg : plr.bones.rightUpperLeg);
            tryBone(plr.bones.isR6 ? plr.bones.leftArm : plr.bones.leftUpperArm);
            tryBone(plr.bones.isR6 ? plr.bones.rightArm : plr.bones.rightUpperArm);
            pos = bestPos;
            break;
        }
        default: break;
        }

        if (variables::Aimbot::prediction) {
            float lead = Clamp(variables::Aimbot::predictionX, 0.02f, 0.35f);
            pos.X += plr.velocity.X * lead;
            pos.Y += plr.velocity.Y * Clamp(variables::Aimbot::predictionY, 0.02f, 0.35f);
            pos.Z += plr.velocity.Z * lead;
        }
        return pos;
    }

    inline bool VisibleEnough(const RBX::Vec3& eye, const PlayerCache::CachedPlayer& plr, const RBX::Vec3& aimWorld)
    {
        if (!variables::Aimbot::requireVisible) return true;
        Visibility::EnsureWorker();
        if (Visibility::boxCount.load() <= 0) return true;
        return Visibility::HasLineOfSight(eye, aimWorld);
    }

    inline bool ScreenPointValid(const RBX::Vec2& scr, float sw, float sh)
    {
        if (scr.X < -1.f || scr.Y < -1.f) return false;
        return scr.X >= -64.f && scr.Y >= -64.f && scr.X <= sw + 64.f && scr.Y <= sh + 64.f;
    }

    inline void ClearLock()
    {
        lockedPlayerAddr = 0;
        hasLockScreen = false;
        lockMissFrames = 0;
        ClearMoveState();
    }

    // --- Silent aim (MouseService / InputObject mouse spoof) ---
    // Roblox reads mouse from InputObject.MousePosition in render/viewport pixels
    // (VisualEngine dimensions), not always 1:1 with overlay client coords.
    inline uintptr_t cachedMouseService = 0;
    inline uintptr_t cachedPlayerMouse = 0;
    inline uintptr_t cachedInputObjects[8]{};
    inline int cachedInputObjectCount = 0;
    inline uintptr_t cachedUserInputState = 0;
    inline uintptr_t silentInputObject = 0;
    inline uintptr_t silentPosOffset = Offsets::MouseService::MousePosition;
    inline bool silentMouseReady = false;
    inline auto lastMouseResolve = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    inline bool UseSilentAim()
    {
        if (variables::Aimbot::aimType == 0 && !variables::Aimbot::silentAim)
            return false;
        if (variables::Aimbot::silentAim) return true;
        if (variables::Aimbot::aimType == 1) return true;
        return false;
    }

    inline void ClientToRenderMouse(float clientX, float clientY, float& rx, float& ry)
    {
        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        float matW = (W2S::renderSW > 50.f) ? W2S::renderSW : sw;
        float matH = (W2S::renderSH > 50.f) ? W2S::renderSH : sh;
        if (sw > 1.f && sh > 1.f &&
            (fabsf(matW - sw) > 1.f || fabsf(matH - sh) > 1.f)) {
            rx = clientX * (matW / sw);
            ry = clientY * (matH / sh);
        } else {
            rx = clientX;
            ry = clientY;
        }
        if (rx < 0.f) rx = 0.f;
        if (ry < 0.f) ry = 0.f;
        if (matW > 1.f && rx > matW) rx = matW;
        if (matH > 1.f && ry > matH) ry = matH;
    }

    inline bool LooksLikeMousePos(float x, float y, float maxW, float maxH)
    {
        if (!std::isfinite(x) || !std::isfinite(y)) return false;
        if (x < -80.f || y < -80.f) return false;
        if (x > maxW + 200.f || y > maxH + 200.f) return false;
        return true;
    }

    inline bool ValidateInputObject(uintptr_t io, float maxW, float maxH)
    {
        if (!io) return false;
        static const uintptr_t posOffs[] = { 0xD4, 0xE4, 0xEC, 0xF4 };
        for (uintptr_t off : posOffs) {
            float x = memory->read<float>(io + off);
            float y = memory->read<float>(io + off + 4);
            if (LooksLikeMousePos(x, y, maxW, maxH)) return true;
        }
        return false;
    }

    inline void PushInputObject(uintptr_t io, float maxW, float maxH, bool trusted = false)
    {
        if (!io || io < 0x10000) return;
        for (int i = 0; i < cachedInputObjectCount; i++)
            if (cachedInputObjects[i] == io) return;
        if (cachedInputObjectCount >= 8) return;
        if (!trusted && !ValidateInputObject(io, maxW, maxH)) return;
        cachedInputObjects[cachedInputObjectCount++] = io;
    }

    inline bool ProbeSilentMouseChannel(float maxW, float maxH)
    {
        silentInputObject = 0;
        silentPosOffset = Offsets::MouseService::MousePosition;
        silentMouseReady = false;

        static const uintptr_t posOffs[] = {
            Offsets::MouseService::MousePosition,
            0xEC, 0xE4, 0xF4, 0xD4
        };

        for (int i = 0; i < cachedInputObjectCount; i++) {
            uintptr_t io = cachedInputObjects[i];
            if (!memory->is_valid_address(io, 0x120)) continue;
            for (uintptr_t off : posOffs) {
                if (!memory->is_valid_address(io + off, 8)) continue;
                float ox = memory->read<float>(io + off);
                float oy = memory->read<float>(io + off + 4);
                if (!LooksLikeMousePos(ox, oy, maxW, maxH)) continue;
                silentInputObject = io;
                silentPosOffset = off;
                silentMouseReady = true;
                return true;
            }
        }
        return false;
    }

    inline bool ResolveMouseService(bool force = false)
    {
        auto now = std::chrono::steady_clock::now();
        if (!force && cachedInputObjectCount > 0 &&
            std::chrono::duration<float>(now - lastMouseResolve).count() < 0.08f)
            return true;

        lastMouseResolve = now;
        cachedMouseService = cachedPlayerMouse = 0;
        cachedUserInputState = 0;
        cachedInputObjectCount = 0;
        silentMouseReady = false;
        silentInputObject = 0;
        for (auto& io : cachedInputObjects) io = 0;
        if (!Globals::dataModel.Addr) return false;

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        float maxW = (W2S::renderSW > 50.f) ? W2S::renderSW : sw;
        float maxH = (W2S::renderSH > 50.f) ? W2S::renderSH : sh;

        auto ms = Globals::dataModel.FindChildByClass("MouseService");
        if (!ms.Addr) ms = Globals::dataModel.FindChild("MouseService");
        if (ms.Addr) {
            cachedMouseService = ms.Addr;
            static const uintptr_t ioOffs[] = {
                Offsets::MouseService::InputObject,
                Offsets::MouseService::InputObject2,
                0xE8, 0xF8, 0x108, 0x110, 0x118, 0x128
            };
            for (uintptr_t off : ioOffs) {
                uintptr_t io = memory->read<uintptr_t>(ms.Addr + off);
                const bool trusted = (off == Offsets::MouseService::InputObject ||
                    off == Offsets::MouseService::InputObject2);
                PushInputObject(io, maxW, maxH, trusted);
            }
        }

        auto uis = Globals::dataModel.FindChildByClass("UserInputService");
        if (!uis.Addr) uis = Globals::dataModel.FindChild("UserInputService");
        if (uis.Addr) {
            cachedUserInputState = memory->read<uintptr_t>(
                uis.Addr + Offsets::UserInputService::WindowInputState);
            if (cachedUserInputState) {
                static const uintptr_t wisOffs[] = { 0x0, 0x8, 0x10, 0x18, 0x20, 0xF0, 0x100, 0xD4 };
                for (uintptr_t off : wisOffs) {
                    uintptr_t io = memory->read<uintptr_t>(cachedUserInputState + off);
                    PushInputObject(io, maxW, maxH);
                }
            }
        }

        if (Globals::localPlayer.Addr) {
            uintptr_t mouseInst = memory->read<uintptr_t>(
                Globals::localPlayer.Addr + Offsets::Player::Mouse);
            if (mouseInst) {
                cachedPlayerMouse = mouseInst;
                static const uintptr_t ioOffs[] = {
                    Offsets::MouseService::InputObject,
                    Offsets::MouseService::InputObject2,
                    0xD4, 0xE4, 0xEC
                };
                for (uintptr_t off : ioOffs) {
                    uintptr_t io = memory->read<uintptr_t>(mouseInst + off);
                    PushInputObject(io, maxW, maxH);
                }
            }
        }
        return cachedInputObjectCount > 0;
    }

    inline bool SetSilentMouse(float clientX, float clientY)
    {
        if (!WindowManager::IsRobloxFocused())
            return false;

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        if (clientX < 0.f) clientX = 0.f;
        if (clientY < 0.f) clientY = 0.f;
        if (clientX > sw) clientX = sw;
        if (clientY > sh) clientY = sh;

        float maxW = (W2S::renderSW > 50.f) ? W2S::renderSW : sw;
        float maxH = (W2S::renderSH > 50.f) ? W2S::renderSH : sh;

        if (!silentMouseReady) {
            if (!ResolveMouseService(true))
                return false;
            if (!ProbeSilentMouseChannel(maxW, maxH))
                return false;
        }

        if (!silentInputObject || !memory->is_valid_address(silentInputObject + silentPosOffset, 8)) {
            silentMouseReady = false;
            return false;
        }

        float rx, ry;
        ClientToRenderMouse(clientX, clientY, rx, ry);
        if (!LooksLikeMousePos(rx, ry, maxW, maxH))
            return false;

        memory->write<float>(silentInputObject + silentPosOffset, rx);
        memory->write<float>(silentInputObject + silentPosOffset + 4, ry);

        float verifyX = memory->read<float>(silentInputObject + silentPosOffset);
        float verifyY = memory->read<float>(silentInputObject + silentPosOffset + 4);
        if (!std::isfinite(verifyX) || !std::isfinite(verifyY)) {
            silentMouseReady = false;
            return false;
        }
        return true;
    }

    inline bool RestoreSilentMouseFromCursor()
    {
        silentSpoofActive = false;
        silentMouseReady = false;
        return true;
    }

    // Shared closest-target picker for silent aim + magic bullet (screen-space FOV gate)
    inline float HitboxPixelRadius(PlayerCache::CachedPlayer& plr, const RBX::Mat4& viewMatrix);

    inline bool FindSpoofTarget(
        const RBX::Mat4& viewMatrix,
        std::vector<PlayerCache::CachedPlayer>& players,
        float fovLimit,
        int boneOverride,
        bool useHitboxRadius,
        bool checkWall,
        RBX::Vec2& outScr,
        RBX::Vec3& outWorld)
    {
        float aimCx = 0.f, aimCy = 0.f;
        GetAimCenter(aimCx, aimCy);
        RBX::Vec3 eye = EyePos();
        float maxDist = variables::Aimbot::maxDistance;
        if (maxDist < 50.f) maxDist = 50.f;
        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);

        const int savedBone = variables::Aimbot::aimTarget;
        if (boneOverride >= 0)
            variables::Aimbot::aimTarget = boneOverride;

        float bestPd = 1e12f;
        bool found = false;

        for (auto& plr : players) {
            if (!plr.isValid) continue;
            if (plr.distance > maxDist) continue;
            if (useHitboxRadius) {
                if (variables::Hitbox::healthCheck && plr.health <= 0.f) continue;
                if (variables::Hitbox::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
            } else {
                if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
                if (variables::healthCheck && plr.health <= 0.f) continue;
            }

            LiveRefresh(plr);
            RBX::Vec3 world = AimPoint(plr, &viewMatrix);
            RBX::Vec2 scr = W2S::WorldToScreen(world, viewMatrix);
            if (!ScreenPointValid(scr, sw, sh)) continue;

            float limit = fovLimit;
            if (useHitboxRadius) {
                float hbR = HitboxPixelRadius(plr, viewMatrix);
                if (hbR > limit) limit = hbR;
            }

            float pd = Dist2(aimCx, aimCy, scr.X, scr.Y);
            if (pd > limit) continue;
            if (checkWall && !VisibleEnough(eye, plr, world)) continue;

            if (pd < bestPd) {
                bestPd = pd;
                outScr = scr;
                outWorld = world;
                found = true;
            }
        }

        variables::Aimbot::aimTarget = savedBone;
        return found;
    }

    // Fire-based silent aim — spoof mouse on each shot (no camera move)
    inline void RunSilentFireAssist(const RBX::Mat4& viewMatrix, std::vector<PlayerCache::CachedPlayer>& players)
    {
        if (!UseSilentAim()) return;
        if (CombatBlocked()) return;
        if (!WindowManager::IsRobloxFocused()) return;

        const bool firing = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (!firing) return;
        if (!variables::Aimbot::alwaysOn && !ShouldAim()) return;

        ResolveMouseService(false);

        float t = Clamp(variables::Aimbot::uiSilentFov * 0.01f, 0.f, 1.f);
        float acquireFov = 20.f + (500.f - 20.f) * t;
        if (acquireFov < 15.f) acquireFov = 15.f;

        RBX::Vec2 scr{};
        RBX::Vec3 world{};
        if (!FindSpoofTarget(viewMatrix, players, acquireFov, -1, false,
                variables::Aimbot::requireVisible, scr, world))
            return;

        if (SetSilentMouse(scr.X, scr.Y))
            silentSpoofActive = true;
    }

    inline void RunSilentFireAssist(const RBX::Mat4& viewMatrix)
    {
        auto players = PlayerCache::snapshotPlayers();
        RunSilentFireAssist(viewMatrix, players);
    }

    inline void SyncUiSliders()
    {
        using namespace variables::Aimbot;
        auto lerp = [](float ui, float lo, float hi) {
            float t = Clamp(ui * 0.01f, 0.f, 1.f);
            return lo + (hi - lo) * t;
        };
        smoothing = lerp(uiSmoothness, 4.f, 30.f);
        damping = lerp(uiStability, 0.f, 0.85f);
        deadzone = lerp(uiLockZone, 0.5f, 12.f);
        maxDistance = lerp(uiRange, 100.f, 10000.f);
        fovRadius = lerp(uiFov, 20.f, 500.f);
        silentFovRadius = lerp(uiSilentFov, 20.f, 500.f);
        holdFovScale = lerp(uiStickyFov, 1.0f, 1.8f);
        maxMove = lerp(uiAimSpeed, 4.f, 28.f);
    }

    inline float ComputeAcquireFov()
    {
        SyncUiSliders();
        const bool silent = UseSilentAim();
        float acquireFov = silent ? variables::Aimbot::silentFovRadius : variables::Aimbot::fovRadius;
        if (acquireFov < 15.f) acquireFov = 15.f;
        if (Games::IsBloxStrike())
            acquireFov *= 1.35f;
        if (variables::Hitbox::enabled || variables::Local::hitboxEnabled || variables::MagicBullet::enabled) {
            float hbBoost = variables::Hitbox::size * 6.f;
            if (hbBoost > acquireFov) acquireFov = hbBoost;
        }
        return acquireFov;
    }

    inline void OnAimReleased()
    {
        ClearMoveState();
        if (silentSpoofActive)
            RestoreSilentMouseFromCursor();
        silentSpoofActive = false;
        ReleaseCursorClip();
    }

    inline void RunAimbot(const RBX::Mat4& viewMatrix, std::vector<PlayerCache::CachedPlayer>& players)
    {
        SyncUiSliders();

        if (!ShouldAim()) {
            if (wasAiming)
                OnAimReleased();
            if (!variables::Aimbot::stickyAim)
                ClearLock();
            wasAiming = false;
            lastAimTick = std::chrono::steady_clock::now();
            return;
        }
        wasAiming = true;

        // Keep physical cursor inside the Roblox client while aiming with mouse movement
        if (!UseSilentAim() || Games::IsBloxStrike())
            ClipCursorToGame();

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastAimTick).count();
        lastAimTick = now;
        dt = Clamp(dt, 0.0008f, 0.033f);

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        float aimCx = 0.f, aimCy = 0.f;
        GetAimCenter(aimCx, aimCy);

        float acquireFov = ComputeAcquireFov();
        float holdFov = acquireFov * variables::Aimbot::holdFovScale;
        if (holdFov < acquireFov) holdFov = acquireFov;

        float maxDist = variables::Aimbot::maxDistance;
        if (maxDist < 50.f) maxDist = 50.f;

        RBX::Vec3 eye = EyePos();

        static bool kickedRebuild = false;
        if (variables::Aimbot::requireVisible) {
            Visibility::EnsureWorker();
            if (!kickedRebuild) {
                Visibility::RequestRebuild();
                kickedRebuild = true;
            }
        } else {
            kickedRebuild = false;
        }

        auto evaluate = [&](PlayerCache::CachedPlayer& plr, float fovLimit, RBX::Vec2& screenOut, float& pixelDist, bool doLos) -> bool {
            if (!plr.isValid) return false;
            if (plr.distance > maxDist) return false;
            if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) return false;
            if (variables::healthCheck && plr.health <= 0.f) return false;

            LiveRefresh(plr);
            RBX::Vec3 world = AimPoint(plr, &viewMatrix);

            screenOut = W2S::WorldToScreen(world, viewMatrix);
            if (!ScreenPointValid(screenOut, sw, sh)) return false;

            pixelDist = Dist2(aimCx, aimCy, screenOut.X, screenOut.Y);
            if (pixelDist > fovLimit) return false;

            if (doLos && !VisibleEnough(eye, plr, world)) return false;
            return true;
        };

        if (!variables::Aimbot::stickyAim) {
            ClearLock();
        } else if (lockedPlayerAddr) {
            bool keep = false;
            for (auto& plr : players) {
                if (plr.playerAddr != lockedPlayerAddr) continue;
                RBX::Vec2 scr{};
                float pd = 0.f;
                keep = evaluate(plr, holdFov, scr, pd, true);
                break;
            }
            if (!keep)
                ClearLock();
        }

        if (!lockedPlayerAddr) {
            float best = 1e12f;
            uintptr_t bestAddr = 0;
            uintptr_t cand[12]{};
            float candScore[12]{};
            int nCand = 0;
            const int prio = variables::Aimbot::targetPriority;
            for (auto& plr : players) {
                RBX::Vec2 scr{};
                float pd = 0.f;
                if (!evaluate(plr, acquireFov, scr, pd, false)) continue;
                float score = pd;
                if (prio == 1) score = plr.health; // lowest HP
                else if (prio == 2) score = plr.distance; // closest world
                if (nCand < 12) {
                    cand[nCand] = plr.playerAddr;
                    candScore[nCand] = score;
                    nCand++;
                } else {
                    int worst = 0;
                    for (int i = 1; i < 12; i++) if (candScore[i] > candScore[worst]) worst = i;
                    if (score < candScore[worst]) { cand[worst] = plr.playerAddr; candScore[worst] = score; }
                }
            }
            for (int i = 0; i < nCand; i++)
                for (int j = i + 1; j < nCand; j++)
                    if (candScore[j] < candScore[i]) {
                        std::swap(candScore[i], candScore[j]);
                        std::swap(cand[i], cand[j]);
                    }
            for (int i = 0; i < nCand; i++) {
                for (auto& plr : players) {
                    if (plr.playerAddr != cand[i]) continue;
                    RBX::Vec2 scr{};
                    float pd = 0.f;
                    if (evaluate(plr, acquireFov, scr, pd, true)) {
                        bestAddr = plr.playerAddr;
                        best = pd;
                    }
                    break;
                }
                if (bestAddr) break;
            }
            if (bestAddr) {
                lockedPlayerAddr = bestAddr;
                lockAcquiredAt = now;
                lockMissFrames = 0;
                prevErrX = prevErrY = 0.f;
                (void)best;
            }
        }

        if (!lockedPlayerAddr) return;

        for (auto& plr : players) {
            if (plr.playerAddr != lockedPlayerAddr) continue;

            RBX::Vec2 scr{};
            float pd = 0.f;
            if (!evaluate(plr, holdFov, scr, pd, true)) {
                lockMissFrames++;
                if (lockMissFrames >= 5) {
                    ClearLock();
                }
                return;
            }
            lockMissFrames = 0;
            lockScreenX = scr.X;
            lockScreenY = scr.Y;
            hasLockScreen = true;

            const bool wantSilent = UseSilentAim();
            const bool hitboxAssist =
                (variables::Hitbox::enabled || variables::Local::hitboxEnabled || variables::MagicBullet::enabled)
                && variables::Hitbox::aimAssist;
            const bool firing = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

            // Pure silent: spoof only while shooting — never move physical mouse (prevents crashes)
            if (wantSilent && !Games::IsBloxStrike()) {
                if (firing && ShouldAim()) {
                    if (!ResolveMouseService(false))
                        ResolveMouseService(true);
                    if (SetSilentMouse(scr.X, scr.Y))
                        silentSpoofActive = true;
                }
                return;
            }

            bool silentOk = false;
            if ((wantSilent || hitboxAssist) && firing) {
                if (!ResolveMouseService(false))
                    ResolveMouseService(true);
                silentOk = SetSilentMouse(scr.X, scr.Y);
                if (silentOk) silentSpoofActive = true;
            }

            if (wantSilent && silentOk) {
                prevErrX = scr.X - aimCx;
                prevErrY = scr.Y - aimCy;
                accumX = accumY = 0.f;
                float errX = scr.X - aimCx;
                float errY = scr.Y - aimCy;
                float dist = sqrtf(errX * errX + errY * errY);
                if (dist < 12.f)
                    return;
            }

            if (wantSilent && !silentOk && firing) {
                ResolveMouseService(true);
                SetSilentMouse(scr.X, scr.Y);
            }

            float errX = scr.X - aimCx;
            float errY = scr.Y - aimCy;
            float dist = sqrtf(errX * errX + errY * errY);

            float dead = variables::Aimbot::deadzone;
            if (dead < 1.2f) dead = 1.2f;
            if (dist < dead) {
                prevErrX = errX;
                prevErrY = errY;
                accumX = accumY = 0.f;
                return;
            }

            float smooth = variables::Aimbot::smoothing;
            if (smooth < 2.f) smooth = 2.f;
            // Exponential lock-on — no PD wiggle, snaps when close
            float speed = 24.f / smooth;
            float t = 1.f - expf(-dt * speed);
            if (dist < 6.f)
                t = 1.f;
            else if (dist < 22.f)
                t = Clamp(t * 1.4f, 0.f, 1.f);

            float mx = errX * t;
            float my = errY * t;

            float cap = Clamp(variables::Aimbot::maxMove, 8.f, 42.f);
            float step = sqrtf(mx * mx + my * my);
            if (step > cap && step > 0.001f) {
                float s = cap / step;
                mx *= s; my *= s;
            }

            prevErrX = errX * (1.f - t);
            prevErrY = errY * (1.f - t);
            MoveMouse(mx, my);
            return;
        }
        ClearLock();
    }

    inline void RunAimbot(const RBX::Mat4& viewMatrix)
    {
        auto players = PlayerCache::snapshotPlayers();
        RunAimbot(viewMatrix, players);
    }

    inline float HitboxPixelRadius(PlayerCache::CachedPlayer& plr, const RBX::Mat4& viewMatrix)
    {
        if (!variables::Hitbox::enabled && !variables::Local::hitboxEnabled)
            return 0.f;
        float half = variables::Hitbox::size * 0.5f;
        if (half < 1.f) return 0.f;
        RBX::Vec3 c = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
        RBX::Vec2 sc = W2S::WorldToScreen(c, viewMatrix);
        if (sc.X == 0.f && sc.Y == 0.f) return 0.f;
        // Approximate screen radius from a world-space offset along X
        RBX::Vec3 edge{ c.X + half, c.Y, c.Z };
        RBX::Vec2 se = W2S::WorldToScreen(edge, viewMatrix);
        float r = (se.X == 0.f && se.Y == 0.f) ? 0.f : Dist2(sc.X, sc.Y, se.X, se.Y);
        if (r < 24.f) {
            // Fallback: distance-scaled estimate
            float d = plr.distance;
            if (d < 8.f) d = 8.f;
            r = (half * 420.f) / d;
        }
        return Clamp(r, 20.f, 380.f);
    }

    inline void RunTriggerbot(const RBX::Mat4& viewMatrix, std::vector<PlayerCache::CachedPlayer>& players)
    {
        PulseClickTick();

        if (CombatBlocked()) return;

        static auto lastShot = std::chrono::steady_clock::now();
        const bool rageShoot = variables::Rage::enabled && variables::Rage::shoot;
        const bool afkTrig = variables::Misc::afkAssist && variables::Trigger::enabled;
        if (!variables::Trigger::enabled && !rageShoot && !afkTrig)
            return;
        if (!WindowManager::IsRobloxFocused())
            return;
        if (variables::Trigger::enabled && variables::Trigger::useHotkey && !afkTrig &&
            !(GetAsyncKeyState(variables::Trigger::key) & 0x8000)) {
            if (!rageShoot) return;
        }
        // Don't queue another click while still holding — but rage can be snappier
        if (clickHeld && !rageShoot) return;

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        float aimCx = 0.f, aimCy = 0.f;
        GetAimCenter(aimCx, aimCy);
        RBX::Vec3 eye = EyePos();

        bool hit = false;
        float bestPd = 1e12f;
        RBX::Vec2 bestScr{};
        RBX::Vec3 bestWorld{};

        for (auto& plr : players) {
            if (!plr.isValid) continue;
            if (!variables::Trigger::targetPlayers && plr.playerAddr) continue;
            if (!variables::Trigger::targetNpc && !plr.playerAddr) continue;
            if (!variables::Trigger::targetDead && plr.health <= 0.f) continue;
            if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
            LiveRefresh(plr);

            RBX::Vec3 worldHead = plr.bones.hasHead ? plr.bones.head : plr.position;
            RBX::Vec3 worldBody = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
            RBX::Vec3 world = variables::Trigger::headOnly ? worldHead : worldBody;

            RBX::Vec2 scr = W2S::WorldToScreen(world, viewMatrix);
            if (!ScreenPointValid(scr, sw, sh)) {
                world = worldBody;
                scr = W2S::WorldToScreen(world, viewMatrix);
                if (!ScreenPointValid(scr, sw, sh)) continue;
            }

            float baseR = variables::Trigger::hitRadius;
            if (baseR < 6.f) baseR = 6.f;
            float r = baseR + Clamp(plr.distance / 70.f, 0.f, 14.f);
            if (rageShoot) r *= 1.35f;

            float hbR = HitboxPixelRadius(plr, viewMatrix);
            if (hbR > r) r = hbR;

            float pd = Dist2(aimCx, aimCy, scr.X, scr.Y);
            // Also accept body if not head-only
            if (!variables::Trigger::headOnly) {
                RBX::Vec2 sb = W2S::WorldToScreen(worldBody, viewMatrix);
                if (ScreenPointValid(sb, sw, sh)) {
                    float pdb = Dist2(aimCx, aimCy, sb.X, sb.Y);
                    if (pdb < pd) { pd = pdb; scr = sb; world = worldBody; }
                }
            }

            if (pd > r) continue;
            if ((variables::Trigger::requireVisible || variables::Aimbot::requireVisible) &&
                !Visibility::HasLineOfSight(eye, world))
                continue;

            if (pd < bestPd) {
                bestPd = pd;
                bestScr = scr;
                bestWorld = world;
                hit = true;
            }
        }
        if (!hit) return;

        // Big hitbox assist: spoof mouse onto the real bone so Arsenal registers the shot
        if ((variables::Hitbox::enabled || variables::Local::hitboxEnabled || variables::MagicBullet::enabled)
            && variables::Hitbox::aimAssist)
            SetSilentMouse(bestScr.X, bestScr.Y);
        else if (rageShoot && UseSilentAim())
            SetSilentMouse(bestScr.X, bestScr.Y);

        auto now = std::chrono::steady_clock::now();
        float delay = variables::Trigger::delayMs;
        if (rageShoot) {
            delay = variables::Rage::delayMs;
            if (delay < 0.f) delay = 0.f;
        }
        if (afkTrig && delay < 40.f) delay = 40.f;
        if (std::chrono::duration<float, std::milli>(now - lastShot).count() < delay)
            return;

        if (variables::Trigger::releaseMs > 1.f || rageShoot)
            PulseClickStart();
        else if (variables::Extra::burstTrigger && !rageShoot) {
            int n = variables::Extra::burstCount;
            if (n < 2) n = 2;
            if (n > 8) n = 8;
            for (int i = 0; i < n; i++)
                ClickMouse();
        }
        else
            ClickMouse();
        lastShot = now;
        (void)bestWorld;
    }

    // Close-range auto swing / click
    inline void RunMeleeAura(std::vector<PlayerCache::CachedPlayer>& players)
    {
        if (!variables::Extra::meleeAura) return;
        static auto lastMelee = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float, std::milli>(now - lastMelee).count() < 90.f) return;

        float range = variables::Extra::meleeRange;
        if (range < 2.f) range = 2.f;
        for (auto& plr : players) {
            if (!plr.isValid || plr.health <= 0.f) continue;
            if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
            if (plr.distance > range) continue;
            ClickMouse();
            lastMelee = now;
            break;
        }
    }

    inline void RunTriggerbot(const RBX::Mat4& viewMatrix)
    {
        auto players = PlayerCache::snapshotPlayers();
        RunTriggerbot(viewMatrix, players);
    }

    // Magic bullet: spoof mouse to expanded hitbox target while firing
    inline void RunMagicBulletAssist(const RBX::Mat4& viewMatrix, std::vector<PlayerCache::CachedPlayer>& players)
    {
        const bool magicOn = variables::MagicBullet::enabled || variables::Hitbox::enabled;
        if (!magicOn) return;
        if (CombatBlocked()) return;
        if (!WindowManager::IsRobloxFocused()) return;
        if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;

        float t = Clamp(variables::MagicBullet::uiFov * 0.01f, 0.f, 1.f);
        float acquireFov = 20.f + (500.f - 20.f) * t;
        variables::MagicBullet::fovRadius = acquireFov;
        float hbBoost = variables::Hitbox::size * 6.f;
        if (hbBoost > acquireFov) acquireFov = hbBoost;

        RBX::Vec2 bestScr{};
        RBX::Vec3 bestWorld{};
        if (!FindSpoofTarget(viewMatrix, players, acquireFov, variables::MagicBullet::hitbox,
                true, variables::Aimbot::requireVisible, bestScr, bestWorld))
            return;

        if (SetSilentMouse(bestScr.X, bestScr.Y))
            silentSpoofActive = true;
    }

    inline void RunMagicBulletAssist(const RBX::Mat4& viewMatrix)
    {
        auto players = PlayerCache::snapshotPlayers();
        RunMagicBulletAssist(viewMatrix, players);
    }
}
