#pragma once
#include "frontier_ui.h"
#include "frontier_theme.h"
#include "../core/cache/cache.h"
#include "../core/globals/globals.h"
#include "../core/features/exploits/gun_mods.h"
#include "../core/games/arsenal.h"
#include "../core/servers/server_browser.h"
#include "../core/audio/custom_music.h"
#include "../render/spotify_player.h"
#include "../core/features/exploits/animation_catalog.h"
#include "../core/config/config.h"
#include "../discord/frontier_presence.h"
#include "../sdk/offsets.h"
#include "../memory/memory.h"
#include <Shellapi.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace FrontierMenu {

    inline void RefreshStatusInfo() {
        float t = (float)ImGui::GetTime();
        if (t - variables::Status::lastRefresh < 0.5f) return;
        variables::Status::lastRefresh = t;

        strncpy_s(variables::Status::clientVersion, Offsets::ClientVersion.c_str(), _TRUNCATE);

        if (Globals::localPlayer.Addr) {
            auto name = Globals::localPlayer.GetName();
            auto disp = Globals::localPlayer.GetDisplayName();
            strncpy_s(variables::Status::username, name.empty() ? "—" : name.c_str(), _TRUNCATE);
            strncpy_s(variables::Status::displayName, disp.empty() ? name.c_str() : disp.c_str(), _TRUNCATE);
            int64_t uid = memory->read<int64_t>(Globals::localPlayer.Addr + Offsets::Player::UserId);
            sprintf_s(variables::Status::userId, "%lld", (long long)uid);
        }

        if (Globals::dataModel.Addr) {
            int64_t place = memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId);
            int64_t game = memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::GameId);
            sprintf_s(variables::Status::placeId, "%lld", (long long)place);
            sprintf_s(variables::Status::gameId, "%lld", (long long)game);
            std::string job = memory->read_string(Globals::dataModel.Addr + Offsets::DataModel::JobId);
            if (!job.empty() && job != "Unknown") {
                strncpy_s(variables::Status::jobId, job.c_str(), _TRUNCATE);
                strncpy_s(variables::Servers::currentId, job.c_str(), _TRUNCATE);
            } else {
                strncpy_s(variables::Status::jobId, "—", _TRUNCATE);
                variables::Servers::currentId[0] = 0;
            }
        }

        auto snap = PlayerCache::snapshotPlayers();
        sprintf_s(variables::Status::playersOnline, "%d", (int)snap.size());
    }

    inline void CopyField(const char* label, const char* value) {
        ImGui::PushID(label);
        const float copyW = 48.f;
        float rowW = ImGui::GetContentRegionAvail().x;
        float labelW = 88.f;
        ImGui::TextColored(FrontierUI::V4(variables::Theme::text), "%s", label);
        ImGui::SameLine(labelW);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (rowW - labelW - copyW - 8.f));
        ImGui::TextUnformatted(value && value[0] ? value : "—");
        ImGui::PopTextWrapPos();
        ImGui::SameLine(rowW > copyW ? rowW - copyW : 0.f);
        if (ImGui::SmallButton("Copy"))
            ImGui::SetClipboardText(value ? value : "");
        ImGui::PopID();
    }

    inline void StatusMetric(const char* label, const char* value, ImU32 accent = 0) {
        ImGui::PushID(label);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        if (w < 80.f) w = 80.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 52.f), IM_COL32(14, 14, 16, 255), 6.f);
        dl->AddRect(p, ImVec2(p.x + w, p.y + 52.f), IM_COL32(255, 255, 255, 18), 6.f);
        if (accent)
            dl->AddRectFilled(p, ImVec2(p.x + 3.f, p.y + 52.f), accent, 6.f, ImDrawFlags_RoundCornersLeft);
        dl->AddText(ImVec2(p.x + 10.f, p.y + 8.f), IM_COL32(255, 255, 255, 90), label);
        dl->AddText(ImVec2(p.x + 10.f, p.y + 26.f), IM_COL32(255, 255, 255, 235), value && value[0] ? value : "—");
        ImGui::Dummy(ImVec2(w, 58.f));
        ImGui::PopID();
    }

    inline void StatusFieldRow(const char* label, const char* value, const char* copyId = nullptr) {
        ImGui::PushID(label);
        float rowW = ImGui::GetContentRegionAvail().x;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, ImVec2(p.x + rowW, p.y + 28.f), IM_COL32(12, 12, 14, 255), 4.f);
        dl->AddText(ImVec2(p.x + 8.f, p.y + 7.f), IM_COL32(255, 255, 255, 100), label);
        const char* show = (value && value[0]) ? value : "—";
        ImVec2 vs = ImGui::CalcTextSize(show);
        float valX = p.x + 92.f;
        float maxValW = rowW - 104.f - (copyId ? 52.f : 0.f);
        if (maxValW < 40.f) maxValW = 40.f;
        if (vs.x > maxValW) {
            std::string trimmed = show;
            while (!trimmed.empty() && ImGui::CalcTextSize((trimmed + "..").c_str()).x > maxValW)
                trimmed.pop_back();
            trimmed += "..";
            dl->AddText(ImVec2(valX, p.y + 7.f), IM_COL32(255, 255, 255, 220), trimmed.c_str());
        } else {
            dl->AddText(ImVec2(valX, p.y + 7.f), IM_COL32(255, 255, 255, 220), show);
        }
        if (copyId && value && value[0]) {
            ImGui::SetCursorScreenPos(ImVec2(p.x + rowW - 46.f, p.y + 4.f));
            if (ImGui::SmallButton(copyId))
                ImGui::SetClipboardText(value);
        }
        ImGui::Dummy(ImVec2(0, 32.f));
        ImGui::PopID();
    }

    inline bool StatusActionButton(const char* label, ImVec4 bg, ImVec4 hover, ImVec2 size = ImVec2(-1, 30)) {
        ImGui::PushStyleColor(ImGuiCol_Button, bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(bg.x * 0.85f, bg.y * 0.85f, bg.z * 0.85f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        return pressed;
    }

    inline void PushHitboxToLocal() {
        variables::Local::hitboxEnabled = variables::Hitbox::enabled;
        variables::Local::hitboxSize = variables::Hitbox::size;
        variables::Local::visualizeHitbox = variables::Hitbox::visualize;
        variables::Local::desyncEnabled = variables::Desync::enabled;
    }

    // 0–100 menu sliders → aimbot tuning values
    inline float AimLerp(float ui01to100, float lo, float hi) {
        float t = ui01to100 * 0.01f;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        return lo + (hi - lo) * t;
    }

    inline float AimUnlerp(float value, float lo, float hi) {
        if (hi <= lo) return 0.f;
        float t = (value - lo) / (hi - lo);
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        return t * 100.f;
    }

    inline void ApplyAimSliders() {
        using namespace variables::Aimbot;
        smoothing = AimLerp(uiSmoothness, 4.f, 30.f);
        damping = AimLerp(uiStability, 0.f, 0.85f);
        deadzone = AimLerp(uiLockZone, 0.5f, 12.f);
        maxDistance = AimLerp(uiRange, 100.f, 10000.f);
        fovRadius = AimLerp(uiFov, 20.f, 500.f);
        silentFovRadius = AimLerp(uiSilentFov, 20.f, 500.f);
        holdFovScale = AimLerp(uiStickyFov, 1.0f, 1.8f);
        maxMove = AimLerp(uiAimSpeed, 4.f, 28.f);
    }

    inline void LoadAimSliders() {
        using namespace variables::Aimbot;
        uiSmoothness = AimUnlerp(smoothing, 4.f, 30.f);
        uiStability = AimUnlerp(damping, 0.f, 0.85f);
        uiLockZone = AimUnlerp(deadzone, 0.5f, 12.f);
        uiRange = AimUnlerp(maxDistance, 100.f, 10000.f);
        uiFov = AimUnlerp(fovRadius, 20.f, 500.f);
        uiSilentFov = AimUnlerp(silentFovRadius, 20.f, 500.f);
        uiStickyFov = AimUnlerp(holdFovScale, 1.0f, 1.8f);
        uiAimSpeed = AimUnlerp(maxMove, 4.f, 28.f);
    }

    inline void ApplyMagicBulletSliders() {
        variables::MagicBullet::fovRadius = AimLerp(variables::MagicBullet::uiFov, 20.f, 500.f);
    }

    inline void SyncMagicBulletToHitbox() {
        variables::Hitbox::enabled = variables::MagicBullet::enabled;
        variables::Hitbox::key = variables::MagicBullet::key;
        variables::Hitbox::aimAssist = true;
        PushHitboxToLocal();
    }

    inline void SyncMagicBulletFromHitbox() {
        variables::MagicBullet::enabled = variables::Hitbox::enabled;
        variables::MagicBullet::key = variables::Hitbox::key;
        variables::MagicBullet::hitbox = variables::Aimbot::aimTarget;
    }

    inline void DrawGunModsPanel() {
        FrontierUI::BeginTwoCol("##guns");
        if (FrontierUI::BeginCard("Gun Mods", true)) {
            ImGui::TextColored(FrontierUI::V4(variables::Theme::text),
                "Arsenal-style mods (matches TRACE script).");
            FrontierUI::Checkbox("Fast Reload", &variables::GunMods::fastReload);
            FrontierUI::Checkbox("Fast Fire", &variables::GunMods::fastFire);
            FrontierUI::Checkbox("Always Auto", &variables::GunMods::alwaysAuto);
            FrontierUI::Checkbox("No Spread", &variables::GunMods::noSpread);
            FrontierUI::Checkbox("No Recoil", &variables::GunMods::noRecoil);
            FrontierUI::Checkbox("Max Penetration", &variables::GunMods::maxPenetration);
            FrontierUI::Checkbox("Infinite Ammo", &variables::GunMods::infiniteAmmo);
            FrontierUI::Checkbox("Aggressive Re-apply", &variables::GunMods::aggressive);
        }
        FrontierUI::EndCard();

        FrontierUI::NextCol();
        if (FrontierUI::BeginCard("Tuning", true)) {
            if (variables::GunMods::fastFire)
                FrontierUI::SliderFloat("Fire Rate", &variables::GunMods::fireRate, 0.02f, 0.5f, "%.2f");
            if (variables::GunMods::fastReload)
                FrontierUI::SliderFloat("Reload Time", &variables::GunMods::reloadTime, 0.01f, 0.5f, "%.2f");
            if (GunMods::entryCount.load() == 0) {
                ImGui::PushTextWrapPos(0);
                ImGui::TextColored(ImVec4(1.f, 0.5f, 0.35f, 1.f), "No weapons found — join a match");
                ImGui::PopTextWrapPos();
            } else {
                ImGui::TextColored(FrontierUI::V4(variables::Theme::text), "Tracked values: %d", GunMods::entryCount.load());
            }
            if (ImGui::SmallButton("Rescan"))
                GunMods::Rebuild();
            ImGui::SameLine();
            if (ImGui::SmallButton("Restore"))
                GunMods::RestoreAll();
        }
        FrontierUI::EndCard();
        FrontierUI::EndTwoCol();
    }

    inline void DrawCombat() {
        SyncMagicBulletFromHitbox();
        const char* parts[] = { "Head", "Body", "L Leg", "R Leg", "L Arm", "R Arm", "Closest" };

        FrontierUI::BeginTwoCol("##c0");
        if (FrontierUI::BeginCard("Aimbot", true, &variables::Aimbot::enabled)) {
            FrontierUI::KeybindRow("Aimbot Hotkey", &variables::Aimbot::aimbotKey);
            bool targetPlayers = !variables::teamCheck;
            if (FrontierUI::OptionCheck("Target Players", &targetPlayers)) {
                variables::teamCheck = !targetPlayers;
                variables::ESP::teamCheck = variables::teamCheck;
            }
            bool wallCheck = variables::Aimbot::requireVisible;
            if (FrontierUI::OptionCheck("Wall Check", &wallCheck)) {
                variables::Aimbot::requireVisible = wallCheck;
                variables::Trigger::requireVisible = wallCheck;
            }
            FrontierUI::OptionCheck("Always On (no key)", &variables::Aimbot::alwaysOn);
            FrontierUI::SliderFloat("Field of View", &variables::Aimbot::uiFov, 0.f, 100.f, "%.0f");
            FrontierUI::SliderFloat("Smooth", &variables::Aimbot::uiSmoothness, 0.f, 100.f, "%.0f");
            FrontierUI::SliderFloat("Curve", &variables::Aimbot::uiStability, 0.f, 100.f, "%.0f");
            FrontierUI::SliderFloat("Aim Distance", &variables::Aimbot::uiRange, 0.f, 100.f, "%.0f");
            FrontierUI::Combo("Hitbox", &variables::Aimbot::aimTarget, parts, 7);
            FrontierUI::OptionCheck("FOV Circle", &variables::Aimbot::showFOV);
            FrontierUI::OptionCheck("FOV Follows Mouse", &variables::Aimbot::fovFollowMouse);
            if (FrontierUI::BeginSettings("Advanced", true)) {
                const char* fovStyles[] = { "Circle", "Rotating Dots" };
                FrontierUI::Combo("FOV Style", &variables::Aimbot::fovStyle, fovStyles, 2);
                const char* aims[] = { "Mouse Move", "Silent Aim" };
                FrontierUI::Combo("Aim Style", &variables::Aimbot::aimType, aims, 2);
                if (variables::Aimbot::aimType == 1)
                    variables::Aimbot::silentAim = true;
                else
                    variables::Aimbot::silentAim = false;
                FrontierUI::Checkbox("Sticky Aim", &variables::Aimbot::stickyAim);
                FrontierUI::Checkbox("Predict Movement", &variables::Aimbot::prediction);
                FrontierUI::Checkbox("Skip Dead", &variables::healthCheck);
                FrontierUI::EndSettings();
            }
        }
        FrontierUI::EndCard();

        if (FrontierUI::BeginCard("Triggerbot", true, &variables::Trigger::enabled)) {
            FrontierUI::OptionCheck("Use Hotkey", &variables::Trigger::useHotkey);
            FrontierUI::KeybindRow("Triggerbot Key", &variables::Trigger::key);
            FrontierUI::OptionCheck("Enable On Launch", &variables::Trigger::enableOnStart);
            FrontierUI::OptionCheck("Target Players", &variables::Trigger::targetPlayers);
            FrontierUI::OptionCheck("Target NPC", &variables::Trigger::targetNpc);
            FrontierUI::OptionCheck("Target Dead", &variables::Trigger::targetDead);
            bool trigWall = variables::Trigger::requireVisible;
            if (FrontierUI::OptionCheck("Wall Check", &trigWall))
                variables::Trigger::requireVisible = variables::Aimbot::requireVisible = trigWall;
            FrontierUI::SliderFloat("Delay", &variables::Trigger::delayMs, 0.f, 200.f, "%.0f");
        }
        FrontierUI::EndCard();

        FrontierUI::NextCol();

        if (FrontierUI::BeginCard("Silent Aim", true, &variables::Aimbot::silentAim)) {
            if (variables::Aimbot::silentAim) {
                variables::Aimbot::aimType = 1;
            } else if (variables::Aimbot::aimType == 1) {
                variables::Aimbot::aimType = 0;
            }
            FrontierUI::OptionCheck("Enable Silent Aim", &variables::Aimbot::silentAim);
            FrontierUI::KeybindRow("Silent Aim Hotkey", &variables::Aimbot::silentAimKey);
            FrontierUI::SliderFloat("Field Of View", &variables::Aimbot::uiSilentFov, 0.f, 100.f, "%.0f");
            FrontierUI::Combo("Hitbox", &variables::Aimbot::aimTarget, parts, 7);
            FrontierUI::OptionCheck("FOV Circle", &variables::Aimbot::showFOV);
        }
        FrontierUI::EndCard();

        if (FrontierUI::BeginCard("Magic Bullet", true, &variables::MagicBullet::enabled)) {
            if (variables::MagicBullet::enabled != variables::Hitbox::enabled)
                SyncMagicBulletToHitbox();
            FrontierUI::OptionCheck("Enable Magic Bullet", &variables::MagicBullet::enabled);
            if (ImGui::IsItemDeactivatedAfterEdit())
                SyncMagicBulletToHitbox();
            FrontierUI::KeybindRow("Magic Bullet Hotkey", &variables::MagicBullet::key);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                variables::Hitbox::key = variables::MagicBullet::key;
                PushHitboxToLocal();
            }
            FrontierUI::SliderFloat("Field Of View", &variables::MagicBullet::uiFov, 0.f, 100.f, "%.0f");
            if (ImGui::IsItemDeactivatedAfterEdit())
                ApplyMagicBulletSliders();
            FrontierUI::Combo("Hitbox", &variables::MagicBullet::hitbox, parts, 7);
            if (ImGui::IsItemDeactivatedAfterEdit())
                variables::Aimbot::aimTarget = variables::MagicBullet::hitbox;
            FrontierUI::OptionCheck("FOV Circle", &variables::MagicBullet::showFov);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::text), "FOV Color");
            ImGui::SameLine();
            FrontierUI::ColorSquare("mbfov", variables::MagicBullet::fovColor);
            if (FrontierUI::BeginSettings("Hitbox Size", true)) {
                FrontierUI::SliderFloat("Size", &variables::Hitbox::size, 2, 50, "%.0f");
                const char* ht[] = { "HRP Only", "Multi-Part" };
                FrontierUI::Combo("Type", &variables::Hitbox::type, ht, 2);
                FrontierUI::Checkbox("Visualize", &variables::Hitbox::visualize);
                ImGui::TextColored(FrontierUI::V4(variables::Theme::text),
                    "Expands hit registration + aim assist.");
                FrontierUI::EndSettings();
            }
        }
        FrontierUI::EndCard();

        FrontierUI::EndTwoCol();
        ApplyAimSliders();
        ApplyMagicBulletSliders();
    }

    inline void DrawVisuals() {
        static const char* subs[] = { "ESP", "HUD", "Style" };
        FrontierUI::SubTabs(subs, 3, &variables::selectedSub);

        if (variables::selectedSub == 0) {
            FrontierUI::BeginTwoCol("##v0");
            if (FrontierUI::BeginCard("ESP", true)) {
                FrontierUI::Checkbox("Enabled", &variables::ESP::enabled);
                FrontierUI::Checkbox("Boxes", &variables::ESP::boxes, variables::ESP::boxColor);
                FrontierUI::Checkbox("Names", &variables::ESP::names, variables::ESP::nameColor);
                FrontierUI::Checkbox("Health Bar", &variables::ESP::healthBar, variables::ESP::healthColor);
                FrontierUI::Checkbox("Distance", &variables::ESP::distance);
                FrontierUI::Checkbox("Skeleton", &variables::ESP::skeleton);
                FrontierUI::SliderFloat("Max Distance", &variables::ESP::maxDistance, 100, 3000, "%.0f");

                if (FrontierUI::BeginSettings("More ESP", true)) {
                    FrontierUI::Checkbox("Fill", &variables::ESP::fillBox, variables::ESP::boxFillColor);
                    FrontierUI::Checkbox("Head Dot", &variables::ESP::headDot, variables::ESP::headDotColor);
                    FrontierUI::Checkbox("Tracers", &variables::ESP::snaplines, variables::ESP::snapColor);
                    if (variables::ESP::snaplines) {
                        const char* tracerFrom[] = { "Top", "Middle", "Bottom", "Mouse" };
                        FrontierUI::Combo("Tracer From", &variables::ESP::snaplinesOrigin, tracerFrom, 4);
                    }
                    FrontierUI::Checkbox("Weapon", &variables::ESP::equippedItem);
                    FrontierUI::Checkbox("Team Check", &variables::ESP::teamCheck);
                    variables::teamCheck = variables::ESP::teamCheck;
                    FrontierUI::Checkbox("Skip Dead", &variables::ESP::deadCheck);
                    FrontierUI::Checkbox("Visible Only", &variables::ESP::visibleOnly);
                    const char* bt[] = { "2D Box", "Cube", "Corners" };
                    FrontierUI::Combo("Box Type", &variables::ESP::boxType, bt, 3);
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::EndCard();

            FrontierUI::NextCol();
            if (FrontierUI::BeginCard("Chams & OOF", true)) {
                FrontierUI::Checkbox("Chams", &variables::ESP::chamsEnabled, variables::ESP::chamsColor);
                FrontierUI::Checkbox("OOF Arrows", &variables::ESP::oofArrows, variables::ESP::oofColor);
                if (variables::ESP::oofArrows)
                    FrontierUI::SliderFloat("OOF Radius", &variables::ESP::oofRadius, 40, 300, "%.0f");
            }
            FrontierUI::EndCard();
            FrontierUI::EndTwoCol();
        }
        else if (variables::selectedSub == 1) {
            FrontierUI::BeginTwoCol("##v1");
            if (FrontierUI::BeginCard("HUD Overlays", true)) {
                FrontierUI::Checkbox("Enemy Counter", &variables::Misc::enemyCounter);
                FrontierUI::Checkbox("Target HUD", &variables::Misc::targetHud);
                FrontierUI::Checkbox("Hit Markers", &variables::Misc::hitMarker);
                FrontierUI::Checkbox("Damage Numbers", &variables::Misc::damageNumbers);
                FrontierUI::Checkbox("Spectator List", &variables::Extra::spectatorList);
            }
            FrontierUI::EndCard();
            FrontierUI::EndTwoCol();
        }
        else {
            FrontierUI::BeginTwoCol("##v2");
            if (FrontierUI::BeginCard("Crosshair", true)) {
                FrontierUI::Checkbox("Enabled", &variables::Crosshair::enabled, variables::Crosshair::color);
                if (variables::Crosshair::enabled) {
                    FrontierUI::SliderFloat("Length", &variables::Crosshair::length, 4, 60, "%.0f");
                    FrontierUI::SliderFloat("Gap", &variables::Crosshair::gap, 0, 30, "%.0f");
                    FrontierUI::SliderFloat("Thickness", &variables::Crosshair::thickness, 1, 8, "%.1f");
                }
            }
            FrontierUI::EndCard();
            FrontierUI::NextCol();
            if (FrontierUI::BeginCard("ESP Style", true)) {
                FrontierUI::SliderFloat("Box Thickness", &variables::ESP::boxThickness, 1.f, 5.f, "%.1f");
                FrontierUI::SliderFloat("Skeleton Thickness", &variables::ESP::skeletonThickness, 1.f, 4.f, "%.1f");
                const char* nt[] = { "Username", "Display Name" };
                FrontierUI::Combo("Name Type", &variables::ESP::nameType, nt, 2);
            }
            FrontierUI::EndCard();
            FrontierUI::EndTwoCol();
        }
    }

    inline void DrawWorld() {
        FrontierUI::BeginTwoCol("##w");
        if (FrontierUI::BeginCard("Lighting", true)) {
            FrontierUI::Checkbox("Fullbright", &variables::World::fullbright);
            FrontierUI::Checkbox("No Fog", &variables::World::noFog);
            FrontierUI::Checkbox("No Shadows", &variables::World::noShadows);
            FrontierUI::Checkbox("Night Mode", &variables::World::nightMode);
            if (variables::World::nightMode)
                variables::World::customClock = false;
            FrontierUI::Checkbox("Remove Atmosphere", &variables::World::removeAtmosphere);
            FrontierUI::Checkbox("Custom Brightness", &variables::World::customBrightness);
            if (variables::World::customBrightness) {
                if (FrontierUI::BeginSettings("Brightness", true)) {
                    FrontierUI::SliderFloat("Brightness", &variables::World::brightness, 0.2f, 8.f, "%.1f");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::Checkbox("Custom Clock", &variables::World::customClock);
            if (variables::World::customClock) {
                if (FrontierUI::BeginSettings("Clock", true)) {
                    FrontierUI::SliderFloat("Clock Time", &variables::World::clockTime, 0.f, 24.f, "%.1f");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::Checkbox("Custom Ambient", &variables::World::customAmbient, variables::World::ambientColor);
            if (variables::World::customAmbient) {
                if (FrontierUI::BeginSettings("Ambient", true)) {
                    FrontierUI::SliderFloat("Ambient R", &variables::World::ambientColor[0], 0.f, 1.f, "%.2f");
                    FrontierUI::SliderFloat("Ambient G", &variables::World::ambientColor[1], 0.f, 1.f, "%.2f");
                    FrontierUI::SliderFloat("Ambient B", &variables::World::ambientColor[2], 0.f, 1.f, "%.2f");
                    FrontierUI::EndSettings();
                }
            }
        }
        FrontierUI::EndCard();
        FrontierUI::NextCol();
        if (FrontierUI::BeginCard("Camera", true)) {
            FrontierUI::Checkbox("Custom FOV", &variables::World::customFov);
            if (variables::World::customFov) {
                if (FrontierUI::BeginSettings("FOV", true)) {
                    FrontierUI::SliderFloat("FOV", &variables::World::fovAmount, 40, 120, "%.0f");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::Checkbox("Force Camera FOV", &variables::World::viewmodelFov);
            if (variables::World::viewmodelFov && !variables::World::customFov) {
                if (FrontierUI::BeginSettings("Force FOV", true)) {
                    FrontierUI::SliderFloat("FOV Amt", &variables::World::viewmodelFovAmt, 40, 120, "%.0f");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::Checkbox("Unlock Zoom", &variables::World::unlockZoom);
            if (variables::World::unlockZoom) {
                if (FrontierUI::BeginSettings("Zoom", true)) {
                    FrontierUI::SliderFloat("Max Zoom", &variables::World::maxZoom, 50, 2000, "%.0f");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::Checkbox("Third Person", &variables::World::thirdPerson);
            if (variables::World::thirdPerson) {
                if (FrontierUI::BeginSettings("Third Person", true)) {
                    FrontierUI::SliderFloat("Distance", &variables::World::thirdPersonDistance, 4, 40, "%.0f");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::Checkbox("Gun Wireframe", &variables::World::gunWireframe, variables::World::gunWireColor);
            if (variables::World::gunWireframe) {
                if (FrontierUI::BeginSettings("Gun Wire", true)) {
                    const char* gw[] = { "Soft", "Hard" };
                    FrontierUI::Combo("Wire Style", &variables::World::gunWireStyle, gw, 2);
                    FrontierUI::SliderFloat("Wire Alpha", &variables::World::gunWireAlpha, 0.05f, 1.f, "%.2f");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::Checkbox("Show Velocity", &variables::World::showVelocity);
        }
        FrontierUI::EndCard();
        FrontierUI::EndTwoCol();
    }

    inline void DrawAnimPicker(const AnimCatalog::Entry* entries, int count, int* selected)
    {
        if (!entries || count <= 0 || !selected) return;
        if (*selected < 0) *selected = 0;
        if (*selected >= count) *selected = count - 1;

        const float cardW = 132.f;
        const float cardH = 56.f;
        const int cols = 2;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (int i = 0; i < count; i++) {
            if (i > 0 && (i % cols) != 0)
                ImGui::SameLine(0, 8.f);

            ImGui::PushID(i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 pMax(p.x + cardW, p.y + cardH);
            bool sel = (*selected == i);

            ImU32 bg = sel ? IM_COL32(70, 28, 28, 220) : IM_COL32(255, 255, 255, 10);
            ImU32 border = sel ? FrontierUI::U32(variables::Theme::brand, 0.95f)
                : FrontierUI::U32(variables::Theme::border, 0.55f);
            dl->AddRectFilled(p, pMax, bg, 8.f);
            dl->AddRect(p, pMax, border, 8.f, 0, sel ? 1.6f : 1.f);

            const char* url = entries[i].assetUrl;
            const char* idStart = url ? strstr(url, "id=") : nullptr;
            char idLine[32] = "—";
            if (idStart)
                strncpy_s(idLine, idStart + 3, _TRUNCATE);

            dl->AddText(ImVec2(p.x + 10.f, p.y + 8.f), IM_COL32(235, 235, 240, 255), entries[i].name);
            dl->AddText(ImVec2(p.x + 10.f, p.y + 28.f), IM_COL32(150, 154, 162, 255), idLine);

            ImGui::SetCursorScreenPos(p);
            if (ImGui::InvisibleButton("##animcard", ImVec2(cardW, cardH)))
                *selected = i;
            ImGui::PopID();
        }
        ImGui::Dummy(ImVec2(0, 4));
    }

    inline void DrawCharacter() {
        static const char* subs[] = { "Move", "Extras", "Anim", "Guns" };
        FrontierUI::SubTabs(subs, 4, &variables::selectedSub);

        if (variables::selectedSub == 0) {
            FrontierUI::BeginTwoCol("##ch");
            if (FrontierUI::BeginCard("Movement", true)) {
                FrontierUI::Checkbox("Speed", &variables::Local::speedEnabled, nullptr, &variables::Local::speedKey);
                if (variables::Local::speedEnabled) {
                    if (FrontierUI::BeginSettings("Speed", true)) {
                        FrontierUI::SliderFloat("Speed Amt", &variables::Local::walkSpeed, 1, 200, "%.0f");
                        const char* sm[] = { "Velocity", "Position", "Slippery" };
                        FrontierUI::Combo("Speed Method", &variables::Local::speedMethod, sm, 3);
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Fly", &variables::Local::flyEnabled, nullptr, &variables::Local::flyKey);
                if (variables::Local::flyEnabled) {
                    if (FrontierUI::BeginSettings("Fly", true)) {
                        FrontierUI::SliderFloat("Fly Amt", &variables::Local::flySpeed, 1, 200, "%.0f");
                        const char* fm[] = { "Velocity", "Position" };
                        FrontierUI::Combo("Fly Method", &variables::Local::flyMethod, fm, 2);
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Jump Power", &variables::Local::jumpEnabled);
                if (variables::Local::jumpEnabled) {
                    if (FrontierUI::BeginSettings("Jump", true)) {
                        FrontierUI::SliderFloat("Jump Amt", &variables::Local::jumpPower, 1, 200, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Inf Jump", &variables::Local::infJump);
                FrontierUI::Checkbox("Bhop", &variables::Local::bhopEnabled);
                if (variables::Local::bhopEnabled) {
                    if (FrontierUI::BeginSettings("Bhop", true)) {
                        FrontierUI::SliderFloat("Bhop Speed", &variables::Local::bhopSpeed, 10, 120, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("No-Clip", &variables::Local::noclip);
                FrontierUI::Checkbox("Click TP", &variables::Local::clickTp, nullptr, &variables::Local::clickTpKey);
                FrontierUI::Checkbox("TP Walk", &variables::Local::tpWalk);
                if (variables::Local::tpWalk) {
                    if (FrontierUI::BeginSettings("TP Walk", true)) {
                        FrontierUI::SliderFloat("TP Step", &variables::Local::tpWalkStep, 0.5f, 8.f, "%.1f");
                        FrontierUI::EndSettings();
                    }
                }
            }
            FrontierUI::EndCard();

            FrontierUI::NextCol();
            if (FrontierUI::BeginCard("Combat Move", true)) {
                FrontierUI::Checkbox("Walk Fling", &variables::Local::walkFling, nullptr, &variables::Local::walkFlingKey);
                if (variables::Local::walkFling) {
                    if (FrontierUI::BeginSettings("Walk Fling", true)) {
                        FrontierUI::SliderFloat("Power", &variables::Local::walkFlingPower, 40, 400, "%.0f");
                        FrontierUI::SliderFloat("Touch Range", &variables::Local::walkFlingRange, 2, 10, "%.1f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Anti-Fling", &variables::Local::antiFling);
                FrontierUI::Checkbox("God Mode", &variables::Local::godMode);
                FrontierUI::Checkbox("Anti-Void", &variables::Local::antiVoid);
                FrontierUI::Checkbox("Auto TP", &variables::Local::autoTp, nullptr, &variables::Local::autoTpKey);
            }
            FrontierUI::EndCard();
            FrontierUI::EndTwoCol();
        }
        else if (variables::selectedSub == 1) {
            FrontierUI::BeginTwoCol("##chex");
            if (FrontierUI::BeginCard("Body", true)) {
                FrontierUI::Checkbox("Float", &variables::Local::floatEnabled);
                if (variables::Local::floatEnabled) {
                    if (FrontierUI::BeginSettings("Float", true)) {
                        FrontierUI::SliderFloat("Float Height", &variables::Local::floatHeight, 1, 40, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Freeze", &variables::Local::freeze, nullptr, &variables::Local::freezeKey);
                FrontierUI::Checkbox("Spin", &variables::Local::spin);
                if (variables::Local::spin) {
                    if (FrontierUI::BeginSettings("Spin", true)) {
                        FrontierUI::SliderFloat("Spin Speed", &variables::Local::spinSpeed, 1, 80, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Hip Height", &variables::Local::hipHeightEnabled);
                if (variables::Local::hipHeightEnabled) {
                    if (FrontierUI::BeginSettings("Hip Height", true)) {
                        FrontierUI::SliderFloat("Hip Amt", &variables::Local::hipHeight, 0.f, 20.f, "%.1f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Custom Gravity", &variables::Local::gravityEnabled);
                if (variables::Local::gravityEnabled) {
                    if (FrontierUI::BeginSettings("Gravity", true)) {
                        FrontierUI::SliderFloat("Gravity", &variables::Local::gravity, 0.f, 196.f, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
            }
            FrontierUI::EndCard();
            FrontierUI::NextCol();
            if (FrontierUI::BeginCard("Utility", true)) {
                FrontierUI::Checkbox("Orbit Player", &variables::Local::orbitPlayer);
                if (variables::Local::orbitPlayer) {
                    if (FrontierUI::BeginSettings("Orbit", true)) {
                        FrontierUI::SliderFloat("Orbit Speed", &variables::Local::orbitSpeed, 0.5f, 12.f, "%.1f");
                        FrontierUI::SliderFloat("Orbit Radius", &variables::Local::orbitRadius, 2.f, 40.f, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Auto Clicker", &variables::Local::autoClicker, nullptr, &variables::Local::autoClickerKey);
                if (variables::Local::autoClicker) {
                    if (FrontierUI::BeginSettings("Auto Clicker", true)) {
                        FrontierUI::SliderFloat("CPS", &variables::Local::autoClickerCps, 1.f, 30.f, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
                FrontierUI::Checkbox("Sit Spam", &variables::Local::sitSpam);
                FrontierUI::Checkbox("Vehicle Boost", &variables::Local::vehicleBoost);
                if (variables::Local::vehicleBoost) {
                    if (FrontierUI::BeginSettings("Vehicle Boost", true)) {
                        FrontierUI::SliderFloat("Boost Amt", &variables::Local::vehicleBoostAmt, 20.f, 200.f, "%.0f");
                        FrontierUI::EndSettings();
                    }
                }
            }
            FrontierUI::EndCard();
            FrontierUI::EndTwoCol();
        }
        else if (variables::selectedSub == 2) {
            if (FrontierUI::BeginCard("Animation Changer", true)) {
                FrontierUI::Checkbox("Enabled", &variables::Exploits::animation_changer);
                ImGui::TextColored(FrontierUI::V4(variables::Theme::text),
                    "Pick animations from the Roblox catalog — names + asset IDs shown.");
                ImGui::Dummy(ImVec2(0, 4));

                static int animCat = 0;
                const char* animTabs[] = { "Idle", "Run", "Walk", "Jump", "Fall", "Climb", "Swim" };

                FrontierUI::SubTabList(animTabs, 7, &animCat, 128.f, 300.f);
                ImGui::SameLine();
                ImGui::BeginChild("##animscroll", ImVec2(0, 300), ImGuiChildFlags_Borders);
                switch (animCat) {
                case 0:
                    DrawAnimPicker(AnimCatalog::kIdle,
                        (int)(sizeof(AnimCatalog::kIdle) / sizeof(AnimCatalog::kIdle[0])),
                        &variables::Exploits::idle_animation);
                    break;
                case 1:
                    DrawAnimPicker(AnimCatalog::kRun,
                        (int)(sizeof(AnimCatalog::kRun) / sizeof(AnimCatalog::kRun[0])),
                        &variables::Exploits::run_animation);
                    break;
                case 2:
                    DrawAnimPicker(AnimCatalog::kWalk,
                        (int)(sizeof(AnimCatalog::kWalk) / sizeof(AnimCatalog::kWalk[0])),
                        &variables::Exploits::walk_animation);
                    break;
                case 3:
                    DrawAnimPicker(AnimCatalog::kJump,
                        (int)(sizeof(AnimCatalog::kJump) / sizeof(AnimCatalog::kJump[0])),
                        &variables::Exploits::jump_animation);
                    break;
                case 4:
                    DrawAnimPicker(AnimCatalog::kFall,
                        (int)(sizeof(AnimCatalog::kFall) / sizeof(AnimCatalog::kFall[0])),
                        &variables::Exploits::fall_animation);
                    break;
                case 5:
                    DrawAnimPicker(AnimCatalog::kClimb,
                        (int)(sizeof(AnimCatalog::kClimb) / sizeof(AnimCatalog::kClimb[0])),
                        &variables::Exploits::climb_animation);
                    break;
                default:
                    DrawAnimPicker(AnimCatalog::kSwim,
                        (int)(sizeof(AnimCatalog::kSwim) / sizeof(AnimCatalog::kSwim[0])),
                        &variables::Exploits::swim_animation);
                    break;
                }
                ImGui::EndChild();
            }
            FrontierUI::EndCard();
        }
        else {
            DrawGunModsPanel();
        }
    }

    inline void DrawConfigs() {
        FrontierUI::BeginTwoCol("##cfg");
        if (FrontierUI::BeginCard("Config", true)) {
            if (ImGui::Button("Save Config", ImVec2(-1, 28))) {
                ApplyAimSliders();
                ApplyMagicBulletSliders();
                if (ConfigIO::Save()) {
                    variables::Toast::show = true; variables::Toast::warning = false; variables::Toast::timer = 3.f;
                    strcpy_s(variables::Toast::title, "Config saved");
                    strcpy_s(variables::Toast::body, "Documents\\FRONTIER\\config.ini");
                    strcpy_s(variables::Toast::footer, "ok");
                }
            }
            if (ImGui::Button("Load Config", ImVec2(-1, 28))) {
                if (ConfigIO::Load()) {
                    LoadAimSliders();
                    ApplyMagicBulletSliders();
                    SyncMagicBulletFromHitbox();
                    FrontierPresence::SyncEnabled(variables::Misc::discordRpc);
                    FrontierTheme::SyncBrandFromAccent();
                    FrontierTheme::MarkDirty();
                    variables::Toast::show = true; variables::Toast::warning = false; variables::Toast::timer = 3.f;
                    strcpy_s(variables::Toast::title, "Config loaded");
                    strcpy_s(variables::Toast::body, "Settings applied");
                    strcpy_s(variables::Toast::footer, "ok");
                } else {
                    variables::Toast::show = true; variables::Toast::warning = true; variables::Toast::timer = 3.f;
                    strcpy_s(variables::Toast::title, "Load failed");
                    strcpy_s(variables::Toast::body, "No config found or file unreadable");
                    strcpy_s(variables::Toast::footer, "Documents\\FRONTIER\\config.ini");
                }
            }
            if (ImGui::Button("Open Config Folder", ImVec2(-1, 28)))
                ConfigIO::OpenFolder();
        }
        FrontierUI::EndCard();
        FrontierUI::NextCol();
        if (FrontierUI::BeginCard("Session Info", true)) {
            RefreshStatusInfo();
            CopyField("User", variables::Status::username);
            CopyField("Place ID", variables::Status::placeId);
            CopyField("Job ID", variables::Status::jobId);
            char fpsBuf[16];
            sprintf_s(fpsBuf, "%d", variables::Perf::currentFps > 0 ? variables::Perf::currentFps : 0);
            CopyField("FPS", fpsBuf);
        }
        FrontierUI::EndCard();
        FrontierUI::EndTwoCol();
    }

    inline void DrawOptions() {
        FrontierUI::BeginTwoCol("##opt");
        if (FrontierUI::BeginCard("General", true)) {
            FrontierUI::Checkbox("Discord Rich Presence", &variables::Misc::discordRpc);
            if (ImGui::IsItemDeactivatedAfterEdit())
                FrontierPresence::SyncEnabled(variables::Misc::discordRpc);
            FrontierUI::Checkbox("Streamproof", &variables::Misc::streamProof);
            if (FrontierUI::BeginSettings("Streaming", true)) {
                FrontierUI::Checkbox("Streamer Mode", &variables::Misc::streamerMode);
                FrontierUI::Checkbox("Streamer Mode+", &variables::Misc::streamerModePlus);
                FrontierUI::EndSettings();
            }
            FrontierUI::Checkbox("Anti-AFK", &variables::Misc::antiAfk);
            FrontierUI::OptionCheck("Aimbot Always On", &variables::Aimbot::alwaysOn);
            if (ImGui::Button("Fake Ban Screen", ImVec2(-1, 0)))
                ServerBrowser::ShowFakeBanScreen();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Roblox-style disconnect prank (local overlay only)");
            FrontierUI::Checkbox("Watermark / FPS", &variables::Misc::showFps);
            FrontierUI::Checkbox("Panic Key", &variables::Misc::panicKey, nullptr, &variables::Misc::panicVk);
            FrontierUI::Checkbox("VSync", &variables::Perf::vsync);
            FrontierUI::SliderInt("FPS Cap", &variables::Perf::targetFps, 0, 240);
        }
        FrontierUI::EndCard();
        FrontierUI::NextCol();
        if (FrontierUI::BeginCard("Appearance", true)) {
            const char* presets[] = { "Frontier Red", "Violet", "Ice", "OLED" };
            if (FrontierUI::Combo("Preset", &variables::Theme::preset, presets, 4))
                FrontierTheme::ApplyPreset(variables::Theme::preset);

            FrontierUI::Checkbox("Link accent to UI", &variables::Theme::linkBrandAccent);
            if (FrontierUI::ThemeColorRow("Accent", variables::Theme::accent, true))
                FrontierTheme::SyncBrandFromAccent();
            if (!variables::Theme::linkBrandAccent)
                FrontierUI::ThemeColorRow("Brand", variables::Theme::brand, false);
            FrontierUI::ThemeColorRow("Background", variables::Theme::bg, false);
            FrontierUI::ThemeColorRow("Cards", variables::Theme::card, false);

            const char* layouts[] = { "Standard Tabs", "Sidebar Rail" };
            if (FrontierUI::Combo("Layout", &variables::Theme::layoutMode, layouts, 2))
                FrontierTheme::MarkDirty();
            const char* subStyles[] = { "Text", "Pill" };
            FrontierUI::Combo("Sub-tabs", &variables::Theme::subTabStyle, subStyles, 2);
            if (FrontierUI::SliderFloat("Menu Scale", &variables::Theme::menuScale, 0.85f, 1.15f, "%.2f"))
                FrontierTheme::MarkDirty();
            FrontierUI::SliderFloat("Float Bar Y", &variables::Theme::headerY, 8.f, 80.f, "%.0f");
        }
        FrontierUI::EndCard();

        if (FrontierUI::BeginCard("Menu", true)) {
            FrontierUI::Checkbox("Floating Icon Header", &variables::Theme::useFloatingHeader);
            {
                static bool wasFloating = variables::Theme::useFloatingHeader;
                if (wasFloating && !variables::Theme::useFloatingHeader) {
                    variables::Misc::floatingPanelOpen = false;
                    variables::menuOpen = true;
                } else if (!wasFloating && variables::Theme::useFloatingHeader) {
                    variables::menuOpen = false;
                    variables::Misc::floatingPanelOpen = true;
                }
                wasFloating = variables::Theme::useFloatingHeader;
            }
            FrontierUI::Checkbox("Menu Ambient FX", &variables::Theme::bgEffect);
            if (variables::Theme::bgEffect) {
                if (FrontierUI::BeginSettings("Ambient FX", true)) {
                    FrontierUI::Checkbox("Snow Particles", &variables::Theme::snowEffect);
                    FrontierUI::EndSettings();
                }
            }
            ImGui::TextColored(FrontierUI::V4(variables::Theme::text), "Menu key");
            FrontierUI::KeybindChip("menuk", &variables::Misc::menuVk);
        }
        FrontierUI::EndCard();
        FrontierUI::EndTwoCol();
    }

    inline void DrawStatus() {
        RefreshStatusInfo();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

        const bool attached = Globals::localPlayer.Addr != 0 && Globals::dataModel.Addr != 0;
        const bool streamOn = variables::Misc::streamProof || variables::Misc::streamerModePlus;
        char fpsBuf[16];
        sprintf_s(fpsBuf, "%d", variables::Perf::currentFps > 0 ? variables::Perf::currentFps : 0);

        if (FrontierUI::BeginCard("Overview", true)) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            float w = ImGui::GetContentRegionAvail().x;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilledMultiColor(
                p, ImVec2(p.x + w, p.y + 78.f),
                IM_COL32(251, 27, 8, 28), IM_COL32(251, 27, 8, 10),
                IM_COL32(8, 8, 10, 0), IM_COL32(8, 8, 10, 0));
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 78.f), IM_COL32(12, 12, 14, 180), 6.f);
            dl->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y), FrontierUI::AccentSoftU32(0.7f), 2.f);

            const char* disp = variables::Status::displayName[0] ? variables::Status::displayName : "Guest";
            const char* user = variables::Status::username[0] ? variables::Status::username : "—";
            dl->AddText(ImVec2(p.x + 12.f, p.y + 12.f), IM_COL32(255, 255, 255, 245), disp);
            char userLine[80];
            sprintf_s(userLine, "@%s", user);
            dl->AddText(ImVec2(p.x + 12.f, p.y + 32.f), IM_COL32(255, 255, 255, 110), userLine);

            ImU32 badgeBg = attached ? IM_COL32(34, 120, 62, 220) : IM_COL32(120, 70, 34, 220);
            ImU32 badgeTx = attached ? IM_COL32(170, 255, 190, 255) : IM_COL32(255, 210, 160, 255);
            const char* badge = attached ? "IN GAME" : "WAITING";
            ImVec2 bs = ImGui::CalcTextSize(badge);
            float bx = p.x + w - bs.x - 22.f;
            float by = p.y + 14.f;
            dl->AddRectFilled(ImVec2(bx - 8.f, by - 3.f), ImVec2(bx + bs.x + 8.f, by + bs.y + 3.f), badgeBg, 10.f);
            dl->AddText(ImVec2(bx, by), badgeTx, badge);

            const char* gameLbl = Games::IsSupported() ? Games::Name() : "Unsupported / idle";
            ImU32 gameCol = Games::IsSupported() ? IM_COL32(130, 230, 150, 230) : IM_COL32(255, 170, 120, 220);
            dl->AddText(ImVec2(p.x + 12.f, p.y + 54.f), gameCol, gameLbl);

            ImGui::Dummy(ImVec2(0, 84.f));

            if (ImGui::BeginTable("##statrow", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
                ImGui::TableNextColumn(); StatusMetric("FPS", fpsBuf, FrontierUI::AccentU32(0.85f));
                ImGui::TableNextColumn(); StatusMetric("Players", variables::Status::playersOnline);
                ImGui::TableNextColumn(); StatusMetric("Build", variables::Status::clientVersion);
                ImGui::EndTable();
            }
        }
        FrontierUI::EndCard();

        FrontierUI::BeginTwoCol("##statuscols");

        if (FrontierUI::BeginCard("Account", true)) {
            StatusFieldRow("Username", variables::Status::username, "Cp");
            StatusFieldRow("Display", variables::Status::displayName, "Cp2");
            StatusFieldRow("User ID", variables::Status::userId, "Cp3");
        }
        FrontierUI::EndCard();

        FrontierUI::NextCol();

        if (FrontierUI::BeginCard("Session", true)) {
            StatusFieldRow("Place ID", variables::Status::placeId, "Cp4");
            StatusFieldRow("Game ID", variables::Status::gameId, "Cp5");
            StatusFieldRow("Job ID", variables::Status::jobId, "Cp6");
            StatusFieldRow("Streamproof", streamOn ? "Active" : "Off");
        }
        FrontierUI::EndCard();

        FrontierUI::EndTwoCol();

        if (FrontierUI::BeginCard("Quick Actions", true)) {
            float btnW = (ImGui::GetContentRegionAvail().x - 8.f) * 0.5f;
            if (btnW < 100.f) btnW = -1.f;

            if (StatusActionButton("Refresh", ImVec4(0.14f, 0.14f, 0.16f, 1.f), ImVec4(0.20f, 0.20f, 0.24f, 1.f), ImVec2(btnW, 30.f)))
                variables::Status::lastRefresh = 0.f;
            if (btnW > 0.f) ImGui::SameLine(0, 8);
            if (StatusActionButton("Copy User ID", ImVec4(0.14f, 0.14f, 0.16f, 1.f), ImVec4(0.20f, 0.20f, 0.24f, 1.f), ImVec2(btnW, 30.f)))
                ImGui::SetClipboardText(variables::Status::userId);

            if (StatusActionButton("Open Site", ImVec4(0.16f, 0.10f, 0.10f, 1.f), ImVec4(0.24f, 0.12f, 0.12f, 1.f), ImVec2(btnW, 30.f)))
                ShellExecuteA(nullptr, "open", "https://trace-host.vercel.app/", nullptr, nullptr, SW_SHOWNORMAL);
            if (btnW > 0.f) ImGui::SameLine(0, 8);
            if (StatusActionButton("Join Discord", ImVec4(0.12f, 0.13f, 0.20f, 1.f), ImVec4(0.16f, 0.18f, 0.28f, 1.f), ImVec2(btnW, 30.f)))
                ShellExecuteA(nullptr, "open", "https://discord.gg/zHGKqd92Pz", nullptr, nullptr, SW_SHOWNORMAL);

            ImGui::Dummy(ImVec2(0, 4));
            if (StatusActionButton("Exit FRONTIER", ImVec4(0.55f, 0.12f, 0.12f, 1.f), ImVec4(0.70f, 0.16f, 0.16f, 1.f), ImVec2(-1, 34)))
                Globals::running = false;
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Closes FRONTIER cleanly.");
        }
        FrontierUI::EndCard();

        ImGui::PopStyleVar();
    }

    inline void DrawServers() {
        RefreshStatusInfo();
        ServerBrowser::EnsureLoaded();
        ServerBrowser::TickAutoRefresh();
        bool busy = ServerBrowser::gLoading.load();
        auto list = ServerBrowser::Snapshot();

        const char* filter = variables::Servers::searchFilter;
        auto matchesFilter = [&](const ServerBrowser::Entry& s) -> bool {
            if (!filter[0]) return true;
            char lowerId[96]{};
            strncpy_s(lowerId, s.id, _TRUNCATE);
            _strlwr_s(lowerId);
            char lowerFilter[64]{};
            strncpy_s(lowerFilter, filter, _TRUNCATE);
            _strlwr_s(lowerFilter);
            if (strstr(lowerId, lowerFilter)) return true;
            char buf[32];
            sprintf_s(buf, "%d", s.playing);
            if (strstr(buf, filter)) return true;
            return false;
        };

        if (FrontierUI::BeginCard("Browse", true)) {
            const char* sortItems[] = { "Most players", "Least players" };
            const char* autoItems[] = { "Off", "5s", "15s" };

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.f);
            ImGui::InputTextWithHint("##srvsearch", "Search job id or player count...", variables::Servers::searchFilter,
                sizeof(variables::Servers::searchFilter));

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Sort");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(132.f);
            ImGui::Combo("##sort", &variables::Servers::sortMode, sortItems, 2);

            ImGui::SameLine(0, 14);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Auto");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(68.f);
            ImGui::Combo("##auto", &variables::Servers::autoRefresh, autoItems, 3);

            ImGui::SameLine(0, 14);
            if (busy) ImGui::BeginDisabled();
            if (StatusActionButton(busy ? "Loading..." : "Refresh",
                    ImVec4(0.14f, 0.14f, 0.16f, 1.f), ImVec4(0.22f, 0.22f, 0.26f, 1.f), ImVec2(92, 28)))
                ServerBrowser::RequestRefresh(true);
            if (busy) ImGui::EndDisabled();

            ImGui::SameLine(0, 8);
            if (StatusActionButton("Server Hop",
                    ImVec4(0.12f, 0.18f, 0.14f, 1.f), ImVec4(0.16f, 0.26f, 0.18f, 1.f), ImVec2(92, 28)))
                ServerBrowser::HopRandomServer();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Join a random public server (not your current one)");

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0, 8));
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float chipY = ImGui::GetCursorScreenPos().y;
            float chipX = ImGui::GetCursorScreenPos().x;
            auto chip = [&](const char* label, ImU32 col) {
                ImVec2 ts = ImGui::CalcTextSize(label);
                ImVec2 p0(chipX, chipY);
                ImVec2 p1(chipX + ts.x + 16.f, chipY + ts.y + 8.f);
                dl->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 10), 6.f);
                dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 22), 6.f);
                dl->AddText(ImVec2(chipX + 8.f, chipY + 4.f), col, label);
                chipX = p1.x + 8.f;
            };
            char placeChip[48];
            sprintf_s(placeChip, "Place %s", variables::Status::placeId);
            chip(placeChip, IM_COL32(210, 212, 220, 255));
            char countChip[32];
            sprintf_s(countChip, "%d servers", variables::Servers::serverCount);
            chip(countChip, IM_COL32(130, 200, 150, 255));
            if (ServerBrowser::gStatus[0]) {
                char stChip[80];
                sprintf_s(stChip, "%s", ServerBrowser::gStatus);
                chip(stChip, IM_COL32(160, 164, 176, 255));
            }
            ImGui::Dummy(ImVec2(0, 28));

            if (ServerBrowser::gError[0])
                ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "%s", ServerBrowser::gError);
        }
        FrontierUI::EndCard();

        float listH = ImGui::GetContentRegionAvail().y - 8.f;
        if (listH < 220.f) listH = 220.f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, FrontierUI::V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, FrontierUI::V4(variables::Theme::border));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
        ImGui::BeginChild("##publicservers", ImVec2(0, listH), ImGuiChildFlags_Borders);

        int shown = 0;
        for (const auto& s : list)
            if (matchesFilter(s)) shown++;

        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Public servers");
        ImGui::SameLine();
        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "(%d)", shown);
        ImGui::Dummy(ImVec2(0, 6));

        if (list.empty()) {
            const char* empty = busy ? "Fetching from Roblox..." : "No servers found — hit Refresh";
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 ts = ImGui::CalcTextSize(empty);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.38f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", empty);
        }
        else if (shown == 0) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            const char* empty = "No servers match your search";
            ImVec2 ts = ImGui::CalcTextSize(empty);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.38f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", empty);
        }
        else {
            ImGui::BeginChild("##srvscroll", ImVec2(0, 0), false);
            for (int i = 0; i < (int)list.size(); i++) {
                auto& s = list[i];
                if (!matchesFilter(s)) continue;
                ImGui::PushID(i);
                bool isCurrent = (variables::Servers::currentId[0] &&
                    strcmp(variables::Servers::currentId, s.id) == 0);

                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 rowMin = ImGui::GetCursorScreenPos();
                float rowW = ImGui::GetContentRegionAvail().x;
                float rowH = 72.f;
                ImVec2 rowMax(rowMin.x + rowW, rowMin.y + rowH);

                ImGui::SetCursorScreenPos(rowMin);
                ImGui::InvisibleButton("##srvrow", ImVec2(rowW, rowH));
                bool hovered = ImGui::IsItemHovered();

                ImU32 bg = isCurrent ? IM_COL32(36, 88, 52, 120)
                    : hovered ? IM_COL32(255, 255, 255, 16)
                    : (i % 2 == 0 ? IM_COL32(255, 255, 255, 7) : IM_COL32(0, 0, 0, 0));
                dl->AddRectFilled(rowMin, rowMax, bg, 10.f);
                if (isCurrent)
                    dl->AddRect(rowMin, rowMax, IM_COL32(80, 200, 110, 200), 10.f, 0, 1.4f);
                else if (hovered)
                    dl->AddRect(rowMin, rowMax, FrontierUI::U32(variables::Theme::border, 0.9f), 10.f, 0, 1.f);

                char line[96];
                if (s.maxPlayers > 0)
                    sprintf_s(line, "%d / %d players", s.playing, s.maxPlayers);
                else
                    sprintf_s(line, "%d players", s.playing);
                dl->AddText(ImVec2(rowMin.x + 14.f, rowMin.y + 10.f),
                    isCurrent ? IM_COL32(130, 240, 160, 255) : IM_COL32(240, 240, 245, 255), line);

                float fill = (s.maxPlayers > 0) ? (float)s.playing / (float)s.maxPlayers : 0.f;
                if (fill < 0.f) fill = 0.f;
                if (fill > 1.f) fill = 1.f;
                ImVec2 barMin(rowMin.x + 14.f, rowMin.y + 30.f);
                ImVec2 barMax(rowMin.x + rowW - 160.f, rowMin.y + 36.f);
                dl->AddRectFilled(barMin, barMax, IM_COL32(255, 255, 255, 12), 4.f);
                if (fill > 0.01f) {
                    ImU32 barCol = fill > 0.85f ? IM_COL32(240, 90, 70, 220)
                        : fill > 0.55f ? IM_COL32(230, 180, 60, 220)
                        : IM_COL32(80, 190, 110, 220);
                    dl->AddRectFilled(barMin, ImVec2(barMin.x + (barMax.x - barMin.x) * fill, barMax.y), barCol, 4.f);
                }

                ImU32 pingCol = s.ping < 80 ? IM_COL32(100, 220, 130, 255)
                    : s.ping < 150 ? IM_COL32(230, 190, 70, 255)
                    : IM_COL32(240, 100, 90, 255);
                char pingLine[32];
                sprintf_s(pingLine, "%d ms", s.ping);
                dl->AddText(ImVec2(rowMin.x + 14.f, rowMin.y + 44.f), pingCol, pingLine);

                char idShort[40];
                strncpy_s(idShort, s.id, 8);
                idShort[8] = 0;
                strcat_s(idShort, "...");
                dl->AddText(ImVec2(rowMin.x + 70.f, rowMin.y + 44.f),
                    IM_COL32(130, 134, 148, 255), idShort);
                if (isCurrent) {
                    dl->AddText(ImVec2(rowMin.x + rowW - 150.f, rowMin.y + 44.f),
                        IM_COL32(90, 200, 120, 230), "YOU ARE HERE");
                }

                float btnW = 64.f;
                float btnX = rowMax.x - btnW * 2.f - 18.f;
                float btnY = rowMin.y + (rowH - 28.f) * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
                if (isCurrent) ImGui::BeginDisabled();
                if (ImGui::Button("Join", ImVec2(btnW, 28)))
                    ServerBrowser::JoinServer(s.id);
                if (isCurrent) ImGui::EndDisabled();
                ImGui::SameLine(0, 8);
                if (ImGui::Button("Copy", ImVec2(btnW, 28)))
                    ServerBrowser::CopyJobId(s.id);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", s.id);
                ImGui::PopStyleVar();

                ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMax.y + 6.f));
                ImGui::Dummy(ImVec2(0, 0));
                ImGui::PopID();
            }
            ImGui::EndChild();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    inline bool MusicSegment(const char* label, bool active, float w) {
        ImGui::PushStyleColor(ImGuiCol_Button, active ? ImVec4(0.18f, 0.42f, 0.26f, 1.f) : ImVec4(0.11f, 0.11f, 0.13f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? ImVec4(0.22f, 0.50f, 0.30f, 1.f) : ImVec4(0.16f, 0.16f, 0.19f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.34f, 0.20f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
        bool pressed = ImGui::Button(label, ImVec2(w, 30));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        return pressed;
    }

    inline void DrawMusic() {
        CustomMusic::Tick();
        if (variables::Audio::musicSource == 0)
            SpotifyPlayer::Refresh();

        const char* nowTitle = "Nothing playing";
        const char* nowArtist = "Select a source below";
        bool nowPlaying = false;

        if (variables::Audio::musicSource == 0 && SpotifyPlayer::connected) {
            nowTitle = SpotifyPlayer::trackTitle;
            nowArtist = SpotifyPlayer::trackArtist;
            nowPlaying = SpotifyPlayer::playing;
        } else if (variables::Audio::musicSource == 1 && variables::Audio::localPath[0]) {
            nowTitle = CustomMusic::PlaylistBaseName(variables::Audio::localPath);
            nowArtist = variables::Audio::localPlaying
                ? (CustomMusic::IsLocalPaused() ? "Paused — local file" : "Playing — local file")
                : "Stopped — local file";
            nowPlaying = variables::Audio::localPlaying && !CustomMusic::IsLocalPaused();
        } else if (variables::Audio::musicSource == 2 && variables::Audio::robloxId[0]) {
            nowTitle = variables::Audio::robloxId;
            nowArtist = "Roblox SoundId";
            nowPlaying = true;
        }

        float heroH = 118.f;
        float heroW = ImGui::GetContentRegionAvail().x;
        ImVec2 heroMin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##musicHero", ImVec2(heroW, heroH));
        ImVec2 heroMax(heroMin.x + heroW, heroMin.y + heroH);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(heroMin, heroMax, IM_COL32(18, 20, 24, 255), 12.f);
        dl->AddRect(heroMin, heroMax, FrontierUI::U32(variables::Theme::border, 0.65f), 12.f);

        const char* srcLabel = variables::Audio::musicSource == 0 ? "Spotify"
            : variables::Audio::musicSource == 1 ? "Local" : "Roblox";
        dl->AddText(ImVec2(heroMin.x + 16.f, heroMin.y + 12.f), IM_COL32(130, 134, 148, 255), srcLabel);

        ImVec2 titleSz = ImGui::CalcTextSize(nowTitle);
        if (titleSz.x > heroW - 32.f) {
            ImGui::PushTextWrapPos(heroMin.x + heroW - 16.f);
            ImGui::SetCursorScreenPos(ImVec2(heroMin.x + 16.f, heroMin.y + 30.f));
            ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.97f, 1.f), "%s", nowTitle);
            ImGui::PopTextWrapPos();
        } else {
            dl->AddText(ImVec2(heroMin.x + 16.f, heroMin.y + 32.f), IM_COL32(242, 242, 248, 255), nowTitle);
        }
        dl->AddText(ImVec2(heroMin.x + 16.f, heroMin.y + 54.f), IM_COL32(150, 154, 168, 255), nowArtist);

        if (nowPlaying) {
            float bx = heroMin.x + 16.f;
            float by = heroMin.y + 78.f;
            for (int b = 0; b < 4; b++) {
                float h = 6.f + (float)((b * 37 + (int)(ImGui::GetTime() * 8.f)) % 14);
                dl->AddRectFilled(ImVec2(bx + b * 8.f, by + 14.f - h), ImVec2(bx + b * 8.f + 4.f, by + 14.f),
                    FrontierUI::U32(variables::Theme::brand, 0.9f), 2.f);
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(heroMin.x + 16.f, heroMin.y + heroH - 36.f));
        ImGui::PushItemWidth(heroW - 32.f);
        float volPct = variables::Audio::musicVolume * 100.f;
        if (FrontierUI::SliderFloat("Volume", &volPct, 0.f, 100.f, "%.0f%%"))
            variables::Audio::musicVolume = volPct * 0.01f;
        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0, 10.f));

        float segW = (ImGui::GetContentRegionAvail().x - 16.f) / 3.f;
        if (MusicSegment("Spotify", variables::Audio::musicSource == 0, segW))
            variables::Audio::musicSource = 0;
        ImGui::SameLine(0, 8);
        if (MusicSegment("Local File", variables::Audio::musicSource == 1, segW))
            variables::Audio::musicSource = 1;
        ImGui::SameLine(0, 8);
        if (MusicSegment("Roblox ID", variables::Audio::musicSource == 2, segW))
            variables::Audio::musicSource = 2;

        ImGui::Dummy(ImVec2(0, 8));

        float transportW = ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("##musictransport", ImVec2(0, 44), false);
        float btnBlock = 220.f;
        ImGui::SetCursorPosX((transportW - btnBlock) * 0.5f);
        if (variables::Audio::musicSource == 0) {
            if (ImGui::Button("|<", ImVec2(40, 32))) SpotifyPlayer::Prev();
            ImGui::SameLine(0, 6);
            if (ImGui::Button(SpotifyPlayer::playing ? "Pause" : "Play", ImVec2(72, 32)))
                SpotifyPlayer::PlayPause();
            ImGui::SameLine(0, 6);
            if (ImGui::Button(">|", ImVec2(40, 32))) SpotifyPlayer::Next();
            ImGui::SameLine(0, 6);
            if (ImGui::Button("-", ImVec2(28, 32))) SpotifyPlayer::VolDown();
            ImGui::SameLine(0, 4);
            if (ImGui::Button("+", ImVec2(28, 32))) SpotifyPlayer::VolUp();
        } else if (variables::Audio::musicSource == 1) {
            if (ImGui::Button("|<", ImVec2(40, 32))) CustomMusic::PlayPlaylistOffset(-1);
            ImGui::SameLine(0, 6);
            bool paused = variables::Audio::localPlaying && CustomMusic::IsLocalPaused();
            if (ImGui::Button(paused || !variables::Audio::localPlaying ? "Play" : "Pause", ImVec2(72, 32))) {
                if (!variables::Audio::localPath[0]) CustomMusic::BrowseLocalFile();
                else if (variables::Audio::localPlaying) CustomMusic::TogglePauseLocal();
                else CustomMusic::PlayLocalPath(variables::Audio::localPath);
            }
            ImGui::SameLine(0, 6);
            if (ImGui::Button(">|", ImVec2(40, 32))) CustomMusic::PlayPlaylistOffset(1);
            ImGui::SameLine(0, 6);
            if (ImGui::Button("Stop", ImVec2(52, 32))) CustomMusic::StopLocal();
        } else {
            if (ImGui::Button("Apply Sound", ImVec2(120, 32)))
                CustomMusic::ApplyRobloxId();
            ImGui::SameLine(0, 8);
            if (ImGui::Button("Open Catalog", ImVec2(120, 32)) && variables::Audio::robloxId[0])
                CustomMusic::OpenRobloxCatalog();
        }
        ImGui::EndChild();

        if (variables::Audio::musicSource == 0) {
            if (FrontierUI::BeginCard("Spotify", true)) {
                if (!SpotifyPlayer::connected)
                    ImGui::TextColored(ImVec4(1.f, 0.55f, 0.45f, 1.f), "Spotify desktop app not detected — open Spotify first.");
                else
                    ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim),
                        "Media keys control the Spotify window. Enable the mini widget in options below.");
                FrontierUI::Checkbox("Spotify Mini Widget", &variables::Audio::spotifyMini);
            }
            FrontierUI::EndCard();
        }
        else if (variables::Audio::musicSource == 1) {
            if (FrontierUI::BeginCard("Local Media", true)) {
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "mp3 · mp4 · wav · m4a");
                if (variables::Audio::localPath[0])
                    ImGui::TextWrapped("%s", variables::Audio::localPath);
                else
                    ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "No file selected");
                FrontierUI::Checkbox("Loop track", &variables::Audio::musicLoop);
                if (StatusActionButton("Browse...", ImVec4(0.14f, 0.14f, 0.16f, 1.f), ImVec4(0.22f, 0.22f, 0.26f, 1.f), ImVec2(-1, 32)))
                    CustomMusic::BrowseLocalFile();
            }
            FrontierUI::EndCard();
        }
        else {
            if (FrontierUI::BeginCard("Roblox Audio", true)) {
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim),
                    "Paste an asset ID or rbxassetid:// — applies to Sound instances in the game.");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##rbxid", "e.g. 1837849285", variables::Audio::robloxId, sizeof(variables::Audio::robloxId));
                FrontierUI::Checkbox("Open catalog page when applying", &variables::Audio::openRobloxCatalog);
            }
            FrontierUI::EndCard();
        }

        if (variables::Audio::playlistCount > 0 && variables::Audio::musicSource == 1) {
            if (FrontierUI::BeginCard("Playlist", true)) {
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%d track(s)", variables::Audio::playlistCount);
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::BeginChild("##playlistscroll", ImVec2(0, 140), ImGuiChildFlags_Borders);
                for (int i = 0; i < variables::Audio::playlistCount; i++) {
                    ImGui::PushID(i);
                    const char* name = CustomMusic::PlaylistBaseName(variables::Audio::playlist[i]);
                    bool active = variables::Audio::localPlaying &&
                        _stricmp(variables::Audio::localPath, variables::Audio::playlist[i]) == 0;
                    if (active)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.16f, 0.38f, 0.24f, 0.85f));
                    if (ImGui::Selectable(name, active, 0, ImVec2(-1, 26)))
                        CustomMusic::PlayLocalPath(variables::Audio::playlist[i]);
                    if (active)
                        ImGui::PopStyleColor();
                    ImGui::PopID();
                }
                ImGui::EndChild();
            }
            FrontierUI::EndCard();
        }

        if (FrontierUI::BeginCard("Game Audio", true)) {
            if (CustomMusic::status[0])
                ImGui::TextWrapped("%s", CustomMusic::status);
            FrontierUI::Checkbox("Hit Sounds", &variables::Audio::hitSounds);
            FrontierUI::Checkbox("Kill Sounds", &variables::Audio::killSounds);
        }
        FrontierUI::EndCard();
    }

    inline void RenderTab(int tab) {
        FrontierUI::g_dropDepth = 0; // prevent leftover stack from a prior frame
        if (tab < 0 || tab >= 9) tab = 0;
        int prevSub = variables::selectedSub;
        variables::selectedSub = variables::Misc::selectedSubByTab[tab];
        switch (tab) {
        case 0: DrawCombat(); break;
        case 1: DrawVisuals(); break;
        case 2: DrawWorld(); break;
        case 3: DrawCharacter(); break;
        case 4: DrawOptions(); break;
        case 5: DrawServers(); break;
        case 6: DrawMusic(); break;
        case 7: DrawStatus(); break;
        case 8: DrawConfigs(); break;
        default: break;
        }
        variables::Misc::selectedSubByTab[tab] = variables::selectedSub;
        variables::selectedSub = prevSub;
    }

    inline void RenderBody() {
        if (variables::selectedTab < 0 || variables::selectedTab >= 9)
            variables::selectedTab = 0;
        RenderTab(variables::selectedTab);
    }
}
