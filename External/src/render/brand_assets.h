#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <cstdio>
#include <vector>
#include "avatar_cache.h"

namespace BrandAssets {

    inline ID3D11ShaderResourceView* logoSrv = nullptr;
    inline int logoW = 0;
    inline int logoH = 0;
    inline bool logoTried = false;

    inline bool ReadFileBytes(const char* path, std::vector<uint8_t>& out)
    {
        FILE* f = nullptr;
        if (fopen_s(&f, path, "rb") != 0 || !f)
            return false;
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz <= 0) {
            fclose(f);
            return false;
        }
        out.resize(static_cast<size_t>(sz));
        size_t got = fread(out.data(), 1, out.size(), f);
        fclose(f);
        return got == out.size();
    }

    inline bool LoadLogo(ID3D11Device* device)
    {
        if (logoSrv)
            return true;
        if (logoTried || !device)
            return false;
        logoTried = true;

        AvatarCache::Init(device);

        char exeDir[MAX_PATH]{};
        if (GetModuleFileNameA(nullptr, exeDir, MAX_PATH) == 0)
            return false;
        char* slash = strrchr(exeDir, '\\');
        if (slash) *slash = 0;

        const char* candidates[] = {
            "%s\\assets\\frontier-discord-rpc.png",
            "%s\\..\\External\\assets\\frontier-discord-rpc.png",
            "%s\\..\\..\\External\\assets\\frontier-discord-rpc.png",
        };

        std::vector<uint8_t> png;
        char path[MAX_PATH]{};
        for (const char* fmt : candidates) {
            sprintf_s(path, fmt, exeDir);
            if (ReadFileBytes(path, png))
                break;
            png.clear();
        }
        if (png.empty())
            return false;

        logoSrv = AvatarCache::CreateSrvFromPng(png, logoW, logoH);
        return logoSrv != nullptr;
    }

    inline ImTextureID LogoTex()
    {
        return reinterpret_cast<ImTextureID>(logoSrv);
    }
}
