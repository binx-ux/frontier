#pragma once
#include <Windows.h>
#include <ShlObj.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include "../variables/variables.h"

// Lightweight config dump next to Documents\FRONTIER\config.ini
namespace ConfigIO {

    inline std::wstring ConfigPath()
    {
        wchar_t docs[MAX_PATH]{};
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs)))
            return L"Frontier-config.ini";
        std::wstring dir = std::wstring(docs) + L"\\FRONTIER";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\config.ini";
    }

    inline void W(FILE* f, const char* k, bool v) { fprintf(f, "%s=%d\n", k, v ? 1 : 0); }
    inline void W(FILE* f, const char* k, int v) { fprintf(f, "%s=%d\n", k, v); }
    inline void W(FILE* f, const char* k, float v) { fprintf(f, "%s=%.4f\n", k, v); }

    inline bool ParseLine(const char* line, char* key, char* val)
    {
        const char* eq = strchr(line, '=');
        if (!eq) return false;
        size_t kn = (size_t)(eq - line);
        if (kn >= 64) kn = 63;
        memcpy(key, line, kn);
        key[kn] = 0;
        strncpy_s(val, 64, eq + 1, _TRUNCATE);
        // strip CR
        char* cr = strchr(val, '\r');
        if (cr) *cr = 0;
        char* nl = strchr(val, '\n');
        if (nl) *nl = 0;
        return key[0] != 0;
    }

    inline bool B(const char* v) { return atoi(v) != 0; }

    inline float AimLerpCfg(float ui01to100, float lo, float hi) {
        float t = ui01to100 * 0.01f;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        return lo + (hi - lo) * t;
    }

    inline float AimUnlerpCfg(float value, float lo, float hi) {
        if (hi <= lo) return 0.f;
        float t = (value - lo) / (hi - lo);
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        return t * 100.f;
    }

    inline void ApplyAimFromUi() {
        using namespace variables::Aimbot;
        smoothing = AimLerpCfg(uiSmoothness, 4.f, 30.f);
        damping = AimLerpCfg(uiStability, 0.f, 0.85f);
        deadzone = AimLerpCfg(uiLockZone, 0.5f, 12.f);
        maxDistance = AimLerpCfg(uiRange, 100.f, 10000.f);
        fovRadius = AimLerpCfg(uiFov, 20.f, 500.f);
        silentFovRadius = AimLerpCfg(uiSilentFov, 20.f, 500.f);
        holdFovScale = AimLerpCfg(uiStickyFov, 1.0f, 1.8f);
        maxMove = AimLerpCfg(uiAimSpeed, 4.f, 28.f);
        variables::MagicBullet::fovRadius = AimLerpCfg(variables::MagicBullet::uiFov, 20.f, 500.f);
    }

    inline void SyncUiFromAimTuning() {
        using namespace variables::Aimbot;
        uiSmoothness = AimUnlerpCfg(smoothing, 4.f, 30.f);
        uiStability = AimUnlerpCfg(damping, 0.f, 0.85f);
        uiLockZone = AimUnlerpCfg(deadzone, 0.5f, 12.f);
        uiRange = AimUnlerpCfg(maxDistance, 100.f, 10000.f);
        uiFov = AimUnlerpCfg(fovRadius, 20.f, 500.f);
        uiSilentFov = AimUnlerpCfg(silentFovRadius, 20.f, 500.f);
        uiStickyFov = AimUnlerpCfg(holdFovScale, 1.0f, 1.8f);
        uiAimSpeed = AimUnlerpCfg(maxMove, 4.f, 28.f);
        variables::MagicBullet::uiFov = AimUnlerpCfg(variables::MagicBullet::fovRadius, 20.f, 500.f);
    }

    inline bool Save()
    {
        std::wstring path = ConfigPath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"w") != 0 || !f) return false;

        fprintf(f, "# FRONTIER config\n");
        W(f, "aim.enabled", variables::Aimbot::enabled);
        W(f, "aim.alwaysOn", variables::Aimbot::alwaysOn);
        W(f, "aim.showFOV", variables::Aimbot::showFOV);
        W(f, "aim.fovStyle", variables::Aimbot::fovStyle);
        W(f, "aim.sticky", variables::Aimbot::stickyAim);
        W(f, "aim.prediction", variables::Aimbot::prediction);
        W(f, "aim.wall", variables::Aimbot::requireVisible);
        W(f, "aim.type", variables::Aimbot::aimType);
        W(f, "aim.bone", variables::Aimbot::aimTarget);
        W(f, "aim.priority", variables::Aimbot::targetPriority);
        W(f, "aim.fov", variables::Aimbot::fovRadius);
        W(f, "aim.silentFov", variables::Aimbot::silentFovRadius);
        W(f, "aim.smooth", variables::Aimbot::smoothing);
        W(f, "aim.uiSmooth", variables::Aimbot::uiSmoothness);
        W(f, "aim.uiStability", variables::Aimbot::uiStability);
        W(f, "aim.uiLockZone", variables::Aimbot::uiLockZone);
        W(f, "aim.uiRange", variables::Aimbot::uiRange);
        W(f, "aim.uiFov", variables::Aimbot::uiFov);
        W(f, "aim.uiSilentFov", variables::Aimbot::uiSilentFov);
        W(f, "aim.uiStickyFov", variables::Aimbot::uiStickyFov);
        W(f, "aim.uiAimSpeed", variables::Aimbot::uiAimSpeed);
        W(f, "aim.key", variables::Aimbot::aimbotKey);
        W(f, "aim.silentKey", variables::Aimbot::silentAimKey);
        W(f, "aim.fovFollowMouse", variables::Aimbot::fovFollowMouse);
        W(f, "aim.toggle", variables::Aimbot::toggleMode);
        W(f, "teamCheck", variables::teamCheck);
        W(f, "healthCheck", variables::healthCheck);

        W(f, "trigger.enabled", variables::Trigger::enabled);
        W(f, "trigger.onStart", variables::Trigger::enableOnStart);
        W(f, "trigger.key", variables::Trigger::key);
        W(f, "trigger.delay", variables::Trigger::delayMs);
        W(f, "trigger.release", variables::Trigger::releaseMs);
        W(f, "trigger.headOnly", variables::Trigger::headOnly);
        W(f, "trigger.visible", variables::Trigger::requireVisible);
        W(f, "trigger.useHotkey", variables::Trigger::useHotkey);
        W(f, "trigger.players", variables::Trigger::targetPlayers);
        W(f, "trigger.npc", variables::Trigger::targetNpc);
        W(f, "trigger.dead", variables::Trigger::targetDead);
        W(f, "trigger.radius", variables::Trigger::hitRadius);

        W(f, "hitbox.enabled", variables::Hitbox::enabled);
        W(f, "hitbox.key", variables::Hitbox::key);
        W(f, "hitbox.size", variables::Hitbox::size);
        W(f, "magic.enabled", variables::MagicBullet::enabled);
        W(f, "magic.key", variables::MagicBullet::key);
        W(f, "magic.uiFov", variables::MagicBullet::uiFov);
        W(f, "magic.hitbox", variables::MagicBullet::hitbox);
        W(f, "magic.showFov", variables::MagicBullet::showFov);

        W(f, "esp.enabled", variables::ESP::enabled);
        W(f, "esp.boxes", variables::ESP::boxes);
        W(f, "esp.names", variables::ESP::names);
        W(f, "esp.health", variables::ESP::healthBar);
        W(f, "esp.distance", variables::ESP::distance);
        W(f, "esp.skeleton", variables::ESP::skeleton);
        W(f, "esp.chams", variables::ESP::chamsEnabled);
        W(f, "esp.tracers", variables::ESP::snaplines);
        W(f, "esp.tracerFrom", variables::ESP::snaplinesOrigin);
        W(f, "esp.oof", variables::ESP::oofArrows);
        W(f, "esp.chinaHat", variables::ESP::chinaHat);
        W(f, "esp.lookDir", variables::ESP::lookDir);
        W(f, "esp.flags", variables::ESP::flags);
        W(f, "esp.armor", variables::ESP::armorBar);
        W(f, "esp.rainbow", variables::ESP::rainbow);
        W(f, "esp.boxGlow", variables::ESP::boxGlow);
        W(f, "esp.pfp", variables::ESP::profilePicture);
        W(f, "esp.maxDist", variables::ESP::maxDistance);
        W(f, "esp.boxType", variables::ESP::boxType);

        W(f, "radar.enabled", variables::Radar::enabled);
        W(f, "crosshair.enabled", variables::Crosshair::enabled);

        W(f, "local.speed", variables::Local::speedEnabled);
        W(f, "local.speedAmt", variables::Local::walkSpeed);
        W(f, "local.fly", variables::Local::flyEnabled);
        W(f, "local.flyAmt", variables::Local::flySpeed);
        W(f, "local.flyKey", variables::Local::flyKey);
        W(f, "local.jump", variables::Local::jumpEnabled);
        W(f, "local.infJump", variables::Local::infJump);
        W(f, "local.noclip", variables::Local::noclip);
        W(f, "local.bhop", variables::Local::bhopEnabled);
        W(f, "local.god", variables::Local::godMode);
        W(f, "local.antiFling", variables::Local::antiFling);

        W(f, "world.fullbright", variables::World::fullbright);
        W(f, "world.noFog", variables::World::noFog);
        W(f, "world.noShadows", variables::World::noShadows);
        W(f, "world.night", variables::World::nightMode);
        W(f, "world.removeAtmo", variables::World::removeAtmosphere);
        W(f, "world.brightnessOn", variables::World::customBrightness);
        W(f, "world.brightness", variables::World::brightness);
        W(f, "world.clockOn", variables::World::customClock);
        W(f, "world.clock", variables::World::clockTime);
        W(f, "world.ambientOn", variables::World::customAmbient);
        W(f, "world.ambientR", variables::World::ambientColor[0]);
        W(f, "world.ambientG", variables::World::ambientColor[1]);
        W(f, "world.ambientB", variables::World::ambientColor[2]);
        W(f, "world.fov", variables::World::customFov);
        W(f, "world.fovAmt", variables::World::fovAmount);
        W(f, "world.forceFov", variables::World::viewmodelFov);
        W(f, "world.forceFovAmt", variables::World::viewmodelFovAmt);
        W(f, "world.third", variables::World::thirdPerson);
        W(f, "world.thirdDist", variables::World::thirdPersonDistance);
        W(f, "world.unlockZoom", variables::World::unlockZoom);
        W(f, "world.gunWire", variables::World::gunWireframe);
        W(f, "world.showVel", variables::World::showVelocity);

        W(f, "misc.streamproof", variables::Misc::streamProof);
        W(f, "misc.discordRpc", variables::Misc::discordRpc);
        W(f, "misc.antiAfk", variables::Misc::antiAfk);
        W(f, "misc.hitMarker", variables::Misc::hitMarker);
        W(f, "misc.dmgNumbers", variables::Misc::damageNumbers);
        W(f, "misc.enemyCounter", variables::Misc::enemyCounter);
        W(f, "misc.targetHud", variables::Misc::targetHud);
        W(f, "misc.spectatorList", variables::Extra::spectatorList);
        W(f, "extra.humanize", variables::Extra::humanizeAim);
        W(f, "extra.humanizeAmt", variables::Extra::humanizeAmount);
        W(f, "extra.aimOnShot", variables::Extra::aimOnShot);
        W(f, "extra.meleeAura", variables::Extra::meleeAura);
        W(f, "extra.burst", variables::Extra::burstTrigger);
        W(f, "extra.burstCount", variables::Extra::burstCount);
        W(f, "gun.fastReload", variables::GunMods::fastReload);
        W(f, "gun.fastFire", variables::GunMods::fastFire);
        W(f, "gun.noSpread", variables::GunMods::noSpread);
        W(f, "gun.noRecoil", variables::GunMods::noRecoil);
        W(f, "gun.infAmmo", variables::GunMods::infiniteAmmo);
        W(f, "gun.maxPen", variables::GunMods::maxPenetration);
        W(f, "gun.fireRate", variables::GunMods::fireRate);
        W(f, "gun.reloadTime", variables::GunMods::reloadTime);
        W(f, "audio.hitSounds", variables::Audio::hitSounds);
        W(f, "audio.killSounds", variables::Audio::killSounds);
        W(f, "audio.musicSource", variables::Audio::musicSource);
        W(f, "audio.musicVolume", variables::Audio::musicVolume);
        W(f, "audio.musicLoop", variables::Audio::musicLoop);
        W(f, "servers.sort", variables::Servers::sortMode);
        W(f, "servers.autoRefresh", variables::Servers::autoRefresh);
        W(f, "spotify", variables::Audio::spotifyMini);
        W(f, "floatingHeader", variables::Theme::useFloatingHeader);
        W(f, "theme.bgEffect", variables::Theme::bgEffect);
        W(f, "theme.snow", variables::Theme::snowEffect);
        W(f, "theme.preset", variables::Theme::preset);
        W(f, "theme.layout", variables::Theme::layoutMode);
        W(f, "theme.subTab", variables::Theme::subTabStyle);
        W(f, "theme.menuScale", variables::Theme::menuScale);
        W(f, "theme.headerY", variables::Theme::headerY);
        W(f, "theme.linkBrand", variables::Theme::linkBrandAccent);
        W(f, "theme.footerLink", variables::Theme::showFooterLink);
        W(f, "theme.brandR", variables::Theme::brand[0]);
        W(f, "theme.brandG", variables::Theme::brand[1]);
        W(f, "theme.brandB", variables::Theme::brand[2]);
        W(f, "theme.accentR", variables::Theme::accent[0]);
        W(f, "theme.accentG", variables::Theme::accent[1]);
        W(f, "theme.accentB", variables::Theme::accent[2]);
        W(f, "theme.bgR", variables::Theme::bg[0]);
        W(f, "theme.bgG", variables::Theme::bg[1]);
        W(f, "theme.bgB", variables::Theme::bg[2]);
        W(f, "theme.cardR", variables::Theme::card[0]);
        W(f, "theme.cardG", variables::Theme::card[1]);
        W(f, "theme.cardB", variables::Theme::card[2]);
        W(f, "misc.menuVk", variables::Misc::menuVk);
        W(f, "misc.panicVk", variables::Misc::panicVk);
        W(f, "misc.panicKey", variables::Misc::panicKey);
        W(f, "misc.showFps", variables::Misc::showFps);
        W(f, "misc.showKeybinds", variables::Misc::showKeybinds);
        W(f, "perf.vsync", variables::Perf::vsync);
        W(f, "perf.targetFps", variables::Perf::targetFps);
        W(f, "crosshair.follow", variables::Crosshair::followTarget);

        fclose(f);
        return true;
    }

    inline void ApplyConfigLine(const char* key, const char* val)
    {
        auto eq = [&](const char* k) { return _stricmp(key, k) == 0; };
        if (eq("aim.enabled")) variables::Aimbot::enabled = B(val);
        else if (eq("aim.alwaysOn")) variables::Aimbot::alwaysOn = B(val);
        else if (eq("aim.showFOV")) variables::Aimbot::showFOV = B(val);
        else if (eq("aim.fovStyle")) variables::Aimbot::fovStyle = atoi(val);
        else if (eq("aim.sticky")) variables::Aimbot::stickyAim = B(val);
        else if (eq("aim.prediction")) variables::Aimbot::prediction = B(val);
        else if (eq("aim.wall")) variables::Aimbot::requireVisible = B(val);
        else if (eq("aim.type")) { variables::Aimbot::aimType = atoi(val); variables::Aimbot::silentAim = variables::Aimbot::aimType == 1; }
        else if (eq("aim.bone")) variables::Aimbot::aimTarget = atoi(val);
        else if (eq("aim.priority")) variables::Aimbot::targetPriority = atoi(val);
        else if (eq("aim.fov")) variables::Aimbot::fovRadius = (float)atof(val);
        else if (eq("aim.silentFov")) variables::Aimbot::silentFovRadius = (float)atof(val);
        else if (eq("aim.smooth")) variables::Aimbot::smoothing = (float)atof(val);
        else if (eq("aim.uiSmooth")) variables::Aimbot::uiSmoothness = (float)atof(val);
        else if (eq("aim.uiStability")) variables::Aimbot::uiStability = (float)atof(val);
        else if (eq("aim.uiLockZone")) variables::Aimbot::uiLockZone = (float)atof(val);
        else if (eq("aim.uiRange")) variables::Aimbot::uiRange = (float)atof(val);
        else if (eq("aim.uiFov")) variables::Aimbot::uiFov = (float)atof(val);
        else if (eq("aim.uiSilentFov")) variables::Aimbot::uiSilentFov = (float)atof(val);
        else if (eq("aim.uiStickyFov")) variables::Aimbot::uiStickyFov = (float)atof(val);
        else if (eq("aim.uiAimSpeed")) variables::Aimbot::uiAimSpeed = (float)atof(val);
        else if (eq("aim.key")) variables::Aimbot::aimbotKey = atoi(val);
        else if (eq("aim.silentKey")) variables::Aimbot::silentAimKey = atoi(val);
        else if (eq("aim.fovFollowMouse")) variables::Aimbot::fovFollowMouse = B(val);
        else if (eq("aim.toggle")) variables::Aimbot::toggleMode = B(val);
        else if (eq("teamCheck")) { variables::teamCheck = B(val); variables::ESP::teamCheck = variables::teamCheck; }
        else if (eq("healthCheck")) variables::healthCheck = B(val);
        else if (eq("trigger.enabled")) variables::Trigger::enabled = B(val);
        else if (eq("trigger.onStart")) variables::Trigger::enableOnStart = B(val);
        else if (eq("trigger.key")) variables::Trigger::key = atoi(val);
        else if (eq("trigger.delay")) variables::Trigger::delayMs = (float)atof(val);
        else if (eq("trigger.release")) variables::Trigger::releaseMs = (float)atof(val);
        else if (eq("trigger.headOnly")) variables::Trigger::headOnly = B(val);
        else if (eq("trigger.visible")) variables::Trigger::requireVisible = B(val);
        else if (eq("trigger.useHotkey")) variables::Trigger::useHotkey = B(val);
        else if (eq("trigger.players")) variables::Trigger::targetPlayers = B(val);
        else if (eq("trigger.npc")) variables::Trigger::targetNpc = B(val);
        else if (eq("trigger.dead")) variables::Trigger::targetDead = B(val);
        else if (eq("trigger.radius")) variables::Trigger::hitRadius = (float)atof(val);
        else if (eq("hitbox.enabled")) {
            variables::Hitbox::enabled = B(val);
            variables::MagicBullet::enabled = variables::Hitbox::enabled;
        }
        else if (eq("hitbox.key")) {
            variables::Hitbox::key = atoi(val);
            variables::MagicBullet::key = variables::Hitbox::key;
        }
        else if (eq("hitbox.size")) variables::Hitbox::size = (float)atof(val);
        else if (eq("magic.enabled")) variables::MagicBullet::enabled = B(val);
        else if (eq("magic.key")) {
            variables::MagicBullet::key = atoi(val);
            variables::Hitbox::key = variables::MagicBullet::key;
        }
        else if (eq("magic.uiFov")) variables::MagicBullet::uiFov = (float)atof(val);
        else if (eq("magic.hitbox")) variables::MagicBullet::hitbox = atoi(val);
        else if (eq("magic.showFov")) variables::MagicBullet::showFov = B(val);
        else if (eq("esp.tracerFrom")) variables::ESP::snaplinesOrigin = atoi(val);
        else if (eq("esp.enabled")) variables::ESP::enabled = B(val);
        else if (eq("esp.boxes")) variables::ESP::boxes = B(val);
        else if (eq("esp.names")) variables::ESP::names = B(val);
        else if (eq("esp.health")) variables::ESP::healthBar = B(val);
        else if (eq("esp.distance")) variables::ESP::distance = B(val);
        else if (eq("esp.skeleton")) variables::ESP::skeleton = B(val);
        else if (eq("esp.chams")) variables::ESP::chamsEnabled = B(val);
        else if (eq("esp.tracers")) variables::ESP::snaplines = B(val);
        else if (eq("esp.oof")) variables::ESP::oofArrows = B(val);
        else if (eq("esp.chinaHat")) variables::ESP::chinaHat = B(val);
        else if (eq("esp.lookDir")) variables::ESP::lookDir = B(val);
        else if (eq("esp.flags")) variables::ESP::flags = B(val);
        else if (eq("esp.armor")) variables::ESP::armorBar = B(val);
        else if (eq("esp.rainbow")) variables::ESP::rainbow = B(val);
        else if (eq("esp.boxGlow")) variables::ESP::boxGlow = B(val);
        else if (eq("esp.pfp")) variables::ESP::profilePicture = B(val);
        else if (eq("esp.maxDist")) variables::ESP::maxDistance = (float)atof(val);
        else if (eq("esp.boxType")) variables::ESP::boxType = atoi(val);
        else if (eq("radar.enabled")) variables::Radar::enabled = B(val);
        else if (eq("crosshair.enabled")) variables::Crosshair::enabled = B(val);
        else if (eq("local.speed")) variables::Local::speedEnabled = B(val);
        else if (eq("local.speedAmt")) variables::Local::walkSpeed = (float)atof(val);
        else if (eq("local.fly")) variables::Local::flyEnabled = B(val);
        else if (eq("local.flyAmt")) variables::Local::flySpeed = (float)atof(val);
        else if (eq("local.flyKey")) variables::Local::flyKey = atoi(val);
        else if (eq("local.jump")) variables::Local::jumpEnabled = B(val);
        else if (eq("local.infJump")) variables::Local::infJump = B(val);
        else if (eq("local.noclip")) variables::Local::noclip = B(val);
        else if (eq("local.bhop")) variables::Local::bhopEnabled = B(val);
        else if (eq("local.god")) variables::Local::godMode = B(val);
        else if (eq("local.antiFling")) variables::Local::antiFling = B(val);
        else if (eq("world.fullbright")) variables::World::fullbright = B(val);
        else if (eq("world.noFog")) variables::World::noFog = B(val);
        else if (eq("world.noShadows")) variables::World::noShadows = B(val);
        else if (eq("world.night")) variables::World::nightMode = B(val);
        else if (eq("world.removeAtmo")) variables::World::removeAtmosphere = B(val);
        else if (eq("world.brightnessOn")) variables::World::customBrightness = B(val);
        else if (eq("world.brightness")) variables::World::brightness = (float)atof(val);
        else if (eq("world.clockOn")) variables::World::customClock = B(val);
        else if (eq("world.clock")) variables::World::clockTime = (float)atof(val);
        else if (eq("world.ambientOn")) variables::World::customAmbient = B(val);
        else if (eq("world.ambientR")) variables::World::ambientColor[0] = (float)atof(val);
        else if (eq("world.ambientG")) variables::World::ambientColor[1] = (float)atof(val);
        else if (eq("world.ambientB")) variables::World::ambientColor[2] = (float)atof(val);
        else if (eq("world.fov")) variables::World::customFov = B(val);
        else if (eq("world.fovAmt")) variables::World::fovAmount = (float)atof(val);
        else if (eq("world.forceFov")) variables::World::viewmodelFov = B(val);
        else if (eq("world.forceFovAmt")) variables::World::viewmodelFovAmt = (float)atof(val);
        else if (eq("world.third")) variables::World::thirdPerson = B(val);
        else if (eq("world.thirdDist")) variables::World::thirdPersonDistance = (float)atof(val);
        else if (eq("world.unlockZoom")) variables::World::unlockZoom = B(val);
        else if (eq("world.gunWire")) variables::World::gunWireframe = B(val);
        else if (eq("world.showVel")) variables::World::showVelocity = B(val);
    }

    inline void ApplyConfigLineMisc(const char* key, const char* val)
    {
        auto eq = [&](const char* k) { return _stricmp(key, k) == 0; };
        if (eq("misc.streamproof")) variables::Misc::streamProof = B(val);
        else if (eq("misc.discordRpc")) variables::Misc::discordRpc = B(val);
        else if (eq("misc.antiAfk")) variables::Misc::antiAfk = B(val);
        else if (eq("misc.hitMarker")) variables::Misc::hitMarker = B(val);
        else if (eq("misc.dmgNumbers")) variables::Misc::damageNumbers = B(val);
        else if (eq("misc.enemyCounter")) variables::Misc::enemyCounter = B(val);
        else if (eq("misc.targetHud")) variables::Misc::targetHud = B(val);
        else if (eq("misc.spectatorList")) variables::Extra::spectatorList = B(val);
        else if (eq("extra.humanize")) variables::Extra::humanizeAim = B(val);
        else if (eq("extra.humanizeAmt")) variables::Extra::humanizeAmount = (float)atof(val);
        else if (eq("extra.aimOnShot")) variables::Extra::aimOnShot = B(val);
        else if (eq("extra.meleeAura")) variables::Extra::meleeAura = B(val);
        else if (eq("extra.burst")) variables::Extra::burstTrigger = B(val);
        else if (eq("extra.burstCount")) variables::Extra::burstCount = atoi(val);
        else if (eq("gun.fastReload")) variables::GunMods::fastReload = B(val);
        else if (eq("gun.fastFire")) variables::GunMods::fastFire = B(val);
        else if (eq("gun.noSpread")) variables::GunMods::noSpread = B(val);
        else if (eq("gun.noRecoil")) variables::GunMods::noRecoil = B(val);
        else if (eq("gun.infAmmo")) variables::GunMods::infiniteAmmo = B(val);
        else if (eq("gun.maxPen")) variables::GunMods::maxPenetration = B(val);
        else if (eq("gun.fireRate")) variables::GunMods::fireRate = (float)atof(val);
        else if (eq("gun.reloadTime")) variables::GunMods::reloadTime = (float)atof(val);
        else if (eq("gun.dmg")) { /* legacy */ }
        else if (eq("gun.range")) { /* legacy */ }
        else if (eq("audio.hitSounds")) variables::Audio::hitSounds = B(val);
        else if (eq("audio.killSounds")) variables::Audio::killSounds = B(val);
        else if (eq("audio.musicSource")) variables::Audio::musicSource = atoi(val);
        else if (eq("audio.musicVolume")) variables::Audio::musicVolume = (float)atof(val);
        else if (eq("audio.musicLoop")) variables::Audio::musicLoop = B(val);
        else if (eq("servers.sort")) variables::Servers::sortMode = atoi(val);
        else if (eq("servers.autoRefresh")) variables::Servers::autoRefresh = atoi(val);
        else if (eq("spotify")) variables::Audio::spotifyMini = B(val);
        else if (eq("floatingHeader")) variables::Theme::useFloatingHeader = B(val);
        else if (eq("theme.bgEffect")) variables::Theme::bgEffect = B(val);
        else if (eq("theme.snow")) variables::Theme::snowEffect = B(val);
        else if (eq("theme.preset")) variables::Theme::preset = atoi(val);
        else if (eq("theme.layout")) variables::Theme::layoutMode = atoi(val);
        else if (eq("theme.subTab")) variables::Theme::subTabStyle = atoi(val);
        else if (eq("theme.menuScale")) variables::Theme::menuScale = (float)atof(val);
        else if (eq("theme.headerY")) variables::Theme::headerY = (float)atof(val);
        else if (eq("theme.linkBrand")) variables::Theme::linkBrandAccent = B(val);
        else if (eq("theme.footerLink")) variables::Theme::showFooterLink = B(val);
        else if (eq("theme.brandR")) variables::Theme::brand[0] = (float)atof(val);
        else if (eq("theme.brandG")) variables::Theme::brand[1] = (float)atof(val);
        else if (eq("theme.brandB")) variables::Theme::brand[2] = (float)atof(val);
        else if (eq("theme.accentR")) variables::Theme::accent[0] = (float)atof(val);
        else if (eq("theme.accentG")) variables::Theme::accent[1] = (float)atof(val);
        else if (eq("theme.accentB")) variables::Theme::accent[2] = (float)atof(val);
        else if (eq("theme.bgR")) variables::Theme::bg[0] = (float)atof(val);
        else if (eq("theme.bgG")) variables::Theme::bg[1] = (float)atof(val);
        else if (eq("theme.bgB")) variables::Theme::bg[2] = (float)atof(val);
        else if (eq("theme.cardR")) variables::Theme::card[0] = (float)atof(val);
        else if (eq("theme.cardG")) variables::Theme::card[1] = (float)atof(val);
        else if (eq("theme.cardB")) variables::Theme::card[2] = (float)atof(val);
        else if (eq("misc.menuVk")) variables::Misc::menuVk = atoi(val);
        else if (eq("misc.panicVk")) variables::Misc::panicVk = atoi(val);
        else if (eq("misc.panicKey")) variables::Misc::panicKey = B(val);
        else if (eq("misc.showFps")) variables::Misc::showFps = B(val);
        else if (eq("misc.showKeybinds")) variables::Misc::showKeybinds = B(val);
        else if (eq("perf.vsync")) variables::Perf::vsync = B(val);
        else if (eq("perf.targetFps")) variables::Perf::targetFps = atoi(val);
        else if (eq("crosshair.follow")) variables::Crosshair::followTarget = B(val);
    }

    inline bool Load()
    {
        std::wstring path = ConfigPath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"r") != 0 || !f) return false;

        bool loadedUi = false;
        char line[256]{}, key[64]{}, val[64]{};
        while (fgets(line, sizeof(line), f)) {
            if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
            if (!ParseLine(line, key, val)) continue;
            if (_strnicmp(key, "aim.ui", 6) == 0 || _stricmp(key, "magic.uiFov") == 0)
                loadedUi = true;
            ApplyConfigLine(key, val);
            ApplyConfigLineMisc(key, val);
        }
        fclose(f);
        if (variables::Trigger::enableOnStart)
            variables::Trigger::enabled = true;
        if (loadedUi)
            ApplyAimFromUi();
        else
            SyncUiFromAimTuning();
        variables::Theme::styleDirty = true;
        return true;
    }

    inline void OpenFolder()
    {
        std::wstring path = ConfigPath();
        size_t slash = path.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            path.resize(slash);
        ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
