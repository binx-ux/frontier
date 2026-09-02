#pragma once
#include <Windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <vector>
#include <string>
#include "loader_config.h"
#include "loader_update.h"
#include "../src/common/url.h"

#pragma comment(lib, "bcrypt.lib")

namespace LoaderLicense {

    inline std::string JsonEscape(const std::string& in)
    {
        std::string out;
        out.reserve(in.size() + 8);
        for (char c : in) {
            switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            default: out.push_back(c); break;
            }
        }
        return out;
    }

    inline void NormalizeKeyInput(const char* in, char* out, size_t outSz)
    {
        if (!out || outSz == 0) return;
        out[0] = 0;
        if (!in) return;
        size_t j = 0;
        for (size_t i = 0; in[i] && j + 1 < outSz; i++) {
            char c = in[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            out[j++] = c;
        }
        out[j] = 0;
    }

    inline void FriendlyLicenseError(const char* raw, char* out, size_t outSz)
    {
        if (!out || outSz < 2) return;
        out[0] = 0;
        if (!raw || !raw[0]) {
            strncpy_s(out, outSz, "License check failed", _TRUNCATE);
            return;
        }
        if (_stricmp(raw, "license_store_unavailable") == 0)
            strncpy_s(out, outSz, "License server temporarily unavailable. Try again in a minute.", _TRUNCATE);
        else if (_stricmp(raw, "invalid_key") == 0)
            strncpy_s(out, outSz, "Invalid license key. Check the key and try again.", _TRUNCATE);
        else if (_stricmp(raw, "key_revoked") == 0)
            strncpy_s(out, outSz, "This license key has been revoked.", _TRUNCATE);
        else if (_stricmp(raw, "hwid_mismatch") == 0)
            strncpy_s(out, outSz, "This license is bound to another PC.", _TRUNCATE);
        else if (_stricmp(raw, "invalid_email") == 0)
            strncpy_s(out, outSz, "No account found for that email. Use your purchase email.", _TRUNCATE);
        else if (_stricmp(raw, "invalid_password") == 0 || _stricmp(raw, "password_required") == 0)
            strncpy_s(out, outSz, "Incorrect password for this account.", _TRUNCATE);
        else if (_stricmp(raw, "email_not_verified") == 0)
            strncpy_s(out, outSz, "Verify your email on ahead.best before signing in.", _TRUNCATE);
        else if (_stricmp(raw, "missing_credentials") == 0)
            strncpy_s(out, outSz, "Enter your email and password.", _TRUNCATE);
        else if (_stricmp(raw, "bind_failed") == 0)
            strncpy_s(out, outSz, "Could not bind this PC. Try again in a minute.", _TRUNCATE);
        else if (_stricmp(raw, "forbidden") == 0)
            strncpy_s(out, outSz, "License server rejected the request. Try again later.", _TRUNCATE);
        else
            strncpy_s(out, outSz, raw, _TRUNCATE);
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

    inline bool ParseJsonStrLast(const std::string& json, const char* key, char* out, size_t outSz)
    {
        out[0] = 0;
        std::string needle = std::string("\"") + key + "\":\"";
        size_t p = json.rfind(needle);
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

    inline void ParseApiMessage(const std::string& json, char* out, size_t outSz)
    {
        if (!out || outSz < 2) return;
        out[0] = 0;
        char errCode[64]{};
        if (ParseJsonStr(json, "error", errCode, sizeof(errCode)) && errCode[0]) {
            FriendlyLicenseError(errCode, out, outSz);
            return;
        }
        ParseJsonStrLast(json, "message", out, outSz);
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

    inline std::string BytesToHex(const unsigned char* data, size_t len)
    {
        static const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve(len * 2);
        for (size_t i = 0; i < len; i++) {
            out.push_back(hex[data[i] >> 4]);
            out.push_back(hex[data[i] & 0xF]);
        }
        return out;
    }

    inline std::string Sha256Hex(const std::string& input)
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
        return BytesToHex(digest.data(), digest.size());
    }

    inline std::string GetHwid()
    {
        char computer[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD csz = MAX_COMPUTERNAME_LENGTH + 1;
        GetComputerNameA(computer, &csz);

        DWORD serial = 0;
        GetVolumeInformationA("C:\\", nullptr, 0, &serial, nullptr, nullptr, nullptr, 0);

        char buf[128];
        sprintf_s(buf, "%s-%08X", computer, serial);
        return Sha256Hex(buf).substr(0, 32);
    }

    inline void LoadSaved(char* token, size_t tokenSz, char* hwid, size_t hwidSz, char* email, size_t emailSz)
    {
        if (token && tokenSz) token[0] = 0;
        if (hwid && hwidSz) hwid[0] = 0;
        if (email && emailSz) email[0] = 0;
        std::wstring ini = LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini");
        if (token && tokenSz) {
            wchar_t wbuf[768]{};
            GetPrivateProfileStringW(L"license", L"token", L"", wbuf, 768, ini.c_str());
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, token, (int)tokenSz, nullptr, nullptr);
        }
        if (hwid && hwidSz) {
            wchar_t wbuf[96]{};
            GetPrivateProfileStringW(L"license", L"hwid", L"", wbuf, 96, ini.c_str());
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, hwid, (int)hwidSz, nullptr, nullptr);
        }
        if (email && emailSz) {
            wchar_t wbuf[128]{};
            GetPrivateProfileStringW(L"license", L"email", L"", wbuf, 128, ini.c_str());
            if (!wbuf[0])
                GetPrivateProfileStringW(L"license", L"key", L"", wbuf, 128, ini.c_str());
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, email, (int)emailSz, nullptr, nullptr);
        }
    }

    inline void SaveLicense(const char* token, const char* hwid, const char* email = nullptr, const char* licenseKey = nullptr)
    {
        std::wstring ini = LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini");
        auto writeA = [&](const wchar_t* key, const char* val) {
            wchar_t wval[768]{};
            MultiByteToWideChar(CP_UTF8, 0, val ? val : "", -1, wval, 768);
            WritePrivateProfileStringW(L"license", key, wval, ini.c_str());
        };
        writeA(L"token", token);
        writeA(L"hwid", hwid);
        if (email && email[0]) {
            writeA(L"email", email);
            WritePrivateProfileStringW(L"license", L"key", nullptr, ini.c_str());
        } else if (licenseKey && licenseKey[0]) {
            writeA(L"key", licenseKey);
        }
    }

    inline void ClearLicense()
    {
        std::wstring ini = LoaderUpdate::PathJoin(LoaderUpdate::GetLoaderDir(), L"loader.ini");
        WritePrivateProfileStringW(L"license", nullptr, nullptr, ini.c_str());
    }

    inline bool ValidateTokenRemote(const char* token, const char* hwid, bool* outLicensed, std::string& err)
    {
        if (outLicensed) *outLicensed = false;
        if (!token || !token[0]) {
            err = "Not signed in";
            return false;
        }
        char url[2048];
        sprintf_s(url, "%s?token=%s&hwid=%s", LoaderConfig::kLicenseApi,
            Net::UrlEncode(token).c_str(), Net::UrlEncode(hwid ? hwid : "").c_str());
        std::string resp;
        if (!LoaderUpdate::HttpGet(url, resp, err))
            return false;
        if (!ParseJsonBool(resp, "ok", false)) {
            char msg[256]{};
            ParseApiMessage(resp, msg, sizeof(msg));
            err = msg[0] ? msg : "Session expired. Sign in again.";
            return false;
        }
        const bool licensed = ParseJsonBool(resp, "allowed", ParseJsonBool(resp, "licensed", true));
        if (outLicensed) *outLicensed = licensed;
        return true;
    }

    inline bool ActivateEmail(const char* email, const char* password, const char* hwid, char* outToken, size_t tokenSz, char* outMsg, size_t msgSz, bool* outLicensed, std::string& err)
    {
        if (outToken && tokenSz) outToken[0] = 0;
        if (outMsg && msgSz) outMsg[0] = 0;
        if (!email || !email[0]) {
            err = "Enter your account email";
            return false;
        }
        if (!password || !password[0]) {
            err = "Enter your account password";
            return false;
        }
        std::string body = std::string("{\"email\":\"") + JsonEscape(email) +
            "\",\"password\":\"" + JsonEscape(password) +
            "\",\"hwid\":\"" + JsonEscape(hwid ? hwid : "") + "\"}";
        std::string resp;
        if (!LoaderUpdate::HttpPost(LoaderConfig::kLicenseApi, "application/json", body, resp, err))
            return false;
        if (!ParseJsonBool(resp, "ok", false)) {
            char friendly[256]{};
            ParseApiMessage(resp, friendly, sizeof(friendly));
            if (!friendly[0]) strncpy_s(friendly, err.c_str(), _TRUNCATE);
            err = friendly;
            if (outMsg && msgSz) strncpy_s(outMsg, msgSz, friendly, _TRUNCATE);
            return false;
        }
        ParseJsonStr(resp, "token", outToken, tokenSz);
        if (!outToken || !outToken[0]) {
            err = "Bad license response";
            return false;
        }
        const bool licensed = ParseJsonBool(resp, "allowed", ParseJsonBool(resp, "licensed", true));
        if (outLicensed) *outLicensed = licensed;
        ParseJsonStrLast(resp, "message", outMsg, msgSz);
        if (outMsg && msgSz && !outMsg[0])
            strncpy_s(outMsg, msgSz, licensed ? "Signed in." : "Signed in. Purchase FRONTIER to unlock.", _TRUNCATE);
        SaveLicense(outToken, hwid, email);
        return true;
    }

    inline bool ActivateKey(const char* key, const char* password, const char* hwid, char* outToken, size_t tokenSz, char* outMsg, size_t msgSz, bool* outLicensed, std::string& err)
    {
        if (outToken && tokenSz) outToken[0] = 0;
        if (outMsg && msgSz) outMsg[0] = 0;
        if (!key || !key[0]) {
            err = "Enter your license key";
            return false;
        }
        std::string body = std::string("{\"key\":\"") + JsonEscape(key) +
            "\",\"hwid\":\"" + JsonEscape(hwid ? hwid : "") + "\"}";
        std::string resp;
        if (!LoaderUpdate::HttpPost(LoaderConfig::kLicenseApi, "application/json", body, resp, err))
            return false;
        if (!ParseJsonBool(resp, "ok", false)) {
            char friendly[256]{};
            ParseApiMessage(resp, friendly, sizeof(friendly));
            if (!friendly[0]) strncpy_s(friendly, err.c_str(), _TRUNCATE);
            err = friendly;
            if (outMsg && msgSz) strncpy_s(outMsg, msgSz, friendly, _TRUNCATE);
            return false;
        }
        ParseJsonStr(resp, "token", outToken, tokenSz);
        if (!outToken || !outToken[0]) {
            err = "Bad license response";
            return false;
        }
        const bool licensed = ParseJsonBool(resp, "allowed", ParseJsonBool(resp, "licensed", true));
        if (outLicensed) *outLicensed = licensed;
        ParseJsonStrLast(resp, "message", outMsg, msgSz);
        if (outMsg && msgSz && !outMsg[0])
            strncpy_s(outMsg, msgSz, licensed ? "License active." : "Signed in. Purchase FRONTIER to unlock.", _TRUNCATE);
        SaveLicense(outToken, hwid, nullptr, key);
        return true;
    }

    inline bool ActivateDiscord(const char* discordId, const char* hwid, char* outToken, size_t tokenSz, char* outMsg, size_t msgSz, std::string& err)
    {
        if (outToken && tokenSz) outToken[0] = 0;
        if (!discordId || !discordId[0]) {
            err = "Discord sign-in failed";
            return false;
        }
        std::string body = std::string("{\"action\":\"discord\",\"discordId\":\"") + discordId +
            "\",\"hwid\":\"" + (hwid ? hwid : "") + "\"}";
        std::string resp;
        if (!LoaderUpdate::HttpPost(LoaderConfig::kLicenseApi, "application/json", body, resp, err))
            return false;
        if (!ParseJsonBool(resp, "ok", false) || !ParseJsonBool(resp, "allowed", false)) {
            ParseJsonStr(resp, "message", outMsg, msgSz);
            err = outMsg && outMsg[0] ? outMsg : "No license on this Discord account";
            return false;
        }
        ParseJsonStr(resp, "token", outToken, tokenSz);
        ParseJsonStr(resp, "message", outMsg, msgSz);
        if (!outToken || !outToken[0]) {
            err = "Bad license response";
            return false;
        }
        SaveLicense(outToken, hwid);
        return true;
    }

    inline bool WriteSessionFile(const std::wstring& workDir, const char* token, const char* hwid)
    {
        if (!token || !token[0]) return false;
        std::wstring path = LoaderUpdate::PathJoin(workDir, L"frontier.session");
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"w") != 0 || !f) return false;
        fprintf(f, "token=%s\nhwid=%s\n", token, hwid ? hwid : "");
        fclose(f);
        return true;
    }
}
