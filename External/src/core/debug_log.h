#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>

namespace DebugLog {

    inline std::mutex gMu;

    inline void Write(const char* fmt, ...)
    {
        char msg[512]{};
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        SYSTEMTIME st{};
        GetLocalTime(&st);

        char line[640];
        sprintf_s(line, "[%02u:%02u:%02u] %s\r\n",
            st.wHour, st.wMinute, st.wSecond, msg);

        std::lock_guard<std::mutex> lock(gMu);

        char path[MAX_PATH]{};
        char exeDir[MAX_PATH]{};
        if (GetModuleFileNameA(nullptr, exeDir, MAX_PATH) > 0) {
            char* slash = strrchr(exeDir, '\\');
            if (slash) *slash = 0;
            char dir[MAX_PATH]{};
            sprintf_s(dir, "%s\\frontier", exeDir);
            CreateDirectoryA(dir, nullptr);
            sprintf_s(path, "%s\\frontier.log", dir);
        } else {
            strcpy_s(path, "frontier.log");
        }

        FILE* f = nullptr;
        if (fopen_s(&f, path, "a") == 0 && f) {
            fputs(line, f);
            fclose(f);
        }

        OutputDebugStringA(line);
    }
}
