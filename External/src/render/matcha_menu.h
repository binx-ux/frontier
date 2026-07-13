#pragma once
#include "matcha_ui.h"
#include "../core/cache/cache.h"
#include "../core/globals/globals.h"
#include "../core/features/aimbot/visibility.h"
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

namespace MatchaMenu {

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
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", label);
        ImGui::SameLine(labelW);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (rowW - labelW - copyW - 8.f));
        ImGui::TextUnformatted(value && value[0] ? value : "—");
        ImGui::PopTextWrapPos();
        ImGui::SameLine(rowW > copyW ? rowW - copyW : 0.f);
        if (ImGui::SmallButton("Copy"))
            ImGui::SetClipboardText(value ? value : "");
        ImGui::PopID();
    }

    inline void SyncLegacyHitbox() {
        variables::Local::hitboxEnabled = variables::Hitbox::enabled;
        variables::Local::hitboxSize = variables::Hitbox::size;
        variables::Local::visualizeHitbox = variables::Hitbox::visualize;
        variables::Local::desyncEnabled = variables::Desync::enabled;
    }

    // Map easy 0–100 sliders → real aimbot values — call every frame while Combat Aimbot is open
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

    inline void SyncAimConfigFromUI() {
        using namespace variables::Aimbot;
        smoothing = AimLerp(uiSmoothness, 4.f, 30.f);
        damping = AimLerp(uiStability, 0.f, 0.85f);
        deadzone = AimLerp(uiLockZone, 0.5f, 12.f);
        maxDistance = AimLerp(uiRange, 100.f, 10000.f);
        fovRadius = AimLerp(uiFov, 20.f, 500.f);
        holdFovScale = AimLerp(uiStickyFov, 1.0f, 1.8f);
        maxMove = AimLerp(uiAimSpeed, 4.f, 28.f);
    }

    inline void SyncAimConfigToUI() {
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
        const bool bloxStrike = Games::IsBloxStrike();
        static const char* subsFull[] = { "Aimbot", "Hitbox", "Desync", "Guns" };
        static const char* subsBS[] = { "Aimbot" };
        if (bloxStrike) {
            variables::selectedSub = 0;
            MatchaUI::SubTabs(subsBS, 1, &variables::selectedSub);
        } else {
            MatchaUI::SubTabs(subsFull, 4, &variables::selectedSub);
        }

        if (variables::selectedSub == 0) {
            MatchaUI::BeginTwoCol("##c0");
            if (MatchaUI::BeginCard("Aimbot", true)) {
                MatchaUI::Checkbox("Enabled", &variables::Aimbot::enabled, nullptr, &variables::Aimbot::aimbotKey);
                MatchaUI::Checkbox("Always On", &variables::Aimbot::alwaysOn);
                MatchaUI::Checkbox("Toggle Mode", &variables::Aimbot::toggleMode);
                MatchaUI::Checkbox("Show FOV", &variables::Aimbot::showFOV, variables::Aimbot::fovColor);
                if (variables::Aimbot::showFOV) {
                    if (MatchaUI::BeginSettings("FOV", true)) {
                        MatchaUI::Checkbox("FOV Fill", &variables::Aimbot::fovFilled);
                        MatchaUI::Checkbox("FOV Glow", &variables::Aimbot::fovGlow);
                        MatchaUI::Checkbox("FOV Rainbow", &variables::Extra::fovRainbow);
                        MatchaUI::EndSettings();
                    }
                }
                if (bloxStrike) {
                    variables::Aimbot::requireVisible = false;
                    ImGui::TextDisabled("Wall Check: off on BloxStrike (meshes break aim)");
                } else {
                    MatchaUI::Checkbox("Wall Check", &variables::Aimbot::requireVisible);
                    if (variables::Aimbot::requireVisible && Visibility::boxCount.load() == 0) {
                        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.35f, 1.f), "Building walls…");
                        if (ImGui::SmallButton("Rebuild Walls"))
                            Visibility::ForceRebuild();
                    }
                }
                MatchaUI::Checkbox("Sticky Aim", &variables::Aimbot::stickyAim);
                MatchaUI::Checkbox("Predict Movement", &variables::Aimbot::prediction);
                MatchaUI::Checkbox("Humanize", &variables::Extra::humanizeAim);
                if (variables::Extra::humanizeAim) {
                    if (MatchaUI::BeginSettings("Humanize", true)) {
                        MatchaUI::SliderFloat("Humanize Amt", &variables::Extra::humanizeAmount, 0.05f, 1.f, "%.2f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Aim On Shot", &variables::Extra::aimOnShot);
                MatchaUI::Checkbox("Random Bone", &variables::Extra::randomBone);
                MatchaUI::Checkbox("Melee Aura", &variables::Extra::meleeAura);
                if (variables::Extra::meleeAura) {
                    if (MatchaUI::BeginSettings("Melee Aura", true)) {
                        MatchaUI::SliderFloat("Melee Range", &variables::Extra::meleeRange, 4.f, 30.f, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Team Check", &variables::teamCheck);
                if (variables::teamCheck != variables::ESP::teamCheck)
                    variables::ESP::teamCheck = variables::teamCheck;
                MatchaUI::Checkbox("Skip Dead", &variables::healthCheck);
                if (MatchaUI::BeginSettings("Targeting", true)) {
                    if (bloxStrike) {
                        variables::Aimbot::aimType = 1;
                        variables::Aimbot::silentAim = true;
                        variables::Aimbot::targetMethod = 1;
                        ImGui::TextDisabled("Aim Style: Silent Aim (required on BloxStrike)");
                    } else {
                        const char* aims[] = { "Mouse Move", "Silent Aim" };
                        MatchaUI::Combo("Aim Style", &variables::Aimbot::aimType, aims, 2);
                        variables::Aimbot::silentAim = (variables::Aimbot::aimType == 1);
                        variables::Aimbot::targetMethod = variables::Aimbot::aimType;
                    }
                    const char* parts[] = { "Head", "Body", "L Leg", "R Leg", "L Arm", "R Arm", "Closest" };
                    MatchaUI::Combo("Aim Bone", &variables::Aimbot::aimTarget, parts, 7);
                    const char* prios[] = { "Crosshair", "Lowest HP", "Closest" };
                    MatchaUI::Combo("Priority", &variables::Aimbot::targetPriority, prios, 3);
                    const char* profiles[] = { "Custom", "Legit", "Smooth", "Rage" };
                    if (MatchaUI::Combo("Preset", &variables::Aimbot::smoothProfile, profiles, 4)) {
                        if (variables::Aimbot::smoothProfile == 1) {
                            variables::Aimbot::uiSmoothness = 70.f; variables::Aimbot::uiFov = 18.f;
                        } else if (variables::Aimbot::smoothProfile == 2) {
                            variables::Aimbot::uiSmoothness = 45.f; variables::Aimbot::uiFov = 30.f;
                        } else if (variables::Aimbot::smoothProfile == 3) {
                            variables::Aimbot::uiSmoothness = 8.f; variables::Aimbot::uiFov = 70.f;
                            variables::Aimbot::aimType = 1; variables::Aimbot::silentAim = true;
                        }
                    }
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::EndCard();

            if (bloxStrike) {
                if (MatchaUI::BeginCard("Feel", true)) {
                    MatchaUI::SliderFloat("Smoothness", &variables::Aimbot::uiSmoothness, 0.f, 100.f, "%.0f");
                    MatchaUI::SliderFloat("FOV Size", &variables::Aimbot::uiFov, 0.f, 100.f, "%.0f");
                }
                MatchaUI::EndCard();
            } else {
                MatchaUI::NextCol();
                if (MatchaUI::BeginCard("Feel", true)) {
                    MatchaUI::SliderFloat("Smoothness", &variables::Aimbot::uiSmoothness, 0.f, 100.f, "%.0f");
                    MatchaUI::SliderFloat("Stability", &variables::Aimbot::uiStability, 0.f, 100.f, "%.0f");
                    MatchaUI::SliderFloat("Lock Zone", &variables::Aimbot::uiLockZone, 0.f, 100.f, "%.0f");
                    MatchaUI::SliderFloat("FOV Size", &variables::Aimbot::uiFov, 0.f, 100.f, "%.0f");
                    MatchaUI::SliderFloat("Sticky FOV", &variables::Aimbot::uiStickyFov, 0.f, 100.f, "%.0f");
                    MatchaUI::SliderFloat("Aim Speed", &variables::Aimbot::uiAimSpeed, 0.f, 100.f, "%.0f");
                    MatchaUI::SliderFloat("Range", &variables::Aimbot::uiRange, 0.f, 100.f, "%.0f");
                }
                MatchaUI::EndCard();

                if (MatchaUI::BeginCard("Triggerbot", true)) {
                    MatchaUI::Checkbox("Enabled", &variables::Trigger::enabled, nullptr, &variables::Trigger::key);
                    if (MatchaUI::BeginSettings("Triggerbot", true)) {
                        MatchaUI::SliderFloat("Delay (ms)", &variables::Trigger::delayMs, 0, 200, "%.0f");
                        MatchaUI::SliderFloat("Release (ms)", &variables::Trigger::releaseMs, 1, 80, "%.0f");
                        MatchaUI::SliderFloat("Hit Radius", &variables::Trigger::hitRadius, 4, 60, "%.0f");
                        MatchaUI::Checkbox("Wall Check", &variables::Trigger::requireVisible);
                        MatchaUI::Checkbox("Head Only", &variables::Trigger::headOnly);
                        MatchaUI::Checkbox("Burst Fire", &variables::Extra::burstTrigger);
                        if (variables::Extra::burstTrigger)
                            MatchaUI::SliderInt("Burst Count", &variables::Extra::burstCount, 2, 8);
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::EndCard();

                if (MatchaUI::BeginCard("Rage", false)) {
                    MatchaUI::Checkbox("Enabled", &variables::Rage::enabled, nullptr, &variables::Rage::key);
                    if (MatchaUI::BeginSettings("Rage", variables::Rage::enabled)) {
                        MatchaUI::Checkbox("Auto Shoot", &variables::Rage::shoot);
                        MatchaUI::Checkbox("Teleport", &variables::Rage::teleport);
                        if (variables::Rage::teleport) {
                            MatchaUI::Checkbox("Unkillable TP", &variables::Rage::unkillable);
                            MatchaUI::SliderFloat("TP Distance", &variables::Rage::tpDistance, 0.5f, 12.f, "%.1f");
                        }
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::EndCard();
            }
            MatchaUI::EndTwoCol();
            SyncAimConfigFromUI();
        }
        else if (variables::selectedSub == 1) {
            if (MatchaUI::BeginCard("Hitbox Extender", true)) {
                if (Games::Detect() == Games::Kind::MiscGunTest) {
                    ImGui::TextColored(ImVec4(1.f, 0.42f, 0.35f, 1.f), "Disabled on MiscGunTest:X (ban risk)");
                    variables::Hitbox::enabled = false;
                    variables::Local::hitboxEnabled = false;
                }
                else {
                    MatchaUI::Checkbox("Enabled", &variables::Hitbox::enabled, nullptr, &variables::Hitbox::key);
                    if (MatchaUI::BeginSettings("Hitbox", true)) {
                        MatchaUI::Checkbox("Visualize", &variables::Hitbox::visualize);
                        MatchaUI::Checkbox("Aim Assist", &variables::Hitbox::aimAssist);
                        MatchaUI::SliderFloat("Size", &variables::Hitbox::size, 2, 50, "%.0f");
                        const char* ht[] = { "HRP Only", "Multi-Part" };
                        MatchaUI::Combo("Type", &variables::Hitbox::type, ht, 2);
                        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Aim Assist is required to register hits.");
                        MatchaUI::EndSettings();
                    }
                }
            }
            MatchaUI::EndCard();
            SyncLegacyHitbox();
        }
        else if (variables::selectedSub == 2) {
            if (MatchaUI::BeginCard("Desync", false)) {
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim),
                    "Desync needs a live physics bandwidth offset — currently unavailable for this build.");
                MatchaUI::Checkbox("Enabled (unavailable)", &variables::Desync::enabled, nullptr, &variables::Desync::key);
                MatchaUI::Checkbox("Reset When Off", &variables::Desync::resetOnOff);
            }
            MatchaUI::EndCard();
            SyncLegacyHitbox();
        }
        else {
            MatchaUI::BeginTwoCol("##guns");
            if (MatchaUI::BeginCard("Gun Mods", true)) {
                MatchaUI::Checkbox("Fast Reload", &variables::GunMods::fastReload);
                MatchaUI::Checkbox("Fast Fire", &variables::GunMods::fastFire);
                MatchaUI::Checkbox("Rapid+", &variables::GunMods::rapidPlus);
                MatchaUI::Checkbox("Always Auto", &variables::GunMods::alwaysAuto);
                MatchaUI::Checkbox("No Spread", &variables::GunMods::noSpread);
                MatchaUI::Checkbox("No Recoil", &variables::GunMods::noRecoil);
                MatchaUI::Checkbox("No Sway", &variables::GunMods::noSway);
                MatchaUI::Checkbox("Infinite Ammo", &variables::GunMods::infiniteAmmo);
                MatchaUI::Checkbox("Auto Reload", &variables::GunMods::autoReload);
                MatchaUI::Checkbox("Instant Equip", &variables::GunMods::instantEquip);
                MatchaUI::Checkbox("Damage Boost", &variables::GunMods::damageBoost);
                MatchaUI::Checkbox("Long Range", &variables::GunMods::longRange);
                MatchaUI::Checkbox("Wallbang Hint", &variables::GunMods::wallbangHint);
                MatchaUI::Checkbox("Instant Kill Push", &variables::Extra::instantKillHint);
                MatchaUI::Checkbox("Aggressive Re-apply", &variables::GunMods::aggressive);
            }
            MatchaUI::EndCard();

            MatchaUI::NextCol();
            if (MatchaUI::BeginCard("Tuning", true)) {
                if (variables::GunMods::fastFire)
                    MatchaUI::SliderFloat("Fire Rate", &variables::GunMods::fireRate, 0.04f, 0.5f, "%.2f");
                if (variables::GunMods::fastReload)
                    MatchaUI::SliderFloat("Reload Time", &variables::GunMods::reloadTime, 0.01f, 0.5f, "%.2f");
                if (variables::GunMods::damageBoost)
                    MatchaUI::SliderFloat("Damage x", &variables::GunMods::damageMultiplier, 1.f, 20.f, "%.1f");
                if (variables::GunMods::longRange)
                    MatchaUI::SliderFloat("Range x", &variables::GunMods::rangeMultiplier, 1.f, 10.f, "%.1f");
                if (GunMods::entryCount.load() == 0) {
                    ImGui::PushTextWrapPos(0);
                    ImGui::TextColored(ImVec4(1.f, 0.5f, 0.35f, 1.f), "No weapons found — join a match");
                    ImGui::PopTextWrapPos();
                } else {
                    ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Tracked values: %d", GunMods::entryCount.load());
                }
                if (ImGui::SmallButton("Rescan"))
                    GunMods::Rebuild();
                ImGui::SameLine();
                if (ImGui::SmallButton("Restore"))
                    GunMods::RestoreAll();
            }
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
        }
    }

    inline void DrawVisuals() {
        static const char* subs[] = { "ESP", "Style", "Crosshair", "Offscreen" };
        MatchaUI::SubTabs(subs, 4, &variables::selectedSub);

        if (variables::selectedSub == 0) {
            MatchaUI::BeginTwoCol("##v0");
            if (MatchaUI::BeginCard("ESP", true)) {
                MatchaUI::Checkbox("Enabled", &variables::ESP::enabled);
                if (MatchaUI::BeginSettings("ESP", true)) {
                    MatchaUI::Checkbox("Boxes", &variables::ESP::boxes, variables::ESP::boxColor);
                    MatchaUI::Checkbox("Fill", &variables::ESP::fillBox, variables::ESP::boxFillColor);
                    MatchaUI::Checkbox("Box Glow", &variables::ESP::boxGlow);
                    MatchaUI::Checkbox("Names", &variables::ESP::names, variables::ESP::nameColor);
                    MatchaUI::Checkbox("Health Bar", &variables::ESP::healthBar, variables::ESP::healthColor);
                    MatchaUI::Checkbox("Health Text", &variables::ESP::healthText);
                    MatchaUI::Checkbox("Armor Bar", &variables::ESP::armorBar);
                    MatchaUI::Checkbox("Distance", &variables::ESP::distance);
                    MatchaUI::Checkbox("Skeleton", &variables::ESP::skeleton);
                    MatchaUI::Checkbox("Wireframe", &variables::ESP::wireframePlayers);
                    MatchaUI::Checkbox("Head Dot", &variables::ESP::headDot, variables::ESP::headDotColor);
                    MatchaUI::Checkbox("China Hat", &variables::ESP::chinaHat);
                    MatchaUI::Checkbox("Look Direction", &variables::ESP::lookDir);
                    MatchaUI::Checkbox("Flags", &variables::ESP::flags);
                    MatchaUI::Checkbox("Tracers", &variables::ESP::snaplines, variables::ESP::snapColor);
                    MatchaUI::Checkbox("Weapon", &variables::ESP::equippedItem);
                    MatchaUI::Checkbox("Profile Pics", &variables::ESP::profilePicture);
                    MatchaUI::Checkbox("Rainbow", &variables::ESP::rainbow);
                    MatchaUI::Checkbox("Team Colors", &variables::ESP::teamColors);
                    MatchaUI::SliderFloat("Max Distance", &variables::ESP::maxDistance, 100, 3000, "%.0f");
                    const char* bt[] = { "2D Box", "Cube", "Corners" };
                    MatchaUI::Combo("Box Type", &variables::ESP::boxType, bt, 3);
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::EndCard();

            MatchaUI::NextCol();
            if (MatchaUI::BeginCard("Chams", true)) {
                MatchaUI::Checkbox("Enabled", &variables::ESP::chamsEnabled, variables::ESP::chamsColor);
                if (MatchaUI::BeginSettings("Chams", true)) {
                    const char* cm[] = { "Soft Body", "Glow" };
                    MatchaUI::Combo("Style", &variables::ESP::chamsMode, cm, 2);
                    MatchaUI::Checkbox("Filled", &variables::ESP::chamsFilled);
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::EndCard();

            if (MatchaUI::BeginCard("Filters", true)) {
                MatchaUI::Checkbox("Team Check", &variables::ESP::teamCheck);
                if (variables::ESP::teamCheck != variables::teamCheck)
                    variables::teamCheck = variables::ESP::teamCheck;
                MatchaUI::Checkbox("Visible Only", &variables::ESP::visibleOnly);
                MatchaUI::Checkbox("Target Highlight", &variables::ESP::targetHighlight);
                MatchaUI::Checkbox("Skip Dead", &variables::ESP::deadCheck);
            }
            MatchaUI::EndCard();

            if (MatchaUI::BeginCard("Combat FX", true)) {
                MatchaUI::Checkbox("Hit Markers", &variables::Misc::hitMarker);
                MatchaUI::Checkbox("Damage Numbers", &variables::Misc::damageNumbers);
                MatchaUI::Checkbox("Enemy Counter", &variables::Misc::enemyCounter);
                MatchaUI::Checkbox("Target HUD", &variables::Misc::targetHud);
                MatchaUI::Checkbox("Spectator List", &variables::Extra::spectatorList);
            }
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
        }
        else if (variables::selectedSub == 1) {
            if (MatchaUI::BeginCard("ESP Style", true)) {
                MatchaUI::SliderFloat("Box Thickness", &variables::ESP::boxThickness, 1.f, 5.f, "%.1f");
                MatchaUI::SliderFloat("Skeleton Thickness", &variables::ESP::skeletonThickness, 1.f, 4.f, "%.1f");
                MatchaUI::Checkbox("Skeleton Outline", &variables::ESP::skeletonOutline);
                MatchaUI::Checkbox("Head Dot Glow", &variables::ESP::headDotGlow);
                const char* nt[] = { "Username", "Display Name" };
                MatchaUI::Combo("Name Type", &variables::ESP::nameType, nt, 2);
            }
            MatchaUI::EndCard();
        }
        else if (variables::selectedSub == 2) {
            if (MatchaUI::BeginCard("Crosshair", true)) {
                MatchaUI::Checkbox("Enabled", &variables::Crosshair::enabled, variables::Crosshair::color);
                if (MatchaUI::BeginSettings("Crosshair", true)) {
                    MatchaUI::SliderFloat("Length", &variables::Crosshair::length, 4, 60, "%.0f");
                    MatchaUI::SliderFloat("Gap", &variables::Crosshair::gap, 0, 30, "%.0f");
                    MatchaUI::SliderFloat("Thickness", &variables::Crosshair::thickness, 1, 8, "%.1f");
                    MatchaUI::Checkbox("Outline", &variables::Crosshair::outline);
                    MatchaUI::Checkbox("Center Dot", &variables::Crosshair::centerDot);
                    MatchaUI::Checkbox("Follow Target", &variables::Crosshair::followTarget);
                    const char* cs[] = { "Static", "Spin" };
                    MatchaUI::Combo("Style", &variables::Crosshair::style, cs, 2);
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::EndCard();
        }
        else {
            if (MatchaUI::BeginCard("Offscreen", true)) {
                MatchaUI::Checkbox("OOF Arrows", &variables::ESP::oofArrows, variables::ESP::oofColor);
                if (variables::ESP::oofArrows) {
                    if (MatchaUI::BeginSettings("OOF", true)) {
                        MatchaUI::Checkbox("OOF Profile Pics", &variables::ESP::oofShowPfp);
                        MatchaUI::SliderFloat("Radius", &variables::ESP::oofRadius, 40, 300, "%.0f");
                        MatchaUI::SliderFloat("Arrow Size", &variables::ESP::oofSize, 4, 48, "%.1f");
                        MatchaUI::SliderFloat("Max Dist", &variables::ESP::oofDistance, 50, 2000, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Radar", &variables::Radar::enabled);
                if (variables::Radar::enabled) {
                    if (MatchaUI::BeginSettings("Radar", true)) {
                        MatchaUI::SliderFloat("Radar Size", &variables::Radar::size, 100, 320, "%.0f");
                        MatchaUI::SliderFloat("Radar Range", &variables::Radar::range, 50, 1000, "%.0f");
                        MatchaUI::Checkbox("Show Names", &variables::Radar::showNames);
                        MatchaUI::Checkbox("Show Distance", &variables::Radar::showDistance);
                        MatchaUI::Checkbox("Rotate w/ Camera", &variables::Radar::rotateWithCamera);
                        const char* rt[] = { "2D", "3D" };
                        MatchaUI::Combo("Radar Type", &variables::Radar::type, rt, 2);
                        MatchaUI::EndSettings();
                    }
                }
            }
            MatchaUI::EndCard();
        }
    }

    inline void DrawWorld() {
        MatchaUI::BeginTwoCol("##w");
        if (MatchaUI::BeginCard("Lighting", true)) {
            MatchaUI::Checkbox("Fullbright", &variables::World::fullbright);
            MatchaUI::Checkbox("No Fog", &variables::World::noFog);
            MatchaUI::Checkbox("No Shadows", &variables::World::noShadows);
            MatchaUI::Checkbox("Night Mode", &variables::World::nightMode);
            MatchaUI::Checkbox("Remove Atmosphere", &variables::World::removeAtmosphere);
            MatchaUI::Checkbox("Custom Brightness", &variables::World::customBrightness);
            if (variables::World::customBrightness) {
                if (MatchaUI::BeginSettings("Brightness", true)) {
                    MatchaUI::SliderFloat("Brightness", &variables::World::brightness, 0.2f, 8.f, "%.1f");
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::Checkbox("Custom Clock", &variables::World::customClock);
            if (variables::World::customClock) {
                if (MatchaUI::BeginSettings("Clock", true)) {
                    MatchaUI::SliderFloat("Clock Time", &variables::World::clockTime, 0.f, 24.f, "%.1f");
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::Checkbox("Custom Ambient", &variables::World::customAmbient, variables::World::ambientColor);
            if (variables::World::customAmbient) {
                if (MatchaUI::BeginSettings("Ambient", true)) {
                    MatchaUI::SliderFloat("Ambient R", &variables::World::ambientColor[0], 0.f, 1.f, "%.2f");
                    MatchaUI::SliderFloat("Ambient G", &variables::World::ambientColor[1], 0.f, 1.f, "%.2f");
                    MatchaUI::SliderFloat("Ambient B", &variables::World::ambientColor[2], 0.f, 1.f, "%.2f");
                    MatchaUI::EndSettings();
                }
            }
        }
        MatchaUI::EndCard();
        MatchaUI::NextCol();
        if (MatchaUI::BeginCard("Camera", true)) {
            MatchaUI::Checkbox("Custom FOV", &variables::World::customFov);
            if (variables::World::customFov) {
                if (MatchaUI::BeginSettings("FOV", true)) {
                    MatchaUI::SliderFloat("FOV", &variables::World::fovAmount, 40, 120, "%.0f");
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::Checkbox("Force Camera FOV", &variables::World::viewmodelFov);
            if (variables::World::viewmodelFov && !variables::World::customFov) {
                if (MatchaUI::BeginSettings("Force FOV", true)) {
                    MatchaUI::SliderFloat("FOV Amt", &variables::World::viewmodelFovAmt, 40, 120, "%.0f");
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::Checkbox("Unlock Zoom", &variables::World::unlockZoom);
            if (variables::World::unlockZoom) {
                if (MatchaUI::BeginSettings("Zoom", true)) {
                    MatchaUI::SliderFloat("Max Zoom", &variables::World::maxZoom, 50, 2000, "%.0f");
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::Checkbox("Third Person", &variables::World::thirdPerson);
            if (variables::World::thirdPerson) {
                if (MatchaUI::BeginSettings("Third Person", true)) {
                    MatchaUI::SliderFloat("Distance", &variables::World::thirdPersonDistance, 4, 40, "%.0f");
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::Checkbox("Gun Wireframe", &variables::World::gunWireframe, variables::World::gunWireColor);
            if (variables::World::gunWireframe) {
                if (MatchaUI::BeginSettings("Gun Wire", true)) {
                    const char* gw[] = { "Soft", "Hard" };
                    MatchaUI::Combo("Wire Style", &variables::World::gunWireStyle, gw, 2);
                    MatchaUI::SliderFloat("Wire Alpha", &variables::World::gunWireAlpha, 0.05f, 1.f, "%.2f");
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::Checkbox("Show Velocity", &variables::World::showVelocity);
        }
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawCharacter() {
        if (Games::IsBloxStrike()) {
            if (MatchaUI::BeginCard("Movement", true)) {
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim),
                    "Movement, fly, speed, TP, and gun mods are disabled in BloxStrike.");
            }
            MatchaUI::EndCard();
            return;
        }

        static const char* subs[] = { "Move", "Extras", "Anim" };
        MatchaUI::SubTabs(subs, 3, &variables::selectedSub);

        if (variables::selectedSub == 0) {
            MatchaUI::BeginTwoCol("##ch");
            if (MatchaUI::BeginCard("Movement", true)) {
                MatchaUI::Checkbox("Speed", &variables::Local::speedEnabled, nullptr, &variables::Local::speedKey);
                if (variables::Local::speedEnabled) {
                    if (MatchaUI::BeginSettings("Speed", true)) {
                        MatchaUI::SliderFloat("Speed Amt", &variables::Local::walkSpeed, 1, 200, "%.0f");
                        const char* sm[] = { "Velocity", "Position", "Slippery" };
                        MatchaUI::Combo("Speed Method", &variables::Local::speedMethod, sm, 3);
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Fly", &variables::Local::flyEnabled, nullptr, &variables::Local::flyKey);
                if (variables::Local::flyEnabled) {
                    if (MatchaUI::BeginSettings("Fly", true)) {
                        MatchaUI::SliderFloat("Fly Amt", &variables::Local::flySpeed, 1, 200, "%.0f");
                        const char* fm[] = { "Velocity", "Position" };
                        MatchaUI::Combo("Fly Method", &variables::Local::flyMethod, fm, 2);
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Jump Power", &variables::Local::jumpEnabled);
                if (variables::Local::jumpEnabled) {
                    if (MatchaUI::BeginSettings("Jump", true)) {
                        MatchaUI::SliderFloat("Jump Amt", &variables::Local::jumpPower, 1, 200, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Inf Jump", &variables::Local::infJump);
                MatchaUI::Checkbox("Bhop", &variables::Local::bhopEnabled);
                if (variables::Local::bhopEnabled) {
                    if (MatchaUI::BeginSettings("Bhop", true)) {
                        MatchaUI::SliderFloat("Bhop Speed", &variables::Local::bhopSpeed, 10, 120, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("No-Clip", &variables::Local::noclip);
                MatchaUI::Checkbox("Click TP", &variables::Local::clickTp, nullptr, &variables::Local::clickTpKey);
                MatchaUI::Checkbox("TP Walk", &variables::Local::tpWalk);
                if (variables::Local::tpWalk) {
                    if (MatchaUI::BeginSettings("TP Walk", true)) {
                        MatchaUI::SliderFloat("TP Step", &variables::Local::tpWalkStep, 0.5f, 8.f, "%.1f");
                        MatchaUI::EndSettings();
                    }
                }
            }
            MatchaUI::EndCard();

            MatchaUI::NextCol();
            if (MatchaUI::BeginCard("Combat Move", true)) {
                MatchaUI::Checkbox("Walk Fling", &variables::Local::walkFling, nullptr, &variables::Local::walkFlingKey);
                if (variables::Local::walkFling) {
                    if (MatchaUI::BeginSettings("Walk Fling", true)) {
                        MatchaUI::SliderFloat("Power", &variables::Local::walkFlingPower, 40, 400, "%.0f");
                        MatchaUI::SliderFloat("Touch Range", &variables::Local::walkFlingRange, 2, 10, "%.1f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Anti-Fling", &variables::Local::antiFling);
                MatchaUI::Checkbox("God Mode", &variables::Local::godMode);
                MatchaUI::Checkbox("Anti-Void", &variables::Local::antiVoid);
                MatchaUI::Checkbox("Auto TP", &variables::Local::autoTp, nullptr, &variables::Local::autoTpKey);
            }
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
        }
        else if (variables::selectedSub == 1) {
            MatchaUI::BeginTwoCol("##chex");
            if (MatchaUI::BeginCard("Body", true)) {
                MatchaUI::Checkbox("Float", &variables::Local::floatEnabled);
                if (variables::Local::floatEnabled) {
                    if (MatchaUI::BeginSettings("Float", true)) {
                        MatchaUI::SliderFloat("Float Height", &variables::Local::floatHeight, 1, 40, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Freeze", &variables::Local::freeze, nullptr, &variables::Local::freezeKey);
                MatchaUI::Checkbox("Spin", &variables::Local::spin);
                if (variables::Local::spin) {
                    if (MatchaUI::BeginSettings("Spin", true)) {
                        MatchaUI::SliderFloat("Spin Speed", &variables::Local::spinSpeed, 1, 80, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Hip Height", &variables::Local::hipHeightEnabled);
                if (variables::Local::hipHeightEnabled) {
                    if (MatchaUI::BeginSettings("Hip Height", true)) {
                        MatchaUI::SliderFloat("Hip Amt", &variables::Local::hipHeight, 0.f, 20.f, "%.1f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Custom Gravity", &variables::Local::gravityEnabled);
                if (variables::Local::gravityEnabled) {
                    if (MatchaUI::BeginSettings("Gravity", true)) {
                        MatchaUI::SliderFloat("Gravity", &variables::Local::gravity, 0.f, 196.f, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
            }
            MatchaUI::EndCard();
            MatchaUI::NextCol();
            if (MatchaUI::BeginCard("Utility", true)) {
                MatchaUI::Checkbox("Orbit Player", &variables::Local::orbitPlayer);
                if (variables::Local::orbitPlayer) {
                    if (MatchaUI::BeginSettings("Orbit", true)) {
                        MatchaUI::SliderFloat("Orbit Speed", &variables::Local::orbitSpeed, 0.5f, 12.f, "%.1f");
                        MatchaUI::SliderFloat("Orbit Radius", &variables::Local::orbitRadius, 2.f, 40.f, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Auto Clicker", &variables::Local::autoClicker, nullptr, &variables::Local::autoClickerKey);
                if (variables::Local::autoClicker) {
                    if (MatchaUI::BeginSettings("Auto Clicker", true)) {
                        MatchaUI::SliderFloat("CPS", &variables::Local::autoClickerCps, 1.f, 30.f, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
                MatchaUI::Checkbox("Sit Spam", &variables::Local::sitSpam);
                MatchaUI::Checkbox("Vehicle Boost", &variables::Local::vehicleBoost);
                if (variables::Local::vehicleBoost) {
                    if (MatchaUI::BeginSettings("Vehicle Boost", true)) {
                        MatchaUI::SliderFloat("Boost Amt", &variables::Local::vehicleBoostAmt, 20.f, 200.f, "%.0f");
                        MatchaUI::EndSettings();
                    }
                }
            }
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
        }
        else {
            if (MatchaUI::BeginCard("Animation Changer", true)) {
                MatchaUI::Checkbox("Enabled", &variables::Exploits::animation_changer);
                if (MatchaUI::BeginSettings("Animations", true)) {
                    MatchaUI::SliderInt("Idle", &variables::Exploits::idle_animation, 0, 20);
                    MatchaUI::SliderInt("Run", &variables::Exploits::run_animation, 0, 20);
                    MatchaUI::SliderInt("Walk", &variables::Exploits::walk_animation, 0, 20);
                    MatchaUI::SliderInt("Jump", &variables::Exploits::jump_animation, 0, 20);
                    MatchaUI::SliderInt("Fall", &variables::Exploits::fall_animation, 0, 20);
                    MatchaUI::EndSettings();
                }
            }
            MatchaUI::EndCard();
        }
    }

    inline void DrawOptions() {
        MatchaUI::BeginTwoCol("##opt");
        if (MatchaUI::BeginCard("Options", true)) {
            MatchaUI::Checkbox("Streamproof", &variables::Misc::streamProof);
            MatchaUI::Checkbox("Streamer Mode", &variables::Misc::streamerMode);
            MatchaUI::Checkbox("Streamer Mode+", &variables::Misc::streamerModePlus);
            MatchaUI::Checkbox("Anti-AFK", &variables::Misc::antiAfk);
            MatchaUI::Checkbox("AFK Assist", &variables::Misc::afkAssist);
            MatchaUI::Checkbox("Panic Key", &variables::Misc::panicKey, nullptr, &variables::Misc::panicVk);
            MatchaUI::Checkbox("Watermark / FPS", &variables::Misc::showFps);
            MatchaUI::Checkbox("Keybind List", &variables::Misc::showKeybinds);
            MatchaUI::Checkbox("VSync", &variables::Perf::vsync);
            MatchaUI::SliderInt("FPS Cap", &variables::Perf::targetFps, 0, 240);
        }
        MatchaUI::EndCard();
        MatchaUI::NextCol();
        if (MatchaUI::BeginCard("Menu", true)) {
            MatchaUI::Checkbox("Floating Icon Header", &variables::Theme::useFloatingHeader);
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
            MatchaUI::Checkbox("Menu Ambient FX", &variables::Theme::bgEffect);
            if (variables::Theme::bgEffect) {
                if (MatchaUI::BeginSettings("Ambient FX", true)) {
                    MatchaUI::Checkbox("Snow Particles", &variables::Theme::snowEffect);
                    MatchaUI::EndSettings();
                }
            }
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Menu key");
            MatchaUI::KeybindChip("menuk", &variables::Misc::menuVk);
            MatchaUI::ColorSquare("accent", variables::Theme::accent);
            ImGui::SameLine();
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Accent");
        }
        MatchaUI::EndCard();

        if (MatchaUI::BeginCard("Config", true)) {
            if (ImGui::Button("Save Config", ImVec2(-1, 28))) {
                SyncAimConfigFromUI();
                if (ConfigIO::Save()) {
                    variables::Toast::show = true; variables::Toast::warning = false; variables::Toast::timer = 3.f;
                    strcpy_s(variables::Toast::title, "Config saved");
                    strcpy_s(variables::Toast::body, "Documents\\Match-Ware\\config.ini");
                    strcpy_s(variables::Toast::footer, "ok");
                }
            }
            if (ImGui::Button("Load Config", ImVec2(-1, 28))) {
                if (ConfigIO::Load()) {
                    SyncAimConfigToUI();
                    variables::Toast::show = true; variables::Toast::warning = false; variables::Toast::timer = 3.f;
                    strcpy_s(variables::Toast::title, "Config loaded");
                    strcpy_s(variables::Toast::body, "Settings applied");
                    strcpy_s(variables::Toast::footer, "ok");
                }
            }
            if (ImGui::Button("Open Config Folder", ImVec2(-1, 28)))
                ConfigIO::OpenFolder();
        }
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawStatus() {
        RefreshStatusInfo();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

        if (MatchaUI::BeginCard("Player", true)) {
        CopyField("Username", variables::Status::username);
        CopyField("Display", variables::Status::displayName);
        CopyField("User ID", variables::Status::userId);
        }
        MatchaUI::EndCard();

        if (MatchaUI::BeginCard("Session", true)) {
        CopyField("Place ID", variables::Status::placeId);
        CopyField("Game ID", variables::Status::gameId);
        {
            const char* gameLbl = "Game: unsupported";
            ImVec4 gameCol = ImVec4(1.f, 0.45f, 0.35f, 1.f);
            if (Arsenal::IsSupportedPlace()) {
                gameCol = ImVec4(0.45f, 0.9f, 0.55f, 1.f);
                switch (Games::Detect()) {
                case Games::Kind::MiscGunTest: gameLbl = "Game: MiscGunTest:X"; break;
                case Games::Kind::BloxStrike: gameLbl = "Game: BloxStrike"; break;
                case Games::Kind::Baseplate: gameLbl = "Game: Baseplate"; break;
                case Games::Kind::Arsenal: gameLbl = "Game: Arsenal"; break;
                default: gameLbl = "Game: supported"; break;
                }
            }
            ImGui::TextColored(gameCol, "%s", gameLbl);
        }
        if (Games::Detect() == Games::Kind::MiscGunTest) {
            ImGui::TextColored(ImVec4(1.f, 0.42f, 0.35f, 1.f), "Do not use Hitbox Extender — ban risk");
        }
        }
        MatchaUI::EndCard();

        if (MatchaUI::BeginCard("Client", true)) {
        CopyField("Version", variables::Status::clientVersion);
        CopyField("Players", variables::Status::playersOnline);
        char fps[16]; sprintf_s(fps, "%d", variables::Perf::currentFps);
        CopyField("FPS", fps);
        }
        MatchaUI::EndCard();

        if (MatchaUI::BeginCard("Actions", true)) {
        float btnW = (ImGui::GetContentRegionAvail().x - 8.f) * 0.5f;
        if (btnW < 100.f) btnW = -1.f;
        if (ImGui::Button("Refresh", ImVec2(btnW, 28)))
            variables::Status::lastRefresh = 0.f;
        if (btnW > 0.f) ImGui::SameLine(0, 8);
        if (ImGui::Button("Copy User ID", ImVec2(btnW, 28)))
            ImGui::SetClipboardText(variables::Status::userId);
        if (ImGui::Button("Open Site", ImVec2(btnW, 28)))
            ShellExecuteA(nullptr, "open", "https://match-ware.vercel.app/", nullptr, nullptr, SW_SHOWNORMAL);
        if (btnW > 0.f) ImGui::SameLine(0, 8);
        if (ImGui::Button("Join Discord", ImVec2(btnW, 28)))
            ShellExecuteA(nullptr, "open", "https://discord.gg/rUXya4U5qM", nullptr, nullptr, SW_SHOWNORMAL);

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.16f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.08f, 0.08f, 1));
        if (ImGui::Button("Eject External", ImVec2(-1, 34)))
            Globals::running = false;
        ImGui::PopStyleColor(3);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Closes Match-Ware cleanly.");
        }
        MatchaUI::EndCard();

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
        if (ImGui::BeginTable("##exptools", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
            ImGui::TableSetupColumn("save", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGui::TableSetupColumn("filter", ImGuiTableColumnFlags_WidthStretch, 0.55f);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Save as");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##savefname", "Filename (no extension)", InstanceExplorer::saveFileName,
                sizeof(InstanceExplorer::saveFileName));
            ImGui::TableNextColumn();
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Filter");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##filter", "Name or class...", InstanceExplorer::filter,
                sizeof(InstanceExplorer::filter));
            ImGui::EndTable();
        }

        ImGui::SetNextItemWidth(180.f);
        ImGui::SliderInt("Depth", &InstanceExplorer::saveMaxDepth, 3, 20);
        ImGui::SameLine(0, 12);
        if (InstanceExplorer::statusMsg[0])
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", InstanceExplorer::statusMsg);

        ImGui::Spacing();
        float splitH = ImGui::GetContentRegionAvail().y - 4.f;
        if (splitH < 180.f) splitH = 180.f;

        if (ImGui::BeginTable("##exp", 2,
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings,
            ImVec2(0, splitH)))
        {
            ImGui::TableSetupColumn("Instances", ImGuiTableColumnFlags_WidthStretch, 0.64f);
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch, 0.36f);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.065f, 0.075f, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 16.f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 1));
            ImGui::BeginChild("##exptree", ImVec2(0, 0), ImGuiChildFlags_Borders,
                ImGuiWindowFlags_AlwaysVerticalScrollbar);
            if (!InstanceExplorer::root.addr)
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Waiting for DataModel...");
            else if (InstanceExplorer::root.children.empty())
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "No services — click Refresh");
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
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Inspector");
            ImGui::Separator();
            if (!InstanceExplorer::selectedAddr) {
                ImGui::Dummy(ImVec2(0, 24));
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Select an instance");
            }
            else {
                ImGui::Spacing();
                InstanceExplorer::DrawSelectedIcon(26.f);
                ImGui::SameLine(0, 10);
                ImGui::BeginGroup();
                ImGui::TextUnformatted(InstanceExplorer::selectedName.c_str());
                ImGui::TextColored(InstanceExplorer::ClassColorV4(InstanceExplorer::selectedClass),
                    "%s", InstanceExplorer::selectedClass.c_str());
                ImGui::EndGroup();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Path");
                ImGui::PushTextWrapPos(0);
                ImGui::TextUnformatted(InstanceExplorer::selectedPath.c_str());
                ImGui::PopTextWrapPos();

                ImGui::Spacing();
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Address");
                ImGui::Text("0x%llX", (unsigned long long)InstanceExplorer::selectedAddr);

                ImGui::Spacing();
                if (ImGui::Button("Copy Path", ImVec2(-1, 28)))
                    ImGui::SetClipboardText(InstanceExplorer::selectedPath.c_str());
                if (ImGui::Button("Copy Address", ImVec2(-1, 28))) {
                    char buf[32];
                    sprintf_s(buf, "0x%llX", (unsigned long long)InstanceExplorer::selectedAddr);
                    ImGui::SetClipboardText(buf);
                }
                ImGui::BeginDisabled(busy);
                if (ImGui::Button("Save This Branch", ImVec2(-1, 28)))
                    InstanceExplorer::SaveSelected();
                ImGui::EndDisabled();
            }
            if (InstanceExplorer::lastSavePath[0]) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Last save");
                ImGui::PushTextWrapPos(0);
                ImGui::TextColored(MatchaUI::V4(variables::Theme::accent), "%s",
                    InstanceExplorer::lastSavePath);
                ImGui::PopTextWrapPos();
            }
            ImGui::Spacing();
            ImGui::TextWrapped("Hierarchy dump (names/classes). Not a .rbxl place file.");
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

        if (MatchaUI::BeginCard("Servers", true)) {
        {
            const char* sortItems[] = { "Most players", "Least players" };
            const char* autoItems[] = { "Off", "5s", "15s" };
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Sort");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(140.f);
            ImGui::Combo("##sort", &variables::Servers::sortMode, sortItems, 2);

            ImGui::SameLine(0, 16);
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Auto");
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
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Place %s", variables::Status::placeId);
        ImGui::SameLine(0, 16);
        ImGui::Text("%d public", variables::Servers::serverCount);
        if (ServerBrowser::gStatus[0]) {
            ImGui::SameLine(0, 16);
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", ServerBrowser::gStatus);
        }
        if (ServerBrowser::gError[0])
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "%s", ServerBrowser::gError);
        }
        MatchaUI::EndCard();

        float listH = ImGui::GetContentRegionAvail().y - 8.f;
        if (listH < 220.f) listH = 220.f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, MatchaUI::V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, MatchaUI::V4(variables::Theme::border));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::BeginChild("##publicservers", ImVec2(0, listH), ImGuiChildFlags_Borders);

        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Public servers");
        ImGui::Dummy(ImVec2(0, 2));
        {
            ImVec2 a = ImGui::GetCursorScreenPos();
            float lw = ImGui::GetContentRegionAvail().x;
            ImGui::GetWindowDrawList()->AddLine(a, ImVec2(a.x + lw, a.y), MatchaUI::U32(variables::Theme::border, 0.7f));
            ImGui::Dummy(ImVec2(0, 8));
        }

        if (list.empty()) {
            const char* empty = busy ? "Fetching from Roblox..." : "No servers found - hit Refresh";
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 ts = ImGui::CalcTextSize(empty);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.35f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f);
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", empty);
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
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "ping %d", s.ping);

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

        if (MatchaUI::BeginCard("Source", true)) {
        const char* src[] = { "Spotify", "Local File", "Roblox ID" };
        MatchaUI::Combo("Play From", &variables::Audio::musicSource, src, 3);
        MatchaUI::SliderFloat("Volume", &variables::Audio::musicVolume, 0.f, 1.f, "%.2f");
        MatchaUI::Checkbox("Loop", &variables::Audio::musicLoop);
        MatchaUI::Checkbox("Spotify Mini Widget", &variables::Audio::spotifyMini);
        }
        MatchaUI::EndCard();

        if (variables::Audio::musicSource == 0) {
            if (MatchaUI::BeginCard("Spotify", true)) {
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim),
                "Uses the Spotify desktop window. Open Spotify, then use the mini player.");
            }
            MatchaUI::EndCard();
        }
        else if (variables::Audio::musicSource == 1) {
            if (MatchaUI::BeginCard("Local Media", true)) {
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "mp3 · mp4 · wav · m4a");
            if (variables::Audio::localPath[0])
                ImGui::TextWrapped("%s", variables::Audio::localPath);
            else
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "No file selected");

            if (ImGui::Button("Browse...", ImVec2(-1, 32)))
                CustomMusic::BrowseLocalFile();
            if (ImGui::Button(variables::Audio::localPlaying ? "Restart" : "Play", ImVec2(-1, 30)))
                CustomMusic::PlayLocalPath(variables::Audio::localPath);
            if (ImGui::Button("Stop", ImVec2(-1, 30)))
                CustomMusic::StopLocal();
            }
            MatchaUI::EndCard();
        }
        else {
            if (MatchaUI::BeginCard("Roblox Audio", true)) {
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim),
                "Paste an asset ID or rbxassetid://... — writes SoundId on found Sounds.");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##rbxid", "e.g. 1837849285", variables::Audio::robloxId, sizeof(variables::Audio::robloxId));
            MatchaUI::Checkbox("Open catalog page", &variables::Audio::openRobloxCatalog);
            if (ImGui::Button("Apply To Game Sounds", ImVec2(-1, 32)))
                CustomMusic::ApplyRobloxId();
            }
            MatchaUI::EndCard();
        }

        if (MatchaUI::BeginCard("Status", true)) {
        ImGui::TextWrapped("%s", CustomMusic::status);
        MatchaUI::Checkbox("Hit Sounds", &variables::Audio::hitSounds);
        MatchaUI::Checkbox("Kill Sounds", &variables::Audio::killSounds);
        }
        MatchaUI::EndCard();
    }

    inline void RenderTab(int tab) {
        MatchaUI::g_dropDepth = 0; // prevent leftover stack from a prior frame
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
