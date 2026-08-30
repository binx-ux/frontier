#pragma once
#include "frontier_ui.h"
#include "frontier_theme.h"
#include "../core/cache/cache.h"
#include "../core/globals/globals.h"
#include "../core/features/exploits/gun_mods.h"
#include "../core/games/arsenal.h"
#include "../core/servers/server_browser.h"
#include "../core/explorer/instance_explorer.h"
#include "../core/audio/custom_music.h"
#include "../core/config/config.h"
#include "../sdk/offsets.h"
#include "../memory/memory.h"
#include <Shellapi.h>
#include <cstdio>
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
            strncpy_s(variables::Status::jobId, job.empty() || job == "Unknown" ? "—" : job.c_str(), _TRUNCATE);
            strncpy_s(variables::Servers::currentId, variables::Status::jobId, _TRUNCATE);
        }

        auto snap = PlayerCache::snapshotPlayers();
        sprintf_s(variables::Status::playersOnline, "%d", (int)snap.size());
    }

    inline void CopyField(const char* label, const char* value) {
        ImGui::PushID(label);
        const float copyW = 48.f;
        float rowW = ImGui::GetContentRegionAvail().x;
        float labelW = 88.f;
        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", label);
        ImGui::SameLine(labelW);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (rowW - labelW - copyW - 8.f));
        ImGui::TextUnformatted(value && value[0] ? value : "—");
        ImGui::PopTextWrapPos();
        ImGui::SameLine(rowW > copyW ? rowW - copyW : 0.f);
        if (ImGui::SmallButton("Copy"))
            ImGui::SetClipboardText(value ? value : "");
        ImGui::PopID();
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
        uiStickyFov = AimUnlerp(holdFovScale, 1.0f, 1.8f);
        uiAimSpeed = AimUnlerp(maxMove, 4.f, 28.f);
    }

    inline void DrawCombat() {
        static const char* subs[] = { "Aimbot", "Hitbox", "Guns" };
        FrontierUI::SubTabs(subs, 3, &variables::selectedSub);

        if (variables::selectedSub == 0) {
            FrontierUI::BeginTwoCol("##c0");
            if (FrontierUI::BeginCard("Aimbot", true)) {
                FrontierUI::Checkbox("Enabled", &variables::Aimbot::enabled, nullptr, &variables::Aimbot::aimbotKey);
                FrontierUI::Checkbox("Show FOV", &variables::Aimbot::showFOV, variables::Aimbot::fovColor);
                FrontierUI::SliderFloat("FOV Size", &variables::Aimbot::uiFov, 0.f, 100.f, "%.0f");
                FrontierUI::SliderFloat("Smoothness", &variables::Aimbot::uiSmoothness, 0.f, 100.f, "%.0f");

                const char* aims[] = { "Mouse Move", "Silent Aim" };
                FrontierUI::Combo("Aim Style", &variables::Aimbot::aimType, aims, 2);
                variables::Aimbot::silentAim = (variables::Aimbot::aimType == 1);
                variables::Aimbot::targetMethod = variables::Aimbot::aimType;
                const char* parts[] = { "Head", "Body", "L Leg", "R Leg", "L Arm", "R Arm", "Closest" };
                FrontierUI::Combo("Aim Bone", &variables::Aimbot::aimTarget, parts, 7);

                if (FrontierUI::BeginSettings("Advanced", true)) {
                    FrontierUI::Checkbox("Always On", &variables::Aimbot::alwaysOn);
                    FrontierUI::Checkbox("Toggle Mode", &variables::Aimbot::toggleMode);
                    FrontierUI::Checkbox("Sticky Aim", &variables::Aimbot::stickyAim);
                    FrontierUI::Checkbox("Predict Movement", &variables::Aimbot::prediction);
                    variables::Aimbot::requireVisible = false;
                    ImGui::TextDisabled("Visibility check runs automatically");
                    FrontierUI::Checkbox("Team Check", &variables::teamCheck);
                    variables::ESP::teamCheck = variables::teamCheck;
                    FrontierUI::Checkbox("Skip Dead", &variables::healthCheck);
                    FrontierUI::SliderFloat("Stability", &variables::Aimbot::uiStability, 0.f, 100.f, "%.0f");
                    FrontierUI::SliderFloat("Range", &variables::Aimbot::uiRange, 0.f, 100.f, "%.0f");
                    const char* profiles[] = { "Custom", "Legit", "Smooth", "Rage" };
                    if (FrontierUI::Combo("Preset", &variables::Aimbot::smoothProfile, profiles, 4)) {
                        if (variables::Aimbot::smoothProfile == 1) {
                            variables::Aimbot::uiSmoothness = 70.f; variables::Aimbot::uiFov = 18.f;
                        } else if (variables::Aimbot::smoothProfile == 2) {
                            variables::Aimbot::uiSmoothness = 45.f; variables::Aimbot::uiFov = 30.f;
                        } else if (variables::Aimbot::smoothProfile == 3) {
                            variables::Aimbot::uiSmoothness = 8.f; variables::Aimbot::uiFov = 70.f;
                            variables::Aimbot::aimType = 1; variables::Aimbot::silentAim = true;
                        }
                    }
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::EndCard();

            FrontierUI::NextCol();
            if (FrontierUI::BeginCard("Trigger & Rage", true)) {
                FrontierUI::Checkbox("Trigger Bot", &variables::Trigger::enabled, nullptr, &variables::Trigger::key);
                if (variables::Trigger::enabled) {
                    FrontierUI::SliderFloat("Delay (ms)", &variables::Trigger::delayMs, 0, 200, "%.0f");
                    FrontierUI::Checkbox("Head Only", &variables::Trigger::headOnly);
                }
                ImGui::Spacing();
                FrontierUI::Checkbox("Rage", &variables::Rage::enabled, nullptr, &variables::Rage::key);
                if (variables::Rage::enabled) {
                    FrontierUI::Checkbox("Auto Shoot", &variables::Rage::shoot);
                    FrontierUI::Checkbox("Teleport", &variables::Rage::teleport);
                }
            }
            FrontierUI::EndCard();
            FrontierUI::EndTwoCol();
            ApplyAimSliders();
        }
        else if (variables::selectedSub == 1) {
            if (FrontierUI::BeginCard("Hitbox Extender", true)) {
                FrontierUI::Checkbox("Enabled", &variables::Hitbox::enabled, nullptr, &variables::Hitbox::key);
                if (FrontierUI::BeginSettings("Hitbox", true)) {
                    FrontierUI::Checkbox("Visualize", &variables::Hitbox::visualize);
                    FrontierUI::Checkbox("Aim Assist", &variables::Hitbox::aimAssist);
                    FrontierUI::SliderFloat("Size", &variables::Hitbox::size, 2, 50, "%.0f");
                    const char* ht[] = { "HRP Only", "Multi-Part" };
                    FrontierUI::Combo("Type", &variables::Hitbox::type, ht, 2);
                    ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Aim Assist is required to register hits.");
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::EndCard();
            PushHitboxToLocal();
        }
        else {
            FrontierUI::BeginTwoCol("##guns");
            if (FrontierUI::BeginCard("Gun Mods", true)) {
                FrontierUI::Checkbox("Fast Reload", &variables::GunMods::fastReload);
                FrontierUI::Checkbox("Fast Fire", &variables::GunMods::fastFire);
                FrontierUI::Checkbox("Rapid+", &variables::GunMods::rapidPlus);
                FrontierUI::Checkbox("Always Auto", &variables::GunMods::alwaysAuto);
                FrontierUI::Checkbox("No Spread", &variables::GunMods::noSpread);
                FrontierUI::Checkbox("No Recoil", &variables::GunMods::noRecoil);
                FrontierUI::Checkbox("No Sway", &variables::GunMods::noSway);
                FrontierUI::Checkbox("Infinite Ammo", &variables::GunMods::infiniteAmmo);
                FrontierUI::Checkbox("Auto Reload", &variables::GunMods::autoReload);
                FrontierUI::Checkbox("Instant Equip", &variables::GunMods::instantEquip);
                FrontierUI::Checkbox("Damage Boost", &variables::GunMods::damageBoost);
                FrontierUI::Checkbox("Long Range", &variables::GunMods::longRange);
                FrontierUI::Checkbox("Wallbang Hint", &variables::GunMods::wallbangHint);
                FrontierUI::Checkbox("Instant Kill Push", &variables::Extra::instantKillHint);
                FrontierUI::Checkbox("Aggressive Re-apply", &variables::GunMods::aggressive);
            }
            FrontierUI::EndCard();

            FrontierUI::NextCol();
            if (FrontierUI::BeginCard("Tuning", true)) {
                if (variables::GunMods::fastFire)
                    FrontierUI::SliderFloat("Fire Rate", &variables::GunMods::fireRate, 0.04f, 0.5f, "%.2f");
                if (variables::GunMods::fastReload)
                    FrontierUI::SliderFloat("Reload Time", &variables::GunMods::reloadTime, 0.01f, 0.5f, "%.2f");
                if (variables::GunMods::damageBoost)
                    FrontierUI::SliderFloat("Damage x", &variables::GunMods::damageMultiplier, 1.f, 20.f, "%.1f");
                if (variables::GunMods::longRange)
                    FrontierUI::SliderFloat("Range x", &variables::GunMods::rangeMultiplier, 1.f, 10.f, "%.1f");
                if (GunMods::entryCount.load() == 0) {
                    ImGui::PushTextWrapPos(0);
                    ImGui::TextColored(ImVec4(1.f, 0.5f, 0.35f, 1.f), "No weapons found — join a match");
                    ImGui::PopTextWrapPos();
                } else {
                    ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Tracked values: %d", GunMods::entryCount.load());
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
            if (FrontierUI::BeginCard("HUD Overlays", true)) {
                FrontierUI::Checkbox("Enemy Counter", &variables::Misc::enemyCounter);
                FrontierUI::Checkbox("Target HUD", &variables::Misc::targetHud);
                FrontierUI::Checkbox("Hit Markers", &variables::Misc::hitMarker);
                FrontierUI::Checkbox("Damage Numbers", &variables::Misc::damageNumbers);
                FrontierUI::Checkbox("Spectator List", &variables::Extra::spectatorList);
            }
            FrontierUI::EndCard();
        }
        else {
            if (FrontierUI::BeginCard("Crosshair", true)) {
                FrontierUI::Checkbox("Enabled", &variables::Crosshair::enabled, variables::Crosshair::color);
                if (variables::Crosshair::enabled) {
                    FrontierUI::SliderFloat("Length", &variables::Crosshair::length, 4, 60, "%.0f");
                    FrontierUI::SliderFloat("Gap", &variables::Crosshair::gap, 0, 30, "%.0f");
                    FrontierUI::SliderFloat("Thickness", &variables::Crosshair::thickness, 1, 8, "%.1f");
                }
            }
            FrontierUI::EndCard();
            if (FrontierUI::BeginCard("ESP Style", true)) {
                FrontierUI::SliderFloat("Box Thickness", &variables::ESP::boxThickness, 1.f, 5.f, "%.1f");
                FrontierUI::SliderFloat("Skeleton Thickness", &variables::ESP::skeletonThickness, 1.f, 4.f, "%.1f");
                const char* nt[] = { "Username", "Display Name" };
                FrontierUI::Combo("Name Type", &variables::ESP::nameType, nt, 2);
            }
            FrontierUI::EndCard();
        }
    }

    inline void DrawWorld() {
        FrontierUI::BeginTwoCol("##w");
        if (FrontierUI::BeginCard("Lighting", true)) {
            FrontierUI::Checkbox("Fullbright", &variables::World::fullbright);
            FrontierUI::Checkbox("No Fog", &variables::World::noFog);
            FrontierUI::Checkbox("No Shadows", &variables::World::noShadows);
            FrontierUI::Checkbox("Night Mode", &variables::World::nightMode);
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

    inline void DrawCharacter() {
        static const char* subs[] = { "Move", "Extras", "Anim" };
        FrontierUI::SubTabs(subs, 3, &variables::selectedSub);

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
        else {
            if (FrontierUI::BeginCard("Animation Changer", true)) {
                FrontierUI::Checkbox("Enabled", &variables::Exploits::animation_changer);
                if (FrontierUI::BeginSettings("Animations", true)) {
                    FrontierUI::SliderInt("Idle", &variables::Exploits::idle_animation, 0, 20);
                    FrontierUI::SliderInt("Run", &variables::Exploits::run_animation, 0, 20);
                    FrontierUI::SliderInt("Walk", &variables::Exploits::walk_animation, 0, 20);
                    FrontierUI::SliderInt("Jump", &variables::Exploits::jump_animation, 0, 20);
                    FrontierUI::SliderInt("Fall", &variables::Exploits::fall_animation, 0, 20);
                    FrontierUI::EndSettings();
                }
            }
            FrontierUI::EndCard();
        }
    }

    inline void DrawOptions() {
        FrontierUI::BeginTwoCol("##opt");
        if (FrontierUI::BeginCard("General", true)) {
            FrontierUI::Checkbox("Discord Rich Presence", &variables::Misc::discordRpc);
            FrontierUI::Checkbox("Streamproof", &variables::Misc::streamProof);
            if (FrontierUI::BeginSettings("Streaming", true)) {
                FrontierUI::Checkbox("Streamer Mode", &variables::Misc::streamerMode);
                FrontierUI::Checkbox("Streamer Mode+", &variables::Misc::streamerModePlus);
                FrontierUI::EndSettings();
            }
            FrontierUI::Checkbox("Anti-AFK", &variables::Misc::antiAfk);
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
            FrontierUI::ThemeColorRow("Accent", variables::Theme::accent, true);
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
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Menu key");
            FrontierUI::KeybindChip("menuk", &variables::Misc::menuVk);
        }
        FrontierUI::EndCard();

        if (FrontierUI::BeginCard("Config", true)) {
            if (ImGui::Button("Save Config", ImVec2(-1, 28))) {
                ApplyAimSliders();
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
        FrontierUI::EndTwoCol();
    }

    inline void DrawStatus() {
        RefreshStatusInfo();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

        if (FrontierUI::BeginCard("Player", true)) {
        CopyField("Username", variables::Status::username);
        CopyField("Display", variables::Status::displayName);
        CopyField("User ID", variables::Status::userId);
        }
        FrontierUI::EndCard();

        if (FrontierUI::BeginCard("Session", true)) {
        CopyField("Place ID", variables::Status::placeId);
        CopyField("Game ID", variables::Status::gameId);
        {
            char gameLbl[64];
            ImVec4 gameCol = ImVec4(1.f, 0.45f, 0.35f, 1.f);
            if (Games::IsSupported()) {
                gameCol = ImVec4(0.45f, 0.9f, 0.55f, 1.f);
                sprintf_s(gameLbl, "Game: %s", Games::Name());
            } else {
                sprintf_s(gameLbl, "Game: waiting");
            }
            ImGui::TextColored(gameCol, "%s", gameLbl);
        }
        }
        FrontierUI::EndCard();

        if (FrontierUI::BeginCard("Client", true)) {
        CopyField("Version", variables::Status::clientVersion);
        CopyField("Players", variables::Status::playersOnline);
        char fps[16]; sprintf_s(fps, "%d", variables::Perf::currentFps);
        CopyField("FPS", fps);
        }
        FrontierUI::EndCard();

        if (FrontierUI::BeginCard("Actions", true)) {
        float btnW = (ImGui::GetContentRegionAvail().x - 8.f) * 0.5f;
        if (btnW < 100.f) btnW = -1.f;
        if (ImGui::Button("Refresh", ImVec2(btnW, 28)))
            variables::Status::lastRefresh = 0.f;
        if (btnW > 0.f) ImGui::SameLine(0, 8);
        if (ImGui::Button("Copy User ID", ImVec2(btnW, 28)))
            ImGui::SetClipboardText(variables::Status::userId);
        if (ImGui::Button("Open Site", ImVec2(btnW, 28)))
            ShellExecuteA(nullptr, "open", "https://trace-host.vercel.app/", nullptr, nullptr, SW_SHOWNORMAL);
        if (btnW > 0.f) ImGui::SameLine(0, 8);
        if (ImGui::Button("Join Discord", ImVec2(btnW, 28)))
            ShellExecuteA(nullptr, "open", "https://discord.gg/zHGKqd92Pz", nullptr, nullptr, SW_SHOWNORMAL);

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.16f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.08f, 0.08f, 1));
        if (ImGui::Button("Eject External", ImVec2(-1, 34)))
            Globals::running = false;
        ImGui::PopStyleColor(3);
        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Closes FRONTIER cleanly.");
        }
        FrontierUI::EndCard();

        ImGui::PopStyleVar();
    }

    inline void DrawExplorer() {
        if (!InstanceExplorer::root.addr && Globals::dataModel.Addr)
            InstanceExplorer::RefreshRoot();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 5));

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        if (ImGui::Button("Refresh", ImVec2(72, 28)))
            InstanceExplorer::RefreshRoot();
        ImGui::SameLine(0, 6);
        const bool busy = InstanceExplorer::saveBusy;
        ImGui::BeginDisabled(busy);
        if (ImGui::Button("Save Game", ImVec2(90, 28)))
            InstanceExplorer::SaveFullGame();
        ImGui::SameLine(0, 6);
        if (ImGui::Button("Save Selected", ImVec2(110, 28)))
            InstanceExplorer::SaveSelected();
        ImGui::EndDisabled();
        ImGui::SameLine(0, 6);
        if (ImGui::Button("Open Folder", ImVec2(96, 28)))
            InstanceExplorer::OpenDumpFolder();
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Quick jump");
        ImGui::SameLine(0, 8);
        if (ImGui::SmallButton("Workspace")) InstanceExplorer::JumpToService("Workspace");
        ImGui::SameLine(0, 4);
        if (ImGui::SmallButton("Players")) InstanceExplorer::JumpToService("Players");
        ImGui::SameLine(0, 4);
        if (ImGui::SmallButton("ReplicatedStorage")) InstanceExplorer::JumpToService("ReplicatedStorage");
        ImGui::SameLine(0, 4);
        if (ImGui::SmallButton("Lighting")) InstanceExplorer::JumpToService("Lighting");
        ImGui::SameLine(0, 4);
        if (ImGui::SmallButton("StarterGui")) InstanceExplorer::JumpToService("StarterGui");

        ImGui::Spacing();
        if (ImGui::BeginTable("##exptools", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
            ImGui::TableSetupColumn("save", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGui::TableSetupColumn("filter", ImGuiTableColumnFlags_WidthStretch, 0.55f);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Save as");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##savefname", "Filename (no extension)", InstanceExplorer::saveFileName,
                sizeof(InstanceExplorer::saveFileName));
            ImGui::TableNextColumn();
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Filter");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##filter", "Name or class...", InstanceExplorer::filter,
                sizeof(InstanceExplorer::filter));
            ImGui::EndTable();
        }

        ImGui::SetNextItemWidth(180.f);
        ImGui::SliderInt("Depth", &InstanceExplorer::saveMaxDepth, 3, 20);
        ImGui::SameLine(0, 12);
        if (InstanceExplorer::statusMsg[0])
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", InstanceExplorer::statusMsg);

        ImGui::Spacing();
        float splitH = ImGui::GetContentRegionAvail().y - 4.f;
        if (splitH < 180.f) splitH = 180.f;

        if (ImGui::BeginTable("##exp", 2,
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings,
            ImVec2(0, splitH)))
        {
            ImGui::TableSetupColumn("Hierarchy", ImGuiTableColumnFlags_WidthStretch, 0.58f);
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch, 0.42f);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.065f, 0.075f, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 16.f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 1));
            ImGui::BeginChild("##exptree", ImVec2(0, 0), ImGuiChildFlags_Borders,
                ImGuiWindowFlags_AlwaysVerticalScrollbar);
            if (!InstanceExplorer::root.addr)
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Waiting for DataModel...");
            else if (InstanceExplorer::root.children.empty())
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "No services — click Refresh");
            else {
                for (int i = 0; i < (int)InstanceExplorer::root.children.size(); i++) {
                    auto& svc = InstanceExplorer::root.children[i];
                    InstanceExplorer::DrawTreeNode(svc, svc.name, 0, i);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();

            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.075f, 0.085f, 1.f));
            ImGui::BeginChild("##expprops", ImVec2(0, 0), ImGuiChildFlags_Borders);
            InstanceExplorer::DrawInspectorPanel(busy);
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();
    }

    inline void DrawServers() {
        ServerBrowser::TickAutoRefresh();
        bool busy = ServerBrowser::gLoading.load();
        auto list = ServerBrowser::Snapshot();

        if (FrontierUI::BeginCard("Servers", true)) {
        {
            const char* sortItems[] = { "Most players", "Least players" };
            const char* autoItems[] = { "Off", "5s", "15s" };
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Sort");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(140.f);
            ImGui::Combo("##sort", &variables::Servers::sortMode, sortItems, 2);

            ImGui::SameLine(0, 16);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Auto");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(72.f);
            ImGui::Combo("##auto", &variables::Servers::autoRefresh, autoItems, 3);

            ImGui::SameLine(0, 16);
            if (busy) ImGui::BeginDisabled();
            if (ImGui::Button(busy ? "Loading..." : "Refresh", ImVec2(96, 0)))
                ServerBrowser::RequestRefresh(true);
            if (busy) ImGui::EndDisabled();

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Place %s", variables::Status::placeId);
        ImGui::SameLine(0, 16);
        ImGui::Text("%d public", variables::Servers::serverCount);
        if (ServerBrowser::gStatus[0]) {
            ImGui::SameLine(0, 16);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", ServerBrowser::gStatus);
        }
        if (ServerBrowser::gError[0])
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "%s", ServerBrowser::gError);
        }
        FrontierUI::EndCard();

        float listH = ImGui::GetContentRegionAvail().y - 8.f;
        if (listH < 220.f) listH = 220.f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, FrontierUI::V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, FrontierUI::V4(variables::Theme::border));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::BeginChild("##publicservers", ImVec2(0, listH), ImGuiChildFlags_Borders);

        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Public servers");
        ImGui::Dummy(ImVec2(0, 2));
        {
            ImVec2 a = ImGui::GetCursorScreenPos();
            float lw = ImGui::GetContentRegionAvail().x;
            ImGui::GetWindowDrawList()->AddLine(a, ImVec2(a.x + lw, a.y), FrontierUI::U32(variables::Theme::border, 0.7f));
            ImGui::Dummy(ImVec2(0, 8));
        }

        if (list.empty()) {
            const char* empty = busy ? "Fetching from Roblox..." : "No servers found - hit Refresh";
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 ts = ImGui::CalcTextSize(empty);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.35f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", empty);
        }
        else {
            ImGui::BeginChild("##srvscroll", ImVec2(0, 0), false);
            for (int i = 0; i < (int)list.size(); i++) {
                auto& s = list[i];
                ImGui::PushID(i);
                bool isCurrent = (variables::Servers::currentId[0] &&
                    strcmp(variables::Servers::currentId, s.id) == 0);

                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 rowMin = ImGui::GetCursorScreenPos();
                float rowW = ImGui::GetContentRegionAvail().x;
                float rowH = 52.f;
                ImVec2 rowMax(rowMin.x + rowW, rowMin.y + rowH);

                if (isCurrent)
                    dl->AddRectFilled(rowMin, rowMax, IM_COL32(40, 70, 50, 90), 8.f);
                else if (i % 2 == 0)
                    dl->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 6), 8.f);

                ImGui::SetCursorScreenPos(ImVec2(rowMin.x + 10.f, rowMin.y + 8.f));
                ImGui::BeginGroup();

                char line[160];
                if (s.maxPlayers > 0)
                    sprintf_s(line, "%d / %d players", s.playing, s.maxPlayers);
                else
                    sprintf_s(line, "%d players", s.playing);

                if (isCurrent)
                    ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.55f, 1.f), "%s  ·  you", line);
                else
                    ImGui::Text("%s", line);

                ImGui::SameLine(0, 14);
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "ping %d", s.ping);

                ImGui::EndGroup();

                float btnW = 64.f;
                float btnX = rowMax.x - btnW * 2.f - 18.f;
                float btnY = rowMin.y + (rowH - 26.f) * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));
                if (ImGui::Button("Join", ImVec2(btnW, 26)))
                    ServerBrowser::JoinServer(s.id);
                ImGui::SameLine(0, 6);
                if (ImGui::Button("Copy", ImVec2(btnW, 26)))
                    ServerBrowser::CopyJobId(s.id);

                ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMax.y + 4.f));
                ImGui::Dummy(ImVec2(0, 0));
                ImGui::PopID();
            }
            ImGui::EndChild();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    inline void DrawMusic() {
        CustomMusic::Tick();

        if (FrontierUI::BeginCard("Source", true)) {
        const char* src[] = { "Spotify", "Local File", "Roblox ID" };
        FrontierUI::Combo("Play From", &variables::Audio::musicSource, src, 3);
        FrontierUI::SliderFloat("Volume", &variables::Audio::musicVolume, 0.f, 1.f, "%.2f");
        FrontierUI::Checkbox("Loop", &variables::Audio::musicLoop);
        FrontierUI::Checkbox("Spotify Mini Widget", &variables::Audio::spotifyMini);
        }
        FrontierUI::EndCard();

        if (variables::Audio::musicSource == 0) {
            if (FrontierUI::BeginCard("Spotify", true)) {
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim),
                "Uses the Spotify desktop window. Open Spotify, then use the mini player.");
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

            if (ImGui::Button("Browse...", ImVec2(-1, 32)))
                CustomMusic::BrowseLocalFile();
            if (ImGui::Button(variables::Audio::localPlaying ? "Restart" : "Play", ImVec2(-1, 30)))
                CustomMusic::PlayLocalPath(variables::Audio::localPath);
            if (ImGui::Button("Stop", ImVec2(-1, 30)))
                CustomMusic::StopLocal();
            }
            FrontierUI::EndCard();
        }
        else {
            if (FrontierUI::BeginCard("Roblox Audio", true)) {
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim),
                "Paste an asset ID or rbxassetid://... — writes SoundId on found Sounds.");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##rbxid", "e.g. 1837849285", variables::Audio::robloxId, sizeof(variables::Audio::robloxId));
            FrontierUI::Checkbox("Open catalog page", &variables::Audio::openRobloxCatalog);
            if (ImGui::Button("Apply To Game Sounds", ImVec2(-1, 32)))
                CustomMusic::ApplyRobloxId();
            }
            FrontierUI::EndCard();
        }

        if (FrontierUI::BeginCard("Status", true)) {
        ImGui::TextWrapped("%s", CustomMusic::status);
        FrontierUI::Checkbox("Hit Sounds", &variables::Audio::hitSounds);
        FrontierUI::Checkbox("Kill Sounds", &variables::Audio::killSounds);
        }
        FrontierUI::EndCard();
    }

    inline void RenderTab(int tab) {
        FrontierUI::g_dropDepth = 0; // prevent leftover stack from a prior frame
        int prevSub = variables::selectedSub;
        if (tab >= 0 && tab < 9)
            variables::selectedSub = variables::Misc::selectedSubByTab[tab];
        switch (tab) {
        case 0: DrawCombat(); break;
        case 1: DrawVisuals(); break;
        case 2: DrawWorld(); break;
        case 3: DrawCharacter(); break;
        case 4: DrawOptions(); break;
        case 5: DrawExplorer(); break;
        case 6: DrawServers(); break;
        case 7: DrawMusic(); break;
        case 8: DrawStatus(); break;
        default: break;
        }
        if (tab >= 0 && tab < 9)
            variables::Misc::selectedSubByTab[tab] = variables::selectedSub;
        variables::selectedSub = prevSub;
    }

    inline void RenderBody() {
        RenderTab(variables::selectedTab);
    }
}
