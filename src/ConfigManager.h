#pragma once

#include <array>
#include <cstdint>
#include <string>

struct AimbotConfig
{
    bool enabled = false;
    bool drawFovCircle = true;
    float fov = 90.0f;
    float smoothness = 6.0f;
    int targetPriority = 0;
    int aimBone = 0;
};

struct VisualsConfig
{
    bool boxEsp = true;
    bool skeletonEsp = false;
    bool healthBar = true;
    bool nameTags = false;
    bool distanceText = true;
    bool snaplines = false;
    bool visibilityCheck = true;
    std::array<float, 4> chamsColor = {0.320f, 0.520f, 0.950f, 1.0f};
    std::array<float, 4> glowColor = {0.180f, 0.800f, 0.443f, 1.0f};
};

struct SkinsConfig
{
    int weaponSkin = 0;
    int knifeSkin = 0;
    int gloveSkin = 0;
};

struct WorldConfig
{
    bool fullbright = false;
    bool noFog = true;
    bool wireframeMode = false;
    bool removeGrass = false;
    std::array<float, 4> skyColor = {0.400f, 0.620f, 1.000f, 1.0f};
};

struct MovementConfig
{
    bool bunnyHop = false;
    bool autoStrafe = true;
    bool noFallDamage = false;
    float speedMultiplier = 1.0f;
};

struct PlayerConfig
{
    bool thirdPersonToggle = false;
    bool antiAfk = true;
    float customFov = 100.0f;
};

struct MiscConfig
{
    bool autoAccept = false;
};

struct PanelConfig
{
    float animationSpeed = 1.0f;
    float panelOpacity = 1.0f;
    bool reduceMotion = false;
    bool startMinimized = false;
    bool autoHideOnFocusLoss = false;
};

struct UiConfigState
{
    AimbotConfig aimbot;
    VisualsConfig visuals;
    SkinsConfig skins;
    WorldConfig world;
    MovementConfig movement;
    PlayerConfig player;
    MiscConfig misc;
    PanelConfig panel;
};

struct ConfigSaveResult
{
    bool success = false;
    std::string path;
    std::string error;
    std::string warning;
};

extern UiConfigState g_ConfigState;

ConfigSaveResult SaveConfigToFile(const std::string& profileName);
ConfigSaveResult LoadConfigFromFile(const std::string& profileName);
ConfigSaveResult DeleteConfigFile(const std::string& profileName);
std::int64_t GetLastSessionTimestamp(const std::string& profileName);
ConfigSaveResult UpdateLastSessionTimestamp(const std::string& profileName, std::int64_t timestamp);
int CountSavedConfigFiles();
