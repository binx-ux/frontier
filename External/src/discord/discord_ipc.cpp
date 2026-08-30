#include "discord_ipc.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <random>

namespace {

    HANDLE gPipe = INVALID_HANDLE_VALUE;
    char gAppId[32]{};
    int gNonce = 1;

    bool WriteFrame(int opcode, const std::string& json)
    {
        if (gPipe == INVALID_HANDLE_VALUE)
            return false;

        uint32_t op = static_cast<uint32_t>(opcode);
        uint32_t len = static_cast<uint32_t>(json.size());
        DWORD written = 0;
        if (!WriteFile(gPipe, &op, sizeof(op), &written, nullptr) || written != sizeof(op))
            return false;
        if (!WriteFile(gPipe, &len, sizeof(len), &written, nullptr) || written != sizeof(len))
            return false;
        if (len == 0)
            return true;
        if (!WriteFile(gPipe, json.data(), len, &written, nullptr) || written != len)
            return false;
        return true;
    }

    bool TryConnectPipe()
    {
        for (int i = 0; i < 10; ++i) {
            char path[64];
            sprintf_s(path, "\\\\.\\pipe\\discord-ipc-%d", i);
            gPipe = CreateFileA(
                path,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            if (gPipe != INVALID_HANDLE_VALUE)
                return true;
        }
        return false;
    }

    std::string JsonEscape(const char* s)
    {
        if (!s) return "";
        std::string out;
        out.reserve(strlen(s) + 8);
        for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
            switch (*p) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (*p < 0x20) {
                    char buf[8];
                    sprintf_s(buf, "\\u%04x", *p);
                    out += buf;
                } else {
                    out.push_back((char)*p);
                }
            }
        }
        return out;
    }

    void AppendField(std::string& json, const char* key, const char* value, bool& first)
    {
        if (!value || !value[0]) return;
        if (!first) json += ',';
        first = false;
        json += '"';
        json += key;
        json += "\":\"";
        json += JsonEscape(value);
        json += '"';
    }

}

namespace DiscordIPC {

    bool Connect(const char* applicationId)
    {
        if (!applicationId || !applicationId[0])
            return false;
        if (IsConnected())
            return true;

        strncpy_s(gAppId, applicationId, _TRUNCATE);
        if (!TryConnectPipe())
            return false;

        char handshake[128];
        sprintf_s(handshake, R"({"v":1,"client_id":"%s"})", gAppId);
        if (!WriteFrame(0, handshake)) {
            Disconnect();
            return false;
        }
        return true;
    }

    void Disconnect()
    {
        if (gPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(gPipe);
            gPipe = INVALID_HANDLE_VALUE;
        }
        gAppId[0] = 0;
    }

    bool IsConnected()
    {
        return gPipe != INVALID_HANDLE_VALUE;
    }

    bool SetActivity(const Activity& activity)
    {
        if (!IsConnected())
            return false;

        std::string act = "{";
        bool first = true;
        AppendField(act, "details", activity.details, first);
        AppendField(act, "state", activity.state, first);

        if (activity.startTimestamp > 0) {
            if (!first) act += ',';
            first = false;
            char ts[64];
            sprintf_s(ts, "\"timestamps\":{\"start\":%lld}", (long long)activity.startTimestamp);
            act += ts;
        }

        if ((activity.largeImageKey && activity.largeImageKey[0]) ||
            (activity.smallImageKey && activity.smallImageKey[0])) {
            if (!first) act += ',';
            first = false;
            act += "\"assets\":{";
            bool af = true;
            if (activity.largeImageKey && activity.largeImageKey[0]) {
                AppendField(act, "large_image", activity.largeImageKey, af);
                AppendField(act, "large_text", activity.largeImageText, af);
            }
            if (activity.smallImageKey && activity.smallImageKey[0]) {
                AppendField(act, "small_image", activity.smallImageKey, af);
                AppendField(act, "small_text", activity.smallImageText, af);
            }
            act += '}';
        }

        if (activity.partyMax > 0) {
            if (!first) act += ',';
            first = false;
            char party[64];
            sprintf_s(party, "\"party\":{\"size\":[%d,%d]}", activity.partySize, activity.partyMax);
            act += party;
        }

        act += '}';

        char payload[1024];
        sprintf_s(payload,
            R"({"cmd":"SET_ACTIVITY","args":{"pid":%lu,"activity":%s},"nonce":"%d"})",
            GetCurrentProcessId(),
            act.c_str(),
            gNonce++);

        return WriteFrame(1, payload);
    }

    void Pump()
    {
        if (gPipe == INVALID_HANDLE_VALUE)
            return;

        DWORD avail = 0;
        if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr))
            return;
        if (avail == 0)
            return;

        std::string buf(avail, '\0');
        DWORD read = 0;
        ReadFile(gPipe, buf.data(), avail, &read, nullptr);
    }

}
