#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "loader_config.h"
#include "loader_update.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

namespace LoaderOAuth {

    struct Session {
        bool verified = false;
        bool allowed = false;
        char token[512] = "";
        char discordId[32] = "";
        char username[64] = "";
        char globalName[64] = "";
        int robloxUserId = 0;
        char message[256] = "";
    };

    inline std::string UrlEncode(const char* s)
    {
        static const char* hex = "0123456789ABCDEF";
        std::string out;
        for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
            if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
                (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.' || *p == '~') {
                out.push_back((char)*p);
            } else {
                out.push_back('%');
                out.push_back(hex[*p >> 4]);
                out.push_back(hex[*p & 0xF]);
            }
        }
        return out;
    }

    inline std::string UrlDecode(const std::string& in)
    {
        std::string out;
        out.reserve(in.size());
        for (size_t i = 0; i < in.size(); i++) {
            char c = in[i];
            if (c == '%' && i + 2 < in.size()) {
                char hex[3] = { in[i + 1], in[i + 2], 0 };
                out.push_back((char)strtol(hex, nullptr, 16));
                i += 2;
            } else if (c == '+') {
                out.push_back(' ');
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    inline std::string JsonEscape(const std::string& in)
    {
        std::string out;
        out.reserve(in.size() + 8);
        for (char c : in) {
            switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
            }
        }
        return out;
    }

    inline std::string Base64UrlEncode(const unsigned char* data, size_t len)
    {
        static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        size_t i = 0;
        while (i < len) {
            unsigned a = i < len ? data[i++] : 0;
            unsigned b = i < len ? data[i++] : 0;
            unsigned c = i < len ? data[i++] : 0;
            unsigned triple = (a << 16) | (b << 8) | c;
            out.push_back(b64[(triple >> 18) & 0x3F]);
            out.push_back(b64[(triple >> 12) & 0x3F]);
            out.push_back(i > len + 1 ? '=' : b64[(triple >> 6) & 0x3F]);
            out.push_back(i > len ? '=' : b64[triple & 0x3F]);
        }
        while (!out.empty() && out.back() == '=') out.pop_back();
        for (char& ch : out) {
            if (ch == '+') ch = '-';
            else if (ch == '/') ch = '_';
        }
        return out;
    }

    inline bool RandomBytes(unsigned char* buf, size_t len)
    {
        return BCryptGenRandom(nullptr, buf, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
    }

    inline std::string GenerateVerifier()
    {
        unsigned char buf[32]{};
        if (!RandomBytes(buf, sizeof(buf))) return "";
        return Base64UrlEncode(buf, sizeof(buf));
    }

    inline std::string GenerateState()
    {
        unsigned char buf[16]{};
        if (!RandomBytes(buf, sizeof(buf))) return "";
        return Base64UrlEncode(buf, sizeof(buf));
    }

    inline std::string Sha256Base64Url(const std::string& input)
    {
        BCRYPT_ALG_HANDLE alg = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
            return "";
        DWORD objLen = 0, hashLen = 0;
        BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &hashLen, 0);
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLen, sizeof(hashLen), &hashLen, 0);
        std::vector<UCHAR> obj(objLen);
        std::vector<UCHAR> digest(hashLen);
        if (BCryptCreateHash(alg, &hash, obj.data(), objLen, nullptr, 0, 0) != 0) {
            BCryptCloseAlgorithmProvider(alg, 0);
            return "";
        }
        BCryptHashData(hash, (PUCHAR)input.data(), (ULONG)input.size(), 0);
        BCryptFinishHash(hash, digest.data(), hashLen, 0);
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg, 0);
        return Base64UrlEncode(digest.data(), digest.size());
    }

    inline bool ParseJsonStr(const std::string& json, const char* key, char* out, size_t outSz)
    {
        out[0] = 0;
        std::string needle = std::string("\"") + key + "\":\"";
        size_t p = json.find(needle);
        if (p == std::string::npos) return false;
        p += needle.size();
        size_t e = json.find('"', p);
        if (e == std::string::npos) return false;
        size_t n = e - p;
        if (n >= outSz) n = outSz - 1;
        memcpy(out, json.data() + p, n);
        out[n] = 0;
        return true;
    }

    inline bool ParseJsonBool(const std::string& json, const char* key, bool fallback)
    {
        std::string needle = std::string("\"") + key + "\":";
        size_t p = json.find(needle);
        if (p == std::string::npos) return fallback;
        p += needle.size();
        while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) p++;
        if (json.compare(p, 4, "true") == 0) return true;
        if (json.compare(p, 5, "false") == 0) return false;
        return fallback;
    }

    inline int ParseJsonInt(const std::string& json, const char* key, int fallback)
    {
        std::string needle = std::string("\"") + key + "\":";
        size_t p = json.find(needle);
        if (p == std::string::npos) return fallback;
        p += needle.size();
        while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) p++;
        return atoi(json.c_str() + p);
    }

    inline bool FetchOAuthConfig(char* clientId, size_t clientIdSz, char* redirectUri, size_t redirectSz)
    {
        std::string body, err;
        if (!LoaderUpdate::HttpGet(LoaderConfig::kDiscordOAuthApi, body, err))
            return false;
        if (!ParseJsonStr(body, "clientId", clientId, clientIdSz))
            return false;
        if (!ParseJsonStr(body, "redirectUri", redirectUri, redirectSz))
            strncpy_s(redirectUri, redirectSz, LoaderConfig::kOAuthRedirectUri, _TRUNCATE);
        return clientId[0] != 0;
    }

    inline bool WaitForCallback(const char* expectedState, char* outCode, size_t codeSz, std::string& err)
    {
        outCode[0] = 0;
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            err = "Network init failed";
            return false;
        }

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            err = "Socket failed";
            return false;
        }

        BOOL reuse = TRUE;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons((u_short)LoaderConfig::kOAuthPort);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
            closesocket(sock);
            WSACleanup();
            err = "Could not bind OAuth callback port";
            return false;
        }
        listen(sock, 1);

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        timeval tv{};
        tv.tv_sec = 180;
        int sel = select(0, &fds, nullptr, nullptr, &tv);
        if (sel <= 0) {
            closesocket(sock);
            WSACleanup();
            err = "Discord sign-in timed out";
            return false;
        }

        SOCKET client = accept(sock, nullptr, nullptr);
        closesocket(sock);
        if (client == INVALID_SOCKET) {
            WSACleanup();
            err = "Callback accept failed";
            return false;
        }

        char buf[4096]{};
        recv(client, buf, sizeof(buf) - 1, 0);

        const char* okPage =
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
            "<!DOCTYPE html><html><head><meta charset=utf-8><title>FRONTIER</title></head>"
            "<body style=\"margin:0;background:#0c0c0c;color:#eee;font-family:Segoe UI,sans-serif;"
            "display:flex;align-items:center;justify-content:center;min-height:100vh\">"
            "<div style=\"text-align:center;max-width:420px;padding:32px\">"
            "<div style=\"width:56px;height:56px;border-radius:14px;background:#eb3847;"
            "margin:0 auto 20px;line-height:56px;font-weight:700;font-size:22px\">F</div>"
            "<h2 style=\"margin:0 0 12px;font-weight:600\">Signed in</h2>"
            "<p style=\"color:#999;margin:0\">Return to the FRONTIER loader. You can close this tab.</p>"
            "</div></body></html>";
        send(client, okPage, (int)strlen(okPage), 0);
        closesocket(client);
        WSACleanup();

        const char* req = strstr(buf, "GET ");
        if (!req) {
            err = "Bad callback request";
            return false;
        }
        const char* pathStart = req + 4;
        const char* pathEnd = strstr(pathStart, " HTTP/");
        if (!pathEnd) {
            err = "Bad callback path";
            return false;
        }
        std::string path(pathStart, pathEnd - pathStart);
        size_t q = path.find('?');
        if (q == std::string::npos) {
            err = "Missing OAuth code";
            return false;
        }
        std::string query = path.substr(q + 1);

        auto getParam = [&](const char* key, char* out, size_t sz) {
            out[0] = 0;
            std::string needle = std::string(key) + "=";
            size_t p = query.find(needle);
            if (p == std::string::npos) return false;
            p += needle.size();
            size_t e = query.find('&', p);
            std::string val = query.substr(p, e == std::string::npos ? std::string::npos : e - p);
            val = UrlDecode(val);
            strncpy_s(out, sz, val.c_str(), _TRUNCATE);
            return out[0] != 0;
        };

        char state[128]{}, code[256]{};
        if (!getParam("code", code, codeSz)) {
            err = "Discord did not return a code";
            return false;
        }
        getParam("state", state, sizeof(state));
        if (expectedState && expectedState[0] && state[0] && strcmp(state, expectedState) != 0) {
            err = "OAuth state mismatch";
            return false;
        }
        strncpy_s(outCode, codeSz, code, _TRUNCATE);
        return true;
    }

    inline void SaveSession(const Session& session);

    inline bool ExchangeCode(const char* code, const char* verifier, const char* redirectUri, Session& session, std::string& err)
    {
        std::string body = std::string("{\"code\":\"") + JsonEscape(code) +
            "\",\"codeVerifier\":\"" + JsonEscape(verifier) +
            "\",\"redirectUri\":\"" + JsonEscape(redirectUri) + "\"}";
        std::string resp;
        if (!LoaderUpdate::HttpPost(LoaderConfig::kDiscordOAuthApi, "application/json", body, resp, err))
            return false;

        if (!ParseJsonBool(resp, "ok", false)) {
            ParseJsonStr(resp, "message", session.message, sizeof(session.message));
            if (!session.message[0]) ParseJsonStr(resp, "error", session.message, sizeof(session.message));
            err = session.message[0] ? session.message : "Discord sign-in failed";
            return false;
        }

        session.allowed = ParseJsonBool(resp, "allowed", false);
        session.verified = session.allowed;
        ParseJsonStr(resp, "token", session.token, sizeof(session.token));
        ParseJsonStr(resp, "discordId", session.discordId, sizeof(session.discordId));
        ParseJsonStr(resp, "username", session.username, sizeof(session.username));
        ParseJsonStr(resp, "globalName", session.globalName, sizeof(session.globalName));
        session.robloxUserId = ParseJsonInt(resp, "robloxUserId", 0);
        if (!ParseJsonStr(resp, "message", session.message, sizeof(session.message)))
            session.message[0] = 0;

        SaveSession(session);

        if (!session.allowed) {
            err = session.message[0] ? session.message : "Run /verify YourRobloxName in Discord";
            return true;
        }
        return true;
    }

    inline bool BuildAuthorizeUrl(const char* clientId, const char* redirectUri,
        const char* state, const char* challenge, char* outUrl, size_t outSz)
    {
        std::string url =
            "https://discord.com/api/oauth2/authorize?client_id=" + UrlEncode(clientId) +
            "&redirect_uri=" + UrlEncode(redirectUri) +
            "&response_type=code&scope=identify" +
            "&state=" + UrlEncode(state) +
            "&code_challenge=" + UrlEncode(challenge) +
            "&code_challenge_method=S256";
        if (url.size() >= outSz) return false;
        strncpy_s(outUrl, outSz, url.c_str(), _TRUNCATE);
        return true;
    }

    inline void LoadSession(Session& session)
    {
        memset(&session, 0, sizeof(session));
        std::wstring ini = LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini");
        session.verified = GetPrivateProfileIntW(L"discord", L"verified", 0, ini.c_str()) != 0;
        session.allowed = session.verified;
        session.robloxUserId = GetPrivateProfileIntW(L"discord", L"robloxUserId", 0, ini.c_str());
        wchar_t wbuf[512]{};
        GetPrivateProfileStringW(L"discord", L"token", L"", wbuf, 512, ini.c_str());
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, session.token, (int)sizeof(session.token), nullptr, nullptr);
        GetPrivateProfileStringW(L"discord", L"username", L"", wbuf, 64, ini.c_str());
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, session.username, (int)sizeof(session.username), nullptr, nullptr);
        GetPrivateProfileStringW(L"discord", L"discordId", L"", wbuf, 32, ini.c_str());
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, session.discordId, (int)sizeof(session.discordId), nullptr, nullptr);
        GetPrivateProfileStringW(L"discord", L"globalName", L"", wbuf, 64, ini.c_str());
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, session.globalName, (int)sizeof(session.globalName), nullptr, nullptr);
    }

    inline void SaveSession(const Session& session)
    {
        std::wstring ini = LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini");
        auto writeA = [&](const wchar_t* key, const char* val) {
            wchar_t wval[512]{};
            MultiByteToWideChar(CP_UTF8, 0, val ? val : "", -1, wval, 512);
            WritePrivateProfileStringW(L"discord", key, wval, ini.c_str());
        };
        wchar_t buf[32];
        swprintf_s(buf, L"%d", session.verified ? 1 : 0);
        WritePrivateProfileStringW(L"discord", L"verified", buf, ini.c_str());
        swprintf_s(buf, L"%d", session.robloxUserId);
        WritePrivateProfileStringW(L"discord", L"robloxUserId", buf, ini.c_str());
        writeA(L"token", session.token);
        writeA(L"username", session.username);
        writeA(L"discordId", session.discordId);
        writeA(L"globalName", session.globalName);
    }

    inline bool SignIn(Session& session, std::string& err)
    {
        char clientId[64]{}, redirectUri[128]{};
        if (!FetchOAuthConfig(clientId, sizeof(clientId), redirectUri, sizeof(redirectUri))) {
            err = "Could not load Discord OAuth config";
            return false;
        }

        std::string verifier = GenerateVerifier();
        std::string state = GenerateState();
        std::string challenge = Sha256Base64Url(verifier);
        if (verifier.empty() || challenge.empty()) {
            err = "PKCE setup failed";
            return false;
        }

        char authUrl[1024]{};
        if (!BuildAuthorizeUrl(clientId, redirectUri, state.c_str(), challenge.c_str(), authUrl, sizeof(authUrl))) {
            err = "Bad OAuth URL";
            return false;
        }

        std::string callbackErr;
        char code[256]{};

        struct WaitCtx {
            const char* expectedState;
            char code[256];
            std::string err;
        } ctx{};
        ctx.expectedState = state.c_str();

        HANDLE waitThread = CreateThread(nullptr, 0,
            [](LPVOID param) -> DWORD {
                auto* c = (WaitCtx*)param;
                if (!WaitForCallback(c->expectedState, c->code, sizeof(c->code), c->err))
                    return 1;
                return 0;
            },
            &ctx, 0, nullptr);

        Sleep(100);
        ShellExecuteA(nullptr, "open", authUrl, nullptr, nullptr, SW_SHOWNORMAL);

        if (waitThread) {
            WaitForSingleObject(waitThread, INFINITE);
            DWORD exitCode = 1;
            GetExitCodeThread(waitThread, &exitCode);
            CloseHandle(waitThread);
            if (exitCode != 0) {
                err = ctx.err.empty() ? "Discord sign-in cancelled" : ctx.err;
                return false;
            }
            strncpy_s(code, ctx.code, _TRUNCATE);
        } else {
            err = "Could not start OAuth listener";
            return false;
        }

        if (!ExchangeCode(code, verifier.c_str(), redirectUri, session, err))
            return false;
        return true;
    }

    inline bool RefreshSession(Session& session, std::string& err)
    {
        if (!session.token[0]) {
            err = "Not signed in";
            return false;
        }
        char url[768];
        std::string enc = UrlEncode(session.token);
        sprintf_s(url, "%s?token=%s", LoaderConfig::kDiscordOAuthApi, enc.c_str());
        std::string resp;
        if (!LoaderUpdate::HttpGet(url, resp, err))
            return false;
        session.allowed = ParseJsonBool(resp, "allowed", false);
        session.verified = session.allowed;
        session.robloxUserId = ParseJsonInt(resp, "robloxUserId", session.robloxUserId);
        ParseJsonStr(resp, "username", session.username, sizeof(session.username));
        ParseJsonStr(resp, "globalName", session.globalName, sizeof(session.globalName));
        if (!session.allowed) {
            ParseJsonStr(resp, "message", session.message, sizeof(session.message));
            err = session.message[0] ? session.message : "Access denied";
            SaveSession(session);
            return false;
        }
        SaveSession(session);
        return true;
    }

    inline void ClearSession()
    {
        Session empty{};
        SaveSession(empty);
        WritePrivateProfileStringW(L"discord", nullptr, nullptr,
            LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini").c_str());
    }
}
