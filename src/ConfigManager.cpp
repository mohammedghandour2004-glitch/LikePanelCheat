#include "ConfigManager.h"

#include "PanelUI.h"
#include "Theme.h"
#include "json.hpp"

#include <windows.h>

#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>

namespace
{
using json = nlohmann::json;

std::string TrimProfileName(const std::string& value)
{
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0)
    {
        ++first;
    }

    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])) != 0)
    {
        --last;
    }

    return value.substr(first, last - first);
}

bool IsValidProfileName(const std::string& profileName)
{
    if (profileName.empty() || profileName == "." || profileName == "..")
    {
        return false;
    }

    for (char ch : profileName)
    {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (value < 32)
        {
            return false;
        }

        switch (ch)
        {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*':
            return false;
        default:
            break;
        }
    }

    return true;
}

std::string GetExecutableDirectory()
{
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
        return {};
    }

    std::string executablePath(path, length);
    const size_t slash = executablePath.find_last_of("\\/");
    if (slash == std::string::npos)
    {
        return {};
    }

    return executablePath.substr(0, slash);
}

bool EnsureDirectoryExists(const std::string& path, std::string& error)
{
    if (CreateDirectoryA(path.c_str(), nullptr) != 0)
    {
        return true;
    }

    const DWORD lastError = GetLastError();
    if (lastError == ERROR_ALREADY_EXISTS)
    {
        return true;
    }

    std::ostringstream stream;
    stream << "Failed to create configs directory (Win32 error " << lastError << ")";
    error = stream.str();
    return false;
}

bool ResolveConfigPath(const std::string& profileName, bool createDirectory, std::string& fullPath, std::string& relativePath, std::string& error)
{
    const std::string safeProfileName = TrimProfileName(profileName);
    if (!IsValidProfileName(safeProfileName))
    {
        error = "Invalid profile name";
        return false;
    }

    const std::string executableDir = GetExecutableDirectory();
    if (executableDir.empty())
    {
        error = "Unable to resolve executable directory";
        return false;
    }

    const std::string configDir = executableDir + "\\configs";
    if (createDirectory && !EnsureDirectoryExists(configDir, error))
    {
        return false;
    }

    fullPath = configDir + "\\" + safeProfileName + ".json";
    relativePath = "configs/" + safeProfileName + ".json";
    return true;
}

bool ResolveConfigDirectory(bool createDirectory, std::string& configDir, std::string& error)
{
    const std::string executableDir = GetExecutableDirectory();
    if (executableDir.empty())
    {
        error = "Unable to resolve executable directory";
        return false;
    }

    configDir = executableDir + "\\configs";
    return !createDirectory || EnsureDirectoryExists(configDir, error);
}

std::int64_t ReadLastSessionTimestampFromFile(const std::string& fullPath)
{
    std::ifstream file(fullPath);
    if (!file)
    {
        return 0;
    }

    try
    {
        json document;
        file >> document;
        return document.value("lastSessionTimestamp", static_cast<std::int64_t>(0));
    }
    catch (const json::exception&)
    {
        return 0;
    }
}

json ColorJson(float r, float g, float b)
{
    return json{
        {"r", r},
        {"g", g},
        {"b", b},
    };
}

json ColorJson(const std::array<float, 4>& color)
{
    return json{
        {"r", color[0]},
        {"g", color[1]},
        {"b", color[2]},
        {"a", color[3]},
    };
}

void ReadColor(const json& source, std::array<float, 4>& target)
{
    target[0] = source.value("r", target[0]);
    target[1] = source.value("g", target[1]);
    target[2] = source.value("b", target[2]);
    target[3] = source.value("a", target[3]);
}
}

ConfigSaveResult SaveConfigToFile(const std::string& profileName)
{
    ConfigSaveResult result;
    std::string fullPath;
    std::string relativePath;
    if (!ResolveConfigPath(profileName, true, fullPath, relativePath, result.error))
    {
        return result;
    }
    const std::int64_t lastSessionTimestamp = ReadLastSessionTimestampFromFile(fullPath);

    const json document = {
        {"profile", GetProfileName()},
        {"profilePicturePath", GetProfilePicturePath()},
        {"lastSessionTimestamp", lastSessionTimestamp},
        {"theme", {
            {"accent", ColorJson(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z)}
        }},
        {"hotkeys", {
            {"togglePanel", g_ToggleHotkey}
        }},
        {"aimbot", {
            {"enabled", g_ConfigState.aimbot.enabled},
            {"drawFovCircle", g_ConfigState.aimbot.drawFovCircle},
            {"fov", g_ConfigState.aimbot.fov},
            {"smoothness", g_ConfigState.aimbot.smoothness},
            {"targetPriority", g_ConfigState.aimbot.targetPriority},
            {"aimBone", g_ConfigState.aimbot.aimBone}
        }},
        {"visuals", {
            {"boxEsp", g_ConfigState.visuals.boxEsp},
            {"skeletonEsp", g_ConfigState.visuals.skeletonEsp},
            {"healthBar", g_ConfigState.visuals.healthBar},
            {"nameTags", g_ConfigState.visuals.nameTags},
            {"distanceText", g_ConfigState.visuals.distanceText},
            {"snaplines", g_ConfigState.visuals.snaplines},
            {"visibilityCheck", g_ConfigState.visuals.visibilityCheck},
            {"chamsColor", ColorJson(g_ConfigState.visuals.chamsColor)},
            {"glowColor", ColorJson(g_ConfigState.visuals.glowColor)}
        }},
        {"skins", {
            {"weaponSkin", g_ConfigState.skins.weaponSkin},
            {"knifeSkin", g_ConfigState.skins.knifeSkin},
            {"gloveSkin", g_ConfigState.skins.gloveSkin}
        }},
        {"world", {
            {"fullbright", g_ConfigState.world.fullbright},
            {"noFog", g_ConfigState.world.noFog},
            {"wireframeMode", g_ConfigState.world.wireframeMode},
            {"removeGrass", g_ConfigState.world.removeGrass},
            {"skyColor", ColorJson(g_ConfigState.world.skyColor)}
        }},
        {"movement", {
            {"bunnyHop", g_ConfigState.movement.bunnyHop},
            {"autoStrafe", g_ConfigState.movement.autoStrafe},
            {"noFallDamage", g_ConfigState.movement.noFallDamage},
            {"speedMultiplier", g_ConfigState.movement.speedMultiplier}
        }},
        {"player", {
            {"thirdPersonToggle", g_ConfigState.player.thirdPersonToggle},
            {"antiAfk", g_ConfigState.player.antiAfk},
            {"customFov", g_ConfigState.player.customFov}
        }},
        {"misc", {
            {"autoAccept", g_ConfigState.misc.autoAccept}
        }},
        {"panel", {
            {"animationSpeed", g_ConfigState.panel.animationSpeed},
            {"panelOpacity", g_ConfigState.panel.panelOpacity},
            {"reduceMotion", g_ConfigState.panel.reduceMotion},
            {"startMinimized", g_ConfigState.panel.startMinimized},
            {"autoHideOnFocusLoss", g_ConfigState.panel.autoHideOnFocusLoss}
        }}
    };

    std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
    if (!file)
    {
        result.error = "Unable to open config file for writing";
        return result;
    }

    file << document.dump(4) << '\n';
    if (!file.good())
    {
        result.error = "Failed while writing config file";
        return result;
    }

    result.success = true;
    result.path = relativePath;
    return result;
}

ConfigSaveResult LoadConfigFromFile(const std::string& profileName)
{
    ConfigSaveResult result;
    std::string fullPath;
    std::string relativePath;
    if (!ResolveConfigPath(profileName, false, fullPath, relativePath, result.error))
    {
        return result;
    }

    std::ifstream file(fullPath);
    if (!file)
    {
        result.error = "Config file does not exist";
        return result;
    }

    json document;
    try
    {
        file >> document;
    }
    catch (const json::exception& exception)
    {
        result.error = std::string("Invalid JSON: ") + exception.what();
        return result;
    }

    try
    {
        if (document.contains("theme") && document["theme"].contains("accent"))
        {
            const json& accent = document["theme"]["accent"];
            g_AccentColor.x = accent.value("r", g_AccentColor.x);
            g_AccentColor.y = accent.value("g", g_AccentColor.y);
            g_AccentColor.z = accent.value("b", g_AccentColor.z);
            g_AccentColor.w = 1.0f;
            ApplyAccentThemeColors();
        }
        if (document.contains("hotkeys"))
        {
            g_ToggleHotkey = document["hotkeys"].value("togglePanel", g_ToggleHotkey);
        }
        SetProfileName(document.value("profile", profileName).c_str());
        const std::string profilePicturePath = document.value("profilePicturePath", std::string());
        if (!profilePicturePath.empty())
        {
            const DWORD attributes = GetFileAttributesA(profilePicturePath.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                if (!LoadProfilePictureFromPath(profilePicturePath.c_str()))
                {
                    ClearProfilePicture();
                    result.warning = "Profile picture not found";
                }
            }
            else
            {
                ClearProfilePicture();
                result.warning = "Profile picture not found";
            }
        }
        else
        {
            ClearProfilePicture();
        }
        if (document.contains("aimbot"))
        {
            const json& value = document["aimbot"];
            g_ConfigState.aimbot.enabled = value.value("enabled", g_ConfigState.aimbot.enabled);
            g_ConfigState.aimbot.drawFovCircle = value.value("drawFovCircle", g_ConfigState.aimbot.drawFovCircle);
            g_ConfigState.aimbot.fov = value.value("fov", g_ConfigState.aimbot.fov);
            g_ConfigState.aimbot.smoothness = value.value("smoothness", g_ConfigState.aimbot.smoothness);
            g_ConfigState.aimbot.targetPriority = value.value("targetPriority", g_ConfigState.aimbot.targetPriority);
            g_ConfigState.aimbot.aimBone = value.value("aimBone", g_ConfigState.aimbot.aimBone);
        }
        if (document.contains("visuals"))
        {
            const json& value = document["visuals"];
            g_ConfigState.visuals.boxEsp = value.value("boxEsp", g_ConfigState.visuals.boxEsp);
            g_ConfigState.visuals.skeletonEsp = value.value("skeletonEsp", g_ConfigState.visuals.skeletonEsp);
            g_ConfigState.visuals.healthBar = value.value("healthBar", g_ConfigState.visuals.healthBar);
            g_ConfigState.visuals.nameTags = value.value("nameTags", g_ConfigState.visuals.nameTags);
            g_ConfigState.visuals.distanceText = value.value("distanceText", g_ConfigState.visuals.distanceText);
            g_ConfigState.visuals.snaplines = value.value("snaplines", g_ConfigState.visuals.snaplines);
            g_ConfigState.visuals.visibilityCheck = value.value("visibilityCheck", g_ConfigState.visuals.visibilityCheck);
            if (value.contains("chamsColor"))
            {
                ReadColor(value["chamsColor"], g_ConfigState.visuals.chamsColor);
            }
            if (value.contains("glowColor"))
            {
                ReadColor(value["glowColor"], g_ConfigState.visuals.glowColor);
            }
        }
        if (document.contains("skins"))
        {
            const json& value = document["skins"];
            g_ConfigState.skins.weaponSkin = value.value("weaponSkin", g_ConfigState.skins.weaponSkin);
            g_ConfigState.skins.knifeSkin = value.value("knifeSkin", g_ConfigState.skins.knifeSkin);
            g_ConfigState.skins.gloveSkin = value.value("gloveSkin", g_ConfigState.skins.gloveSkin);
        }
        if (document.contains("world"))
        {
            const json& value = document["world"];
            g_ConfigState.world.fullbright = value.value("fullbright", g_ConfigState.world.fullbright);
            g_ConfigState.world.noFog = value.value("noFog", g_ConfigState.world.noFog);
            g_ConfigState.world.wireframeMode = value.value("wireframeMode", g_ConfigState.world.wireframeMode);
            g_ConfigState.world.removeGrass = value.value("removeGrass", g_ConfigState.world.removeGrass);
            if (value.contains("skyColor"))
            {
                ReadColor(value["skyColor"], g_ConfigState.world.skyColor);
            }
        }
        if (document.contains("movement"))
        {
            const json& value = document["movement"];
            g_ConfigState.movement.bunnyHop = value.value("bunnyHop", g_ConfigState.movement.bunnyHop);
            g_ConfigState.movement.autoStrafe = value.value("autoStrafe", g_ConfigState.movement.autoStrafe);
            g_ConfigState.movement.noFallDamage = value.value("noFallDamage", g_ConfigState.movement.noFallDamage);
            g_ConfigState.movement.speedMultiplier = value.value("speedMultiplier", g_ConfigState.movement.speedMultiplier);
        }
        if (document.contains("player"))
        {
            const json& value = document["player"];
            g_ConfigState.player.thirdPersonToggle = value.value("thirdPersonToggle", g_ConfigState.player.thirdPersonToggle);
            g_ConfigState.player.antiAfk = value.value("antiAfk", g_ConfigState.player.antiAfk);
            g_ConfigState.player.customFov = value.value("customFov", g_ConfigState.player.customFov);
        }
        if (document.contains("misc"))
        {
            g_ConfigState.misc.autoAccept = document["misc"].value("autoAccept", g_ConfigState.misc.autoAccept);
        }
        if (document.contains("panel"))
        {
            const json& value = document["panel"];
            g_ConfigState.panel.animationSpeed = value.value("animationSpeed", g_ConfigState.panel.animationSpeed);
            g_ConfigState.panel.panelOpacity = value.value("panelOpacity", g_ConfigState.panel.panelOpacity);
            g_ConfigState.panel.reduceMotion = value.value("reduceMotion", g_ConfigState.panel.reduceMotion);
            g_ConfigState.panel.startMinimized = value.value("startMinimized", g_ConfigState.panel.startMinimized);
            g_ConfigState.panel.autoHideOnFocusLoss = value.value("autoHideOnFocusLoss", g_ConfigState.panel.autoHideOnFocusLoss);
        }
    }
    catch (const json::exception& exception)
    {
        result.error = std::string("Config schema error: ") + exception.what();
        return result;
    }

    result.success = true;
    result.path = relativePath;
    return result;
}

ConfigSaveResult DeleteConfigFile(const std::string& profileName)
{
    ConfigSaveResult result;
    std::string fullPath;
    std::string relativePath;
    if (!ResolveConfigPath(profileName, false, fullPath, relativePath, result.error))
    {
        return result;
    }

    if (DeleteFileA(fullPath.c_str()) == 0)
    {
        const DWORD lastError = GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND || lastError == ERROR_PATH_NOT_FOUND)
        {
            result.error = "Config file does not exist";
        }
        else
        {
            std::ostringstream stream;
            stream << "Failed to delete config file (Win32 error " << lastError << ")";
            result.error = stream.str();
        }
        return result;
    }

    result.success = true;
    result.path = relativePath;
    return result;
}

std::int64_t GetLastSessionTimestamp(const std::string& profileName)
{
    ConfigSaveResult result;
    std::string fullPath;
    std::string relativePath;
    if (!ResolveConfigPath(profileName, false, fullPath, relativePath, result.error))
    {
        return 0;
    }

    return ReadLastSessionTimestampFromFile(fullPath);
}

ConfigSaveResult UpdateLastSessionTimestamp(const std::string& profileName, std::int64_t timestamp)
{
    ConfigSaveResult result;
    std::string fullPath;
    std::string relativePath;
    if (!ResolveConfigPath(profileName, true, fullPath, relativePath, result.error))
    {
        return result;
    }

    json document = json::object();
    {
        std::ifstream input(fullPath);
        if (input)
        {
            try
            {
                input >> document;
            }
            catch (const json::exception& exception)
            {
                result.error = std::string("Invalid JSON: ") + exception.what();
                return result;
            }
        }
    }

    document["profile"] = GetProfileName();
    document["lastSessionTimestamp"] = timestamp;

    std::ofstream output(fullPath, std::ios::out | std::ios::trunc);
    if (!output)
    {
        result.error = "Unable to open config file for writing";
        return result;
    }

    output << document.dump(4) << '\n';
    if (!output.good())
    {
        result.error = "Failed while writing config file";
        return result;
    }

    result.success = true;
    result.path = relativePath;
    return result;
}

int CountSavedConfigFiles()
{
    std::string configDir;
    std::string error;
    if (!ResolveConfigDirectory(false, configDir, error))
    {
        return 0;
    }

    const std::string searchPattern = configDir + "\\*.json";
    WIN32_FIND_DATAA findData = {};
    HANDLE findHandle = FindFirstFileA(searchPattern.c_str(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    int count = 0;
    do
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            ++count;
        }
    } while (FindNextFileA(findHandle, &findData) != 0);

    FindClose(findHandle);
    return count;
}
