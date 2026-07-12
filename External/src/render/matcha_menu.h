#pragma once
#include "matcha_ui.h"
#include "../core/cache/cache.h"
#include "../core/globals/globals.h"
#include "../core/features/aimbot/visibility.h"
#include "../core/features/exploits/gun_mods.h"
#include "../core/games/arsenal.h"
#include "../core/features/mtc/mtc.h"
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
        ImGui::Text("%s", label);
        ImGui::SameLine(140);
        ImGui::TextWrapped("%s", value);
        ImGui::SameLine();
        ImGui::PushID(label);
        if (ImGui::SmallButton("Copy"))
            ImGui::SetClipboardText(value);
        ImGui::PopID();
    }

    inline void SyncLegacyHitbox() {
        variables::Local::hitboxEnabled = variables::Hitbox::enabled;
        variables::Local::hitboxSize = variables::Hitbox::size;
        variables::Local::visualizeHitbox = variables::Hitbox::visualize;
        variables::Local::desyncEnabled = variables::Desync::enabled;
    }

    // Map easy 0–100 sliders → real aimbot values
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
        static const char* subs[] = { "Aimbot", "Hitbox", "Desync", "Guns" };
        MatchaUI::SubTabs(subs, 4, &variables::selectedSub);

        if (variables::selectedSub == 0) {
            MatchaUI::BeginTwoCol("##c0");
            MatchaUI::BeginCard("Targeting");
            MatchaUI::Checkbox("Enabled", &variables::Aimbot::enabled, nullptr, &variables::Aimbot::aimbotKey);
            MatchaUI::Checkbox("Show FOV Circle", &variables::Aimbot::showFOV, variables::Aimbot::fovColor);
            MatchaUI::Checkbox("Wall Check", &variables::Aimbot::requireVisible);
            if (variables::Aimbot::requireVisible) {
                char wb[64];
                sprintf_s(wb, "Wall cache: %d parts", Visibility::boxCount.load());
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", wb);
                if (Visibility::boxCount.load() == 0)
                    ImGui::TextColored(ImVec4(1.f, 0.45f, 0.35f, 1.f), "Building walls… don't aim yet");
                if (ImGui::SmallButton("Rebuild Walls"))
                    Visibility::ForceRebuild();
            }
            MatchaUI::Checkbox("Sticky Aim", &variables::Aimbot::stickyAim);
            MatchaUI::Checkbox("Always On", &variables::Aimbot::alwaysOn);
            MatchaUI::Checkbox("Predict Movement", &variables::Aimbot::prediction);
            MatchaUI::Checkbox("Resolver", &variables::Aimbot::resolver);
            MatchaUI::Checkbox("Humanize Aim", &variables::Extra::humanizeAim);
            MatchaUI::SliderFloat("Humanize Amt", &variables::Extra::humanizeAmount, 0.05f, 1.f, "%.2f");
            MatchaUI::Checkbox("Random Bone", &variables::Extra::randomBone);
            MatchaUI::Checkbox("Aim On Shot", &variables::Extra::aimOnShot);
            MatchaUI::Checkbox("FOV Rainbow", &variables::Extra::fovRainbow);
            MatchaUI::Checkbox("Team Check", &variables::teamCheck);
            MatchaUI::Checkbox("Skip Dead", &variables::healthCheck);
            const char* methods[] = { "Closest To Crosshair", "Silent Aim Target" };
            MatchaUI::Combo("Target Mode", &variables::Aimbot::targetMethod, methods, 2);
            const char* aims[] = { "Mouse Move", "Silent Aim" };
            MatchaUI::Combo("Aim Style", &variables::Aimbot::aimType, aims, 2);
            if (variables::Aimbot::aimType == 1 || variables::Aimbot::targetMethod == 1)
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim),
                    "Silent: spoofs mouse hit (no cursor move)");
            MatchaUI::EndCard();

            MatchaUI::BeginCard("Aim Settings");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "All sliders are 0–100 (easy)");
            MatchaUI::SliderFloat("Smoothness", &variables::Aimbot::uiSmoothness, 0.f, 100.f, "%.0f");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "0 = snappy   100 = very smooth");
            MatchaUI::SliderFloat("Stability", &variables::Aimbot::uiStability, 0.f, 100.f, "%.0f");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Higher = less shake / overshoot");
            MatchaUI::SliderFloat("Lock Zone", &variables::Aimbot::uiLockZone, 0.f, 100.f, "%.0f");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Stop aiming when already on target");
            MatchaUI::SliderFloat("Max Range", &variables::Aimbot::uiRange, 0.f, 100.f, "%.0f");
            MatchaUI::SliderFloat("FOV Size", &variables::Aimbot::uiFov, 0.f, 100.f, "%.0f");
            MatchaUI::SliderFloat("Sticky FOV", &variables::Aimbot::uiStickyFov, 0.f, 100.f, "%.0f");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Extra FOV while locked on");
            MatchaUI::SliderFloat("Aim Speed", &variables::Aimbot::uiAimSpeed, 0.f, 100.f, "%.0f");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Max mouse move per frame");
            const char* parts[] = { "Head", "Body", "Left Leg", "Right Leg", "Left Arm", "Right Arm", "Closest Part" };
            MatchaUI::Combo("Aim Bone", &variables::Aimbot::aimTarget, parts, 7);
            SyncAimConfigFromUI();
            MatchaUI::EndCard();

            MatchaUI::NextCol();
            MatchaUI::BeginCard("Trigger");
            MatchaUI::Checkbox("Enabled", &variables::Trigger::enabled, nullptr, &variables::Trigger::key);
            MatchaUI::SliderFloat("Delay (ms)", &variables::Trigger::delayMs, 0, 200, "%.0f");
            MatchaUI::SliderFloat("Hold (ms)", &variables::Trigger::releaseMs, 1, 60, "%.0f");
            MatchaUI::SliderFloat("Hit Radius", &variables::Trigger::hitRadius, 6, 60, "%.0f");
            MatchaUI::Checkbox("Wall Check", &variables::Trigger::requireVisible);
            MatchaUI::Checkbox("Head Only", &variables::Trigger::headOnly);
            MatchaUI::Checkbox("Burst Fire", &variables::Extra::burstTrigger);
            MatchaUI::SliderInt("Burst Count", &variables::Extra::burstCount, 2, 8);
            MatchaUI::EndCard();

            MatchaUI::BeginCard("Rage");
            MatchaUI::Checkbox("Enabled", &variables::Rage::enabled, nullptr, &variables::Rage::key);
            MatchaUI::Checkbox("Auto Shoot", &variables::Rage::shoot);
            MatchaUI::Checkbox("Teleport", &variables::Rage::teleport);
            MatchaUI::Checkbox("Unkillable TP", &variables::Rage::unkillable);
            MatchaUI::Checkbox("Melee Aura", &variables::Extra::meleeAura);
            MatchaUI::SliderFloat("Melee Range", &variables::Extra::meleeRange, 4, 30, "%.0f");
            MatchaUI::SliderFloat("Cycle Delay", &variables::Rage::delayMs, 0, 200, "%.0f");
            MatchaUI::SliderFloat("TP Distance", &variables::Rage::tpDistance, 0.5f, 12.f, "%.1f");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Unkillable = behind enemy + health stick");
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
        }
        else if (variables::selectedSub == 1) {
            MatchaUI::BeginTwoCol("##c1");
            MatchaUI::BeginCard("Hitbox Extender");
            if (Games::Detect() == Games::Kind::MiscGunTest) {
                ImGui::TextColored(ImVec4(1.f, 0.42f, 0.35f, 1.f), "WARNING — MiscGunTest:X");
                ImGui::TextColored(ImVec4(1.f, 0.55f, 0.45f, 1.f), "Do NOT use Hitbox Extender.");
                ImGui::TextColored(ImVec4(1.f, 0.55f, 0.45f, 1.f), "You will be banned.");
                ImGui::Spacing();
                variables::Hitbox::enabled = false;
                variables::Local::hitboxEnabled = false;
            }
            MatchaUI::Checkbox("Enabled", &variables::Hitbox::enabled, nullptr, &variables::Hitbox::key);
            MatchaUI::Checkbox("Visualize Hitbox", &variables::Hitbox::visualize);
            MatchaUI::Checkbox("Team Check", &variables::Hitbox::teamCheck);
            MatchaUI::Checkbox("Health Check", &variables::Hitbox::healthCheck);
            MatchaUI::Checkbox("Aim Assist (needed to hit)", &variables::Hitbox::aimAssist);
            MatchaUI::SliderFloat("Hitbox Size", &variables::Hitbox::size, 2, 50, "%.0f");
            const char* ht[] = { "HRP Only", "Multi Part" };
            MatchaUI::Combo("Type", &variables::Hitbox::type, ht, 2);
            MatchaUI::EndCard();
            MatchaUI::NextCol();
            MatchaUI::BeginCard("Misc");
            if (Games::Detect() == Games::Kind::MiscGunTest) {
                ImGui::TextColored(ImVec4(1.f, 0.45f, 0.35f, 1.f), "Hitbox expand is disabled here.");
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Use aimbot / silent aim instead.");
            } else {
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Arsenal ignores client Size alone.");
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Aim Assist spoofs shots to the bone");
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "when your crosshair is in the big box.");
            }
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
            SyncLegacyHitbox();
        }
        else if (variables::selectedSub == 2) {
            MatchaUI::BeginCard("Desync");
            MatchaUI::Checkbox("Enabled", &variables::Desync::enabled, nullptr, &variables::Desync::key);
            MatchaUI::Checkbox("Remove Walk Animation", &variables::Desync::removeWalkAnim);
            MatchaUI::Checkbox("Display Server Position", &variables::Desync::displayServerPos);
            MatchaUI::Checkbox("Reset on Desync Off", &variables::Desync::resetOnOff);
            MatchaUI::Checkbox("Use Tick", &variables::Desync::useTick);
            MatchaUI::EndCard();
            SyncLegacyHitbox();
        }
        else {
            MatchaUI::BeginTwoCol("##guns");
            MatchaUI::BeginCard("Gun Mods");
            MatchaUI::Checkbox("Fast Reload", &variables::GunMods::fastReload);
            MatchaUI::Checkbox("Fast Fire Rate", &variables::GunMods::fastFire);
            MatchaUI::Checkbox("Rapid+ (min delay)", &variables::GunMods::rapidPlus);
            MatchaUI::Checkbox("Always Auto", &variables::GunMods::alwaysAuto);
            MatchaUI::Checkbox("No Spread", &variables::GunMods::noSpread);
            MatchaUI::Checkbox("No Recoil", &variables::GunMods::noRecoil);
            MatchaUI::Checkbox("No Sway", &variables::GunMods::noSway);
            MatchaUI::Checkbox("Infinite Ammo", &variables::GunMods::infiniteAmmo);
            MatchaUI::Checkbox("Auto Reload", &variables::GunMods::autoReload);
            MatchaUI::Checkbox("Damage Boost", &variables::GunMods::damageBoost);
            MatchaUI::Checkbox("Long Range", &variables::GunMods::longRange);
            MatchaUI::Checkbox("Wall Range+", &variables::GunMods::wallbangHint);
            MatchaUI::Checkbox("Instant Equip", &variables::GunMods::instantEquip);
            MatchaUI::Checkbox("Aggressive Rewrite", &variables::GunMods::aggressive);
            MatchaUI::EndCard();

            MatchaUI::NextCol();
            MatchaUI::BeginCard("Tuning");
            MatchaUI::SliderFloat("Reload Time", &variables::GunMods::reloadTime, 0.01f, 0.5f, "%.2f");
            MatchaUI::SliderFloat("Fire Rate", &variables::GunMods::fireRate, 0.04f, 0.5f, "%.2f");
            MatchaUI::SliderFloat("Damage x", &variables::GunMods::damageMultiplier, 1.f, 20.f, "%.1f");
            MatchaUI::SliderFloat("Range x", &variables::GunMods::rangeMultiplier, 1.f, 10.f, "%.1f");
            char gc[64];
            sprintf_s(gc, "Weapon values: %d", GunMods::entryCount.load());
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", gc);
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Arsenal + MiscGunTest:X — RS.Weapons / Guns");
            if (GunMods::entryCount.load() == 0)
                ImGui::TextColored(ImVec4(1.f, 0.5f, 0.35f, 1.f), "No Weapons found — spawn into a match");
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Keep Fire Rate >= 0.04 or guns won't shoot");
            char vo[48];
            sprintf_s(vo, "Value offset: 0x%llX", (unsigned long long)GunMods::valueOff);
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", vo);
            if (ImGui::SmallButton("Rescan Weapons"))
                GunMods::Rebuild();
            if (ImGui::SmallButton("Restore Defaults"))
                GunMods::RestoreAll();
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
        }
    }

    inline void DrawVisuals() {
        static const char* subs[] = { "ESP", "Crosshair", "Indicators", "OOF", "Radar" };
        MatchaUI::SubTabs(subs, 5, &variables::selectedSub);

        if (variables::selectedSub == 0) {
            MatchaUI::BeginTwoCol("##v0");
            MatchaUI::BeginCard("Box");
            MatchaUI::Checkbox("Enabled", &variables::ESP::boxes, variables::ESP::boxColor);
            MatchaUI::Checkbox("Fill Box", &variables::ESP::fillBox, variables::ESP::boxFillColor);
            const char* bt[] = { "2D", "Cube", "Corners" };
            MatchaUI::Combo("Box Type", &variables::ESP::boxType, bt, 3);
            MatchaUI::SliderFloat("Thickness", &variables::ESP::boxThickness, 1, 5, "%.1f");
            MatchaUI::EndCard();

            MatchaUI::BeginCard("Name");
            MatchaUI::Checkbox("Enabled", &variables::ESP::names, variables::ESP::nameColor);
            const char* nt[] = { "Name", "Display Name" };
            MatchaUI::Combo("Type", &variables::ESP::nameType, nt, 2);
            MatchaUI::EndCard();

            MatchaUI::BeginCard("Tracer");
            MatchaUI::Checkbox("Enabled", &variables::ESP::snaplines, variables::ESP::snapColor);
            const char* orgs[] = { "Top", "Middle", "Bottom", "Mouse" };
            MatchaUI::Combo("Origin", &variables::ESP::snaplinesOrigin, orgs, 4);
            MatchaUI::EndCard();

            MatchaUI::NextCol();
            MatchaUI::BeginCard("Health");
            MatchaUI::Checkbox("Health Bar", &variables::ESP::healthBar, variables::ESP::healthColor);
            MatchaUI::Checkbox("Health Based", &variables::ESP::healthBasedColor);
            MatchaUI::Checkbox("Health Text", &variables::ESP::healthText);
            const char* hp[] = { "Above Name", "Below Name" };
            MatchaUI::Combo("Text Pos", &variables::ESP::healthTextPos, hp, 2);
            MatchaUI::EndCard();

            MatchaUI::BeginCard("Chams");
            const char* cm[] = { "Default", "Engine" };
            MatchaUI::Combo("Mode", &variables::ESP::chamsMode, cm, 2);
            MatchaUI::Checkbox("Enabled", &variables::ESP::chamsEnabled);
            MatchaUI::Checkbox("Filled", &variables::ESP::chamsFilled);
            const char* cr[] = { "Static", "Pulse" };
            MatchaUI::Combo("Rendering Type", &variables::ESP::chamsRender, cr, 2);
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "External stand-in");
            MatchaUI::EndCard();

            MatchaUI::BeginCard("Master");
            MatchaUI::Checkbox("ESP Enabled", &variables::ESP::enabled);
            MatchaUI::SliderFloat("Distance", &variables::ESP::maxDistance, 100, 10000, "%.0f");
            MatchaUI::Checkbox("Rainbow ESP", &variables::ESP::rainbow);
            MatchaUI::Checkbox("Box Glow", &variables::ESP::boxGlow);
            MatchaUI::Checkbox("Team Colors", &variables::ESP::teamColors);
            MatchaUI::Checkbox("Visible Only", &variables::ESP::visibleOnly);
            MatchaUI::Checkbox("Target Highlight", &variables::ESP::targetHighlight);
            MatchaUI::EndCard();
            MatchaUI::EndTwoCol();
        }
        else if (variables::selectedSub == 1) {
            MatchaUI::BeginCard("Style");
            MatchaUI::Checkbox("Enable", &variables::Crosshair::enabled);
            const char* st[] = { "Static", "Spin" };
            MatchaUI::Combo("Style", &variables::Crosshair::style, st, 2);
            MatchaUI::Checkbox("Follow Target", &variables::Crosshair::followTarget);
            MatchaUI::SliderFloat("Length", &variables::Crosshair::length, 4, 60, "%.0f");
            MatchaUI::SliderFloat("Gap", &variables::Crosshair::gap, 0, 30, "%.0f");
            MatchaUI::SliderFloat("Thickness", &variables::Crosshair::thickness, 1, 8, "%.1f");
            {
                float seg = (float)variables::Crosshair::segments;
                if (MatchaUI::SliderFloat("Segments", &seg, 2, 8, "%.0f"))
                    variables::Crosshair::segments = (int)seg;
            }
            MatchaUI::Checkbox("Outline", &variables::Crosshair::outline, variables::Crosshair::outlineColor);
            MatchaUI::SliderFloat("Outline Thickness", &variables::Crosshair::outlineThickness, 0.5f, 4.f, "%.1f");
            MatchaUI::Checkbox("Center Dot", &variables::Crosshair::centerDot, variables::Crosshair::centerDotColor);
            MatchaUI::EndCard();
        }
        else if (variables::selectedSub == 2) {
            MatchaUI::BeginCard("Indicators");
            MatchaUI::Checkbox("Distance", &variables::ESP::distance);
            MatchaUI::Checkbox("Equipped Item", &variables::ESP::equippedItem);
            MatchaUI::Checkbox("Skeleton", &variables::ESP::skeleton);
            MatchaUI::Checkbox("Head Dot", &variables::ESP::headDot, variables::ESP::headDotColor);
            MatchaUI::Checkbox("Head Dot Glow", &variables::ESP::headDotGlow);
            MatchaUI::Checkbox("China Hat", &variables::ESP::chinaHat);
            MatchaUI::Checkbox("Look Direction", &variables::ESP::lookDir);
            MatchaUI::Checkbox("Flags", &variables::ESP::flags);
            MatchaUI::Checkbox("Armor Bar", &variables::ESP::armorBar);
            MatchaUI::Checkbox("Wireframe Players", &variables::ESP::wireframePlayers);
            MatchaUI::Checkbox("Profile Picture", &variables::ESP::profilePicture);
            MatchaUI::Checkbox("Dead Check", &variables::ESP::deadCheck);
            MatchaUI::EndCard();
        }
        else if (variables::selectedSub == 3) {
            MatchaUI::BeginCard("OOF Arrow");
            MatchaUI::Checkbox("Enabled", &variables::ESP::oofArrows, variables::ESP::oofColor);
            MatchaUI::Checkbox("Show Profile Pictures", &variables::ESP::oofShowPfp);
            MatchaUI::SliderFloat("Radius", &variables::ESP::oofRadius, 40, 300, "%.0f");
            MatchaUI::SliderFloat("Arrow Size", &variables::ESP::oofSize, 4, 48, "%.1f");
            MatchaUI::SliderFloat("Max Distance", &variables::ESP::oofDistance, 100, 2000, "%.0f");
            MatchaUI::EndCard();
        }
        else {
            MatchaUI::BeginCard("Radar");
            MatchaUI::Checkbox("Enabled", &variables::Radar::enabled);
            const char* rt[] = { "2D", "3D" };
            MatchaUI::Combo("Type", &variables::Radar::type, rt, 2);
            MatchaUI::Checkbox("Rotate With Camera", &variables::Radar::rotateWithCamera);
            MatchaUI::Checkbox("Show Names", &variables::Radar::showNames);
            MatchaUI::SliderFloat("Size", &variables::Radar::size, 100, 320, "%.0f");
            MatchaUI::SliderFloat("Range", &variables::Radar::range, 50, 1000, "%.0f");
            MatchaUI::EndCard();
        }
    }

    inline void DrawWorld() {
        MatchaUI::BeginTwoCol("##w");
        MatchaUI::BeginCard("Lighting");
        MatchaUI::Checkbox("Fullbright", &variables::World::fullbright);
        MatchaUI::Checkbox("Night Mode", &variables::World::nightMode);
        MatchaUI::Checkbox("No Fog", &variables::World::noFog);
        MatchaUI::Checkbox("No Shadows", &variables::World::noShadows);
        MatchaUI::Checkbox("Remove Atmosphere", &variables::World::removeAtmosphere);
        MatchaUI::Checkbox("Custom Brightness", &variables::World::customBrightness);
        MatchaUI::SliderFloat("Brightness", &variables::World::brightness, 0, 5, "%.1f");
        MatchaUI::Checkbox("Custom Clock", &variables::World::customClock);
        MatchaUI::SliderFloat("Clock Time", &variables::World::clockTime, 0, 24, "%.1f");
        MatchaUI::Checkbox("Custom Ambient", &variables::World::customAmbient);
        MatchaUI::SliderFloat("Ambient R", &variables::World::ambientR, 0, 1, "%.2f");
        MatchaUI::SliderFloat("Ambient G", &variables::World::ambientG, 0, 1, "%.2f");
        MatchaUI::SliderFloat("Ambient B", &variables::World::ambientB, 0, 1, "%.2f");
        MatchaUI::EndCard();
        MatchaUI::NextCol();
        MatchaUI::BeginCard("Camera");
        MatchaUI::Checkbox("Custom FOV", &variables::World::customFov);
        MatchaUI::SliderFloat("FOV Amount", &variables::World::fovAmount, 40, 120, "%.0f");
        MatchaUI::Checkbox("Viewmodel FOV", &variables::World::viewmodelFov);
        MatchaUI::SliderFloat("VM FOV", &variables::World::viewmodelFovAmt, 40, 120, "%.0f");
        MatchaUI::Checkbox("Third Person", &variables::World::thirdPerson);
        MatchaUI::SliderFloat("TP Distance", &variables::World::thirdPersonDistance, 4, 40, "%.0f");
        MatchaUI::Checkbox("Unlock Zoom", &variables::World::unlockZoom);
        MatchaUI::SliderFloat("Max Zoom", &variables::World::maxZoom, 50, 2000, "%.0f");
        MatchaUI::Checkbox("Gun Wireframe", &variables::World::gunWireframe, variables::World::gunWireColor);
        MatchaUI::Checkbox("Show Velocity", &variables::World::showVelocity);
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawCharacter() {
        MatchaUI::BeginTwoCol("##ch");
        MatchaUI::BeginCard("Speed");
        MatchaUI::Checkbox("Enabled", &variables::Local::speedEnabled, nullptr, &variables::Local::speedKey);
        const char* sm[] = { "Velocity", "Position", "Slippery" };
        MatchaUI::Combo("Method", &variables::Local::speedMethod, sm, 3);
        MatchaUI::SliderFloat("Amount", &variables::Local::walkSpeed, 1, 200, "%.0f");
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Flight");
        MatchaUI::Checkbox("Enabled", &variables::Local::flyEnabled, nullptr, &variables::Local::flyKey);
        const char* fm[] = { "Velocity", "Position" };
        MatchaUI::Combo("Method", &variables::Local::flyMethod, fm, 2);
        MatchaUI::SliderFloat("Amount", &variables::Local::flySpeed, 1, 200, "%.0f");
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Jump");
        MatchaUI::Checkbox("Jump Power", &variables::Local::jumpEnabled);
        MatchaUI::SliderFloat("Power", &variables::Local::jumpPower, 1, 200, "%.0f");
        MatchaUI::Checkbox("Inf Jump", &variables::Local::infJump);
        MatchaUI::Checkbox("Bunny Hop", &variables::Local::bhopEnabled);
        MatchaUI::SliderFloat("Bhop Speed", &variables::Local::bhopSpeed, 10, 100, "%.0f");
        MatchaUI::EndCard();

        MatchaUI::NextCol();
        MatchaUI::BeginCard("Float");
        MatchaUI::Checkbox("Enabled", &variables::Local::floatEnabled);
        MatchaUI::SliderFloat("Height", &variables::Local::floatHeight, 1, 50, "%.0f");
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Movement");
        MatchaUI::Checkbox("Anti-Fling", &variables::Local::antiFling);
        MatchaUI::Checkbox("No-Clip", &variables::Local::noclip);
        MatchaUI::Checkbox("Click TP", &variables::Local::clickTp, nullptr, &variables::Local::clickTpKey);
        MatchaUI::Checkbox("Auto TP Loop", &variables::Local::autoTp, nullptr, &variables::Local::autoTpKey);
        MatchaUI::SliderFloat("TP Delay (s)", &variables::Local::autoTpDelay, 0.f, 0.5f, "%.2f");
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "0 = every frame (fastest)");
        MatchaUI::Checkbox("TP Walk", &variables::Local::tpWalk);
        MatchaUI::SliderFloat("TP Walk Step", &variables::Local::tpWalkStep, 0.5f, 8.f, "%.1f");
        MatchaUI::Checkbox("Orbit Target", &variables::Local::orbitPlayer);
        MatchaUI::SliderFloat("Orbit Speed", &variables::Local::orbitSpeed, 0.5f, 12.f, "%.1f");
        MatchaUI::SliderFloat("Orbit Radius", &variables::Local::orbitRadius, 2.f, 30.f, "%.0f");
        MatchaUI::Checkbox("Freeze", &variables::Local::freeze, nullptr, &variables::Local::freezeKey);
        MatchaUI::Checkbox("Spin", &variables::Local::spin);
        MatchaUI::SliderFloat("Spin Speed", &variables::Local::spinSpeed, 1, 80, "%.0f");
        MatchaUI::Checkbox("Hip Height", &variables::Local::hipHeightEnabled);
        MatchaUI::SliderFloat("Hip Amount", &variables::Local::hipHeight, 0, 20, "%.1f");
        MatchaUI::Checkbox("Custom Gravity", &variables::Local::gravityEnabled);
        MatchaUI::SliderFloat("Gravity", &variables::Local::gravity, 0, 196, "%.0f");
        MatchaUI::Checkbox("God Mode (HP stick)", &variables::Local::godMode);
        MatchaUI::Checkbox("Anti-Void", &variables::Local::antiVoid);
        MatchaUI::Checkbox("Auto Clicker", &variables::Local::autoClicker, nullptr, &variables::Local::autoClickerKey);
        MatchaUI::SliderFloat("CPS", &variables::Local::autoClickerCps, 1, 30, "%.0f");
        MatchaUI::Checkbox("Sit Spam", &variables::Local::sitSpam);
        MatchaUI::Checkbox("Vehicle Boost", &variables::Local::vehicleBoost);
        MatchaUI::SliderFloat("Vehicle Speed", &variables::Local::vehicleBoostAmt, 20, 200, "%.0f");
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Animations");
        MatchaUI::Checkbox("Animation Changer", &variables::Exploits::animation_changer);
        MatchaUI::SliderInt("Idle", &variables::Exploits::idle_animation, 0, 22);
        MatchaUI::SliderInt("Run", &variables::Exploits::run_animation, 0, 11);
        MatchaUI::SliderInt("Walk", &variables::Exploits::walk_animation, 0, 11);
        MatchaUI::SliderInt("Jump", &variables::Exploits::jump_animation, 0, 11);
        MatchaUI::SliderInt("Fall", &variables::Exploits::fall_animation, 0, 11);
        MatchaUI::SliderInt("Climb", &variables::Exploits::climb_animation, 0, 4);
        MatchaUI::SliderInt("Swim", &variables::Exploits::swim_animation, 0, 4);
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawOptions() {
        MatchaUI::BeginTwoCol("##opt");
        MatchaUI::BeginCard("Options");
        MatchaUI::Checkbox("Streamproof", &variables::Misc::streamProof);
        MatchaUI::Checkbox("Streamer Mode", &variables::Misc::streamerMode);
        MatchaUI::Checkbox("Anti-AFK", &variables::Misc::antiAfk);
        MatchaUI::Checkbox("AFK Assist", &variables::Misc::afkAssist);
        if (variables::Misc::afkAssist) {
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Leave-running: always-on aim + anti-AFK.");
            variables::Misc::antiAfk = true;
            variables::Aimbot::alwaysOn = true;
            if (!variables::Aimbot::enabled) variables::Aimbot::enabled = true;
        }
        MatchaUI::SliderFloat("AFK Interval (s)", &variables::Misc::antiAfkSeconds, 6.f, 30.f, "%.0f");
        MatchaUI::Checkbox("Panic Key", &variables::Misc::panicKey, nullptr, &variables::Misc::panicVk);
        MatchaUI::Checkbox("Watermark / FPS", &variables::Misc::showFps);
        MatchaUI::Checkbox("Enemy Counter", &variables::Misc::enemyCounter);
        MatchaUI::Checkbox("Target HUD", &variables::Misc::targetHud);
        MatchaUI::Checkbox("Hit Markers", &variables::Misc::hitMarker);
        MatchaUI::Checkbox("Damage Numbers", &variables::Misc::damageNumbers);
        MatchaUI::Checkbox("Keybind List", &variables::Misc::showKeybinds);
        MatchaUI::Checkbox("FPS Boost Mode", &variables::Misc::fpsBoost);
        MatchaUI::Checkbox("Auto Rejoin Hint", &variables::Misc::autoRejoin);
        MatchaUI::Checkbox("VSync", &variables::Perf::vsync);
        MatchaUI::SliderInt("FPS Cap", &variables::Perf::targetFps, 0, 240);
        MatchaUI::SliderFloat("Menu Anim Speed", &variables::Misc::menuAnimSpeed, 6.f, 28.f, "%.0f");
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "0 FPS cap = uncapped");
        MatchaUI::EndCard();
        MatchaUI::NextCol();
        MatchaUI::BeginCard("Menu");
        MatchaUI::Checkbox("Floating Icon Header", &variables::Theme::useFloatingHeader);
        MatchaUI::Checkbox("Background FX", &variables::Theme::bgEffect);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Menu key");
        MatchaUI::KeybindChip("menuk", &variables::Misc::menuVk);
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.16f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.08f, 0.08f, 1));
        if (ImGui::Button("Eject", ImVec2(-1, 34)))
            Globals::running = false;
        ImGui::PopStyleColor(3);
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawStatus() {
        RefreshStatusInfo();
        MatchaUI::BeginTwoCol("##st");
        MatchaUI::BeginCard("Player");
        CopyField("Username", variables::Status::username);
        CopyField("Display", variables::Status::displayName);
        CopyField("User ID", variables::Status::userId);
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Session");
        CopyField("Place ID", variables::Status::placeId);
        CopyField("Game ID", variables::Status::gameId);
        CopyField("Job ID", variables::Status::jobId);
        ImGui::TextColored(
            Arsenal::IsSupportedPlace() ? ImVec4(0.45f, 0.9f, 0.55f, 1.f) : ImVec4(1.f, 0.45f, 0.35f, 1.f),
            "Game: %s", Games::Name());
        if (Games::Detect() == Games::Kind::MiscGunTest) {
            ImGui::TextColored(ImVec4(1.f, 0.42f, 0.35f, 1.f), "Do not use Hitbox Extender — ban risk");
        }
        if (Games::Detect() == Games::Kind::MTC) {
            ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.65f, 1.f), "MTC slim mode — ESP + auto range");
        }
        MatchaUI::EndCard();

        MatchaUI::NextCol();
        MatchaUI::BeginCard("Client");
        CopyField("Version", variables::Status::clientVersion);
        CopyField("Players", variables::Status::playersOnline);
        char fps[16]; sprintf_s(fps, "%d", variables::Perf::currentFps);
        CopyField("FPS", fps);
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Actions");
        if (ImGui::Button("Refresh", ImVec2(-1, 30)))
            variables::Status::lastRefresh = 0.f;
        if (ImGui::Button("Copy Job ID", ImVec2(-1, 30)))
            ImGui::SetClipboardText(variables::Status::jobId);
        if (ImGui::Button("Copy User ID", ImVec2(-1, 30)))
            ImGui::SetClipboardText(variables::Status::userId);
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.16f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.08f, 0.08f, 1));
        if (ImGui::Button("Eject External", ImVec2(-1, 36)))
            Globals::running = false;
        ImGui::PopStyleColor(3);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Closes Match-Ware cleanly.");
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawConfigs() {
        MatchaUI::BeginTwoCol("##cfg");
        MatchaUI::BeginCard("Audio");
        MatchaUI::Checkbox("Hit Sounds", &variables::Audio::hitSounds);
        MatchaUI::Checkbox("Kill Sounds", &variables::Audio::killSounds);
        MatchaUI::Checkbox("Spotify Mini", &variables::Audio::spotifyMini);
        MatchaUI::EndCard();
        MatchaUI::NextCol();
        MatchaUI::BeginCard("Configs");
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Save / Load coming soon");
        if (ImGui::Button("Open Site", ImVec2(-1, 32)))
            ShellExecuteA(nullptr, "open", "https://match-ware.vercel.app/", nullptr, nullptr, SW_SHOWNORMAL);
        if (ImGui::Button("Join Discord", ImVec2(-1, 32)))
            ShellExecuteA(nullptr, "open", "https://discord.gg/rUXya4U5qM", nullptr, nullptr, SW_SHOWNORMAL);
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawServers() {
        MatchaUI::BeginTwoCol("##srv");
        MatchaUI::BeginCard("Filters");
        const char* sort[] = { "Descending", "Ascending" };
        MatchaUI::Combo("Sort", &variables::Servers::sortMode, sort, 2);
        const char* reg[] = { "All" };
        MatchaUI::Combo("Region", &variables::Servers::region, reg, 1);
        const char* ar[] = { "Disabled", "5s", "15s" };
        MatchaUI::Combo("Auto Refresh", &variables::Servers::autoRefresh, ar, 3);
        if (ImGui::Button("Refresh", ImVec2(-1, 30)))
            variables::Servers::serverCount = 0;
        MatchaUI::EndCard();
        MatchaUI::BeginCard("Info");
        ImGui::Text("Servers: %d", variables::Servers::serverCount);
        ImGui::TextWrapped("Current: %s", variables::Servers::currentId);
        MatchaUI::EndCard();

        MatchaUI::NextCol();
        MatchaUI::BeginCard("Servers");
        ImGui::Text("Servers [%d]", variables::Servers::serverCount);
        ImGui::Dummy(ImVec2(0, 40));
        ImVec2 avail = ImGui::GetContentRegionAvail();
        const char* empty = "No servers found";
        ImVec2 ts = ImGui::CalcTextSize(empty);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", empty);
        MatchaUI::EndCard();
        MatchaUI::EndTwoCol();
    }

    inline void DrawMusic() {
        MatchaUI::BeginCard("Music");
        MatchaUI::Checkbox("Show Player Widget", &variables::Audio::spotifyMini);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Uses Spotify window title when available.");
        MatchaUI::EndCard();
    }

    inline void DrawMTC() {
        MatchaUI::BeginCard("MTC Mode");
        ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.65f, 1.f), "Multicrew Tank Combat");
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim),
            "Slim build — player ESP + auto range only. No fly, aimbot, or gun mods.");
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Player ESP");
        MatchaUI::Checkbox("Enable Player ESP", &variables::MTC::playerEsp);
        MatchaUI::Checkbox("Boxes", &variables::MTC::espBoxes);
        MatchaUI::Checkbox("Names", &variables::MTC::espNames);
        MatchaUI::Checkbox("Distance", &variables::MTC::espDistance);
        MatchaUI::Checkbox("Health Bar", &variables::MTC::espHealth);
        MatchaUI::Checkbox("Team Check", &variables::MTC::teamCheck);
        MatchaUI::SliderFloat("Max Distance", &variables::ESP::maxDistance, 100.f, 5000.f, "%.0f");
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Auto Range");
        MatchaUI::Checkbox("Auto Rangefinder", &variables::MTC::autoRange);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim),
            "Writes meters into Gunner.Data.Rangefinder for the player nearest your crosshair.");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.94f, 1.f), "%s", MTC::statusBuf);
        MatchaUI::EndCard();

        MatchaUI::BeginCard("Options");
        MatchaUI::Checkbox("Show FPS", &variables::Misc::showFps);
        MatchaUI::Checkbox("Spotify Mini", &variables::Audio::spotifyMini);
        MatchaUI::EndCard();
    }

    inline void RenderBody() {
        if (Games::IsMTC()) {
            DrawMTC();
            return;
        }
        switch (variables::selectedTab) {
        case 0: DrawCombat(); break;
        case 1: DrawVisuals(); break;
        case 2: DrawWorld(); break;
        case 3: DrawCharacter(); break;
        case 4: DrawOptions(); break;
        case 5: DrawConfigs(); break;
        case 6: DrawServers(); break;
        case 7: DrawMusic(); break;
        case 8: DrawStatus(); break;
        default: break;
        }
    }
}
