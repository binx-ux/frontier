#include "discord_ipc.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

namespace {

    enum : int {
        OpHandshake = 0,
        OpFrame = 1,
        OpClose = 2,
        OpPing = 3,
        OpPong = 4,
    };

    std::mutex gMutex;
    HANDLE gPipe = INVALID_HANDLE_VALUE;
    char gAppId[32]{};
    int gNonce = 1;

    void ClosePipeLocked()
    {
        if (gPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(gPipe);
            gPipe = INVALID_HANDLE_VALUE;
        }
        gAppId[0] = 0;
    }

    bool WriteFrameLocked(int opcode, const std::string& json)
    {
        if (gPipe == INVALID_HANDLE_VALUE)
            return false;

        const uint32_t op = static_cast<uint32_t>(opcode);
        const uint32_t len = static_cast<uint32_t>(json.size());
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

    bool ReadFrameLocked(int& opcode, std::string& json)
    {
        if (gPipe == INVALID_HANDLE_VALUE)
            return false;

        uint32_t op = 0;
        uint32_t len = 0;
        DWORD read = 0;
        if (!ReadFile(gPipe, &op, sizeof(op), &read, nullptr) || read != sizeof(op))
            return false;
        if (!ReadFile(gPipe, &len, sizeof(len), &read, nullptr) || read != sizeof(len))
            return false;
        if (len > 65536)
            return false;

        opcode = static_cast<int>(op);
        json.clear();
        if (len == 0)
            return true;

        json.resize(len);
        if (!ReadFile(gPipe, json.data(), len, &read, nullptr) || read != len)
            return false;
        return true;
    }

    bool PipeAliveLocked()
    {
        if (gPipe == INVALID_HANDLE_VALUE)
            return false;
        DWORD err = 0;
        if (!GetNamedPipeInfo(gPipe, nullptr, nullptr, nullptr, &err))
            return false;
        return true;
    }

    bool TryConnectPipeLocked()
    {
        for (int i = 0; i < 10; ++i) {
            char path[64];
            sprintf_s(path, "\\\\.\\pipe\\discord-ipc-%d", i);
            HANDLE pipe = CreateFileA(
                path,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            if (pipe == INVALID_HANDLE_VALUE)
                continue;

            DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
            if (!SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr)) {
                CloseHandle(pipe);
                continue;
            }

            gPipe = pipe;
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
                    out.push_back(static_cast<char>(*p));
                }
            }
        }
        return out;
    }

    void AppendStrField(std::string& json, const char* key, const char* value, bool& first)
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

    bool JsonHasEvent(const std::string& json, const char* evt)
    {
        if (evt == nullptr || !evt[0]) return false;
        char needle[64];
        sprintf_s(needle, "\"evt\":\"%s\"", evt);
        if (json.find(needle) != std::string::npos) return true;
        sprintf_s(needle, "\"evt\": \"%s\"", evt);
        return json.find(needle) != std::string::npos;
    }

    bool WaitForReadyLocked()
    {
        const DWORD start = GetTickCount();
        while (GetTickCount() - start < 5000) {
            DWORD avail = 0;
            if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr))
                return false;
            if (avail < 8) {
                Sleep(25);
                continue;
            }

            int op = 0;
            std::string json;
            if (!ReadFrameLocked(op, json))
                return false;

            if (op == OpPing) {
                if (!WriteFrameLocked(OpPong, json))
                    return false;
                continue;
            }
            if (op == OpClose)
                return false;
            if (op != OpFrame)
                continue;

            if (JsonHasEvent(json, "READY"))
                return true;
            if (JsonHasEvent(json, "ERROR"))
                return false;
        }
        return false;
    }

    bool BuildActivityJson(const DiscordIPC::Activity& activity, std::string& act, bool nullActivity)
    {
        if (nullActivity) {
            act = "null";
            return true;
        }

        act = "{";
        bool first = true;
        AppendStrField(act, "details", activity.details, first);
        AppendStrField(act, "state", activity.state, first);

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
                AppendStrField(act, "large_image", activity.largeImageKey, af);
                AppendStrField(act, "large_text", activity.largeImageText, af);
            }
            if (activity.smallImageKey && activity.smallImageKey[0]) {
                AppendStrField(act, "small_image", activity.smallImageKey, af);
                AppendStrField(act, "small_text", activity.smallImageText, af);
            }
            act += '}';
        }

        if (activity.partyMax > 0 && activity.partySize >= 0) {
            if (!first) act += ',';
            first = false;
            char party[64];
            sprintf_s(party, "\"party\":{\"size\":[%d,%d]}", activity.partySize, activity.partyMax);
            act += party;
        }

        const bool hasBtn1 = activity.button1Label && activity.button1Label[0] &&
            activity.button1Url && activity.button1Url[0];
        const bool hasBtn2 = activity.button2Label && activity.button2Label[0] &&
            activity.button2Url && activity.button2Url[0];
        if (hasBtn1 || hasBtn2) {
            if (!first) act += ',';
            first = false;
            act += "\"buttons\":[";
            bool bf = true;
            if (hasBtn1) {
                act += "{\"label\":\"";
                act += JsonEscape(activity.button1Label);
                act += "\",\"url\":\"";
                act += JsonEscape(activity.button1Url);
                act += "\"}";
                bf = false;
            }
            if (hasBtn2) {
                if (!bf) act += ',';
                act += "{\"label\":\"";
                act += JsonEscape(activity.button2Label);
                act += "\",\"url\":\"";
                act += JsonEscape(activity.button2Url);
                act += "\"}";
            }
            act += ']';
        }

        act += '}';
        return true;
    }

    bool SendActivityLocked(const DiscordIPC::Activity& activity, bool nullActivity)
    {
        if (gPipe == INVALID_HANDLE_VALUE)
            return false;

        std::string act;
        if (!BuildActivityJson(activity, act, nullActivity))
            return false;

        std::string payload;
        payload.reserve(act.size() + 128);
        char header[128];
        sprintf_s(header,
            R"({"cmd":"SET_ACTIVITY","args":{"pid":%lu,"activity":)",
            GetCurrentProcessId());
        payload += header;
        payload += act;
        sprintf_s(header, R"(},"nonce":"%d"})", gNonce++);
        payload += header;

        if (!WriteFrameLocked(OpFrame, payload))
            return false;

        const DWORD start = GetTickCount();
        while (GetTickCount() - start < 1500) {
            DWORD avail = 0;
            if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr))
                return false;
            if (avail < 8) {
                Sleep(20);
                continue;
            }

            int op = 0;
            std::string json;
            if (!ReadFrameLocked(op, json))
                return false;

            if (op == OpPing) {
                if (!WriteFrameLocked(OpPong, json))
                    return false;
                continue;
            }
            if (op == OpClose)
                return false;
            if (op != OpFrame)
                continue;

            if (JsonHasEvent(json, "ERROR"))
                return false;

            const char* nonceKey = "\"nonce\":";
            const size_t pos = json.find(nonceKey);
            if (pos != std::string::npos)
                return true;
            if (json.find("SET_ACTIVITY") != std::string::npos)
                return true;
        }

        return true;
    }

}

namespace DiscordIPC {

    bool Connect(const char* applicationId)
    {
        if (!applicationId || !applicationId[0])
            return false;

        std::lock_guard<std::mutex> lock(gMutex);
        if (gPipe != INVALID_HANDLE_VALUE) {
            if (PipeAliveLocked() && gAppId[0] && _stricmp(gAppId, applicationId) == 0)
                return true;
            ClosePipeLocked();
        }

        strncpy_s(gAppId, applicationId, _TRUNCATE);
        if (!TryConnectPipeLocked())
            return false;

        char handshake[128];
        sprintf_s(handshake, R"({"v":1,"client_id":"%s"})", gAppId);
        if (!WriteFrameLocked(OpHandshake, handshake)) {
            ClosePipeLocked();
            return false;
        }

        if (!WaitForReadyLocked()) {
            ClosePipeLocked();
            return false;
        }
        return true;
    }

    void Disconnect()
    {
        std::lock_guard<std::mutex> lock(gMutex);
        ClosePipeLocked();
    }

    bool IsConnected()
    {
        std::lock_guard<std::mutex> lock(gMutex);
        return gPipe != INVALID_HANDLE_VALUE && PipeAliveLocked();
    }

    bool SetActivity(const Activity& activity)
    {
        std::lock_guard<std::mutex> lock(gMutex);
        return SendActivityLocked(activity, false);
    }

    bool ClearActivity()
    {
        std::lock_guard<std::mutex> lock(gMutex);
        Activity empty{};
        return SendActivityLocked(empty, true);
    }

    void Pump(int maxFrames)
    {
        std::lock_guard<std::mutex> lock(gMutex);
        if (gPipe == INVALID_HANDLE_VALUE)
            return;

        for (int i = 0; i < maxFrames; ++i) {
            DWORD avail = 0;
            if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr)) {
                ClosePipeLocked();
                break;
            }
            if (avail < 8)
                break;

            int op = 0;
            std::string json;
            if (!ReadFrameLocked(op, json)) {
                ClosePipeLocked();
                break;
            }

            if (op == OpPing) {
                if (!WriteFrameLocked(OpPong, json))
                    ClosePipeLocked();
                continue;
            }
            if (op == OpClose) {
                ClosePipeLocked();
                break;
            }
        }
    }

}
