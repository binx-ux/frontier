#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include "discord_ipc.h"

namespace FrontierPresence {

    inline constexpr const char* kAppId = "1534002486812348446";
    inline constexpr const char* kLargeImage = "frontier-discord-rpc";
    inline constexpr const char* kSmallImage = "frontier-discord-rpc";

    inline std::atomic<bool> gEnabled{ true };
    inline std::atomic<bool> gRunning{ false };
    inline int64_t gStartTs = 0;

    struct PendingActivity {
        char details[96]{ "FRONTIER" };
        char state[96]{ "Starting…" };
        int partySize = 0;
        int partyMax = 0;
    };

    inline std::mutex gPendingMutex;
    inline PendingActivity gPending{};
    inline std::atomic<bool> gPendingDirty{ true };

    inline int64_t NowUnix()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    inline void Queue(const char* details, const char* state, int partySize = 0, int partyMax = 0)
    {
        {
            std::lock_guard<std::mutex> lock(gPendingMutex);
            if (details && details[0])
                strncpy_s(gPending.details, details, _TRUNCATE);
            if (state && state[0])
                strncpy_s(gPending.state, state, _TRUNCATE);
            gPending.partySize = partySize;
            gPending.partyMax = partyMax;
        }
        gPendingDirty.store(true);
    }

    inline bool FlushLocked()
    {
        if (!gEnabled.load())
            return false;

        if (!DiscordIPC::IsConnected() && !DiscordIPC::Connect(kAppId))
            return false;

        PendingActivity copy{};
        {
            std::lock_guard<std::mutex> lock(gPendingMutex);
            copy = gPending;
        }

        DiscordIPC::Activity act{};
        act.details = copy.details;
        act.state = copy.state;
        act.largeImageKey = kLargeImage;
        act.largeImageText = "FRONTIER";
        act.smallImageKey = kSmallImage;
        act.smallImageText = "Roblox";
        act.startTimestamp = gStartTs > 0 ? gStartTs : NowUnix();
        act.partySize = copy.partySize;
        act.partyMax = copy.partyMax;

        if (DiscordIPC::SetActivity(act)) {
            gPendingDirty.store(false);
            return true;
        }
        DiscordIPC::Disconnect();
        return false;
    }

    inline void SetWaiting()
    {
        Queue("FRONTIER", "Waiting for Roblox…");
    }

    inline void SetLoading(const char* step)
    {
        char details[96];
        sprintf_s(details, "Loading — %s", step ? step : "…");
        Queue(details, "FRONTIER External");
    }

    inline void SetInGame(const char* gameName, int64_t placeId, int players, int maxPlayers)
    {
        char details[96];
        char state[96];
        if (gameName && gameName[0])
            sprintf_s(details, "Playing %s", gameName);
        else
            strncpy_s(details, "In game", _TRUNCATE);

        if (placeId > 0)
            sprintf_s(state, "Place %lld", (long long)placeId);
        else if (players > 0)
            sprintf_s(state, "%d players", players);
        else
            strncpy_s(state, "Roblox", _TRUNCATE);

        Queue(details, state, players, maxPlayers);
    }

    inline void StartWorker()
    {
        if (gRunning.exchange(true))
            return;

        gStartTs = NowUnix();
        Queue("FRONTIER", "Starting…");

        std::thread([]() {
            while (gRunning.load()) {
                if (gEnabled.load() && (gPendingDirty.load() || !DiscordIPC::IsConnected())) {
                    FlushLocked();
                }
                DiscordIPC::Pump();
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            }
            DiscordIPC::Disconnect();
        }).detach();
    }

    inline void Stop()
    {
        gRunning.store(false);
        DiscordIPC::Disconnect();
    }

}
