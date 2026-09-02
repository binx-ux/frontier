#pragma once
#include <Windows.h>
#include <atomic>

namespace variables {
    inline bool menuOpen = false;
    inline std::atomic<bool> pendingMenuReveal{ false };
    inline int selectedTab = 0;
    inline int selectedSub = 0;
    inline bool waitingForKey = false;
    inline int* keyToRebind = nullptr;
    inline bool teamCheck = false;
    inline bool healthCheck = false;

    namespace Loading {
        inline bool active = true;
        inline float progress = 0.0f;
        inline char status[128] = "Loading session";
        inline bool failed = false;
        inline char error[256] = "";
        inline float minSeconds = 2.0f;
    }

    namespace Toast {
        inline bool show = false;
        inline float timer = 0.0f;
        inline char title[64] = "Session Validated";
        inline char body[128] = "Welcome to FRONTIER";
        inline char footer[64] = "alpha";
        inline bool warning = false; // red accent toast
    }

    namespace Theme {
        inline float brand[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        inline float accent[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        inline float bg[4] = { 0.039f, 0.039f, 0.039f, 0.98f };
        inline float card[4] = { 0.118f, 0.118f, 0.118f, 1.0f };
        inline float border[4] = { 1.0f, 1.0f, 1.0f, 0.05f };
        inline float text[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline float textDim[4] = { 0.612f, 0.639f, 0.686f, 1.0f };
        inline bool bgEffect = false;
        inline bool snowEffect = false;
        inline bool useFloatingHeader = false;
        inline float headerY = 18.0f;
        inline int layoutMode = 0;
        inline int subTabStyle = 1;
        inline float menuScale = 1.0f;
        inline bool linkBrandAccent = true;
        inline bool styleDirty = true;
        inline int preset = 7; // AHEAD Premium default
        inline bool showFooterLink = false;
        inline bool bgVideoEnabled = false;
        inline float bgVideoOpacity = 0.55f;
        inline char bgVideoPath[512] = "";
    }

    namespace Perf {
        inline bool vsync = false;
        inline int targetFps = 144; // soft cap when vsync off (0 = uncapped)
        inline int playerUpdateEveryNFrames = 2;
        inline bool skipSkeletonWhenLowFps = true;
        inline int lowFpsThreshold = 45;
        inline int currentFps = 0;
    }

    namespace Aimbot {
        inline bool enabled = false;
        inline bool showFOV = false;
        inline int fovStyle = 0; // 0 Circle  1 Rotating dots
        inline bool fovGlow = false;
        inline bool fovFilled = false;
        inline bool stickyAim = true;
        inline bool prediction = false;
        inline bool requireVisible = false; // wall check
        inline bool alwaysOn = false;   // aim without holding key
        inline bool multiTarget = false;
        inline bool resolver = false;
        inline int aimType = 0;       // 0 Mouse move  1 Silent (MouseService)
        inline int targetMethod = 0; // 0 Closest to crosshair  1 Force silent target
        inline bool silentAim = false; // mirrors aimType==1 for clarity / menu toggle
        inline float fovRadius = 150.0f;
        inline float silentFovRadius = 180.0f;
        inline float holdFovScale = 1.35f; // sticky FOV hysteresis
        inline float fovOpacity = 0.5f;
        inline float fovColor[4] = { 1.f, 1.f, 1.f, 1.f };
        inline float smoothing = 8.0f;  // higher = slower (P gain = 1/smooth)
        inline float damping = 0.28f;    // D term — cuts overshoot
        inline float deadzone = 1.5f;
        inline float maxMove = 18.0f;
        inline float predictionX = 0.10f;
        inline float predictionY = 0.08f;
        inline float maxDistance = 10000.0f;
        inline float uiSmoothness = 28.f;
        inline float uiStability = 35.f;
        inline float uiLockZone = 10.f;
        inline float uiRange = 100.f;
        inline float uiFov = 35.f;
        inline float uiSilentFov = 40.f;
        inline float uiStickyFov = 44.f;
        inline float uiAimSpeed = 52.f;
        inline int aimTarget = 0;
        inline int aimbotKey = 0x02;
        inline int silentAimKey = 0;
        inline bool toggleMode = false;
        inline bool toggledOn = false;
        inline int smoothProfile = 0;
        inline int targetPriority = 0;
    }

    namespace Trigger {
        inline bool enabled = false;
        inline bool enableOnStart = false; // off by default; opt-in via Options
        inline int key = 0x05; // XBUTTON1 — separate from aim RMB
        inline float delayMs = 5.0f;
        inline float releaseMs = 12.0f;
        inline bool headOnly = false;
        inline bool requireVisible = false;
        inline bool useHotkey = true;
        inline bool targetPlayers = true;
        inline bool targetNpc = true;
        inline bool targetDead = true;
        inline float hitRadius = 18.0f; // base crosshair radius (px)
    }

    namespace Rage {
        inline bool enabled = false;
        inline float delayMs = 40.0f;
        inline bool shoot = true;
        inline bool teleport = false;
        inline float tpDistance = 2.5f;
        inline int key = 0x47; // G — toggle rage
        inline bool unkillable = true; // behind-target TP + health stick
    }

    namespace GunMods {
        inline bool fastReload = false;
        inline bool fastFire = false;
        inline bool alwaysAuto = false;
        inline bool noSpread = false;
        inline bool noRecoil = false;
        inline bool noSway = false;
        inline bool infiniteAmmo = false;
        inline bool maxPenetration = false;
        inline bool aggressive = true;   // Arsenal resets Values — re-apply often
        inline float reloadTime = 0.01f;
        inline float fireRate = 0.02f; // TRACE default — seconds between shots
    }

    namespace Hitbox {
        inline bool enabled = false;
        inline bool visualize = false;
        inline bool teamCheck = false;
        inline bool healthCheck = false;
        inline float size = 12.0f;
        inline int type = 1; // 0 HRP only  1 multi-part (recommended)
        inline int key = 0x42; // B — toggle hitbox / magic bullet
        inline bool aimAssist = true; // spoof mouse to bone when crosshair in big box
    }

    namespace MagicBullet {
        inline bool enabled = false;
        inline int key = 0x42; // B
        inline float uiFov = 40.f;
        inline float fovRadius = 150.f;
        inline int hitbox = 0; // 0 Head … 6 Closest (aim-assist bone)
        inline bool showFov = false;
        inline float fovColor[4] = { 0.f, 0.85f, 0.85f, 1.f };
    }

    namespace Desync {
        inline bool enabled = false;
        inline int key = 0x4E; // N — toggle desync
        inline bool removeWalkAnim = false;
        inline bool displayServerPos = false;
        inline bool resetOnOff = true;
        inline bool useTick = false;
    }

    namespace ESP {
        inline bool enabled = false;
        inline bool boxes = false;
        inline bool fillBox = false;
        inline bool cornerBox = false;
        inline bool names = false;
        inline int nameType = 0; // 0 Name 1 DisplayName
        inline bool healthText = false;
        inline int healthTextPos = 0; // 0 Above Name 1 Below
        inline bool distance = false;
        inline bool healthBar = false;
        inline bool healthBasedColor = true;
        inline bool snaplines = false;
        inline int snaplinesOrigin = 0; // 0 Top 1 Middle 2 Bottom 3 Mouse
        inline int snaplinesDestination = 0;
        inline int snaplinesStyle = 0;
        inline float snaplinesThickness = 1.0f;
        inline bool snaplinesOutline = true;
        inline bool skeleton = false;
        inline float skeletonThickness = 1.5f;
        inline bool skeletonOutline = true;
        inline bool deadCheck = false;
        inline bool teamCheck = false;
        inline bool headDot = false;
        inline bool headDotGlow = false;
        inline bool equippedItem = false;
        inline bool profilePicture = false;
        inline float boxThickness = 2.0f;
        inline float maxDistance = 1200.0f;
        inline int boxType = 0; // 0 2D 1 Cube 2 Corners
        inline int boxStyle = 1;
        inline bool oofArrows = false;
        inline bool oofShowPfp = true;
        inline float oofSize = 7.5f;
        inline float oofRadius = 100.0f;
        inline float oofDistance = 500.0f;
        inline float oofColor[4] = { 1, 1, 1, 1 };
        inline bool showPreview = false;
        inline bool selfEsp = false;
        inline bool weaponLabels = false;
        inline bool chamsEnabled = false;
        inline bool chamsFilled = true;
        inline int chamsMode = 0; // 0 Soft body  1 Glow
        inline int chamsRender = 0; // 0 Static  1 Pulse
        inline float chamsColor[4] = { 0.32f, 0.78f, 1.0f, 0.42f };
        inline float chamsOutline[4] = { 0.55f, 0.92f, 1.0f, 0.95f };
        inline float boxColor[4] = { 1, 1, 1, 1 };
        inline float boxFillColor[4] = { 1, 1, 1, 0.15f };
        inline float nameColor[4] = { 1, 1, 1, 1 };
        inline float snapColor[4] = { 1, 1, 1, 1 };
        inline float healthColor[4] = { 0.2f, 1.0f, 0.35f, 1 };
        inline float headDotColor[4] = { 1, 1, 1, 1 };
        inline bool rainbow = false;
        inline bool boxGlow = false;
        inline bool chinaHat = false;
        inline bool lookDir = false;
        inline bool flags = false;
        inline bool armorBar = false;
        inline bool teamColors = false;
        inline bool visibleOnly = false;
        inline bool wireframePlayers = false;
        inline bool offscreenPulse = false;
        inline bool targetHighlight = false;
    }

    namespace Crosshair {
        inline bool enabled = false;
        inline bool outline = true;
        inline bool centerDot = false;
        inline bool followTarget = false;
        inline int style = 0; // 0 Static 1 Spin
        inline float size = 10.0f;
        inline float length = 20.0f;
        inline float thickness = 2.0f;
        inline float gap = 4.0f;
        inline float outlineThickness = 1.5f;
        inline int segments = 4;
        inline float opacity = 1.0f;
        inline float color[4] = { 1, 1, 1, 1 };
        inline float outlineColor[4] = { 0, 0, 0, 1 };
        inline float centerDotColor[4] = { 1, 1, 1, 1 };
    }

    namespace Radar {
        inline bool enabled = false;
        inline int type = 0; // 0 2D 1 3D
        inline float size = 200.0f;
        inline float range = 300.0f;
        inline bool showNames = true;
        inline bool showDistance = true;
        inline bool rotateWithCamera = true;
        inline float posX = 20.0f;
        inline float posY = 20.0f;
    }

    namespace World {
        inline bool unlockZoom = false;
        inline float maxZoom = 400.0f;
        inline bool customFov = false;
        inline float fovAmount = 70.0f;
        inline bool showVelocity = false;
        inline bool fullbright = false;
        inline bool noFog = false;
        inline bool customBrightness = false;
        inline float brightness = 2.0f;
        inline bool gunWireframe = false;
        inline int gunWireStyle = 0;
        inline float gunWireAlpha = 0.35f;
        inline float gunWireColor[4] = { 0.82f, 0.82f, 0.86f, 1 };
        inline bool viewmodelFov = false;
        inline float viewmodelFovAmt = 70.0f;
        inline bool nightMode = false;
        inline bool noShadows = false;
        inline bool customClock = false;
        inline float clockTime = 14.f;
        inline bool customAmbient = false;
        inline float ambientR = 0.45f, ambientG = 0.55f, ambientB = 0.85f;
        inline float ambientColor[4] = { 0.45f, 0.55f, 0.85f, 1.f };
        inline bool removeAtmosphere = false;
        inline bool thirdPerson = false;
        inline float thirdPersonDistance = 14.f;
    }

    namespace Audio {
        inline bool hitSounds = false;
        inline bool killSounds = false;
        inline float hitVolume = 0.55f;
        inline float killVolume = 0.65f;
        inline bool music = false;
        inline float musicVolume = 0.55f;
        inline bool spotifyMini = false;
        inline bool musicLoop = true;
        inline int musicSource = 0; // 0 Spotify  1 Local file  2 Roblox ID
        inline char localPath[MAX_PATH] = "";
        inline char robloxId[64] = "";
        inline bool localPlaying = false;
        inline bool robloxApplied = false;
        inline bool openRobloxCatalog = false;
        inline char playlist[8][MAX_PATH]{};
        inline int playlistCount = 0;
    }

    namespace Exploits {
        inline bool animation_changer = false;
        inline int idle_animation = 0;
        inline int run_animation = 0;
        inline int walk_animation = 0;
        inline int jump_animation = 0;
        inline int fall_animation = 0;
        inline int climb_animation = 0;
        inline int swim_animation = 0;
    }

    namespace Local {
        inline bool speedEnabled = false;
        inline float walkSpeed = 16.0f;
        inline int speedMethod = 0; // 0 Velocity 1 Position 2 Slippery
        inline bool jumpEnabled = false;
        inline float jumpPower = 50.0f;
        inline bool flyEnabled = false;
        inline float flySpeed = 50.0f;
        inline int flyMethod = 0; // 0 Velocity 1 Position
        inline int flyKey = 'F';
        inline bool flyActive = false;
        inline bool desyncEnabled = false;
        inline bool bhopEnabled = false;
        inline float bhopSpeed = 30.0f;
        inline bool hitboxEnabled = false;
        inline float hitboxSize = 10.0f;
        inline bool visualizeHitbox = false;
        inline bool infJump = false;
        inline bool autoTp = false;
        inline int autoTpKey = 0x59; // Y
        inline float autoTpDelay = 0.0f; // 0 = every frame (fastest)
        inline bool floatEnabled = false;
        inline float floatHeight = 10.0f;
        inline bool noclip = false;
        inline bool antiFling = false;
        inline bool walkFling = false;
        inline float walkFlingPower = 180.f;
        inline float walkFlingRange = 4.5f; // touch distance
        inline int walkFlingKey = 0; // 0 = always when enabled
        inline bool clickTp = false;
        inline int clickTpKey = 0x48; // H
        inline bool freeze = false;
        inline int freezeKey = 0;
        inline bool spin = false;
        inline float spinSpeed = 20.0f;
        inline bool hipHeightEnabled = false;
        inline float hipHeight = 2.0f;
        inline int speedKey = 0;
        inline bool gravityEnabled = false;
        inline float gravity = 35.f;
        inline bool godMode = false;
        inline bool antiVoid = false;
        inline bool tpWalk = false;
        inline float tpWalkStep = 1.8f;
        inline bool orbitPlayer = false;
        inline float orbitSpeed = 3.5f;
        inline float orbitRadius = 10.f;
        inline bool autoClicker = false;
        inline int autoClickerKey = 0;
        inline float autoClickerCps = 10.f;
        inline bool sitSpam = false;
        inline bool vehicleBoost = false;
        inline float vehicleBoostAmt = 90.f;
    }

    namespace Servers {
        inline int sortMode = 0; // 0 Descending 1 Ascending
        inline int region = 0;   // 0 All
        inline int autoRefresh = 0; // 0 Disabled
        inline char currentId[64] = "—";
        inline int serverCount = 0;
        inline bool redirecting = false;
        inline float redirectTimer = 0.f;
        inline int disconnectKind = 0; // 0 redirect, 1 ban troll
        inline float redirectProgress = 0.f;
        inline char redirectMsg[160] = "Match is redirecting you, please wait";
        inline char redirectStatus[64] = "Joining server...";
        inline char searchFilter[64] = "";
    }

    namespace Status {
        inline char username[64] = "—";
        inline char displayName[64] = "—";
        inline char userId[32] = "—";
        inline char placeId[32] = "—";
        inline char gameId[32] = "—";
        inline char jobId[96] = "—";
        inline char clientVersion[64] = "—";
        inline char playersOnline[16] = "—";
        inline float lastRefresh = 0.f;
    }

    namespace Misc {
        inline bool streamProof = false;
        inline bool streamerMode = false;
        inline bool streamerModePlus = false;
        inline bool showFps = false;
        inline bool showKeybinds = false;
        inline bool panicKey = true;
        inline int panicVk = 0x2E;
        inline int menuVk = VK_INSERT;
        inline bool antiAfk = false;
        inline bool afkAssist = false; // leave-running: always-on aim + strong anti-AFK
        inline float antiAfkSeconds = 12.0f;
        inline bool menuHovered = false;
        inline bool menuDragging = false;
        inline float menuX = 0, menuY = 0, menuW = 600, menuH = 820;
        inline float spotX = 0, spotY = 0, spotW = 0, spotH = 0;
        inline float floatX = 0, floatY = 0, floatW = 0, floatH = 0;
        inline bool floatingPanelOpen = false;
        inline float floatingPanelAnim = 0.f;
        inline float tabContentAnim = 1.f;
        inline int lastMenuTab = 0;
        inline float headerIntro = 0.f;
        inline float toastAnim = 0.f;
        inline float navHover[9]{};
        inline float iconHover[9]{};
        inline float panelX = 0, panelY = 0, panelW = 0, panelH = 0;
        inline int selectedSubByTab[9] = {};
        inline HWND overlayHwnd = nullptr;
        inline int onlineCount = 0;
        inline float menuAnim = 0.f;
        inline float menuAnimSpeed = 14.f;
        inline bool watermark = false;
        inline bool enemyCounter = false;
        inline bool hitMarker = false;
        inline bool damageNumbers = false;
        inline bool targetHud = false;
        inline bool autoRejoin = false;
        inline bool fpsBoost = false;
        inline bool hideGui = false;
        inline bool discordRpc = false;
    }

    namespace Spoof {
        inline bool userIdEnabled = false;
        inline int64_t fakeUserId = 1;
        inline bool displayNameEnabled = false;
        inline char fakeDisplayName[64] = "Guest";
        inline bool usernameEnabled = false;
        inline char fakeUsername[64] = "Guest";
    }

    namespace Extra {
        inline bool autoClicker = false;
        inline int autoClickerKey = 0;
        inline float autoClickerCps = 12.f;
        inline bool rapidTap = false;
        inline bool meleeAura = false;
        inline float meleeRange = 12.f;
        inline bool autoReload = false;
        inline bool instantKillHint = false; // max damage push via gun mods
        inline bool noSway = false;
        inline bool pierceHint = false;
        inline bool burstTrigger = false;
        inline int burstCount = 3;
        inline bool humanizeAim = false;
        inline float humanizeAmount = 0.35f;
        inline bool randomBone = false;
        inline bool aimOnShot = false;
        inline bool fovRainbow = false;
        inline bool chinaHat = false;
        inline bool lookDirection = false;
        inline bool flagsEsp = false;
        inline bool armorBar = false;
        inline bool rainbowEsp = false;
        inline bool boxGlow = false;
        inline bool visibleOnlyEsp = false;
        inline bool teamColorEsp = false;
        inline bool spectatorList = false;
        inline bool thirdPerson = false;
        inline float thirdPersonDist = 12.f;
        inline bool freecam = false;
        inline float freecamSpeed = 40.f;
        inline bool nightMode = false;
        inline bool customAmbient = false;
        inline float ambientColor[4] = { 0.55f, 0.55f, 0.6f, 1.f };
        inline bool customClock = false;
        inline float clockTime = 14.f;
        inline bool noShadows = false;
        inline bool noAtmosphere = false;
        inline bool godMode = false;
        inline bool antiVoid = false;
        inline bool antiStun = false;
        inline bool gravityEnabled = false;
        inline float gravity = 50.f;
        inline bool tpWalk = false;
        inline float tpWalkAmount = 2.5f;
        inline bool orbit = false;
        inline float orbitSpeed = 4.f;
        inline float orbitRadius = 8.f;
        inline bool sitSpam = false;
        inline bool flingAssist = false;
        inline bool vehicleSpeed = false;
        inline float vehicleSpeedAmt = 80.f;
        inline bool zoomHack = false;
        inline float zoomHackAmt = 0.2f;
        inline bool removeTextures = false;
        inline bool wirePlayers = false;
        inline bool crosshairPulse = false;
        inline bool killEffect = false;
        inline bool spotifyOverlay = false;
    }
}
