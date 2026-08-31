#pragma once
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"

namespace UIMotion {

    inline float Clamp01(float v)
    {
        if (v < 0.f) return 0.f;
        if (v > 1.f) return 1.f;
        return v;
    }

    inline float EaseOutCubic(float t)
    {
        t = Clamp01(t);
        const float u = 1.f - t;
        return 1.f - u * u * u;
    }

    inline float EaseOutQuart(float t)
    {
        t = Clamp01(t);
        const float u = 1.f - t;
        return 1.f - u * u * u * u;
    }

    inline float EaseInOutCubic(float t)
    {
        t = Clamp01(t);
        return t < 0.5f ? 4.f * t * t * t : 1.f - powf(-2.f * t + 2.f, 3.f) * 0.5f;
    }

    inline float SmoothTowards(float cur, float target, float dt, float speed = 14.f)
    {
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;
        return cur + (target - cur) * (1.f - expf(-speed * dt));
    }

    inline void NotifyTabChanged(int newTab)
    {
        if (variables::Misc::lastMenuTab != newTab) {
            variables::Misc::lastMenuTab = newTab;
            variables::Misc::tabContentAnim = 0.f;
        }
    }

    inline void Tick(float dt)
    {
        const float panelTarget = variables::Misc::floatingPanelOpen ? 1.f : 0.f;
        variables::Misc::floatingPanelAnim = SmoothTowards(
            variables::Misc::floatingPanelAnim, panelTarget, dt, 20.f);

        variables::Misc::tabContentAnim = SmoothTowards(
            variables::Misc::tabContentAnim, 1.f, dt, 14.f);

        if (variables::Misc::headerIntro < 1.f)
            variables::Misc::headerIntro = SmoothTowards(variables::Misc::headerIntro, 1.f, dt, 9.f);

        if (variables::Toast::show)
            variables::Misc::toastAnim = SmoothTowards(variables::Misc::toastAnim, 1.f, dt, 18.f);
        else
            variables::Misc::toastAnim = SmoothTowards(variables::Misc::toastAnim, 0.f, dt, 22.f);

        static bool toastWasShowing = false;
        if (variables::Toast::show && !toastWasShowing)
            variables::Misc::toastAnim = 0.f;
        toastWasShowing = variables::Toast::show;
    }

    inline float NavHover(int index)
    {
        if (index < 0 || index >= 9) return 0.f;
        return variables::Misc::navHover[index];
    }

    inline float IconHover(int index)
    {
        if (index < 0 || index >= 9) return 0.f;
        return variables::Misc::iconHover[index];
    }

    inline void SetNavHoverTarget(int index, bool hovered, float dt)
    {
        if (index < 0 || index >= 9) return;
        variables::Misc::navHover[index] = SmoothTowards(
            variables::Misc::navHover[index], hovered ? 1.f : 0.f, dt, 16.f);
    }

    inline void SetIconHoverTarget(int index, bool hovered, float dt)
    {
        if (index < 0 || index >= 9) return;
        variables::Misc::iconHover[index] = SmoothTowards(
            variables::Misc::iconHover[index], hovered ? 1.f : 0.f, dt, 18.f);
    }
}
