#pragma once
#include "../../sdk/sdk.h"
#include "../../sdk/offsets.h"
#include "../../core/globals/globals.h"
#include "../../memory/memory.h"
#include <cstdint>

// Supported FPS experiences for Match-Ware External.
namespace Games {

    // https://www.roblox.com/games/286090429/Arsenal
    constexpr int64_t kArsenalPlaceId = 286090429LL;
    constexpr int64_t kArsenalUniverseId = 111958650LL; // game.GameId

    // https://www.roblox.com/games/9157605735/MiscGunTest-X
    // PlaceId 9157605735 → UniverseId (GameId) 3437409320
    constexpr int64_t kMiscGunTestUniverseId = 3437409320LL;
    constexpr int64_t kMiscGunTestRootPlaceId = 9157605735LL;
    // Known MiscGunTest:X places under the same universe
    constexpr int64_t kMiscGunTestPlaces[] = {
        9157605735LL,   // root
        11575563846LL,  // VC Server
        11671999447LL,  // Zombies
    };

    // https://www.roblox.com/games/95721658376580 — [MTC] Multicrew Tank Combat
    constexpr int64_t kMtcPlaceId = 95721658376580LL;

    enum class Kind : int {
        None = 0,
        Arsenal,
        MiscGunTest,
        MTC,
    };

    inline int64_t ReadPlaceId()
    {
        if (!Globals::dataModel.Addr) return 0;
        return memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId);
    }

    inline int64_t ReadGameId()
    {
        if (!Globals::dataModel.Addr) return 0;
        return memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::GameId);
    }

    inline bool HasWeaponsFolder()
    {
        if (!Globals::dataModel.Addr) return false;
        auto rs = Globals::dataModel.FindChildByClass("ReplicatedStorage");
        if (!rs.Addr) rs = Globals::dataModel.FindChild("ReplicatedStorage");
        if (!rs.Addr) return false;
        if (rs.FindChild("Weapons").Addr != 0) return true;
        // MiscGunTest variants sometimes nest under Gun / Guns
        if (rs.FindChild("Gun").Addr != 0) return true;
        if (rs.FindChild("Guns").Addr != 0) return true;
        return false;
    }

    inline bool IsMiscGunTestPlace(int64_t place, int64_t gameId)
    {
        if (gameId == kMiscGunTestUniverseId) return true;
        for (int64_t p : kMiscGunTestPlaces) {
            if (place == p) return true;
        }
        return false;
    }

    inline bool IsArsenalPlace(int64_t place, int64_t gameId)
    {
        if (place == kArsenalPlaceId) return true;
        if (gameId == kArsenalUniverseId) return true;
        return false;
    }

    inline bool IsMtcPlace(int64_t place, int64_t /*gameId*/)
    {
        return place == kMtcPlaceId;
    }

    inline Kind Detect()
    {
        const int64_t place = ReadPlaceId();
        const int64_t gameId = ReadGameId();
        if (IsArsenalPlace(place, gameId)) return Kind::Arsenal;
        if (IsMiscGunTestPlace(place, gameId)) return Kind::MiscGunTest;
        if (IsMtcPlace(place, gameId)) return Kind::MTC;
        return Kind::None;
    }

    inline bool IsSupported()
    {
        return Detect() != Kind::None;
    }

    inline const char* Name()
    {
        switch (Detect()) {
        case Kind::Arsenal: return "Arsenal";
        case Kind::MiscGunTest: return "MiscGunTest:X";
        case Kind::MTC: return "MTC";
        default: return "Unsupported";
        }
    }

    inline const char* SupportedListShort()
    {
        return "Arsenal, MiscGunTest:X, or MTC";
    }

    inline bool IsMTC() { return Detect() == Kind::MTC; }
}

// Back-compat aliases used across the codebase
namespace Arsenal {
    constexpr int64_t kPlaceId = Games::kArsenalPlaceId;
    constexpr int64_t kUniverseId = Games::kArsenalUniverseId;

    inline int64_t ReadPlaceId() { return Games::ReadPlaceId(); }
    inline int64_t ReadGameId() { return Games::ReadGameId(); }
    inline bool HasWeaponsFolder() { return Games::HasWeaponsFolder(); }

    inline bool IsArsenalPlace()
    {
        return Games::Detect() == Games::Kind::Arsenal;
    }

    // Prefer this for attach / feature gates
    inline bool IsSupportedPlace()
    {
        return Games::IsSupported();
    }
}
