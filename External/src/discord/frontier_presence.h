#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include "discord_ipc.h"

namespace FrontierPresence {

    inline constexpr const char* kAppId = "1534002486812348446";
    inline constexpr const char* kLargeImage = "frontier-discord-rpc";
    inline constexpr const char* kSmallImage = "frontier-discord-rpc";

    inline std::atomic<bool> gEnabled{ true };
    inline std::atomic<bool> gRunning{ false };
    inline int64_t gStartTs = 0;

    inline int64_t NowUnix()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    inline void Push(const char* details, const char* state, int partySize = 0, int partyMax = 0)
    {
        if (!gEnabled.load() || !DiscordIPC::IsConnected())
            return;

        DiscordIPC::Activity act{};
        act.details = details;
        act.state = state;
        act.largeImageKey = kLargeImage;
        act.largeImageText = "FRONTIER";
        act.smallImageKey = kSmallImage;
        act.smallImageText = "Roblox";
        act.startTimestamp = gStartTs > 0 ? gStartTs : NowUnix();
        act.partySize = partySize;
        act.partyMax = partyMax;
        DiscordIPC::SetActivity(act);
    }

    inline void SetWaiting()
    {
        Push("FRONTIER", "Waiting for Roblox…");
    }

    inline void SetLoading(const char* step)
    {
        char details[96];
        sprintf_s(details, "Loading — %s", step ? step : "…");
        Push(details, "FRONTIER External");
    }

    inline void SetInGame(const char* gameName, int64_t placeId, int players, int maxPlayers)
    {
        char details[96];
        char state[96];
        if (gameName && gameName[0]) {
            sprintf_s(details, "Playing %s", gameName);
        } else {
            strncpy_s(details, "In game", _TRUNCATE);
        }
        if (placeId > 0)
            sprintf_s(state, "Place %lld", (long long)placeId);
        else
            strncpy_s(state, "Roblox", _TRUNCATE);

        int partyMax = maxPlayers > 0 ? maxPlayers : 0;
        int partySize = players > 0 ? players : 0;
        Push(details, state, partySize, partyMax);
    }

    inline void StartWorker()
    {
        if (gRunning.exchange(true))
            return;

        gStartTs = NowUnix();
        std::thread([]() {
            if (!DiscordIPC::Connect(kAppId)) {
                gRunning.store(false);
                return;
            }
            Push("FRONTIER", "Starting…");
            while (gRunning.load()) {
                DiscordIPC::Pump();
                if (!DiscordIPC::IsConnected()) {
                    DiscordIPC::Connect(kAppId);
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
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
