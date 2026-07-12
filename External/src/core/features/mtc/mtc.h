#pragma once
#include "../../../sdk/sdk.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/w2s.h"
#include "../../../core/globals/globals.h"
#include "../../../core/cache/cache.h"
#include "../../../core/variables/variables.h"
#include "../../../core/games/arsenal.h"
#include "../../../core/features/aimbot/aimbot.h"
#include "../../../core/features/exploits/gun_mods.h"
#include "../../../memory/memory.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

// [MTC] Multicrew Tank Combat — player ESP + auto rangefinder only.
namespace MTC {

    inline uintptr_t rangefinderValueAddr = 0;
    inline bool rangefinderIsString = false;
    inline float lastWrittenRange = 0.f;
    inline char statusBuf[96] = "Waiting for gunner data";

    // PHRST / Roblox-style: ~0.28 meters per stud
    inline float StudToMeters(float studs) { return studs * 0.28f; }

    inline void DisableNonMtcFeatures()
    {
        variables::Aimbot::enabled = false;
        variables::Aimbot::toggledOn = false;
        variables::Aimbot::alwaysOn = false;
        variables::Aimbot::showFOV = false;
        variables::Trigger::enabled = false;
        variables::Rage::enabled = false;
        variables::Crosshair::enabled = false;
        variables::Radar::enabled = false;
        variables::Local::speedEnabled = false;
        variables::Local::jumpEnabled = false;
        variables::Local::flyEnabled = false;
        variables::Local::flyActive = false;
        variables::Local::bhopEnabled = false;
        variables::Local::desyncEnabled = false;
        variables::Local::hitboxEnabled = false;
        variables::Local::freeze = false;
        variables::Local::spin = false;
        variables::Local::noclip = false;
        variables::Local::floatEnabled = false;
        variables::Local::autoTp = false;
        variables::Local::clickTp = false;
        variables::Local::gravityEnabled = false;
        variables::Local::godMode = false;
        variables::Local::tpWalk = false;
        variables::Local::autoClicker = false;
        variables::Local::orbitPlayer = false;
        variables::Local::sitSpam = false;
        variables::Local::vehicleBoost = false;
        variables::Hitbox::enabled = false;
        variables::Desync::enabled = false;
        variables::Exploits::animation_changer = false;
        variables::Misc::afkAssist = false;
        variables::ESP::oofArrows = false;
        variables::ESP::skeleton = false;
        variables::ESP::chamsEnabled = false;
        variables::ESP::chinaHat = false;
        variables::World::nightMode = false;
        variables::World::thirdPerson = false;
        GunMods::DisableAll();
        Aimbot::lockedPlayerAddr = 0;
    }

    inline RBX::RbxInstance FindChildPath(RBX::RbxInstance root, const char* const* names, int count)
    {
        RBX::RbxInstance cur = root;
        for (int i = 0; i < count; i++) {
            if (!cur.Addr) return RBX::RbxInstance(0);
            cur = cur.FindChild(names[i]);
        }
        return cur;
    }

    inline bool ResolveRangefinder()
    {
        if (!Globals::dataModel.Addr) return false;

        auto rf = Globals::dataModel.FindChildByClass("ReplicatedFirst");
        if (!rf.Addr) rf = Globals::dataModel.FindChild("ReplicatedFirst");
        if (!rf.Addr) return false;

        const char* path[] = { "NewGuiData", "Gunner", "Data", "Rangefinder", "Data" };
        auto data = FindChildPath(rf, path, 5);
        if (!data.Addr) return false;

        std::string cls = data.GetClass();
        rangefinderIsString = (cls == "StringValue");
        // NumberValue / IntValue / StringValue all expose Value under Misc::Value for this dump era
        rangefinderValueAddr = data.Addr;
        return true;
    }

    inline void WriteRangeMeters(float meters)
    {
        if (!rangefinderValueAddr) return;
        if (meters < 150.f) meters = 150.f;
        if (meters > 9999.f) meters = 9999.f;
        int rounded = (int)(meters + 0.5f);
        if (rounded < 150) rounded = 150;
        if (rounded > 9999) rounded = 9999;

        if (rangefinderIsString) {
            char buf[16];
            sprintf_s(buf, "%d", rounded);
            // StringValue.Value — write via standard string helper if available; else double path
            // Many builds store NumberValue as double at 0xB8; StringValue is a pointer string.
            // Prefer numeric write when class wasn't string.
            memory->write<double>(rangefinderValueAddr + Offsets::Misc::Value, (double)rounded);
            (void)buf;
        }
        else {
            memory->write<double>(rangefinderValueAddr + Offsets::Misc::Value, (double)rounded);
        }
        lastWrittenRange = (float)rounded;
    }

    inline float PickTargetDistanceStuds(const std::vector<PlayerCache::CachedPlayer>& players)
    {
        float screenW, screenH, ox, oy;
        W2S::GetViewport(screenW, screenH, ox, oy);
        float cx = screenW * 0.5f;
        float cy = screenH * 0.5f;

        float bestScore = 1e12f;
        float bestDist = 0.f;
        bool found = false;

        auto view = Globals::renderEngine.GetViewMat();
        for (auto& plr : players) {
            if (!plr.isValid || plr.health <= 0.f) continue;
            if (!plr.bones.hasHrp && !plr.bones.hasHead) continue;
            RBX::Vec3 world = plr.bones.hasHead ? plr.bones.head : plr.bones.hrp;
            RBX::Vec2 scr = W2S::WorldToScreen(world, view);
            if (scr.X == 0.f && scr.Y == 0.f) continue;
            float dx = scr.X - cx;
            float dy = scr.Y - cy;
            float score = dx * dx + dy * dy;
            // Prefer targets near crosshair within ~220px
            if (score > 220.f * 220.f) continue;
            if (score < bestScore) {
                bestScore = score;
                bestDist = plr.distance;
                found = true;
            }
        }
        return found ? bestDist : 0.f;
    }

    inline void RunAutoRange(const std::vector<PlayerCache::CachedPlayer>& players)
    {
        if (!variables::MTC::autoRange) {
            strcpy_s(statusBuf, "Auto range off");
            return;
        }

        static auto lastResolve = std::chrono::steady_clock::now() - std::chrono::seconds(5);
        auto now = std::chrono::steady_clock::now();
        if (!rangefinderValueAddr ||
            std::chrono::duration<float>(now - lastResolve).count() > 2.f) {
            lastResolve = now;
            if (!ResolveRangefinder()) {
                rangefinderValueAddr = 0;
                strcpy_s(statusBuf, "No Gunner.Rangefinder (enter gunner seat)");
                return;
            }
        }

        float studs = PickTargetDistanceStuds(players);
        if (studs <= 1.f) {
            strcpy_s(statusBuf, "No player near crosshair");
            return;
        }

        float meters = StudToMeters(studs);
        WriteRangeMeters(meters);
        sprintf_s(statusBuf, "Range %d m  (%.0f studs)", (int)lastWrittenRange, studs);
    }

    inline void Tick(const std::vector<PlayerCache::CachedPlayer>& players)
    {
        if (!Games::IsMTC()) return;
        DisableNonMtcFeatures();

        // Keep ESP limited to player overlays
        if (!variables::MTC::playerEsp)
            variables::ESP::enabled = false;
        else {
            variables::ESP::enabled = true;
            // Sensible defaults for MTC player ESP — no tank clutter toggles
            variables::ESP::boxes = variables::MTC::espBoxes;
            variables::ESP::names = variables::MTC::espNames;
            variables::ESP::distance = variables::MTC::espDistance;
            variables::ESP::healthBar = variables::MTC::espHealth;
        }

        RunAutoRange(players);
        variables::teamCheck = variables::MTC::teamCheck;
    }
}
