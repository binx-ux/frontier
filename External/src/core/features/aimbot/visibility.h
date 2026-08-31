#pragma once
#include "../../../sdk/sdk.h"
#include "../../../sdk/offsets.h"
#include "../../../core/globals/globals.h"
#include "../../../core/variables/variables.h"
#include "../../../memory/memory.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <algorithm>

// External wall-check (no engine Raycast API).
// Research takeaways that broke the old version:
//  1) Axis-aligned boxes ignore part Rotation → thin/rotated walls are missed → aim through walls.
//  2) Roblox spatial queries hit CanCollide parts (and CanQuery when CanCollide is off).
//  3) Correct test is ray vs OBB: transform ray into part local space, then slab-test Size/2.
//  4) Aim scripts only treat a target as visible if the ray to THAT bone is clear (not head OR chest).
namespace Visibility {

    struct OBB {
        RBX::Vec3 center{};
        RBX::Vec3 axisX{}, axisY{}, axisZ{}; // orthonormal (Right, Up, Look)
        RBX::Vec3 half{};                    // half extents
        // World AABB for broadphase
        RBX::Vec3 wmin{}, wmax{};

        bool Valid() const {
            return half.X > 0.01f && half.Y > 0.01f && half.Z > 0.01f;
        }
    };

    inline std::vector<OBB> boxes;
    inline std::mutex boxesMutex;
    inline std::atomic<bool> everBuilt{ false };
    inline std::atomic<bool> building{ false };
    inline std::atomic<bool> workerStarted{ false };
    inline std::atomic<bool> rebuildRequested{ false };
    inline std::atomic<int> boxCount{ 0 };
    inline uintptr_t lastWorkspace = 0;
    inline auto lastRefresh = std::chrono::steady_clock::now() - std::chrono::seconds(30);

    inline float Dot(const RBX::Vec3& a, const RBX::Vec3& b) {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

    inline RBX::Vec3 NormalizeSafe(RBX::Vec3 v) {
        float len = sqrtf(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
        if (len < 1e-8f) return { 1, 0, 0 };
        v.X /= len; v.Y /= len; v.Z /= len;
        return v;
    }

    inline void ExpandWorldAABB(OBB& b)
    {
        // 8 corners of the OBB → world AABB (tight broadphase)
        float sx[2] = { -b.half.X, b.half.X };
        float sy[2] = { -b.half.Y, b.half.Y };
        float sz[2] = { -b.half.Z, b.half.Z };
        b.wmin = { 1e12f, 1e12f, 1e12f };
        b.wmax = { -1e12f, -1e12f, -1e12f };
        for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
        for (int k = 0; k < 2; k++) {
            RBX::Vec3 p{
                b.center.X + b.axisX.X * sx[i] + b.axisY.X * sy[j] + b.axisZ.X * sz[k],
                b.center.Y + b.axisX.Y * sx[i] + b.axisY.Y * sy[j] + b.axisZ.Y * sz[k],
                b.center.Z + b.axisX.Z * sx[i] + b.axisY.Z * sy[j] + b.axisZ.Z * sz[k],
            };
            if (p.X < b.wmin.X) b.wmin.X = p.X; if (p.X > b.wmax.X) b.wmax.X = p.X;
            if (p.Y < b.wmin.Y) b.wmin.Y = p.Y; if (p.Y > b.wmax.Y) b.wmax.Y = p.Y;
            if (p.Z < b.wmin.Z) b.wmin.Z = p.Z; if (p.Z > b.wmax.Z) b.wmax.Z = p.Z;
        }
    }

    inline bool SegOverlapsAABB(const RBX::Vec3& a, const RBX::Vec3& b, const RBX::Vec3& mn, const RBX::Vec3& mx, float pad)
    {
        float sminX = (a.X < b.X ? a.X : b.X) - pad;
        float smaxX = (a.X > b.X ? a.X : b.X) + pad;
        float sminY = (a.Y < b.Y ? a.Y : b.Y) - pad;
        float smaxY = (a.Y > b.Y ? a.Y : b.Y) + pad;
        float sminZ = (a.Z < b.Z ? a.Z : b.Z) - pad;
        float smaxZ = (a.Z > b.Z ? a.Z : b.Z) + pad;
        if (mx.X < sminX || mn.X > smaxX) return false;
        if (mx.Y < sminY || mn.Y > smaxY) return false;
        if (mx.Z < sminZ || mn.Z > smaxZ) return false;
        return true;
    }

    // Ray vs local AABB (slab). dir must be unit-ish; returns true if hit in [0, maxT].
    inline bool RayHitsLocalAABB(const RBX::Vec3& o, const RBX::Vec3& d, float maxT,
        float hx, float hy, float hz, float& outT)
    {
        float tmin = 0.0f, tmax = maxT;
        const float oA[3] = { o.X, o.Y, o.Z };
        const float dA[3] = { d.X, d.Y, d.Z };
        const float mn[3] = { -hx, -hy, -hz };
        const float mx[3] = { hx, hy, hz };
        for (int i = 0; i < 3; i++) {
            if (fabsf(dA[i]) < 1e-8f) {
                if (oA[i] < mn[i] || oA[i] > mx[i]) return false;
                continue;
            }
            float inv = 1.0f / dA[i];
            float t1 = (mn[i] - oA[i]) * inv;
            float t2 = (mx[i] - oA[i]) * inv;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
        // Inside start → blocked immediately
        if (tmin < 0.0f) {
            if (tmax >= 0.0f) { outT = 0.0f; return true; }
            return false;
        }
        outT = tmin;
        return tmin <= maxT;
    }

    inline bool RayHitsOBB(const RBX::Vec3& origin, const RBX::Vec3& dir, float maxT, const OBB& b, float& outT)
    {
        // World → local (dot with axes)
        RBX::Vec3 rel{ origin.X - b.center.X, origin.Y - b.center.Y, origin.Z - b.center.Z };
        RBX::Vec3 oL{ Dot(rel, b.axisX), Dot(rel, b.axisY), Dot(rel, b.axisZ) };
        RBX::Vec3 dL{ Dot(dir, b.axisX), Dot(dir, b.axisY), Dot(dir, b.axisZ) };
        return RayHitsLocalAABB(oL, dL, maxT, b.half.X, b.half.Y, b.half.Z, outT);
    }

    inline bool PartBlocksRays(uintptr_t prim, uintptr_t instAddr = 0)
    {
        (void)instAddr;
        if (!prim) return false;
        uint8_t flags = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
        return (flags & Offsets::PrimitiveFlags::CanCollide) != 0;
    }

    inline bool IsWallLike(const RBX::Vec3& size)
    {
        float mn = size.X; if (size.Y < mn) mn = size.Y; if (size.Z < mn) mn = size.Z;
        float mx = size.X; if (size.Y > mx) mx = size.Y; if (size.Z > mx) mx = size.Z;
        float vol = size.X * size.Y * size.Z;
        // Thin panels / long walls / solid props — keep low volume floor so thin walls aren't dropped
        if (vol < 0.008f) return false;
        if (vol > 250000.0f) return false;
        if (mx > 800.f) return false;
        if (mx < 0.15f) return false;
        return true;
    }

    inline bool MakeOBB(uintptr_t prim, OBB& out)
    {
        RBX::Vec3 size = memory->read<RBX::Vec3>(prim + Offsets::Primitive::Size);
        if (!IsWallLike(size)) return false;

        RBX::CFrame cf = memory->read<RBX::CFrame>(prim + Offsets::Primitive::Rotation);
        RBX::Vec3 pos = memory->read<RBX::Vec3>(prim + Offsets::Primitive::Position);
        // Prefer embedded CFrame position when present
        RBX::Vec3 cfPos = cf.GetPosition();
        if (fabsf(cfPos.X) + fabsf(cfPos.Y) + fabsf(cfPos.Z) > 0.01f)
            pos = cfPos;

        out.center = pos;
        out.axisX = NormalizeSafe(cf.GetRightVector());
        out.axisY = NormalizeSafe(cf.GetUpVector());
        out.axisZ = NormalizeSafe(cf.GetLookVector());
        // Slight inflate so we don't leak through seams / float error
        out.half = { size.X * 0.5f * 1.02f, size.Y * 0.5f * 1.02f, size.Z * 0.5f * 1.02f };
        ExpandWorldAABB(out);
        return out.Valid();
    }

    inline void CollectParts(RBX::RbxInstance inst, int depth, int& count, std::vector<OBB>& out)
    {
        if (inst.Addr == 0 || depth > 22 || count >= 15000) return;
        std::string cls = inst.GetClass();
        if (cls.empty()) return;

        // Skip non-geometry / characters
        if (cls == "Humanoid" || cls == "Camera" || cls == "Terrain" ||
            cls == "Players" || cls == "Sound" || cls == "Script" || cls == "LocalScript" ||
            cls == "ModuleScript" || cls == "Beam" || cls == "Trail" || cls == "ParticleEmitter" ||
            cls == "Attachment" || cls == "Bone" || cls == "Motor6D" || cls == "Weld" ||
            cls == "Decal" || cls == "Texture" || cls == "Fire" || cls == "Smoke" ||
            cls == "Sparkles" || cls == "PointLight" || cls == "SpotLight" || cls == "SurfaceLight")
            return;

        if (cls == "Model" && inst.FindChildByClass("Humanoid").Addr != 0)
            return;

        if (cls == "Part" || cls == "MeshPart" || cls == "UnionOperation" || cls == "WedgePart" ||
            cls == "TrussPart" || cls == "CornerWedgePart" || cls == "SpawnLocation" ||
            cls == "Seat" || cls == "VehicleSeat" || cls == "PartOperation") {
            auto prim = inst.GetPrimitivePtr();
            if (prim && PartBlocksRays(prim, inst.Addr)) {
                OBB box;
                if (MakeOBB(prim, box)) {
                    out.push_back(box);
                    count++;
                }
            }
            return; // parts don't nest geometry we care about
        }

        for (auto& ch : inst.GetChildList()) {
            CollectParts(ch, depth + 1, count, out);
            if (count >= 15000) return;
        }
    }

    inline void BuildNow()
    {
        if (Globals::workspace.Addr == 0) return;

        if (Globals::workspace.Addr != lastWorkspace) {
            lastWorkspace = Globals::workspace.Addr;
            std::lock_guard<std::mutex> lock(boxesMutex);
            boxes.clear();
            everBuilt = false;
            boxCount = 0;
        }

        std::vector<OBB> next;
        next.reserve(15000);
        int count = 0;

        // Prefer map folders first (often named Map / Arena / Stages) then everything else
        auto children = Globals::workspace.GetChildList();
        std::vector<RBX::RbxInstance> priority, rest;
        for (auto& ch : children) {
            std::string c = ch.GetClass();
            if (c == "Camera" || c == "Terrain") continue;
            if (ch.FindChildByClass("Humanoid").Addr != 0) continue;
            std::string n = ch.GetName();
            bool pri = false;
            if (!n.empty()) {
                // case-insensitive-ish cheap checks
                if (n.find("Map") != std::string::npos || n.find("map") != std::string::npos ||
                    n.find("Arena") != std::string::npos || n.find("Stage") != std::string::npos ||
                    n.find("World") != std::string::npos || n.find("Baseplate") != std::string::npos ||
                    n.find("Building") != std::string::npos || n.find("House") != std::string::npos ||
                    n.find("Terrain") != std::string::npos || n.find("Geometry") != std::string::npos ||
                    n.find("Environment") != std::string::npos || n.find("Level") != std::string::npos ||
                    n.find("Lobby") != std::string::npos || n.find("Zone") != std::string::npos)
                    pri = true;
            }
            if (c == "Folder" || c == "Model") pri = true;
            (pri ? priority : rest).push_back(ch);
        }
        for (auto& ch : priority) {
            CollectParts(ch, 1, count, next);
            if (count >= 15000) break;
        }
        if (count < 15000) {
            for (auto& ch : rest) {
                CollectParts(ch, 1, count, next);
                if (count >= 15000) break;
            }
        }

        // Prefer larger / flatter occluders first for early-out during ray tests
        std::sort(next.begin(), next.end(), [](const OBB& a, const OBB& b) {
            float va = a.half.X * a.half.Y * a.half.Z;
            float vb = b.half.X * b.half.Y * b.half.Z;
            return va > vb;
        });

        {
            std::lock_guard<std::mutex> lock(boxesMutex);
            boxes = std::move(next);
            everBuilt = !boxes.empty();
            boxCount = (int)boxes.size();
        }
    }

    inline void EnsureWorker()
    {
        bool expected = false;
        if (!workerStarted.compare_exchange_strong(expected, true))
            return;
        std::thread([] {
            while (Globals::running) {
                // Keep wall mesh warm automatically whenever features are in use
                bool need = variables::Aimbot::enabled || variables::Trigger::enabled
                    || variables::ESP::enabled || variables::ESP::visibleOnly
                    || variables::Aimbot::requireVisible || variables::Trigger::requireVisible
                    || variables::Misc::afkAssist || variables::Aimbot::alwaysOn
                    || variables::MagicBullet::enabled || variables::Hitbox::enabled;
                if (!need) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }
                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(now - lastRefresh).count();
                float interval = everBuilt ? 0.75f : 0.2f;
                bool force = rebuildRequested.exchange(false);
                if ((force || elapsed >= interval) && !building.exchange(true)) {
                    lastRefresh = now;
                    BuildNow();
                    building = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        }).detach();
    }

    // true = clear shot, false = blocked / unknown
    inline bool HasLineOfSight(const RBX::Vec3& from, const RBX::Vec3& to)
    {
        EnsureWorker();

        RBX::Vec3 dir{ to.X - from.X, to.Y - from.Y, to.Z - from.Z };
        float len = sqrtf(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z);
        if (len < 0.4f) return true;
        dir.X /= len; dir.Y /= len; dir.Z /= len;

        // Stop short of the target bone so we don't self-hit their character.
        float maxT = len - 0.35f;
        if (maxT < 0.15f) return true;

        std::lock_guard<std::mutex> lock(boxesMutex);

        // Mesh still building — fail open so aim isn't dead for the first seconds
        if (!everBuilt || boxes.empty()) {
            rebuildRequested.store(true);
            return true;
        }

        for (const auto& b : boxes) {
            if (!b.Valid()) continue;
            if (!SegOverlapsAABB(from, to, b.wmin, b.wmax, 1.5f)) continue;

            float t = 0.f;
            if (RayHitsOBB(from, dir, maxT, b, t)) {
                if (t >= 0.5f && t <= maxT)
                    return false;
            }
        }
        return true;
    }

    inline void ForceRebuild()
    {
        if (building.exchange(true)) return;
        everBuilt = false;
        BuildNow();
        building = false;
        lastRefresh = std::chrono::steady_clock::now();
    }

    // Kick a rebuild on the worker — never block the render thread
    inline void RequestRebuild()
    {
        rebuildRequested.store(true);
        EnsureWorker();
    }
}
