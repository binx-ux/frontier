#pragma once
#include "debug_log.h"
#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"
#include <Windows.h>
#include <cstdio>

namespace UILog {

    inline bool Enabled()
    {
        static int cached = -1;
        if (cached >= 0)
            return cached != 0;

        char v[16]{};
        DWORD n = GetEnvironmentVariableA("FRONTIER_UI_LOG", v, sizeof(v));
        if (n > 0) {
            cached = (_stricmp(v, "0") == 0 || _stricmp(v, "false") == 0) ? 0 : 1;
        } else {
            cached = 1; // default on for crash diagnosis
        }
        return cached != 0;
    }

    inline void Write(const char* fmt, ...)
    {
        if (!Enabled())
            return;
        char msg[512]{};
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);
        DebugLog::Write("[UI] %s", msg);
    }

    inline void LogImGuiStacks(const char* where)
    {
        if (!Enabled())
            return;
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (!ctx) {
            Write("%s: no ImGui context", where);
            return;
        }
        ImGuiContext& g = *ctx;
        Write("%s stacks win=%d col=%d var=%d font=%d",
            where,
            g.CurrentWindowStack.Size,
            g.ColorStack.Size,
            g.StyleVarStack.Size,
            g.FontStack.Size);
    }
}
