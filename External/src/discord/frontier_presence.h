#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include "../core/variables/variables.h"
#include "../core/globals/globals.h"
#include "../core/games/arsenal.h"
#include "../core/cache/cache.h"
#include "../memory/memory.h"
#include "../sdk/offsets.h"
#include "../sdk/window_manager.h"
#include "../render/brand.h"
#include "discord_ipc.h"

namespace FrontierPresence {

    inline constexpr const char* kAppId = "1534002486812348446";
    inline constexpr const char* kLargeImage = "frontier-discord-rpc";
    inline constexpr const char* kSmallImage = "roblox";
    inline constexpr const char* kWebsiteUrl = "https://trace-host.vercel.app/";
    inline constexpr const char* kDiscordUrl = "https://discord.gg/zHGKqd92Pz";

    inline std::atomic<bool> gEnabled{ false };
    inline std::atomic<bool> gRunning{ false };
    inline int64_t gStartTs = 0;
    inline int64_t gLastPlaceId = 0;

    struct PendingActivity {
        char details[128]{};
        char state[128]{ "Starting…" };
        int partySize = 0;
        int partyMax = 0;

        PendingActivity()
        {
            strncpy_s(details, Frontier::kName, _TRUNCATE);
        }
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
        act.largeImageText = Frontier::kName;
        act.smallImageKey = kSmallImage;
        act.smallImageText = "Roblox";
        act.startTimestamp = gStartTs > 0 ? gStartTs : NowUnix();
        act.partySize = copy.partySize;
        act.partyMax = copy.partyMax;
        act.button1Label = "Website";
        act.button1Url = kWebsiteUrl;
        act.button2Label = "Discord";
        act.button2Url = kDiscordUrl;

        DiscordIPC::Pump(8);
        if (DiscordIPC::SetActivity(act)) {
            gPendingDirty.store(false);
            return true;
        }

        // Some Discord app configs reject buttons; retry without them.
        act.button1Label = act.button1Url = nullptr;
        act.button2Label = act.button2Url = nullptr;
        if (DiscordIPC::SetActivity(act)) {
            gPendingDirty.store(false);
            return true;
        }

        DiscordIPC::Disconnect();
        return false;
    }

    inline void ClearPresence()
    {
        if (DiscordIPC::IsConnected())
            DiscordIPC::ClearActivity();
        gPendingDirty.store(false);
    }

    inline void SyncEnabled(bool enabled)
    {
        const bool was = gEnabled.exchange(enabled);
        if (enabled) {
            if (gStartTs <= 0)
                gStartTs = NowUnix();
            gPendingDirty.store(true);
        } else if (was) {
            ClearPresence();
            DiscordIPC::Disconnect();
        }
    }

    inline void SetWaiting()
    {
        Queue(Frontier::kName, "Waiting for Roblox…");
    }

    inline void SetLoading(const char* step)
    {
        char state[128];
        if (step && step[0])
            sprintf_s(state, "Loading — %s", step);
        else
            strncpy_s(state, "Loading session…", _TRUNCATE);
        Queue(Frontier::kName, state);
    }

    inline void SetInGame(const char* gameName, int64_t placeId, int players, int partyMax = 0)
    {
        char state[128];

        if (gameName && gameName[0] && _stricmp(gameName, "Roblox") != 0)
            sprintf_s(state, "Playing %s", gameName);
        else if (placeId > 0)
            sprintf_s(state, "Playing Roblox");
        else
            strncpy_s(state, "In Roblox", _TRUNCATE);

        const char* user = variables::Status::username;
        char suffix[96]{};
        if (user && user[0] && _stricmp(user, "—") != 0) {
            if (placeId > 0)
                sprintf_s(suffix, "  •  @%s  •  Place %lld", user, (long long)placeId);
            else if (players > 0)
                sprintf_s(suffix, "  •  @%s  •  %d players nearby", user, players);
            else
                sprintf_s(suffix, "  •  %s", user);
        } else if (placeId > 0) {
            sprintf_s(suffix, "  •  Place %lld", (long long)placeId);
        } else if (players > 0) {
            sprintf_s(suffix, "  •  %d players in server", players);
        }

        if (suffix[0] && strlen(state) + strlen(suffix) + 1 < sizeof(state))
            strcat_s(state, suffix);

        int maxParty = partyMax;
        if (maxParty <= 0 && players > 0)
            maxParty = players < 12 ? 12 : (players < 24 ? 24 : 50);

        Queue(Frontier::kName, state, players > 0 ? players : 0, maxParty);
        gLastPlaceId = placeId;
    }

    inline void UpdateFromSession()
    {
        if (!gEnabled.load())
            return;

        if (!WindowManager::IsRobloxOpen() && memory->find_process_id("RobloxPlayerBeta.exe") == 0) {
            SetWaiting();
            return;
        }

        if (!Globals::dataModel.Addr || !Games::IsGameLoaded()) {
            Queue(Frontier::kName, "In Roblox — joining…");
            gPendingDirty.store(true);
            return;
        }

        const int64_t placeId = Games::ReadPlaceId();
        int players = 0;
        for (auto& p : PlayerCache::snapshotPlayers()) {
            if (p.isValid && p.health > 0.f)
                ++players;
        }

        if (placeId != gLastPlaceId)
            gStartTs = NowUnix();

        SetInGame(Games::Name(), placeId, players, 0);

        if (variables::menuOpen || variables::Misc::floatingPanelOpen) {
            char state[128];
            strncpy_s(state, gPending.state, _TRUNCATE);
            if (strlen(state) + 18 < sizeof(state)) {
                strcat_s(state, "  •  Menu open");
                Queue(gPending.details, state, gPending.partySize, gPending.partyMax);
            }
        }
    }

    inline void StartWorker()
    {
        if (gRunning.exchange(true))
            return;

        if (gStartTs <= 0)
            gStartTs = NowUnix();
        Queue(Frontier::kName, "Starting…");

        std::thread([]() {
            int failStreak = 0;
            while (gRunning.load()) {
                if (gEnabled.load()) {
                    if (gPendingDirty.load() || !DiscordIPC::IsConnected()) {
                        if (FlushLocked())
                            failStreak = 0;
                        else
                            ++failStreak;
                    }
                    DiscordIPC::Pump(16);
                } else {
                    DiscordIPC::Pump(4);
                }

                int sleepMs = 800;
                if (!DiscordIPC::IsConnected())
                    sleepMs = failStreak > 4 ? 2500 : 1200;
                else if (!gPendingDirty.load())
                    sleepMs = 2000;

                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            }
            ClearPresence();
            DiscordIPC::Disconnect();
        }).detach();
    }

    inline void Stop()
    {
        gRunning.store(false);
        ClearPresence();
        DiscordIPC::Disconnect();
    }

}
