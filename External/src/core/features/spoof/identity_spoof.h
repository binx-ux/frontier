#pragma once
#include "../../variables/variables.h"
#include "../../globals/globals.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/sdk.h"
#include "../../../memory/memory.h"
#include <cstdio>
#include <cstring>

// Client-side Roblox identity spoof — patches local Player memory for UI/ESP/tools.
// Does NOT change your real account on Roblox servers.
namespace IdentitySpoof {

    inline int64_t savedUserId = 0;
    inline bool userIdSaved = false;
    inline char savedDisplayName[96]{};
    inline bool displaySaved = false;

    inline bool AnyEnabled()
    {
        return variables::Spoof::userIdEnabled
            || variables::Spoof::displayNameEnabled
            || variables::Spoof::usernameEnabled;
    }

    inline void Restore()
    {
        if (!Globals::localPlayer.Addr || !memory || !memory->IsConnected())
            return;

        if (userIdSaved) {
            memory->write<int64_t>(
                Globals::localPlayer.Addr + Offsets::Player::UserId, savedUserId);
            userIdSaved = false;
        }
        if (displaySaved && savedDisplayName[0]) {
            RBX::WriteString(
                Globals::localPlayer.Addr + Offsets::Player::DisplayName, savedDisplayName);
            displaySaved = false;
            savedDisplayName[0] = 0;
        }
    }

    inline void Apply()
    {
        if (!Globals::localPlayer.Addr || !memory || !memory->IsConnected()) {
            Restore();
            return;
        }

        if (!AnyEnabled()) {
            Restore();
            return;
        }

        const uintptr_t plr = Globals::localPlayer.Addr;

        if (variables::Spoof::userIdEnabled && variables::Spoof::fakeUserId > 0) {
            if (!userIdSaved) {
                savedUserId = memory->read<int64_t>(plr + Offsets::Player::UserId);
                userIdSaved = true;
            }
            if (memory->is_valid_address(plr + Offsets::Player::UserId, sizeof(int64_t)))
                memory->write<int64_t>(plr + Offsets::Player::UserId, variables::Spoof::fakeUserId);
        } else if (userIdSaved) {
            memory->write<int64_t>(plr + Offsets::Player::UserId, savedUserId);
            userIdSaved = false;
        }

        if (variables::Spoof::displayNameEnabled && variables::Spoof::fakeDisplayName[0]) {
            if (!displaySaved) {
                std::string cur = Globals::localPlayer.GetDisplayName();
                strncpy_s(savedDisplayName, cur.c_str(), _TRUNCATE);
                displaySaved = true;
            }
            RBX::WriteString(plr + Offsets::Player::DisplayName, variables::Spoof::fakeDisplayName);
        } else if (displaySaved) {
            RBX::WriteString(plr + Offsets::Player::DisplayName, savedDisplayName);
            displaySaved = false;
            savedDisplayName[0] = 0;
        }

        (void)variables::Spoof::usernameEnabled;
        (void)variables::Spoof::fakeUsername;
    }

    inline void Tick()
    {
        Apply();
    }

    inline void FormatUserId(char* buf, size_t bufN)
    {
        if (!buf || bufN == 0) return;
        if (variables::Spoof::userIdEnabled && variables::Spoof::fakeUserId > 0)
            sprintf_s(buf, bufN, "%lld", (long long)variables::Spoof::fakeUserId);
        else if (Globals::localPlayer.Addr && memory && memory->IsConnected()) {
            int64_t uid = memory->read<int64_t>(Globals::localPlayer.Addr + Offsets::Player::UserId);
            sprintf_s(buf, bufN, "%lld", (long long)uid);
        } else {
            strncpy_s(buf, bufN, "—", _TRUNCATE);
        }
    }

    inline const char* EffectiveDisplayName()
    {
        if (variables::Spoof::displayNameEnabled && variables::Spoof::fakeDisplayName[0])
            return variables::Spoof::fakeDisplayName;
        if (variables::Spoof::usernameEnabled && variables::Spoof::fakeUsername[0])
            return variables::Spoof::fakeUsername;
        return nullptr;
    }
}
