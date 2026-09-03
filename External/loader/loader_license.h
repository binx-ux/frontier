#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <string>

// Open-source build: no remote license API, keys, or HWID binding.
namespace LoaderLicense {

    inline constexpr const char* kOpenToken = "opensource";

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

    inline std::string GetHwid() { return "opensource"; }

    inline void LoadSaved(char* token, size_t tokenSz, char* hwid, size_t hwidSz, char* email, size_t emailSz)
    {
        if (token && tokenSz) strncpy_s(token, tokenSz, kOpenToken, _TRUNCATE);
        if (hwid && hwidSz) strncpy_s(hwid, hwidSz, "opensource", _TRUNCATE);
        if (email && emailSz) email[0] = 0;
    }

    inline void SaveLicense(const char*, const char*, const char* = nullptr, const char* = nullptr) {}
    inline void ClearLicense() {}

    inline bool ValidateTokenRemote(const char*, const char*, bool* outLicensed, std::string& err)
    {
        err.clear();
        if (outLicensed) *outLicensed = true;
        return true;
    }

    inline bool ActivateEmail(const char*, const char*, const char* hwid, char* outToken, size_t tokenSz, char* outMsg, size_t msgSz, bool* outLicensed, std::string& err)
    {
        err.clear();
        if (outToken && tokenSz) strncpy_s(outToken, tokenSz, kOpenToken, _TRUNCATE);
        if (outMsg && msgSz) strncpy_s(outMsg, msgSz, "Open source — no key required.", _TRUNCATE);
        if (outLicensed) *outLicensed = true;
        SaveLicense(kOpenToken, hwid);
        return true;
    }

    inline bool ActivateKey(const char*, const char*, const char* hwid, char* outToken, size_t tokenSz, char* outMsg, size_t msgSz, bool* outLicensed, std::string& err)
    {
        return ActivateEmail(nullptr, nullptr, hwid, outToken, tokenSz, outMsg, msgSz, outLicensed, err);
    }

    inline bool ActivateDiscord(const char*, const char* hwid, char* outToken, size_t tokenSz, char* outMsg, size_t msgSz, std::string& err)
    {
        bool licensed = true;
        return ActivateEmail(nullptr, nullptr, hwid, outToken, tokenSz, outMsg, msgSz, &licensed, err);
    }

    inline bool WriteSessionFile(const std::wstring& workDir, const char* token, const char* hwid)
    {
        CreateDirectoryW(workDir.c_str(), nullptr);
        std::wstring path = workDir + L"\\frontier.session";
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"w") != 0 || !f)
            return false;
        fprintf(f, "token=%s\nhwid=%s\n", token && token[0] ? token : kOpenToken, hwid ? hwid : "opensource");
        fclose(f);
        return true;
    }
}
