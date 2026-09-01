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
        case 0: // Frontier Green
            variables::Theme::accent[0] = 0.f; variables::Theme::accent[1] = 1.f;
            variables::Theme::accent[2] = 0.f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bgEffect = false;
            variables::Theme::snowEffect = false;
            variables::Theme::bg[0] = 0.039f; variables::Theme::bg[1] = 0.039f;
            variables::Theme::bg[2] = 0.039f; variables::Theme::bg[3] = 0.98f;
            variables::Theme::card[0] = 0.118f; variables::Theme::card[1] = 0.118f;
            variables::Theme::card[2] = 0.118f; variables::Theme::card[3] = 1.f;
            variables::Theme::border[0] = 1.f; variables::Theme::border[1] = 1.f;
            variables::Theme::border[2] = 1.f; variables::Theme::border[3] = 0.05f;
            variables::Theme::text[0] = 1.f; variables::Theme::text[1] = 1.f;
            variables::Theme::text[2] = 1.f; variables::Theme::text[3] = 1.f;
            variables::Theme::textDim[0] = 0.612f; variables::Theme::textDim[1] = 0.639f;
            variables::Theme::textDim[2] = 0.686f; variables::Theme::textDim[3] = 1.f;
            break;
        case 1: // Violet
            variables::Theme::accent[0] = 0.718f; variables::Theme::accent[1] = 0.627f;
            variables::Theme::accent[2] = 0.965f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bgEffect = false;
            variables::Theme::snowEffect = false;
            variables::Theme::bg[0] = 0.055f; variables::Theme::bg[1] = 0.055f;
            variables::Theme::bg[2] = 0.063f; variables::Theme::bg[3] = 0.96f;
            variables::Theme::card[0] = 0.090f; variables::Theme::card[1] = 0.090f;
            variables::Theme::card[2] = 0.102f; variables::Theme::card[3] = 1.f;
            variables::Theme::border[0] = 0.718f; variables::Theme::border[1] = 0.627f;
            variables::Theme::border[2] = 0.965f; variables::Theme::border[3] = 0.12f;
            variables::Theme::text[0] = 0.98f; variables::Theme::text[1] = 0.96f;
            variables::Theme::text[2] = 1.f; variables::Theme::text[3] = 1.f;
            variables::Theme::textDim[0] = 0.65f; variables::Theme::textDim[1] = 0.62f;
            variables::Theme::textDim[2] = 0.78f; variables::Theme::textDim[3] = 1.f;
            break;
        case 2: // Ice
            variables::Theme::accent[0] = 0.45f; variables::Theme::accent[1] = 0.78f;
            variables::Theme::accent[2] = 0.98f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bgEffect = false;
            variables::Theme::snowEffect = true;
            variables::Theme::bg[0] = 0.04f; variables::Theme::bg[1] = 0.05f;
            variables::Theme::bg[2] = 0.07f; variables::Theme::bg[3] = 0.98f;
            variables::Theme::card[0] = 0.07f; variables::Theme::card[1] = 0.09f;
            variables::Theme::card[2] = 0.12f; variables::Theme::card[3] = 1.f;
            variables::Theme::border[0] = 0.45f; variables::Theme::border[1] = 0.78f;
            variables::Theme::border[2] = 0.98f; variables::Theme::border[3] = 0.10f;
            variables::Theme::text[0] = 0.92f; variables::Theme::text[1] = 0.96f;
            variables::Theme::text[2] = 1.f; variables::Theme::text[3] = 1.f;
            variables::Theme::textDim[0] = 0.58f; variables::Theme::textDim[1] = 0.68f;
            variables::Theme::textDim[2] = 0.78f; variables::Theme::textDim[3] = 1.f;
            break;
        case 3: // OLED
            variables::Theme::accent[0] = 1.f; variables::Theme::accent[1] = 1.f;
            variables::Theme::accent[2] = 1.f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bgEffect = false;
            variables::Theme::snowEffect = false;
            variables::Theme::bg[0] = 0.f; variables::Theme::bg[1] = 0.f;
            variables::Theme::bg[2] = 0.f; variables::Theme::bg[3] = 1.f;
            variables::Theme::card[0] = 0.04f; variables::Theme::card[1] = 0.04f;
            variables::Theme::card[2] = 0.04f; variables::Theme::card[3] = 1.f;
            variables::Theme::border[0] = 1.f; variables::Theme::border[1] = 1.f;
            variables::Theme::border[2] = 1.f; variables::Theme::border[3] = 0.08f;
            variables::Theme::text[0] = 1.f; variables::Theme::text[1] = 1.f;
            variables::Theme::text[2] = 1.f; variables::Theme::text[3] = 1.f;
            variables::Theme::textDim[0] = 0.55f; variables::Theme::textDim[1] = 0.55f;
            variables::Theme::textDim[2] = 0.55f; variables::Theme::textDim[3] = 1.f;
            break;
        case 4: // Liquid Glass
            variables::Theme::accent[0] = 0.48f; variables::Theme::accent[1] = 0.78f;
            variables::Theme::accent[2] = 1.f; variables::Theme::accent[3] = 0.98f;
            variables::Theme::bg[0] = 0.045f; variables::Theme::bg[1] = 0.065f;
            variables::Theme::bg[2] = 0.11f; variables::Theme::bg[3] = 0.82f;
            variables::Theme::card[0] = 0.14f; variables::Theme::card[1] = 0.19f;
            variables::Theme::card[2] = 0.28f; variables::Theme::card[3] = 0.55f;
            variables::Theme::border[0] = 0.62f; variables::Theme::border[1] = 0.84f;
            variables::Theme::border[2] = 1.f; variables::Theme::border[3] = 0.28f;
            variables::Theme::text[0] = 0.97f; variables::Theme::text[1] = 0.98f;
            variables::Theme::text[2] = 1.f; variables::Theme::text[3] = 1.f;
            variables::Theme::textDim[0] = 0.68f; variables::Theme::textDim[1] = 0.76f;
            variables::Theme::textDim[2] = 0.88f; variables::Theme::textDim[3] = 1.f;
            variables::Theme::bgEffect = true;
            variables::Theme::snowEffect = false;
            break;
        case 5: // Crimson
            variables::Theme::accent[0] = 0.98f; variables::Theme::accent[1] = 0.22f;
            variables::Theme::accent[2] = 0.18f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bgEffect = false;
            variables::Theme::snowEffect = false;
            variables::Theme::bg[0] = 0.05f; variables::Theme::bg[1] = 0.03f;
            variables::Theme::bg[2] = 0.04f; variables::Theme::bg[3] = 0.98f;
            variables::Theme::card[0] = 0.14f; variables::Theme::card[1] = 0.06f;
            variables::Theme::card[2] = 0.06f; variables::Theme::card[3] = 1.f;
            variables::Theme::border[0] = 0.98f; variables::Theme::border[1] = 0.22f;
            variables::Theme::border[2] = 0.18f; variables::Theme::border[3] = 0.15f;
            variables::Theme::text[0] = 1.f; variables::Theme::text[1] = 0.94f;
            variables::Theme::text[2] = 0.92f; variables::Theme::text[3] = 1.f;
            variables::Theme::textDim[0] = 0.72f; variables::Theme::textDim[1] = 0.55f;
            variables::Theme::textDim[2] = 0.52f; variables::Theme::textDim[3] = 1.f;
            break;
        case 6: // Midnight
            variables::Theme::accent[0] = 0.38f; variables::Theme::accent[1] = 0.55f;
            variables::Theme::accent[2] = 0.98f; variables::Theme::accent[3] = 1.f;
            variables::Theme::bgEffect = true;
            variables::Theme::snowEffect = false;
            variables::Theme::bg[0] = 0.02f; variables::Theme::bg[1] = 0.025f;
            variables::Theme::bg[2] = 0.045f; variables::Theme::bg[3] = 0.99f;
            variables::Theme::card[0] = 0.05f; variables::Theme::card[1] = 0.06f;
            variables::Theme::card[2] = 0.09f; variables::Theme::card[3] = 1.f;
            variables::Theme::border[0] = 0.38f; variables::Theme::border[1] = 0.55f;
            variables::Theme::border[2] = 0.98f; variables::Theme::border[3] = 0.10f;
            variables::Theme::text[0] = 0.88f; variables::Theme::text[1] = 0.90f;
            variables::Theme::text[2] = 0.98f; variables::Theme::text[3] = 1.f;
            variables::Theme::textDim[0] = 0.48f; variables::Theme::textDim[1] = 0.54f;
            variables::Theme::textDim[2] = 0.68f; variables::Theme::textDim[3] = 1.f;
            break;
        default:
            break;
        }
        SyncBrandFromAccent();
        MarkDirty();
    }

    inline const char* LayoutLabel() {
        return "Universal";
    }
}
