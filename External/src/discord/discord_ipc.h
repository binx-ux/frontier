#pragma once
#include <cstdint>

namespace DiscordIPC {

    struct Activity {
        const char* details = nullptr;
        const char* state = nullptr;
        const char* largeImageKey = nullptr;
        const char* largeImageText = nullptr;
        const char* smallImageKey = nullptr;
        const char* smallImageText = nullptr;
        int64_t startTimestamp = 0;
        int partySize = 0;
        int partyMax = 0;
    };

    bool Connect(const char* applicationId);
    void Disconnect();
    bool IsConnected();

    bool SetActivity(const Activity& activity);
    void Pump();

}
