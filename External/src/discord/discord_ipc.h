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
        const char* button1Label = nullptr;
        const char* button1Url = nullptr;
        const char* button2Label = nullptr;
        const char* button2Url = nullptr;
    };

    bool Connect(const char* applicationId);
    void Disconnect();
    bool IsConnected();

    bool SetActivity(const Activity& activity);
    bool ClearActivity();
    void Pump(int maxFrames = 16);

}
