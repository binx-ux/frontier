#pragma once
#include "../../../src/sdk/sdk.h"
#include "../globals/globals.h"
#include "../variables/variables.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace PlayerCache {

    struct BoneSet {
        RBX::Vec3 head{};
        RBX::Vec3 hrp{};
        RBX::Vec3 upperTorso{};
        RBX::Vec3 lowerTorso{};
        RBX::Vec3 leftUpperArm{};
        RBX::Vec3 leftLowerArm{};
        RBX::Vec3 leftHand{};
        RBX::Vec3 rightUpperArm{};
        RBX::Vec3 rightLowerArm{};
        RBX::Vec3 rightHand{};
        RBX::Vec3 leftUpperLeg{};
        RBX::Vec3 leftLowerLeg{};
        RBX::Vec3 leftFoot{};
        RBX::Vec3 rightUpperLeg{};
        RBX::Vec3 rightLowerLeg{};
        RBX::Vec3 rightFoot{};
        // R6
        RBX::Vec3 torso{};
        RBX::Vec3 leftArm{};
        RBX::Vec3 rightArm{};
        RBX::Vec3 leftLeg{};
        RBX::Vec3 rightLeg{};
        bool hasHead = false;
        bool hasHrp = false;
        bool isR6 = false;
    };

    // Part addresses for cheap per-frame GetPos (filled once per character)
    struct BoneParts {
        uintptr_t head = 0;
        uintptr_t hrp = 0;
        uintptr_t upperTorso = 0;
        uintptr_t lowerTorso = 0;
        uintptr_t leftUpperArm = 0;
        uintptr_t leftLowerArm = 0;
        uintptr_t leftHand = 0;
        uintptr_t rightUpperArm = 0;
        uintptr_t rightLowerArm = 0;
        uintptr_t rightHand = 0;
        uintptr_t leftUpperLeg = 0;
        uintptr_t leftLowerLeg = 0;
        uintptr_t leftFoot = 0;
        uintptr_t rightUpperLeg = 0;
        uintptr_t rightLowerLeg = 0;
        uintptr_t rightFoot = 0;
        uintptr_t torso = 0;
        uintptr_t leftArm = 0;
        uintptr_t rightArm = 0;
        uintptr_t leftLeg = 0;
        uintptr_t rightLeg = 0;
        bool isR6 = false;
        bool ready = false;
    };

    struct CachedPlayer {
        uintptr_t playerAddr = 0;
        uintptr_t characterAddr = 0;
        uintptr_t humanoidAddr = 0;
        uintptr_t rootPartAddr = 0;
        uintptr_t headAddr = 0;
        uintptr_t teamAddr = 0;
        int64_t userId = 0;

        std::string name;
        std::string displayName;
        std::string equippedTool;
        RBX::Vec3 position{};
        RBX::Vec3 velocity{};
        float health = 0.0f;
        float maxHealth = 100.0f;
        float distance = 0.0f;
        BoneSet bones{};
        BoneParts boneParts{};
        bool isValid = false;
    };

    inline std::vector<CachedPlayer> players;
    inline std::mutex playersMutex;
    inline RBX::Vec3 localPlayerPos{};
    inline RBX::Vec3 localPlayerLook{ 0, 0, -1 };
    inline uintptr_t localPlayerTeam = 0;
    inline uintptr_t localRootAddr = 0;
    inline std::atomic<bool> cacheWorkerStarted{ false };

    // Per-frame live head/HRP only — no prediction (prediction caused boxes to drift off players)
    inline void refreshLivePositions()
    {
        std::lock_guard<std::mutex> lock(playersMutex);

        if (Globals::localPlayer.Addr != 0) {
            if (!localRootAddr) {
                auto localChar = Globals::localPlayer.GetModelRef();
                if (localChar.Addr) {
                    auto localRoot = localChar.FindChild("HumanoidRootPart");
                    localRootAddr = localRoot.Addr;
                }
            }
            if (localRootAddr) {
                RBX::Vec3 p = RBX::RbxInstance(localRootAddr).GetPos();
                if (p.X != 0.f || p.Y != 0.f || p.Z != 0.f)
                    localPlayerPos = p;
                else
                    localRootAddr = 0; // respawned
            }
        }
        else {
            localRootAddr = 0;
        }

        if (Globals::camera.Addr != 0) {
            auto cf = Globals::camera.GetCameraCFrame();
            localPlayerLook = cf.GetLookVector();
        }

        for (auto& plr : players) {
            if (!plr.isValid) continue;
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
            float dx = plr.position.X - localPlayerPos.X;
            float dy = plr.position.Y - localPlayerPos.Y;
            float dz = plr.position.Z - localPlayerPos.Z;
            plr.distance = sqrtf(dx * dx + dy * dy + dz * dz);
            if (plr.humanoidAddr)
                plr.health = memory->read<float>(plr.humanoidAddr + Offsets::Humanoid::Health);
        }
    }

    // Snapshot for render/aim (avoids holding lock during draw)
    inline std::vector<CachedPlayer> snapshotPlayers()
    {
        std::lock_guard<std::mutex> lock(playersMutex);
        return players;
    }

    inline RBX::Vec3 readPartPos(RBX::RbxInstance character, const char* name, bool& ok)
    {
        auto part = character.FindChild(name);
        if (part.Addr == 0) {
            ok = false;
            return {};
        }
        ok = true;
        return part.GetPos();
    }

    inline uintptr_t FindNamedAddr(std::vector<RBX::RbxInstance>& children, const char* name)
    {
        for (auto& ch : children) {
            if (ch.GetName() == name)
                return ch.Addr;
        }
        return 0;
    }

    inline void fillBoneParts(RBX::RbxInstance character, BoneParts& bp)
    {
        bp = {};
        if (!character.Addr) return;
        auto children = character.GetChildList();
        bp.head = FindNamedAddr(children, "Head");
        bp.hrp = FindNamedAddr(children, "HumanoidRootPart");
        bp.torso = FindNamedAddr(children, "Torso");
        bp.isR6 = (bp.torso != 0);
        if (bp.isR6) {
            bp.leftArm = FindNamedAddr(children, "Left Arm");
            bp.rightArm = FindNamedAddr(children, "Right Arm");
            bp.leftLeg = FindNamedAddr(children, "Left Leg");
            bp.rightLeg = FindNamedAddr(children, "Right Leg");
        }
        else {
            bp.upperTorso = FindNamedAddr(children, "UpperTorso");
            bp.lowerTorso = FindNamedAddr(children, "LowerTorso");
            bp.leftUpperArm = FindNamedAddr(children, "LeftUpperArm");
            bp.leftLowerArm = FindNamedAddr(children, "LeftLowerArm");
            bp.leftHand = FindNamedAddr(children, "LeftHand");
            bp.rightUpperArm = FindNamedAddr(children, "RightUpperArm");
            bp.rightLowerArm = FindNamedAddr(children, "RightLowerArm");
            bp.rightHand = FindNamedAddr(children, "RightHand");
            bp.leftUpperLeg = FindNamedAddr(children, "LeftUpperLeg");
            bp.leftLowerLeg = FindNamedAddr(children, "LeftLowerLeg");
            bp.leftFoot = FindNamedAddr(children, "LeftFoot");
            bp.rightUpperLeg = FindNamedAddr(children, "RightUpperLeg");
            bp.rightLowerLeg = FindNamedAddr(children, "RightLowerLeg");
            bp.rightFoot = FindNamedAddr(children, "RightFoot");
        }
        bp.ready = (bp.head != 0 || bp.hrp != 0);
    }

    inline RBX::Vec3 posOf(uintptr_t addr, bool& ok)
    {
        if (!addr) { ok = false; return {}; }
        ok = true;
        return RBX::RbxInstance(addr).GetPos();
    }

    inline void refreshBonePositions(const BoneParts& bp, BoneSet& bones)
    {
        if (!bp.ready) return;
        bool ok = false;
        bones = {};
        bones.isR6 = bp.isR6;
        bones.head = posOf(bp.head, ok); bones.hasHead = ok;
        bones.hrp = posOf(bp.hrp, ok); bones.hasHrp = ok;
        if (bp.isR6) {
            bones.torso = posOf(bp.torso, ok);
            bones.leftArm = posOf(bp.leftArm, ok);
            bones.rightArm = posOf(bp.rightArm, ok);
            bones.leftLeg = posOf(bp.leftLeg, ok);
            bones.rightLeg = posOf(bp.rightLeg, ok);
            return;
        }
        bones.upperTorso = posOf(bp.upperTorso, ok);
        bones.lowerTorso = posOf(bp.lowerTorso, ok);
        bones.leftUpperArm = posOf(bp.leftUpperArm, ok);
        bones.leftLowerArm = posOf(bp.leftLowerArm, ok);
        bones.leftHand = posOf(bp.leftHand, ok);
        bones.rightUpperArm = posOf(bp.rightUpperArm, ok);
        bones.rightLowerArm = posOf(bp.rightLowerArm, ok);
        bones.rightHand = posOf(bp.rightHand, ok);
        bones.leftUpperLeg = posOf(bp.leftUpperLeg, ok);
        bones.leftLowerLeg = posOf(bp.leftLowerLeg, ok);
        bones.leftFoot = posOf(bp.leftFoot, ok);
        bones.rightUpperLeg = posOf(bp.rightUpperLeg, ok);
        bones.rightLowerLeg = posOf(bp.rightLowerLeg, ok);
        bones.rightFoot = posOf(bp.rightFoot, ok);
    }

    inline void cacheBones(RBX::RbxInstance character, BoneSet& bones)
    {
        BoneParts bp;
        fillBoneParts(character, bp);
        refreshBonePositions(bp, bones);
    }

    inline void cacheBones(RBX::RbxInstance character, BoneSet& bones, BoneParts& bp)
    {
        fillBoneParts(character, bp);
        refreshBonePositions(bp, bones);
    }

    inline void updateplayers() {
        if (Globals::players.Addr == 0 || Globals::localPlayer.Addr == 0) {
            std::lock_guard<std::mutex> lock(playersMutex);
            players.clear();
            return;
        }

        auto playerList = Globals::players.GetChildList();

        auto localChar = Globals::localPlayer.GetModelRef();
        if (localChar.Addr == 0) {
            std::lock_guard<std::mutex> lock(playersMutex);
            players.clear();
            return;
        }

        auto localRoot = localChar.FindChild("HumanoidRootPart");
        if (localRoot.Addr == 0) {
            std::lock_guard<std::mutex> lock(playersMutex);
            players.clear();
            return;
        }

        RBX::Vec3 locPos = localRoot.GetPos();
        uintptr_t locTeam = memory->read<uintptr_t>(Globals::localPlayer.Addr + Offsets::Player::Team);
        RBX::Vec3 locLook = localPlayerLook;
        if (Globals::camera.Addr != 0) {
            auto cf = Globals::camera.GetCameraCFrame();
            locLook = cf.GetLookVector();
        }

        std::vector<CachedPlayer> updatedPlayers;
        updatedPlayers.reserve(playerList.size());

        const bool needBones = variables::ESP::skeleton || variables::Aimbot::enabled
            || variables::Aimbot::alwaysOn || variables::Misc::afkAssist
            || variables::Aimbot::aimTarget >= 2;

        for (auto& plr : playerList) {
            if (plr.Addr == Globals::localPlayer.Addr) continue;

            auto character = plr.GetModelRef();
            if (character.Addr == 0) continue;

            auto humanoid = character.FindChildByClass("Humanoid");
            if (humanoid.Addr == 0) continue;

            auto rootPart = character.FindChild("HumanoidRootPart");
            if (rootPart.Addr == 0) continue;

            auto head = character.FindChild("Head");

            float health = memory->read<float>(humanoid.Addr + Offsets::Humanoid::Health);
            if (variables::ESP::deadCheck && health <= 0.0f) continue;

            uintptr_t teamAddr = memory->read<uintptr_t>(plr.Addr + Offsets::Player::Team);
            if (variables::teamCheck && teamAddr != 0 && teamAddr == locTeam) continue;

            float distance = rootPart.CalcDistance(locPos);
            if (distance > variables::ESP::maxDistance && distance > variables::Aimbot::maxDistance)
                continue;

            CachedPlayer cachedPlayer{};
            cachedPlayer.playerAddr = plr.Addr;
            cachedPlayer.characterAddr = character.Addr;
            cachedPlayer.humanoidAddr = humanoid.Addr;
            cachedPlayer.rootPartAddr = rootPart.Addr;
            cachedPlayer.headAddr = head.Addr;
            cachedPlayer.teamAddr = teamAddr;
            cachedPlayer.userId = memory->read<int64_t>(plr.Addr + Offsets::Player::UserId);
            cachedPlayer.name = plr.GetName();
            cachedPlayer.displayName = plr.GetDisplayName();
            cachedPlayer.position = rootPart.GetPos();
            cachedPlayer.health = health;
            cachedPlayer.maxHealth = memory->read<float>(humanoid.Addr + Offsets::Humanoid::MaxHealth);
            cachedPlayer.distance = distance;
            cachedPlayer.isValid = true;

            if (variables::ESP::equippedItem) {
                for (auto& ch : character.GetChildList()) {
                    if (ch.GetClass() == "Tool") {
                        cachedPlayer.equippedTool = ch.GetName();
                        break;
                    }
                }
            }

            auto prim = rootPart.GetPrimitivePtr();
            if (prim != 0) {
                cachedPlayer.velocity = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
            }

            if (needBones) {
                cacheBones(character, cachedPlayer.bones, cachedPlayer.boneParts);
                if (!cachedPlayer.bones.hasHead && head.Addr != 0) {
                    cachedPlayer.bones.head = head.GetPos();
                    cachedPlayer.bones.hasHead = true;
                    cachedPlayer.boneParts.head = head.Addr;
                }
                if (!cachedPlayer.bones.hasHrp) {
                    cachedPlayer.bones.hrp = cachedPlayer.position;
                    cachedPlayer.bones.hasHrp = true;
                    cachedPlayer.boneParts.hrp = rootPart.Addr;
                }
                cachedPlayer.boneParts.ready = true;
            }
            else {
                if (head.Addr != 0) {
                    cachedPlayer.bones.head = head.GetPos();
                    cachedPlayer.bones.hasHead = true;
                }
                cachedPlayer.bones.hrp = cachedPlayer.position;
                cachedPlayer.bones.hasHrp = true;
                cachedPlayer.bones.isR6 = character.FindChild("Torso").Addr != 0;
                cachedPlayer.boneParts.head = head.Addr;
                cachedPlayer.boneParts.hrp = rootPart.Addr;
                cachedPlayer.boneParts.isR6 = cachedPlayer.bones.isR6;
                cachedPlayer.boneParts.ready = true;
            }

            updatedPlayers.push_back(std::move(cachedPlayer));
        }

        {
            std::lock_guard<std::mutex> lock(playersMutex);
            players = std::move(updatedPlayers);
            localPlayerPos = locPos;
            localPlayerTeam = locTeam;
            localPlayerLook = locLook;
        }
    }

    inline void EnsureCacheWorker()
    {
        bool expected = false;
        if (!cacheWorkerStarted.compare_exchange_strong(expected, true))
            return;
        std::thread([] {
            while (Globals::running) {
                if (!variables::Loading::active && Globals::players.Addr != 0)
                    updateplayers();
                int n = variables::Perf::playerUpdateEveryNFrames;
                if (n < 1) n = 1;
                // Aimbot wants fresher bones; ESP-only can idle longer
                bool hot = variables::Aimbot::enabled || variables::Aimbot::alwaysOn
                    || variables::Misc::afkAssist || variables::Rage::enabled;
                int ms = hot ? 16 : (16 * n);
                if (ms > 48) ms = 48;
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        }).detach();
    }
}
