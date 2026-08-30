#pragma once
#include <cstring>
#include "../core/variables/variables.h"

namespace FrontierTheme {

    inline void SyncBrandFromAccent() {
        if (!variables::Theme::linkBrandAccent) return;
        memcpy(variables::Theme::brand, variables::Theme::accent, sizeof(float) * 4);
    }

    inline void MarkDirty() {
        variables::Theme::styleDirty = true;
    }

    inline void ApplyPreset(int preset) {
        variables::Theme::preset = preset;
        switch (preset) {
        case 0: // Frontier Red
            variables::Theme::accent[0] = 0.92f; variables::Theme::accent[1] = 0.22f;
            variables::Theme::accent[2] = 0.28f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bg[0] = 0.047f; variables::Theme::bg[1] = 0.047f;
            variables::Theme::bg[2] = 0.047f; variables::Theme::bg[3] = 0.98f;
            variables::Theme::card[0] = 0.082f; variables::Theme::card[1] = 0.082f;
            variables::Theme::card[2] = 0.082f; variables::Theme::card[3] = 1.f;
            break;
        case 1: // Violet
            variables::Theme::accent[0] = 0.718f; variables::Theme::accent[1] = 0.627f;
            variables::Theme::accent[2] = 0.965f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bg[0] = 0.055f; variables::Theme::bg[1] = 0.055f;
            variables::Theme::bg[2] = 0.063f; variables::Theme::bg[3] = 0.96f;
            variables::Theme::card[0] = 0.090f; variables::Theme::card[1] = 0.090f;
            variables::Theme::card[2] = 0.102f; variables::Theme::card[3] = 1.f;
            break;
        case 2: // Ice
            variables::Theme::accent[0] = 0.45f; variables::Theme::accent[1] = 0.78f;
            variables::Theme::accent[2] = 0.98f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bg[0] = 0.04f; variables::Theme::bg[1] = 0.05f;
            variables::Theme::bg[2] = 0.07f; variables::Theme::bg[3] = 0.98f;
            variables::Theme::card[0] = 0.07f; variables::Theme::card[1] = 0.09f;
            variables::Theme::card[2] = 0.12f; variables::Theme::card[3] = 1.f;
            break;
        case 3: // OLED
            variables::Theme::accent[0] = 1.f; variables::Theme::accent[1] = 1.f;
            variables::Theme::accent[2] = 1.f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bg[0] = 0.f; variables::Theme::bg[1] = 0.f;
            variables::Theme::bg[2] = 0.f; variables::Theme::bg[3] = 1.f;
            variables::Theme::card[0] = 0.04f; variables::Theme::card[1] = 0.04f;
            variables::Theme::card[2] = 0.04f; variables::Theme::card[3] = 1.f;
            break;
        default:
            break;
        }
        SyncBrandFromAccent();
        MarkDirty();
    }

    inline const char* LayoutLabel() {
        switch (variables::Theme::layoutMode) {
        case 1: return "Rail";
        default: return "Standard";
        }
    }
}
