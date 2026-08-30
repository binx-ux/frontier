#pragma once
#include "../../sdk/sdk.h"
#include "../../sdk/offsets.h"
#include "../../core/globals/globals.h"
#include "../../memory/memory.h"
#include <cstdint>

// Game detection for optional compatibility tuning — not a whitelist.
namespace Games {

    constexpr int64_t kArsenalPlaceId = 286090429LL;
    constexpr int64_t kArsenalUniverseId = 111958650LL;

    constexpr int64_t kMiscGunTestUniverseId = 3437409320LL;
    constexpr int64_t kMiscGunTestRootPlaceId = 9157605735LL;
    constexpr int64_t kMiscGunTestPlaces[] = {
        9157605735LL,
        11575563846LL,
        11671999447LL,
    };

    constexpr int64_t kBaseplatePlaceId = 95206881LL;

    constexpr int64_t kBloxStrikePlaceId = 114234929420007LL;
    constexpr int64_t kBloxStrikeUniverseId = 7633926880LL;

    enum class Kind : int {
        None = 0,
        Generic,
        Arsenal,
        MiscGunTest,
        Baseplate,
        BloxStrike,
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

    inline bool IsGameLoaded()
    {
        if (!Globals::dataModel.Addr) return false;
        return memory->read<uint8_t>(Globals::dataModel.Addr + Offsets::DataModel::GameLoaded) != 0;
    }

    inline bool HasWeaponsFolder()
    {
        if (!Globals::dataModel.Addr) return false;
        auto rs = Globals::dataModel.FindChildByClass("ReplicatedStorage");
        if (!rs.Addr) rs = Globals::dataModel.FindChild("ReplicatedStorage");
        if (!rs.Addr) return false;
        if (rs.FindChild("Weapons").Addr != 0) return true;
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

    inline bool IsBaseplatePlace(int64_t place, int64_t /*gameId*/)
    {
        return place == kBaseplatePlaceId;
    }

    inline bool IsBloxStrikePlace(int64_t place, int64_t gameId)
    {
        if (place == kBloxStrikePlaceId) return true;
        if (gameId == kBloxStrikeUniverseId) return true;
        return false;
    }

    inline Kind Detect()
    {
        const int64_t place = ReadPlaceId();
        const int64_t gameId = ReadGameId();
        if (place <= 0) return Kind::None;
        if (IsArsenalPlace(place, gameId)) return Kind::Arsenal;
        if (IsMiscGunTestPlace(place, gameId)) return Kind::MiscGunTest;
        if (IsBloxStrikePlace(place, gameId)) return Kind::BloxStrike;
        if (IsBaseplatePlace(place, gameId)) return Kind::Baseplate;
        return Kind::Generic;
    }

    // Universal — any loaded Roblox experience with a valid place id.
    inline bool IsSupported()
    {
        return ReadPlaceId() > 0 && IsGameLoaded();
    }

    inline bool IsBloxStrike()
    {
        return Detect() == Kind::BloxStrike;
    }

    inline const char* Name()
    {
        switch (Detect()) {
        case Kind::Arsenal: return "Arsenal";
        case Kind::MiscGunTest: return "MiscGunTest:X";
        case Kind::BloxStrike: return "BloxStrike";
        case Kind::Baseplate: return "Baseplate";
        case Kind::Generic: return "Roblox";
        default: return "Roblox";
        }
    }

    inline const char* SupportedListShort()
    {
        return "any Roblox game";
    }
}

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

    inline bool IsSupportedPlace()
    {
        return Games::IsSupported();
    }
}
