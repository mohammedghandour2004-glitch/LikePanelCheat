#include "PanelUI.h"

#include "ConfigManager.h"
#include "ImageLoader.h"
#include "Theme.h"
#include "imgui.h"

#include <commdlg.h>
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

int g_ToggleHotkey = VK_F2;
bool g_IsCapturingToggleHotkey = false;
UiConfigState g_ConfigState;
ID3D11Device* g_PanelD3DDevice = nullptr;
ID3D11ShaderResourceView* g_ProfileAvatarTexture = nullptr;
int g_ProfileAvatarWidth = 0;
int g_ProfileAvatarHeight = 0;
std::string g_ProfilePicturePath;

void SetPanelD3DDevice(ID3D11Device* device)
{
    g_PanelD3DDevice = device;
}

const char* GetProfilePicturePath()
{
    return g_ProfilePicturePath.c_str();
}

void ClearProfilePicture()
{
    if (g_ProfileAvatarTexture != nullptr)
    {
        g_ProfileAvatarTexture->Release();
        g_ProfileAvatarTexture = nullptr;
    }
    g_ProfileAvatarWidth = 0;
    g_ProfileAvatarHeight = 0;
    g_ProfilePicturePath.clear();
}

bool LoadProfilePictureFromPath(const char* path)
{
    if (path == nullptr || path[0] == '\0' || g_PanelD3DDevice == nullptr)
    {
        ClearProfilePicture();
        return false;
    }

    int loadedWidth = 0;
    int loadedHeight = 0;
    ID3D11ShaderResourceView* texture = LoadTextureFromFile(path, g_PanelD3DDevice, &loadedWidth, &loadedHeight);
    if (texture == nullptr)
    {
        ClearProfilePicture();
        return false;
    }

    if (g_ProfileAvatarTexture != nullptr)
    {
        g_ProfileAvatarTexture->Release();
    }
    g_ProfileAvatarTexture = texture;
    g_ProfileAvatarWidth = loadedWidth;
    g_ProfileAvatarHeight = loadedHeight;
    g_ProfilePicturePath = path;
    return true;
}

const char* GetToggleHotkeyName()
{
    switch (g_ToggleHotkey)
    {
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    case VK_INSERT: return "INSERT";
    case VK_HOME: return "HOME";
    case VK_END: return "END";
    case VK_DELETE: return "DELETE";
    default:
        break;
    }

    static char keyName[2] = {};
    if ((g_ToggleHotkey >= 'A' && g_ToggleHotkey <= 'Z') || (g_ToggleHotkey >= '0' && g_ToggleHotkey <= '9'))
    {
        keyName[0] = static_cast<char>(g_ToggleHotkey);
        keyName[1] = '\0';
        return keyName;
    }
    return "F2";
}

namespace
{
constexpr float kSidebarWidth = 180.0f;
constexpr ImVec4 kSidebarBg = ImVec4(0.060f, 0.060f, 0.070f, 1.0f);
constexpr ImVec4 kSectionBg = ImVec4(0.075f, 0.075f, 0.086f, 1.0f);
constexpr ImVec4 kSectionBorder = ImVec4(0.170f, 0.150f, 0.220f, 1.0f);
constexpr ImVec4 kMutedText = ImVec4(0.580f, 0.580f, 0.640f, 1.0f);
constexpr float kRowWidgetWidth = 200.0f;
constexpr float kOuterWindowRounding = 16.0f;
constexpr float kSectionRounding = 12.0f;
constexpr float kButtonRounding = 9.0f;

enum class Category
{
    Aimbot,
    Visuals,
    Skins,
    World,
    Movement,
    Player,
    Misc,
    Config
};

enum class ToastType
{
    Success,
    Info,
    Warning
};

struct CategoryItem
{
    Category id;
    const char* label;
};

struct Toast
{
    std::string message;
    ToastType type;
    double startTime;
    float duration;
    float yOffset;
};

constexpr CategoryItem kCategories[] = {
    { Category::Aimbot, "Aimbot" },
    { Category::Visuals, "Visuals" },
    { Category::Skins, "Skins" },
    { Category::World, "World" },
    { Category::Movement, "Movement" },
    { Category::Player, "Player" },
    { Category::Misc, "Misc" },
    { Category::Config, "Config" },
};

std::vector<Toast> g_Toasts;
char g_ProfileName[32] = "Default";

struct KeyOption
{
    int vk;
    const char* name;
};

constexpr KeyOption kCaptureKeys[] = {
    { VK_F1, "F1" },
    { VK_F2, "F2" },
    { VK_F3, "F3" },
    { VK_F4, "F4" },
    { VK_F5, "F5" },
    { VK_F6, "F6" },
    { VK_F7, "F7" },
    { VK_F8, "F8" },
    { VK_F9, "F9" },
    { VK_F10, "F10" },
    { VK_F11, "F11" },
    { VK_F12, "F12" },
    { VK_INSERT, "INSERT" },
    { VK_HOME, "HOME" },
    { VK_END, "END" },
    { VK_DELETE, "DELETE" },
    { 'A', "A" },
    { 'B', "B" },
    { 'C', "C" },
    { 'D', "D" },
    { 'E', "E" },
    { 'F', "F" },
    { 'G', "G" },
    { 'H', "H" },
    { 'I', "I" },
    { 'J', "J" },
    { 'K', "K" },
    { 'L', "L" },
    { 'M', "M" },
    { 'N', "N" },
    { 'O', "O" },
    { 'P', "P" },
    { 'Q', "Q" },
    { 'R', "R" },
    { 'S', "S" },
    { 'T', "T" },
    { 'U', "U" },
    { 'V', "V" },
    { 'W', "W" },
    { 'X', "X" },
    { 'Y', "Y" },
    { 'Z', "Z" },
    { '0', "0" },
    { '1', "1" },
    { '2', "2" },
    { '3', "3" },
    { '4', "4" },
    { '5', "5" },
    { '6', "6" },
    { '7', "7" },
    { '8', "8" },
    { '9', "9" },
};

ImVec4 WithAlpha(ImVec4 color, float alpha)
{
    color.w = alpha;
    return color;
}

ImVec4 ScaleColor(ImVec4 color, float scale, float alpha = 1.0f)
{
    const float r = color.x * scale > 1.0f ? 1.0f : color.x * scale;
    const float g = color.y * scale > 1.0f ? 1.0f : color.y * scale;
    const float b = color.z * scale > 1.0f ? 1.0f : color.z * scale;
    return ImVec4(r, g, b, alpha);
}

ImVec4 LerpColor(ImVec4 a, ImVec4 b, float t)
{
    const float clamped = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return ImVec4(
        a.x + (b.x - a.x) * clamped,
        a.y + (b.y - a.y) * clamped,
        a.z + (b.z - a.z) * clamped,
        a.w + (b.w - a.w) * clamped);
}

ImU32 AccentU32(float alpha = 1.0f)
{
    return ImGui::GetColorU32(WithAlpha(g_AccentColor, alpha));
}

float Clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

float EaseOutCubic(float value)
{
    const float t = 1.0f - Clamp01(value);
    return 1.0f - t * t * t;
}

float Approach(float current, float target, float responseSeconds)
{
    const float dt = ImGui::GetIO().DeltaTime;
    const float step = responseSeconds > 0.0f ? Clamp01(dt / responseSeconds) : 1.0f;
    return current + (target - current) * step;
}

void ShowToast(const char* message, ToastType type, float duration = 2.5f)
{
    g_Toasts.push_back({ message, type, ImGui::GetTime(), duration, 0.0f });
}

ImVec4 ToastAccentColor(ToastType type)
{
    switch (type)
    {
    case ToastType::Success:
        return ImVec4(0.160f, 0.780f, 0.420f, 1.0f);
    case ToastType::Warning:
        return ImVec4(0.980f, 0.500f, 0.180f, 1.0f);
    case ToastType::Info:
    default:
        return g_AccentColor;
    }
}

void DrawToastIcon(ImDrawList* drawList, ToastType type, ImVec2 center, float alpha)
{
    const ImU32 iconColor = ImGui::GetColorU32(WithAlpha(ToastAccentColor(type), alpha));
    const ImU32 textColor = ImGui::GetColorU32(ImVec4(0.940f, 0.940f, 0.970f, alpha));
    switch (type)
    {
    case ToastType::Success:
        drawList->AddCircle(center, 8.0f, iconColor, 24, 1.5f);
        drawList->AddLine(ImVec2(center.x - 4.0f, center.y), ImVec2(center.x - 1.0f, center.y + 3.5f), textColor, 1.8f);
        drawList->AddLine(ImVec2(center.x - 1.0f, center.y + 3.5f), ImVec2(center.x + 5.0f, center.y - 4.0f), textColor, 1.8f);
        break;
    case ToastType::Warning:
        drawList->AddTriangle(
            ImVec2(center.x, center.y - 8.0f),
            ImVec2(center.x - 8.0f, center.y + 7.0f),
            ImVec2(center.x + 8.0f, center.y + 7.0f),
            iconColor,
            1.6f);
        drawList->AddLine(ImVec2(center.x, center.y - 2.5f), ImVec2(center.x, center.y + 2.5f), textColor, 1.8f);
        drawList->AddCircleFilled(ImVec2(center.x, center.y + 5.2f), 1.1f, textColor, 8);
        break;
    case ToastType::Info:
    default:
        drawList->AddCircle(center, 8.0f, iconColor, 24, 1.5f);
        drawList->AddLine(ImVec2(center.x, center.y - 1.0f), ImVec2(center.x, center.y + 5.0f), textColor, 1.8f);
        drawList->AddCircleFilled(ImVec2(center.x, center.y - 4.8f), 1.2f, textColor, 8);
        break;
    }
}

void RenderToastStack(ImVec2 panelMin, ImVec2 panelMax)
{
    const double now = ImGui::GetTime();
    const float toastWidth = 282.0f;
    const float minToastHeight = 44.0f;
    const float margin = 16.0f;
    const float gap = 8.0f;
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    for (int i = static_cast<int>(g_Toasts.size()) - 1; i >= 0; --i)
    {
        const float age = static_cast<float>(now - g_Toasts[static_cast<size_t>(i)].startTime);
        if (age > g_Toasts[static_cast<size_t>(i)].duration + 0.20f)
        {
            g_Toasts.erase(g_Toasts.begin() + i);
        }
    }

    float targetY = margin;
    for (int i = static_cast<int>(g_Toasts.size()) - 1; i >= 0; --i)
    {
        Toast& toast = g_Toasts[static_cast<size_t>(i)];
        const float age = static_cast<float>(now - toast.startTime);
        const float inT = EaseOutCubic(age / 0.20f);
        const float outT = age > toast.duration ? Clamp01((age - toast.duration) / 0.20f) : 0.0f;
        const float alpha = Clamp01(inT * (1.0f - outT));
        const float slide = (1.0f - inT + outT) * 34.0f;
        const ImVec2 textSize = ImGui::CalcTextSize(toast.message.c_str());
        const float toastHeight = textSize.y + 24.0f > minToastHeight ? textSize.y + 24.0f : minToastHeight;
        toast.yOffset = Approach(toast.yOffset, targetY, 0.12f);

        const ImVec2 min(panelMax.x - margin - toastWidth + slide, panelMin.y + toast.yOffset);
        const ImVec2 max(panelMax.x - margin + slide, min.y + toastHeight);
        const ImVec4 accent = ToastAccentColor(toast.type);
        const ImU32 cardColor = ImGui::GetColorU32(ImVec4(0.060f, 0.060f, 0.072f, 0.94f * alpha));
        const ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.170f, 0.150f, 0.220f, 0.85f * alpha));
        const ImU32 accentColor = ImGui::GetColorU32(WithAlpha(accent, alpha));
        const ImU32 textColor = ImGui::GetColorU32(ImVec4(0.925f, 0.925f, 0.950f, alpha));

        for (int shadow = 0; shadow < 3; ++shadow)
        {
            const float spread = 2.0f + static_cast<float>(shadow) * 2.0f;
            drawList->AddRectFilled(
                ImVec2(min.x + 3.0f - spread, min.y + 5.0f - spread),
                ImVec2(max.x + 3.0f + spread, max.y + 5.0f + spread),
                ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, (0.16f - static_cast<float>(shadow) * 0.045f) * alpha)),
                7.0f + spread);
        }

        drawList->AddRectFilled(min, max, cardColor, 7.0f);
        drawList->AddRect(min, max, borderColor, 7.0f, 0, 1.0f);
        drawList->AddRectFilled(min, ImVec2(min.x + 4.0f, max.y), accentColor, 7.0f, ImDrawFlags_RoundCornersLeft);
        DrawToastIcon(drawList, toast.type, ImVec2(min.x + 23.0f, min.y + toastHeight * 0.5f), alpha);
        drawList->AddText(ImVec2(min.x + 42.0f, min.y + (toastHeight - textSize.y) * 0.5f), textColor, toast.message.c_str());

        targetY += toastHeight + gap;
    }
}

void DrawSoftShadow(ImDrawList* drawList, ImVec2 min, ImVec2 max, float rounding)
{
    for (int i = 0; i < 5; ++i)
    {
        const float spread = 2.0f + static_cast<float>(i) * 2.0f;
        const float alpha = 0.120f - static_cast<float>(i) * 0.018f;
        drawList->AddRectFilled(
            ImVec2(min.x + 4.0f - spread, min.y + 5.0f - spread),
            ImVec2(max.x + 4.0f + spread, max.y + 5.0f + spread),
            ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, alpha)),
            rounding + spread);
    }
}

void DrawLayeredPanelShadow(ImDrawList* drawList, ImVec2 min, ImVec2 max, float rounding)
{
    constexpr float alphas[] = { 0.16f, 0.12f, 0.08f, 0.045f, 0.025f };
    for (int i = 0; i < 5; ++i)
    {
        const float spread = 6.0f + static_cast<float>(i) * 5.0f;
        const float offset = 5.0f + static_cast<float>(i) * 1.8f;
        drawList->AddRectFilled(
            ImVec2(min.x + offset - spread, min.y + offset - spread),
            ImVec2(max.x + offset + spread, max.y + offset + spread),
            ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, alphas[i])),
            rounding + spread);
    }
    drawList->AddRectFilled(
        ImVec2(min.x - 5.0f, min.y - 5.0f),
        ImVec2(max.x + 5.0f, max.y + 5.0f),
        AccentU32(0.030f),
        rounding + 5.0f);
}

void DrawAcrylicGrain(ImDrawList* drawList, ImVec2 min, ImVec2 max, float rounding)
{
    struct GrainDot
    {
        float x;
        float y;
        float alpha;
        float radius;
    };

    static std::vector<GrainDot> grain;
    constexpr int kGrainCount = 440;
    if (grain.empty())
    {
        grain.reserve(kGrainCount);
        unsigned int seed = 0x8E4A9B1Du;
        for (int i = 0; i < kGrainCount; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            const float x = static_cast<float>((seed >> 8) & 0xFFFFu) / 65535.0f;
            seed = seed * 1664525u + 1013904223u;
            const float y = static_cast<float>((seed >> 8) & 0xFFFFu) / 65535.0f;
            seed = seed * 1664525u + 1013904223u;
            const float alpha = 0.012f + static_cast<float>((seed >> 12) & 0xFFu) / 255.0f * 0.030f;
            seed = seed * 1664525u + 1013904223u;
            const float radius = 0.55f + static_cast<float>((seed >> 13) & 0x7Fu) / 127.0f * 0.45f;
            grain.push_back({ x, y, alpha, radius });
        }
    }

    drawList->PushClipRect(min, max, true);
    for (const GrainDot& dot : grain)
    {
        const ImVec2 pos(min.x + (max.x - min.x) * dot.x, min.y + (max.y - min.y) * dot.y);
        drawList->AddCircleFilled(pos, dot.radius, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, dot.alpha)), 6);
    }
    drawList->PopClipRect();
}

void DrawAcrylicPanelSurface(ImDrawList* drawList, ImVec2 min, ImVec2 max, float rounding)
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    if (width <= 8.0f || height <= 8.0f)
    {
        return;
    }

    drawList->AddRectFilled(
        min,
        max,
        ImGui::GetColorU32(ImVec4(0.043f, 0.043f, 0.054f, 1.0f)),
        rounding);
    drawList->AddRectFilledMultiColor(
        ImVec2(min.x + rounding, min.y + 1.0f),
        ImVec2(max.x - rounding, max.y - 1.0f),
        ImGui::GetColorU32(ImVec4(0.050f, 0.050f, 0.062f, 0.82f)),
        ImGui::GetColorU32(ImVec4(0.044f, 0.043f, 0.055f, 0.82f)),
        ImGui::GetColorU32(ImVec4(0.031f, 0.031f, 0.040f, 0.72f)),
        ImGui::GetColorU32(ImVec4(0.037f, 0.037f, 0.048f, 0.72f)));

    const float highlightHeight = height * 0.22f;
    drawList->AddRectFilledMultiColor(
        ImVec2(min.x + rounding, min.y + 1.0f),
        ImVec2(max.x - rounding, min.y + highlightHeight),
        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.115f)),
        ImGui::GetColorU32(WithAlpha(GetAccentHoverColor(), 0.055f)),
        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.000f)),
        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.000f)));
    drawList->AddCircleFilled(
        ImVec2(min.x + width * 0.28f, min.y + height * 0.18f),
        width * 0.38f,
        ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.028f)),
        96);

    DrawAcrylicGrain(drawList, min, max, rounding);

    drawList->AddRect(
        ImVec2(min.x + 0.5f, min.y + 0.5f),
        ImVec2(max.x - 0.5f, max.y - 0.5f),
        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.165f)),
        rounding,
        0,
        1.0f);
    drawList->AddRect(
        ImVec2(min.x + 1.8f, min.y + 1.8f),
        ImVec2(max.x - 1.8f, max.y - 1.8f),
        AccentU32(0.105f),
        rounding - 1.0f,
        0,
        1.0f);
    drawList->AddRect(
        ImVec2(min.x + 3.0f, min.y + 3.0f),
        ImVec2(max.x - 3.0f, max.y - 3.0f),
        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.040f)),
        rounding - 2.0f,
        0,
        1.0f);
}

ImVec2 DrawLicensedBadge(ImDrawList* drawList, ImVec2 badgeMax)
{
    const char* label = "LICENSED";
    if (g_FontSmall != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
    }
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    if (g_FontSmall != nullptr)
    {
        ImGui::PopFont();
    }

    const ImVec2 badgeSize(textSize.x + 42.0f, 22.0f);
    const ImVec2 badgeMin(badgeMax.x - badgeSize.x, badgeMax.y - badgeSize.y);
    drawList->AddRectFilled(badgeMin, badgeMax, ImGui::GetColorU32(ImVec4(0.070f, 0.066f, 0.086f, 0.92f)), badgeSize.y * 0.5f);
    drawList->AddRect(badgeMin, badgeMax, AccentU32(0.42f), badgeSize.y * 0.5f, 0, 1.0f);
    drawList->AddRect(ImVec2(badgeMin.x + 1.0f, badgeMin.y + 1.0f), ImVec2(badgeMax.x - 1.0f, badgeMax.y - 1.0f), AccentU32(0.14f), badgeSize.y * 0.5f - 1.0f, 0, 1.0f);
    drawList->AddRect(ImVec2(badgeMin.x + 2.0f, badgeMin.y + 2.0f), ImVec2(badgeMax.x - 2.0f, badgeMax.y - 2.0f), AccentU32(0.06f), badgeSize.y * 0.5f - 2.0f, 0, 1.0f);

    const ImVec2 shieldCenter(badgeMin.x + 16.0f, badgeMin.y + badgeSize.y * 0.5f);
    const ImU32 iconColor = AccentU32(0.88f);
    drawList->AddLine(ImVec2(shieldCenter.x, shieldCenter.y - 6.0f), ImVec2(shieldCenter.x - 5.0f, shieldCenter.y - 3.8f), iconColor, 1.2f);
    drawList->AddLine(ImVec2(shieldCenter.x - 5.0f, shieldCenter.y - 3.8f), ImVec2(shieldCenter.x - 3.8f, shieldCenter.y + 3.0f), iconColor, 1.2f);
    drawList->AddLine(ImVec2(shieldCenter.x - 3.8f, shieldCenter.y + 3.0f), ImVec2(shieldCenter.x, shieldCenter.y + 6.0f), iconColor, 1.2f);
    drawList->AddLine(ImVec2(shieldCenter.x, shieldCenter.y + 6.0f), ImVec2(shieldCenter.x + 3.8f, shieldCenter.y + 3.0f), iconColor, 1.2f);
    drawList->AddLine(ImVec2(shieldCenter.x + 3.8f, shieldCenter.y + 3.0f), ImVec2(shieldCenter.x + 5.0f, shieldCenter.y - 3.8f), iconColor, 1.2f);
    drawList->AddLine(ImVec2(shieldCenter.x + 5.0f, shieldCenter.y - 3.8f), ImVec2(shieldCenter.x, shieldCenter.y - 6.0f), iconColor, 1.2f);
    drawList->AddLine(ImVec2(shieldCenter.x - 2.4f, shieldCenter.y + 0.3f), ImVec2(shieldCenter.x - 0.6f, shieldCenter.y + 2.2f), iconColor, 1.35f);
    drawList->AddLine(ImVec2(shieldCenter.x - 0.6f, shieldCenter.y + 2.2f), ImVec2(shieldCenter.x + 3.0f, shieldCenter.y - 2.5f), iconColor, 1.35f);

    if (g_FontSmall != nullptr)
    {
        drawList->AddText(g_FontSmall, g_FontSmall->LegacySize, ImVec2(badgeMin.x + 32.0f, badgeMin.y + (badgeSize.y - textSize.y) * 0.5f), ImGui::GetColorU32(ImVec4(0.865f, 0.845f, 0.930f, 1.0f)), label);
    }
    else
    {
        drawList->AddText(ImVec2(badgeMin.x + 32.0f, badgeMin.y + (badgeSize.y - textSize.y) * 0.5f), ImGui::GetColorU32(ImVec4(0.865f, 0.845f, 0.930f, 1.0f)), label);
    }

    return badgeMin;
}

void DrawPremiumTopStrip(ImDrawList* drawList, ImVec2 min, ImVec2 max)
{
    const float width = max.x - min.x;
    if (width <= 16.0f)
    {
        return;
    }

    const float time = static_cast<float>(ImGui::GetTime());
    const float stripHeight = 5.0f;
    const float edgeInset = kOuterWindowRounding - 1.0f;
    const float stripMinX = min.x + edgeInset;
    const float stripMaxX = max.x - edgeInset;
    const float stripWidth = stripMaxX - stripMinX;
    const ImVec4 accent = g_AccentColor;
    const ImVec4 lightAccent = GetAccentHoverColor();
    const ImVec4 deepAccent = ScaleColor(g_AccentColor, 0.58f, 1.0f);
    constexpr int kSegments = 36;

    for (int i = 0; i < kSegments; ++i)
    {
        const float x0 = stripMinX + stripWidth * (static_cast<float>(i) / static_cast<float>(kSegments));
        const float x1 = stripMinX + stripWidth * (static_cast<float>(i + 1) / static_cast<float>(kSegments));
        const float phase0 = fmodf(static_cast<float>(i) / static_cast<float>(kSegments) + time * 0.075f, 1.0f);
        const float phase1 = fmodf(static_cast<float>(i + 1) / static_cast<float>(kSegments) + time * 0.075f, 1.0f);
        const auto sample = [&](float phase) {
            if (phase < 0.34f)
            {
                return LerpColor(deepAccent, accent, phase / 0.34f);
            }
            if (phase < 0.67f)
            {
                return LerpColor(accent, lightAccent, (phase - 0.34f) / 0.33f);
            }
            return LerpColor(lightAccent, deepAccent, (phase - 0.67f) / 0.33f);
        };
        const ImU32 left = ImGui::GetColorU32(WithAlpha(sample(phase0), 0.96f));
        const ImU32 right = ImGui::GetColorU32(WithAlpha(sample(phase1), 0.96f));
        drawList->AddRectFilledMultiColor(
            ImVec2(x0, min.y),
            ImVec2(x1 + 1.0f, min.y + stripHeight),
            left,
            right,
            right,
            left);
    }

    for (int i = 0; i < 5; ++i)
    {
        const float y0 = min.y + stripHeight + static_cast<float>(i) * 3.0f;
        const float y1 = y0 + 4.0f;
        const float alphaTop = 0.18f - static_cast<float>(i) * 0.030f;
        const float alphaBottom = alphaTop * 0.22f;
        drawList->AddRectFilledMultiColor(
            ImVec2(stripMinX, y0),
            ImVec2(stripMaxX, y1),
            AccentU32(alphaTop),
            ImGui::GetColorU32(WithAlpha(lightAccent, alphaTop * 0.72f)),
            ImGui::GetColorU32(WithAlpha(lightAccent, alphaBottom)),
            AccentU32(alphaBottom));
    }
}

void DrawSectionCardShadow(ImDrawList* drawList, ImVec2 min, ImVec2 max, float rounding)
{
    constexpr float alphas[] = { 0.070f, 0.045f, 0.024f };
    for (int i = 0; i < 3; ++i)
    {
        const float spread = 2.0f + static_cast<float>(i) * 2.0f;
        const float offset = 2.0f + static_cast<float>(i) * 1.2f;
        drawList->AddRectFilled(
            ImVec2(min.x + offset - spread, min.y + offset - spread),
            ImVec2(max.x + offset + spread, max.y + offset + spread),
            ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, alphas[i])),
            rounding + spread);
    }
}

void DrawInnerGlow(ImDrawList* drawList, ImVec2 min, ImVec2 max, float rounding, float alpha)
{
    drawList->AddRect(
        ImVec2(min.x + 1.0f, min.y + 1.0f),
        ImVec2(max.x - 1.0f, max.y - 1.0f),
        AccentU32(alpha),
        rounding,
        0,
        1.0f);
    drawList->AddRect(
        ImVec2(min.x + 2.0f, min.y + 2.0f),
        ImVec2(max.x - 2.0f, max.y - 2.0f),
        AccentU32(alpha * 0.45f),
        rounding - 1.0f,
        0,
        1.0f);
}

void DrawSidebarSeparation(ImDrawList* drawList, ImVec2 top, float height)
{
    drawList->AddRectFilledMultiColor(
        ImVec2(top.x - 8.0f, top.y),
        ImVec2(top.x + 10.0f, top.y + height),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.34f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.34f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)));
    drawList->AddRectFilledMultiColor(
        ImVec2(top.x, top.y),
        ImVec2(top.x + 2.0f, top.y + height),
        AccentU32(0.16f),
        AccentU32(0.02f),
        AccentU32(0.02f),
        AccentU32(0.16f));
}

void DrawContentBackgroundDepth(ImDrawList* drawList, ImVec2 min, ImVec2 max)
{
    drawList->AddRectFilledMultiColor(
        min,
        max,
        ImGui::GetColorU32(ImVec4(0.080f, 0.078f, 0.095f, 0.62f)),
        ImGui::GetColorU32(ImVec4(0.066f, 0.062f, 0.080f, 0.26f)),
        ImGui::GetColorU32(ImVec4(0.042f, 0.042f, 0.050f, 0.18f)),
        ImGui::GetColorU32(ImVec4(0.050f, 0.048f, 0.064f, 0.34f)));
    drawList->AddCircleFilled(
        ImVec2(min.x + (max.x - min.x) * 0.38f, min.y + 18.0f),
        (max.x - min.x) * 0.42f,
        AccentU32(0.018f),
        96);
}

void DrawAnimatedGradientMesh(ImDrawList* drawList, ImVec2 min, ImVec2 max, float alphaScale)
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    const float time = static_cast<float>(ImGui::GetTime());
    const ImVec2 center(min.x + width * 0.5f, min.y + height * 0.5f);
    const ImVec4 accentHover = GetAccentHoverColor();
    const ImVec4 secondary = ImVec4(
        accentHover.x * 0.68f,
        accentHover.y * 0.82f,
        accentHover.z,
        1.0f);
    const ImVec4 dimAccent = ScaleColor(g_AccentColor, 0.78f);

    struct Blob
    {
        float radius;
        float orbitX;
        float orbitY;
        float speed;
        float phase;
        ImVec4 color;
        float alpha;
    };

    const Blob blobs[] = {
        { width * 0.42f, width * 0.22f, height * 0.18f, 0.21f, 0.20f, g_AccentColor, 0.080f },
        { width * 0.34f, width * 0.26f, height * 0.22f, -0.17f, 2.25f, secondary, 0.062f },
        { width * 0.30f, width * 0.18f, height * 0.28f, 0.13f, 4.35f, dimAccent, 0.050f },
    };

    drawList->PushClipRect(min, max, true);
    for (const Blob& blob : blobs)
    {
        const float angle = time * blob.speed + blob.phase;
        const ImVec2 pos(
            center.x + cosf(angle) * blob.orbitX + sinf(angle * 0.63f) * width * 0.05f,
            center.y + sinf(angle) * blob.orbitY + cosf(angle * 0.71f) * height * 0.05f);
        drawList->AddCircleFilled(pos, blob.radius * 1.35f, ImGui::GetColorU32(WithAlpha(blob.color, blob.alpha * 0.28f * alphaScale)), 96);
        drawList->AddCircleFilled(pos, blob.radius, ImGui::GetColorU32(WithAlpha(blob.color, blob.alpha * alphaScale)), 96);
    }
    drawList->AddRectFilledMultiColor(
        min,
        max,
        AccentU32(0.012f * alphaScale),
        ImGui::GetColorU32(WithAlpha(secondary, 0.020f * alphaScale)),
        AccentU32(0.006f * alphaScale),
        ImGui::GetColorU32(WithAlpha(dimAccent, 0.014f * alphaScale)));
    drawList->PopClipRect();
}

void DrawCircuitTrace(ImDrawList* drawList, const ImVec2* points, int count, ImU32 lineColor, ImU32 dotColor, ImU32 glowColor, float phase)
{
    for (int i = 1; i < count; ++i)
    {
        drawList->AddLine(points[i - 1], points[i], lineColor, 1.0f);
    }

    const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 2.6f + phase);
    drawList->AddCircleFilled(points[0], 3.4f + pulse * 2.0f, glowColor, 18);
    drawList->AddCircleFilled(points[0], 1.45f + pulse * 0.35f, dotColor, 12);
    drawList->AddCircleFilled(points[count - 1], 3.1f + pulse * 1.8f, glowColor, 18);
    drawList->AddCircleFilled(points[count - 1], 1.35f + pulse * 0.35f, dotColor, 12);
}

void DrawAnimatedTechPattern(ImDrawList* drawList, ImVec2 min, ImVec2 max, float alphaScale)
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    if (width <= 12.0f || height <= 12.0f)
    {
        return;
    }

    const float time = static_cast<float>(ImGui::GetTime());
    const float radius = 22.0f;
    const float stepX = 74.0f;
    const float stepY = 64.0f;
    const float drift = fmodf(time * 1.25f, stepX);
    const ImU32 gridColor = ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.095f * alphaScale));
    const ImU32 traceColor = ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.180f * alphaScale));
    const ImU32 dotColor = ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.250f * alphaScale));
    const ImU32 glowColor = ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.080f * alphaScale));

    drawList->PushClipRect(min, max, true);

    const float startX = min.x - stepX + drift;
    const float startY = min.y - stepY + fmodf(time * 0.72f, stepY);
    for (float y = startY; y < max.y + stepY; y += stepY)
    {
        const int row = static_cast<int>((y - startY) / stepY);
        const float rowOffset = (row % 2 == 0) ? 0.0f : stepX * 0.5f;
        for (float x = startX + rowOffset; x < max.x + stepX; x += stepX)
        {
            ImVec2 hex[6];
            for (int i = 0; i < 6; ++i)
            {
                const float angle = 0.5235988f + static_cast<float>(i) * 1.0471976f;
                hex[i] = ImVec2(x + cosf(angle) * radius, y + sinf(angle) * radius);
            }

            for (int i = 0; i < 6; ++i)
            {
                drawList->AddLine(hex[i], hex[(i + 1) % 6], gridColor, 1.0f);
            }
        }
    }

    const float inset = 18.0f;
    const float smallWidth = width < 260.0f ? width * 0.58f : 132.0f;
    const float mediumWidth = width < 260.0f ? width * 0.44f : 165.0f;
    const ImVec2 upperTrace[] = {
        ImVec2(min.x + inset, min.y + 24.0f),
        ImVec2(min.x + inset + smallWidth * 0.38f, min.y + 24.0f),
        ImVec2(min.x + inset + smallWidth * 0.38f, min.y + 46.0f),
        ImVec2(min.x + inset + smallWidth, min.y + 46.0f),
    };
    DrawCircuitTrace(drawList, upperTrace, 4, traceColor, dotColor, glowColor, 0.2f);

    const ImVec2 lowerTrace[] = {
        ImVec2(max.x - inset - mediumWidth, max.y - 38.0f),
        ImVec2(max.x - inset - mediumWidth * 0.52f, max.y - 38.0f),
        ImVec2(max.x - inset - mediumWidth * 0.52f, max.y - 64.0f),
        ImVec2(max.x - inset, max.y - 64.0f),
    };
    DrawCircuitTrace(drawList, lowerTrace, 4, traceColor, dotColor, glowColor, 1.9f);

    if (width > 320.0f)
    {
        const ImVec2 sideTrace[] = {
            ImVec2(max.x - 34.0f, min.y + height * 0.34f),
            ImVec2(max.x - 86.0f, min.y + height * 0.34f),
            ImVec2(max.x - 86.0f, min.y + height * 0.34f + 34.0f),
            ImVec2(max.x - 142.0f, min.y + height * 0.34f + 34.0f),
        };
        DrawCircuitTrace(drawList, sideTrace, 4, traceColor, dotColor, glowColor, 3.0f);
    }

    drawList->PopClipRect();
}

void DrawSectionLift(ImDrawList* drawList, ImVec2 min, ImVec2 max, float rounding)
{
    drawList->AddRectFilledMultiColor(
        ImVec2(min.x + 8.0f, max.y - 2.0f),
        ImVec2(max.x - 6.0f, max.y + 10.0f),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.11f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.11f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)));
    drawList->AddRectFilledMultiColor(
        ImVec2(max.x - 2.0f, min.y + 8.0f),
        ImVec2(max.x + 9.0f, max.y - 6.0f),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.09f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.09f)));
    drawList->AddRect(
        ImVec2(min.x + 1.0f, min.y + 1.0f),
        ImVec2(max.x - 1.0f, max.y - 1.0f),
        AccentU32(0.055f),
        rounding,
        0,
        1.0f);
}

void GradientSeparator(float height = 1.0f)
{
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilledMultiColor(
        pos,
        ImVec2(pos.x + width, pos.y + height),
        AccentU32(0.55f),
        AccentU32(0.0f),
        AccentU32(0.0f),
        AccentU32(0.55f));
    ImGui::Dummy(ImVec2(width, height + 3.0f));
}

bool GradientButton(const char* label, ImVec2 size)
{
    ImGui::PushID(label);
    const ImGuiID id = ImGui::GetID("##gradient-button-hover");
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float* hoverT = storage->GetFloatRef(id, 0.0f);
    float* pressT = storage->GetFloatRef(ImGui::GetID("##gradient-button-press"), 0.0f);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##gradient-button", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const float target = hovered || active ? 1.0f : 0.0f;
    *hoverT = Approach(*hoverT, target, 0.10f);
    if (pressed)
    {
        *pressT = 1.0f;
    }
    *pressT = active ? 1.0f : Approach(*pressT, 0.0f, 0.10f);

    const float brighten = 1.0f + (*hoverT * 0.15f) + (active ? 0.08f : 0.0f);
    const ImVec4 top = ScaleColor(g_AccentColor, 0.46f * brighten);
    const ImVec4 bottom = ScaleColor(g_AccentColor, 0.26f * brighten);
    const float drawScale = 1.0f - (*pressT * 0.03f);
    const ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    const ImVec2 drawSize(size.x * drawScale, size.y * drawScale);
    const ImVec2 drawPos(center.x - drawSize.x * 0.5f, center.y - drawSize.y * 0.5f);
    const ImVec2 max(drawPos.x + drawSize.x, drawPos.y + drawSize.y);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(drawPos, max, ImGui::GetColorU32(bottom), kButtonRounding);
    drawList->AddRectFilledMultiColor(
        ImVec2(drawPos.x + kButtonRounding * 0.55f, drawPos.y),
        ImVec2(max.x - kButtonRounding * 0.55f, max.y),
        ImGui::GetColorU32(top),
        ImGui::GetColorU32(top),
        ImGui::GetColorU32(bottom),
        ImGui::GetColorU32(bottom));
    drawList->AddRect(drawPos, max, ImGui::GetColorU32(GetAccentHoverColor()), kButtonRounding, 0, 1.0f);
    if (hovered || active)
    {
        drawList->AddRect(ImVec2(drawPos.x - 1.0f, drawPos.y - 1.0f), ImVec2(max.x + 1.0f, max.y + 1.0f), AccentU32(0.24f), kButtonRounding + 1.0f, 0, 2.0f);
    }

    const ImVec2 textSize = ImGui::CalcTextSize(label);
    drawList->AddText(
        ImVec2(drawPos.x + (drawSize.x - textSize.x) * 0.5f, drawPos.y + (drawSize.y - textSize.y) * 0.5f),
        ImGui::GetColorU32(ImVec4(0.940f, 0.930f, 0.970f, 1.0f)),
        label);

    ImGui::PopID();
    return pressed;
}

void Tooltip(const char* text)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::SetTooltip("%s", text);
    }
}

void ToggleRow(const char* label, bool* value, const char* tooltip)
{
    ToggleSwitch(label, value);
    Tooltip(tooltip);
}

float BeginRightAlignedWidgetRow(const char* label, float preferredWidth)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float rowStartX = ImGui::GetCursorPosX();
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float labelWidth = ImGui::CalcTextSize(label).x;
    const float minWidgetWidth = 120.0f;
    const float maxWidgetWidth = availableWidth - labelWidth - style.ItemSpacing.x;
    float widgetWidth = preferredWidth;
    if (widgetWidth > maxWidgetWidth)
    {
        widgetWidth = maxWidgetWidth;
    }
    if (widgetWidth < minWidgetWidth)
    {
        widgetWidth = minWidgetWidth;
    }

    ImGui::TextUnformatted(label);
    ImGui::SameLine();

    const float widgetX = rowStartX + availableWidth - widgetWidth;
    if (ImGui::GetCursorPosX() < widgetX)
    {
        ImGui::SetCursorPosX(widgetX);
    }
    ImGui::SetNextItemWidth(widgetWidth);
    return widgetWidth;
}

void PushRowWidgetStyle()
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.095f, 0.095f, 0.112f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.125f, 0.120f, 0.150f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.150f, 0.105f, 0.240f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, kSectionBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
}

void PopRowWidgetStyle()
{
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
}

void SliderFloatRow(const char* label, float* value, float minValue, float maxValue, const char* format, const char* tooltip)
{
    ImGui::PushID(label);
    BeginRightAlignedWidgetRow(label, kRowWidgetWidth);
    PushRowWidgetStyle();
    ImGui::SliderFloat("##slider", value, minValue, maxValue, format);
    PopRowWidgetStyle();
    Tooltip(tooltip);
    ImGui::PopID();
}

void ColorRow(const char* label, float color[4], const char* tooltip)
{
    ImGui::ColorEdit4(label, color, ImGuiColorEditFlags_NoInputs);
    Tooltip(tooltip);
}

void ComboRow(const char* label, int* selected, const char* const* items, int itemCount, const char* tooltip)
{
    ImGui::PushID(label);
    BeginRightAlignedWidgetRow(label, kRowWidgetWidth);
    const int safeSelected = (*selected >= 0 && *selected < itemCount) ? *selected : 0;
    PushRowWidgetStyle();
    if (ImGui::BeginCombo("##combo", items[safeSelected]))
    {
        for (int i = 0; i < itemCount; ++i)
        {
            const bool isSelected = *selected == i;
            if (ImGui::Selectable(items[i], isSelected))
            {
                *selected = i;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    PopRowWidgetStyle();
    Tooltip(tooltip);
    ImGui::PopID();
}

void KeybindDisplay(const char* label, const char* keyName, const char* tooltip)
{
    ImGui::TextUnformatted(label);
    ImGui::SameLine(170.0f);
    ImGui::Button(keyName, ImVec2(96.0f, 0.0f));
    Tooltip(tooltip);
}

void HotkeySelectorRow()
{
    static double captureStartTime = 0.0;

    ImGui::TextUnformatted("Show/Hide Panel");
    ImGui::SameLine(170.0f);

    const char* buttonText = g_IsCapturingToggleHotkey ? "Press any key..." : GetToggleHotkeyName();
    const ImVec2 buttonSize(132.0f, 0.0f);
    if (ImGui::Button(buttonText, buttonSize))
    {
        g_IsCapturingToggleHotkey = true;
        captureStartTime = ImGui::GetTime();
    }
    ImGui::SameLine();
    char inlineHint[96] = {};
    std::snprintf(inlineHint, sizeof(inlineHint), "(Press %s anytime to toggle)", GetToggleHotkeyName());
    if (g_FontSmall != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
    }
    ImGui::TextColored(kMutedText, "%s", inlineHint);
    if (g_FontSmall != nullptr)
    {
        ImGui::PopFont();
    }

    if (g_IsCapturingToggleHotkey)
    {
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 8.0f);
        ImGui::GetWindowDrawList()->AddRect(
            ImVec2(min.x - 1.0f, min.y - 1.0f),
            ImVec2(max.x + 1.0f, max.y + 1.0f),
            AccentU32(0.42f + pulse * 0.32f),
            7.0f,
            0,
            1.6f);

        if (ImGui::GetTime() - captureStartTime > 0.10)
        {
            for (const KeyOption& key : kCaptureKeys)
            {
                if ((GetAsyncKeyState(key.vk) & 0x0001) != 0)
                {
                    g_ToggleHotkey = key.vk;
                    g_IsCapturingToggleHotkey = false;
                    std::string message = "Hotkey updated to ";
                    message += key.name;
                    ShowToast(message.c_str(), ToastType::Info);
                    break;
                }
            }
        }
    }

    Tooltip("Click, then press a supported key to update the panel show/hide hotkey.");
}

void SectionBegin(const char* title)
{
    const ImVec2 sectionShadowMin = ImGui::GetCursorScreenPos();
    const ImVec2 sectionShadowMax(sectionShadowMin.x + ImGui::GetContentRegionAvail().x, sectionShadowMin.y + 96.0f);
    DrawSectionCardShadow(ImGui::GetWindowDrawList(), sectionShadowMin, sectionShadowMax, kSectionRounding);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, WithAlpha(kSectionBg, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_Border, kSectionBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, kSectionRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild(title, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
    const ImVec2 headerPos = ImGui::GetCursorScreenPos();
    const float headerWidth = ImGui::GetContentRegionAvail().x;
    constexpr float headerHeight = 27.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilledMultiColor(
        headerPos,
        ImVec2(headerPos.x + headerWidth, headerPos.y + headerHeight),
        AccentU32(0.26f),
        AccentU32(0.02f),
        AccentU32(0.00f),
        AccentU32(0.16f));
    const ImVec2 titlePos(headerPos.x + 8.0f, headerPos.y + 5.0f);
    if (g_FontSection != nullptr)
    {
        drawList->AddText(g_FontSection, g_FontSection->LegacySize, titlePos, ImGui::GetColorU32(g_AccentColor), title);
    }
    else
    {
        drawList->AddText(titlePos, ImGui::GetColorU32(g_AccentColor), title);
    }
    ImGui::Dummy(ImVec2(headerWidth, headerHeight));
    GradientSeparator();
}

void SectionEnd()
{
    ImGui::EndChild();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    DrawSectionLift(ImGui::GetWindowDrawList(), min, max, kSectionRounding);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(ImVec2(min.x + 0.5f, min.y + 0.5f), ImVec2(max.x - 0.5f, max.y - 0.5f), ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.095f)), kSectionRounding, 0, 1.0f);
    drawList->AddRect(ImVec2(min.x + 2.0f, min.y + 2.0f), ImVec2(max.x - 2.0f, max.y - 2.0f), AccentU32(0.055f), kSectionRounding - 1.0f, 0, 1.0f);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::Spacing();
}

void DrawSidebarIcon(ImDrawList* drawList, Category category, ImVec2 center, ImU32 color)
{
    constexpr float stroke = 1.65f;
    switch (category)
    {
    case Category::Aimbot:
        drawList->AddCircle(center, 6.3f, color, 32, stroke);
        drawList->AddCircleFilled(center, 1.5f, color, 16);
        drawList->AddLine(ImVec2(center.x - 9.2f, center.y), ImVec2(center.x - 5.4f, center.y), color, stroke);
        drawList->AddLine(ImVec2(center.x + 5.4f, center.y), ImVec2(center.x + 9.2f, center.y), color, stroke);
        drawList->AddLine(ImVec2(center.x, center.y - 9.2f), ImVec2(center.x, center.y - 5.4f), color, stroke);
        drawList->AddLine(ImVec2(center.x, center.y + 5.4f), ImVec2(center.x, center.y + 9.2f), color, stroke);
        break;
    case Category::Visuals:
        drawList->AddBezierCubic(ImVec2(center.x - 9.0f, center.y), ImVec2(center.x - 4.8f, center.y - 5.6f), ImVec2(center.x + 4.8f, center.y - 5.6f), ImVec2(center.x + 9.0f, center.y), color, stroke);
        drawList->AddBezierCubic(ImVec2(center.x - 9.0f, center.y), ImVec2(center.x - 4.8f, center.y + 5.6f), ImVec2(center.x + 4.8f, center.y + 5.6f), ImVec2(center.x + 9.0f, center.y), color, stroke);
        drawList->AddCircle(center, 3.0f, color, 20, stroke);
        drawList->AddCircleFilled(center, 1.3f, color, 12);
        break;
    case Category::Skins:
        drawList->AddRect(ImVec2(center.x - 8.0f, center.y - 4.8f), ImVec2(center.x + 4.2f, center.y + 7.4f), color, 2.8f, 0, stroke);
        drawList->AddRect(ImVec2(center.x - 4.8f, center.y - 8.0f), ImVec2(center.x + 7.4f, center.y + 4.2f), color, 2.8f, 0, stroke);
        drawList->AddLine(ImVec2(center.x - 5.8f, center.y - 2.0f), ImVec2(center.x - 2.0f, center.y - 5.8f), color, 1.2f);
        break;
    case Category::World:
        drawList->AddCircle(center, 7.4f, color, 32, stroke);
        drawList->AddBezierCubic(ImVec2(center.x - 6.5f, center.y - 2.0f), ImVec2(center.x - 2.8f, center.y - 0.6f), ImVec2(center.x + 2.8f, center.y - 0.6f), ImVec2(center.x + 6.5f, center.y - 2.0f), color, 1.15f);
        drawList->AddBezierCubic(ImVec2(center.x - 6.5f, center.y + 2.0f), ImVec2(center.x - 2.8f, center.y + 0.6f), ImVec2(center.x + 2.8f, center.y + 0.6f), ImVec2(center.x + 6.5f, center.y + 2.0f), color, 1.15f);
        drawList->AddBezierCubic(ImVec2(center.x, center.y - 7.4f), ImVec2(center.x - 3.2f, center.y - 3.2f), ImVec2(center.x - 3.2f, center.y + 3.2f), ImVec2(center.x, center.y + 7.4f), color, 1.15f);
        drawList->AddBezierCubic(ImVec2(center.x, center.y - 7.4f), ImVec2(center.x + 3.2f, center.y - 3.2f), ImVec2(center.x + 3.2f, center.y + 3.2f), ImVec2(center.x, center.y + 7.4f), color, 1.15f);
        break;
    case Category::Movement:
        drawList->AddLine(ImVec2(center.x - 8.0f, center.y - 4.8f), ImVec2(center.x - 3.2f, center.y - 4.8f), color, 1.35f);
        drawList->AddLine(ImVec2(center.x - 9.0f, center.y), ImVec2(center.x - 4.5f, center.y), color, 1.35f);
        drawList->AddLine(ImVec2(center.x - 8.0f, center.y + 4.8f), ImVec2(center.x - 3.2f, center.y + 4.8f), color, 1.35f);
        drawList->AddLine(ImVec2(center.x - 1.5f, center.y - 7.2f), ImVec2(center.x + 7.4f, center.y), color, stroke);
        drawList->AddLine(ImVec2(center.x + 7.4f, center.y), ImVec2(center.x - 1.5f, center.y + 7.2f), color, stroke);
        drawList->AddLine(ImVec2(center.x - 1.5f, center.y + 7.2f), ImVec2(center.x + 1.0f, center.y), color, stroke);
        break;
    case Category::Player:
        drawList->AddCircle(ImVec2(center.x, center.y - 4.4f), 3.0f, color, 24, stroke);
        drawList->AddBezierCubic(ImVec2(center.x - 7.0f, center.y + 7.0f), ImVec2(center.x - 5.3f, center.y + 1.5f), ImVec2(center.x + 5.3f, center.y + 1.5f), ImVec2(center.x + 7.0f, center.y + 7.0f), color, stroke);
        drawList->AddLine(ImVec2(center.x - 7.0f, center.y + 7.0f), ImVec2(center.x + 7.0f, center.y + 7.0f), color, stroke);
        break;
    case Category::Misc:
        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                drawList->AddCircleFilled(ImVec2(center.x - 5.0f + static_cast<float>(x) * 5.0f, center.y - 5.0f + static_cast<float>(y) * 5.0f), 1.45f, color, 12);
            }
        }
        break;
    case Category::Config:
        drawList->AddCircle(center, 5.2f, color, 28, stroke);
        drawList->AddCircle(center, 1.9f, color, 18, 1.35f);
        for (int i = 0; i < 8; ++i)
        {
            const float angle = (3.14159265f * 2.0f * static_cast<float>(i)) / 8.0f;
            const ImVec2 inner(center.x + cosf(angle) * 6.7f, center.y + sinf(angle) * 6.7f);
            const ImVec2 outer(center.x + cosf(angle) * 9.0f, center.y + sinf(angle) * 9.0f);
            drawList->AddLine(inner, outer, color, 1.45f);
        }
        break;
    default:
        drawList->AddCircleFilled(center, 4.0f, color, 16);
        break;
    }
}

void SidebarButton(const CategoryItem& item, Category* selected, ImVec2* selectedRowMin, ImVec2* selectedRowMax)
{
    const bool isSelected = *selected == item.id;
    ImGui::PushID(item.label);

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 rowSize(ImGui::GetContentRegionAvail().x, 38.0f);
    if (ImGui::InvisibleButton("##sidebar-row", rowSize))
    {
        *selected = item.id;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float* hoverT = storage->GetFloatRef(ImGui::GetID("##sidebar-hover"), 0.0f);
    *hoverT = Approach(*hoverT, hovered && !isSelected ? 1.0f : 0.0f, 0.10f);
    const ImVec4 unselectedText = kMutedText;
    const ImVec4 hoveredText = ImVec4(0.780f, 0.760f, 0.840f, 1.0f);
    const ImVec4 textMix(
        unselectedText.x + (hoveredText.x - unselectedText.x) * *hoverT,
        unselectedText.y + (hoveredText.y - unselectedText.y) * *hoverT,
        unselectedText.z + (hoveredText.z - unselectedText.z) * *hoverT,
        1.0f);
    const ImVec4 unselectedIcon = ImVec4(0.360f, 0.340f, 0.420f, 1.0f);
    const ImVec4 hoveredIcon = ImVec4(0.580f, 0.560f, 0.640f, 1.0f);
    const ImVec4 iconMix(
        unselectedIcon.x + (hoveredIcon.x - unselectedIcon.x) * *hoverT,
        unselectedIcon.y + (hoveredIcon.y - unselectedIcon.y) * *hoverT,
        unselectedIcon.z + (hoveredIcon.z - unselectedIcon.z) * *hoverT,
        1.0f);
    const ImU32 textColor = ImGui::GetColorU32(isSelected ? g_AccentColor : textMix);
    const ImU32 iconColor = ImGui::GetColorU32(isSelected ? g_AccentColor : iconMix);
    const ImVec2 rowMax(pos.x + rowSize.x, pos.y + rowSize.y);

    if (isSelected)
    {
        *selectedRowMin = pos;
        *selectedRowMax = rowMax;
    }

    if (isSelected)
    {
        for (int i = 0; i < 3; ++i)
        {
            const float spread = static_cast<float>(i + 1) * 2.0f;
            drawList->AddRectFilled(
                ImVec2(pos.x - spread, pos.y - spread),
                ImVec2(rowMax.x + spread, rowMax.y + spread),
                AccentU32(0.080f - static_cast<float>(i) * 0.018f),
                8.0f + spread);
        }
        drawList->AddRectFilled(pos, rowMax, ImGui::GetColorU32(ScaleColor(g_AccentColor, 0.24f)), 7.0f);
        DrawInnerGlow(drawList, pos, rowMax, 7.0f, 0.30f);
    }
    else if (*hoverT > 0.001f)
    {
        drawList->AddRectFilled(pos, rowMax, ImGui::GetColorU32(ImVec4(0.105f, 0.105f, 0.125f, 0.70f * *hoverT)), 7.0f);
    }

    DrawSidebarIcon(drawList, item.id, ImVec2(pos.x + 18.0f, pos.y + rowSize.y * 0.5f), iconColor);
    drawList->AddText(ImVec2(pos.x + 38.0f, pos.y + (rowSize.y - ImGui::GetTextLineHeight()) * 0.5f), textColor, item.label);

    ImGui::PopID();
}

const char* CategoryTitle(Category category)
{
    switch (category)
    {
    case Category::Aimbot: return "Aimbot";
    case Category::Visuals: return "Visuals";
    case Category::Skins: return "Skins";
    case Category::World: return "World";
    case Category::Movement: return "Movement";
    case Category::Player: return "Player";
    case Category::Misc: return "Misc";
    case Category::Config: return "Config";
    default: return "Settings";
    }
}

void RenderEzMonogram(ImDrawList* drawList, ImVec2 topLeft, float size)
{
    const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 2.4f);
    const ImVec2 center(topLeft.x + size * 0.5f, topLeft.y + size * 0.5f);
    const ImVec4 accentHover = GetAccentHoverColor();
    const ImU32 accent = ImGui::GetColorU32(g_AccentColor);
    const ImU32 accentSoft = ImGui::GetColorU32(accentHover);
    const ImU32 glow = ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.12f + pulse * 0.16f));
    const ImU32 outline = ImGui::GetColorU32(ImVec4(0.165f, 0.130f, 0.230f, 1.0f));
    const float stroke = 3.1f;

    drawList->AddCircleFilled(center, size * (0.55f + pulse * 0.05f), glow, 48);
    drawList->AddCircleFilled(center, size * (0.72f + pulse * 0.07f), ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.035f + pulse * 0.035f)), 48);
    drawList->AddCircle(center, size * 0.49f, ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.32f + pulse * 0.26f)), 48, 1.5f);
    drawList->AddCircle(center, size * 0.40f, outline, 48, 1.0f);

    const float left = topLeft.x + 7.0f;
    const float right = topLeft.x + size - 6.0f;
    const float top = topLeft.y + 7.0f;
    const float mid = topLeft.y + size * 0.50f;
    const float bottom = topLeft.y + size - 7.0f;
    const float zLeft = topLeft.x + size * 0.45f;

    drawList->AddLine(ImVec2(left, top), ImVec2(left, bottom), accent, stroke);
    drawList->AddLine(ImVec2(left, top), ImVec2(zLeft, top), accentSoft, stroke);
    drawList->AddLine(ImVec2(left, mid), ImVec2(zLeft - 1.0f, mid), accent, stroke);
    drawList->AddLine(ImVec2(left, bottom), ImVec2(zLeft, bottom), accentSoft, stroke);

    drawList->AddLine(ImVec2(zLeft, top), ImVec2(right, top), accentSoft, stroke);
    drawList->AddLine(ImVec2(right, top), ImVec2(zLeft, bottom), accent, stroke);
    drawList->AddLine(ImVec2(zLeft, bottom), ImVec2(right, bottom), accentSoft, stroke);
    drawList->AddBezierCubic(
        ImVec2(zLeft - 1.0f, bottom + 1.0f),
        ImVec2(topLeft.x + size * 0.57f, topLeft.y + size * 0.82f),
        ImVec2(topLeft.x + size * 0.73f, topLeft.y + size * 0.62f),
        ImVec2(right - 1.0f, top + 1.0f),
        ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.28f)),
        1.2f);
}

void RenderBrandHeader()
{
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImVec2 logoPos(cursor.x + 2.0f, cursor.y + 2.0f);
    RenderEzMonogram(ImGui::GetWindowDrawList(), logoPos, 32.0f);

    ImGui::Dummy(ImVec2(36.0f, 36.0f));
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (g_FontSection != nullptr)
    {
        ImGui::PushFont(g_FontSection);
    }
    ImGui::TextColored(g_AccentColor, "EazyE HEX");
    if (g_FontSection != nullptr)
    {
        ImGui::PopFont();
    }
    if (g_FontSmall != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
    }
    ImGui::TextColored(kMutedText, "Panel UI v1.0");
    if (g_FontSmall != nullptr)
    {
        ImGui::PopFont();
    }
    ImGui::EndGroup();
}

void RenderBootSplashInternal(float progress, float alpha)
{
    const ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowMax(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
    const ImVec2 center(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
    const float clampedProgress = Clamp01(progress);
    const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 4.2f);
    const float logoSize = windowSize.y * 0.18f < 94.0f ? windowSize.y * 0.18f : 94.0f;
    const float barWidth = windowSize.x * 0.70f < 300.0f ? windowSize.x * 0.70f : 300.0f;
    const float barHeight = 6.0f;
    const float logoTop = windowPos.y + windowSize.y * 0.18f;
    const float titleY = logoTop + logoSize + 24.0f;
    const float barY = titleY + 64.0f;
    const float statusY = barY + 26.0f;

    DrawLayeredPanelShadow(drawList, windowPos, windowMax, kOuterWindowRounding);
    DrawAcrylicPanelSurface(drawList, windowPos, windowMax, kOuterWindowRounding);
    DrawPremiumTopStrip(drawList, windowPos, windowMax);
    DrawAnimatedGradientMesh(drawList, windowPos, windowMax, 0.62f * alpha);
    DrawAnimatedTechPattern(drawList, windowPos, windowMax, 0.50f * alpha);
    drawList->AddCircleFilled(center, windowSize.x * 0.32f + pulse * 8.0f, AccentU32((0.032f + pulse * 0.030f) * alpha), 96);
    drawList->AddCircleFilled(ImVec2(center.x - windowSize.x * 0.16f, center.y - windowSize.y * 0.10f), windowSize.x * 0.22f, AccentU32(0.020f * alpha), 96);
    drawList->AddCircleFilled(ImVec2(center.x + windowSize.x * 0.18f, center.y + windowSize.y * 0.12f), windowSize.x * 0.24f, ImGui::GetColorU32(WithAlpha(GetAccentHoverColor(), 0.020f * alpha)), 96);

    const ImVec2 logoPos(center.x - logoSize * 0.5f, logoTop);
    RenderEzMonogram(drawList, logoPos, logoSize);

    const char* title = "EazyE HEX";
    if (g_FontSection != nullptr)
    {
        const ImVec2 titleSize = g_FontSection->CalcTextSizeA(g_FontSection->LegacySize, FLT_MAX, 0.0f, title);
        drawList->AddText(g_FontSection, g_FontSection->LegacySize, ImVec2(center.x - titleSize.x * 0.5f, titleY), ImGui::GetColorU32(WithAlpha(g_AccentColor, alpha)), title);
    }
    else
    {
        const ImVec2 titleSize = ImGui::CalcTextSize(title);
        drawList->AddText(ImVec2(center.x - titleSize.x * 0.5f, titleY), ImGui::GetColorU32(WithAlpha(g_AccentColor, alpha)), title);
    }

    const ImVec2 barMin(center.x - barWidth * 0.5f, barY);
    const ImVec2 barMax(center.x + barWidth * 0.5f, barY + barHeight);
    drawList->AddRectFilled(barMin, barMax, ImGui::GetColorU32(ImVec4(0.085f, 0.082f, 0.105f, 0.90f * alpha)), 3.0f);
    const ImVec2 fillMax(barMin.x + (barMax.x - barMin.x) * clampedProgress, barMax.y);
    drawList->AddRectFilledMultiColor(
        barMin,
        fillMax,
        AccentU32(0.92f * alpha),
        ImGui::GetColorU32(WithAlpha(GetAccentHoverColor(), 0.96f * alpha)),
        ImGui::GetColorU32(WithAlpha(GetAccentHoverColor(), 0.74f * alpha)),
        AccentU32(0.68f * alpha));
    drawList->AddRect(barMin, barMax, AccentU32(0.34f * alpha), 3.0f, 0, 1.0f);

    const char* status = "Initializing EazyE HEX...";
    if (clampedProgress >= 0.75f)
    {
        status = "Ready";
    }
    else if (clampedProgress >= 0.40f)
    {
        status = "Loading interface...";
    }

    if (g_FontSmall != nullptr)
    {
        const ImVec2 statusSize = g_FontSmall->CalcTextSizeA(g_FontSmall->LegacySize, FLT_MAX, 0.0f, status);
        drawList->AddText(g_FontSmall, g_FontSmall->LegacySize, ImVec2(center.x - statusSize.x * 0.5f, statusY), ImGui::GetColorU32(WithAlpha(kMutedText, alpha)), status);
    }
    else
    {
        const ImVec2 statusSize = ImGui::CalcTextSize(status);
        drawList->AddText(ImVec2(center.x - statusSize.x * 0.5f, statusY), ImGui::GetColorU32(WithAlpha(kMutedText, alpha)), status);
    }
}

bool OpenAvatarFileDialog(char* path, DWORD pathSize)
{
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathSize;
    ofn.lpstrFilter = "Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0PNG Files (*.png)\0*.png\0JPEG Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofn) != FALSE;
}

void DrawAvatarPlaceholder(ImDrawList* drawList, ImVec2 center, float radius)
{
    drawList->AddCircleFilled(center, radius, ImGui::GetColorU32(ImVec4(0.088f, 0.082f, 0.105f, 1.0f)), 48);
    drawList->AddCircle(center, radius, AccentU32(0.42f), 48, 1.4f);
    drawList->AddCircle(center, radius - 2.0f, AccentU32(0.12f), 48, 1.0f);

    const ImU32 icon = ImGui::GetColorU32(ImVec4(0.620f, 0.590f, 0.700f, 1.0f));
    drawList->AddCircle(ImVec2(center.x, center.y - radius * 0.26f), radius * 0.22f, icon, 28, 1.7f);
    drawList->AddBezierCubic(
        ImVec2(center.x - radius * 0.46f, center.y + radius * 0.44f),
        ImVec2(center.x - radius * 0.34f, center.y + radius * 0.06f),
        ImVec2(center.x + radius * 0.34f, center.y + radius * 0.06f),
        ImVec2(center.x + radius * 0.46f, center.y + radius * 0.44f),
        icon,
        1.7f);
}

void RenderProfileAvatar()
{
    constexpr float diameter = 46.0f;
    const ImVec2 avatarSize(diameter, diameter);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton("##ProfileAvatar", avatarSize);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 center(pos.x + diameter * 0.5f, pos.y + diameter * 0.5f);
    const float radius = diameter * 0.5f;

    if (g_ProfileAvatarTexture != nullptr)
    {
        drawList->AddImageRounded(
            reinterpret_cast<ImTextureID>(g_ProfileAvatarTexture),
            pos,
            ImVec2(pos.x + diameter, pos.y + diameter),
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f),
            ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)),
            radius);
        drawList->AddCircle(center, radius, AccentU32(hovered ? 0.68f : 0.44f), 48, hovered ? 2.0f : 1.4f);
    }
    else
    {
        DrawAvatarPlaceholder(drawList, center, radius);
        if (hovered)
        {
            drawList->AddCircle(center, radius + 1.5f, AccentU32(0.46f), 48, 1.6f);
        }
    }

    if (clicked)
    {
        if (g_PanelD3DDevice == nullptr)
        {
            ShowToast("Profile picture unavailable: D3D device missing", ToastType::Warning);
        }
        else
        {
            char path[MAX_PATH] = {};
            if (OpenAvatarFileDialog(path, MAX_PATH))
            {
                if (LoadProfilePictureFromPath(path))
                {
                    ShowToast("Profile picture updated", ToastType::Success);
                }
                else
                {
                    ShowToast("Failed to load profile picture", ToastType::Warning);
                }
            }
        }
    }

    if (hovered)
    {
        ImGui::SetTooltip("Choose profile picture");
    }
    (void)g_ProfileAvatarWidth;
    (void)g_ProfileAvatarHeight;
}

void RenderSidebar(Category* selected)
{
    constexpr float headerHeight = 72.0f;
    constexpr float footerHeight = 82.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, WithAlpha(kSidebarBg, 0.88f));
    DrawSoftShadow(
        ImGui::GetWindowDrawList(),
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + kSidebarWidth, ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y),
        8.0f);
    ImGui::BeginChild("Sidebar", ImVec2(kSidebarWidth, 0.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawAnimatedGradientMesh(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        0.45f);
    DrawAnimatedTechPattern(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        0.65f);

    ImGui::BeginChild("SidebarHeader", ImVec2(0.0f, headerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawAnimatedTechPattern(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        0.75f);
    RenderBrandHeader();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    GradientSeparator();
    ImGui::EndChild();

    const float sidebarRemainingHeight = ImGui::GetContentRegionAvail().y;
    const float middleHeight = sidebarRemainingHeight > footerHeight
        ? sidebarRemainingHeight - footerHeight
        : 0.0f;

    ImGui::BeginChild("SidebarCategories", ImVec2(0.0f, middleHeight), false);
    DrawAnimatedTechPattern(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        0.70f);
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    ImVec2 selectedRowMin(0.0f, 0.0f);
    ImVec2 selectedRowMax(0.0f, 0.0f);
    for (const CategoryItem& item : kCategories)
    {
        SidebarButton(item, selected, &selectedRowMin, &selectedRowMax);
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
    }

    if (selectedRowMax.y > selectedRowMin.y)
    {
        static float indicatorY = -1.0f;
        static float indicatorHeight = 38.0f;
        if (indicatorY < 0.0f)
        {
            indicatorY = selectedRowMin.y;
        }
        indicatorY = Approach(indicatorY, selectedRowMin.y, 0.15f);
        indicatorHeight = Approach(indicatorHeight, selectedRowMax.y - selectedRowMin.y, 0.15f);
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(selectedRowMin.x, indicatorY),
            ImVec2(selectedRowMin.x + 4.0f, indicatorY + indicatorHeight),
            ImGui::GetColorU32(g_AccentColor),
            2.0f);
    }

    ImGui::EndChild();

    ImGui::BeginChild("SidebarFooter", ImVec2(0.0f, footerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawAnimatedTechPattern(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        0.70f);
    GradientSeparator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    RenderProfileAvatar();
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (g_FontSmall != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
    }
    ImGui::TextColored(kMutedText, "Profile");
    if (g_FontSmall != nullptr)
    {
        ImGui::PopFont();
    }
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.075f, 0.075f, 0.086f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.105f, 0.105f, 0.125f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(g_AccentColor.x * 0.22f, g_AccentColor.y * 0.22f, g_AccentColor.z * 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, kSectionBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##ProfileName", g_ProfileName, sizeof(g_ProfileName));
    if (ImGui::IsItemActive())
    {
        ImGui::GetWindowDrawList()->AddRect(
            ImGui::GetItemRectMin(),
            ImGui::GetItemRectMax(),
            ImGui::GetColorU32(g_AccentColor),
            6.0f,
            0,
            1.5f);
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    ImGui::EndGroup();
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void RenderLiveStatsBar()
{
    static const double launchTime = ImGui::GetTime();
    static double nextPingUpdate = 0.0;
    static unsigned int pingSeed = 0xA53C91u;
    static float pingValue = 24.0f;
    static float pingTarget = 24.0f;

    const double now = ImGui::GetTime();
    if (now >= nextPingUpdate)
    {
        pingSeed = pingSeed * 1664525u + 1013904223u;
        pingTarget = 18.0f + static_cast<float>((pingSeed >> 16) % 18u);
        nextPingUpdate = now + 1.15 + static_cast<double>((pingSeed >> 8) % 70u) / 100.0;
    }
    pingValue = Approach(pingValue, pingTarget, 0.75f);

    const int elapsed = static_cast<int>(now - launchTime);
    const int hours = elapsed / 3600;
    const int minutes = (elapsed / 60) % 60;
    const int seconds = elapsed % 60;
    char pingText[32] = {};
    char uptimeText[32] = {};
    std::snprintf(pingText, sizeof(pingText), "Ping: %dms", static_cast<int>(pingValue + 0.5f));
    std::snprintf(uptimeText, sizeof(uptimeText), "Uptime: %02d:%02d:%02d", hours, minutes, seconds);

    constexpr float barHeight = 30.0f;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const ImVec2 max(pos.x + width, pos.y + barHeight);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(pos, max, ImGui::GetColorU32(ImVec4(0.066f, 0.066f, 0.078f, 0.88f)), 7.0f);
    drawList->AddRectFilledMultiColor(
        ImVec2(pos.x, max.y - 1.0f),
        ImVec2(max.x, max.y),
        AccentU32(0.25f),
        AccentU32(0.03f),
        AccentU32(0.03f),
        AccentU32(0.25f));

    if (g_FontSmall != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
    }

    const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(now) * 4.6f);
    const ImVec2 dotCenter(pos.x + 16.0f, pos.y + barHeight * 0.5f);
    const ImU32 dotGlow = ImGui::GetColorU32(ImVec4(0.160f, 0.780f, 0.420f, 0.14f + pulse * 0.22f));
    const ImU32 dotColor = ImGui::GetColorU32(ImVec4(0.160f, 0.900f, 0.470f, 1.0f));
    drawList->AddCircleFilled(dotCenter, 7.0f + pulse * 3.0f, dotGlow, 24);
    drawList->AddCircleFilled(dotCenter, 3.2f, dotColor, 18);

    const ImU32 muted = ImGui::GetColorU32(kMutedText);
    const ImU32 textColor = ImGui::GetColorU32(ImVec4(0.820f, 0.815f, 0.865f, 1.0f));
    const char* statusText = "Status: Connected";
    const ImVec2 statusSize = ImGui::CalcTextSize(statusText);
    const ImVec2 pingSize = ImGui::CalcTextSize(pingText);
    const ImVec2 uptimeSize = ImGui::CalcTextSize(uptimeText);
    const float textY = pos.y + (barHeight - ImGui::GetTextLineHeight()) * 0.5f;
    const float statusX = pos.x + 28.0f;
    const float pingX = pos.x + width * 0.48f - pingSize.x * 0.5f;
    const ImVec2 badgeMax(max.x - 12.0f, pos.y + (barHeight + 22.0f) * 0.5f);
    const ImVec2 badgeMin = DrawLicensedBadge(drawList, badgeMax);
    const float uptimeX = badgeMin.x - uptimeSize.x - 24.0f;

    drawList->AddText(ImVec2(statusX, textY), textColor, statusText);
    drawList->AddText(ImVec2(pingX, textY), muted, pingText);
    drawList->AddText(ImVec2(uptimeX, textY), muted, uptimeText);

    const float dividerA = statusX + statusSize.x + 18.0f;
    const float dividerB = pingX + pingSize.x + 18.0f;
    drawList->AddLine(ImVec2(dividerA, pos.y + 8.0f), ImVec2(dividerA, max.y - 8.0f), ImGui::GetColorU32(ImVec4(0.190f, 0.165f, 0.250f, 0.75f)), 1.0f);
    drawList->AddLine(ImVec2(dividerB, pos.y + 8.0f), ImVec2(dividerB, max.y - 8.0f), ImGui::GetColorU32(ImVec4(0.190f, 0.165f, 0.250f, 0.75f)), 1.0f);

    if (g_FontSmall != nullptr)
    {
        ImGui::PopFont();
    }

    ImGui::Dummy(ImVec2(width, barHeight + 8.0f));
}

void PopupButton(const char* label, const char* popupId, const char* popupText, const char* toastMessage, ToastType toastType)
{
    if (GradientButton(label, ImVec2(118.0f, 28.0f)))
    {
        ShowToast(toastMessage, toastType);
        ImGui::OpenPopup(popupId);
    }
    Tooltip("Opens a mock configuration dialog. No file I/O is performed.");

    if (ImGui::BeginPopupModal(popupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted(popupText);
        if (ImGui::Button("OK", ImVec2(96.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SaveConfigButton()
{
    if (GradientButton("Save Config", ImVec2(118.0f, 28.0f)))
    {
        const ConfigSaveResult result = SaveConfigToFile(g_ProfileName);
        if (result.success)
        {
            const std::string message = "Configuration saved to " + result.path;
            ShowToast(message.c_str(), ToastType::Success);
        }
        else
        {
            const std::string message = "Save failed: " + result.error;
            ShowToast(message.c_str(), ToastType::Warning);
        }
    }
    Tooltip("Writes the current UI state to a JSON config next to the executable.");
}

void LoadConfigButton()
{
    if (GradientButton("Load Config", ImVec2(118.0f, 28.0f)))
    {
        const ConfigSaveResult result = LoadConfigFromFile(g_ProfileName);
        if (result.success)
        {
            const std::string message = "Configuration loaded from " + result.path;
            ShowToast(message.c_str(), ToastType::Info);
            if (!result.warning.empty())
            {
                ShowToast(result.warning.c_str(), ToastType::Warning);
            }
        }
        else
        {
            const std::string message = "Load failed: " + result.error;
            ShowToast(message.c_str(), ToastType::Warning);
        }
    }
    Tooltip("Loads the current profile JSON from the configs folder.");
}

void DeleteConfigButton()
{
    if (GradientButton("Delete Config", ImVec2(118.0f, 28.0f)))
    {
        const ConfigSaveResult result = DeleteConfigFile(g_ProfileName);
        if (result.success)
        {
            ShowToast("Configuration file deleted", ToastType::Warning);
        }
        else
        {
            const char* message = result.error == "Config file does not exist"
                ? "No configuration file found to delete"
                : result.error.c_str();
            ShowToast(message, ToastType::Warning);
        }
    }
    Tooltip("Deletes the current profile JSON from the configs folder.");
}

void AccentSwatch(const char* label, ImVec4 color)
{
    ImGui::PushID(label);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size(18.0f, 18.0f);
    const bool pressed = ImGui::InvisibleButton("##swatch", size);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

    drawList->AddCircleFilled(center, 7.0f, ImGui::GetColorU32(color), 24);
    drawList->AddCircle(center, hovered ? 8.5f : 8.0f, ImGui::GetColorU32(hovered ? ImVec4(0.950f, 0.950f, 0.980f, 1.0f) : kSectionBorder), 24, hovered ? 1.6f : 1.0f);

    if (pressed)
    {
        g_AccentColor = color;
        ApplyAccentThemeColors();
        ShowToast("Theme updated", ToastType::Info);
    }
    Tooltip(label);
    ImGui::PopID();
}

void RenderAppearanceControls()
{
    SectionBegin("Appearance");
    if (ImGui::ColorEdit4("Theme Color", reinterpret_cast<float*>(&g_AccentColor), ImGuiColorEditFlags_NoAlpha))
    {
        g_AccentColor.w = 1.0f;
        ApplyAccentThemeColors();
    }
    Tooltip("Changes the live accent color used throughout this UI session.");

    ImGui::TextColored(kMutedText, "Presets");
    ImGui::SameLine(92.0f);
    AccentSwatch("Purple", ImVec4(0.486f, 0.227f, 0.929f, 1.0f));
    ImGui::SameLine();
    AccentSwatch("Cyan", ImVec4(0.000f, 0.760f, 0.900f, 1.0f));
    ImGui::SameLine();
    AccentSwatch("Red", ImVec4(0.925f, 0.180f, 0.260f, 1.0f));
    ImGui::SameLine();
    AccentSwatch("Green", ImVec4(0.160f, 0.780f, 0.420f, 1.0f));
    ImGui::SameLine();
    AccentSwatch("Pink", ImVec4(0.940f, 0.260f, 0.640f, 1.0f));
    SectionEnd();
}

void RenderHotkeyControls()
{
    SectionBegin("Hotkeys");
    HotkeySelectorRow();
    SectionEnd();
}
}

void RenderBootSplash(float progress, float alpha)
{
    RenderBootSplashInternal(progress, alpha);
}

std::string FormatElapsedSince(std::int64_t timestamp)
{
    if (timestamp <= 0)
    {
        return "First launch";
    }

    const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
    std::int64_t elapsed = now > timestamp ? now - timestamp : 0;
    if (elapsed < 60)
    {
        return "Just now";
    }
    if (elapsed < 3600)
    {
        const std::int64_t minutes = elapsed / 60;
        return std::to_string(minutes) + (minutes == 1 ? " minute ago" : " minutes ago");
    }
    if (elapsed < 86400)
    {
        const std::int64_t hours = elapsed / 3600;
        return std::to_string(hours) + (hours == 1 ? " hour ago" : " hours ago");
    }

    const std::int64_t days = elapsed / 86400;
    return std::to_string(days) + (days == 1 ? " day ago" : " days ago");
}

void DrawWelcomeInfoRow(ImDrawList* drawList, ImVec2 pos, const char* label, const char* value, bool drawSwatch)
{
    const ImU32 labelColor = ImGui::GetColorU32(kMutedText);
    const ImU32 valueColor = ImGui::GetColorU32(ImVec4(0.900f, 0.895f, 0.940f, 1.0f));
    drawList->AddText(pos, labelColor, label);
    if (drawSwatch)
    {
        const ImVec2 swatchCenter(pos.x + 126.0f, pos.y + ImGui::GetTextLineHeight() * 0.5f);
        drawList->AddCircleFilled(swatchCenter, 5.0f, ImGui::GetColorU32(g_AccentColor), 20);
        drawList->AddCircle(swatchCenter, 6.0f, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.24f)), 20, 1.0f);
        drawList->AddText(ImVec2(pos.x + 140.0f, pos.y), valueColor, value);
    }
    else
    {
        drawList->AddText(ImVec2(pos.x + 126.0f, pos.y), valueColor, value);
    }
}

bool RenderWelcomeScreen(float alpha)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const ImVec2 authSize(
        windowSize.x < 360.0f ? windowSize.x : 360.0f,
        windowSize.y < 440.0f ? windowSize.y : 440.0f);
    const ImVec2 authPos(
        windowPos.x + (windowSize.x - authSize.x) * 0.5f,
        windowPos.y + (windowSize.y - authSize.y) * 0.5f);
    const ImVec2 authMax(authPos.x + authSize.x, authPos.y + authSize.y);

    DrawLayeredPanelShadow(drawList, authPos, authMax, kOuterWindowRounding);
    DrawAcrylicPanelSurface(drawList, authPos, authMax, kOuterWindowRounding);
    DrawPremiumTopStrip(drawList, authPos, authMax);
    DrawAnimatedGradientMesh(drawList, authPos, authMax, 0.70f);
    DrawAnimatedTechPattern(drawList, authPos, authMax, 0.55f);

    const float centerX = authPos.x + authSize.x * 0.5f;
    const float logoSize = 56.0f;
    RenderEzMonogram(drawList, ImVec2(centerX - logoSize * 0.5f, authPos.y + 44.0f), logoSize);

    const char* title = "Welcome to EazyE HEX";
    ImFont* titleFont = g_FontSection != nullptr ? g_FontSection : ImGui::GetFont();
    const float titleSizePx = titleFont->LegacySize;
    const ImVec2 titleSize = titleFont->CalcTextSizeA(titleSizePx, FLT_MAX, 0.0f, title);
    drawList->AddText(titleFont, titleSizePx, ImVec2(centerX - titleSize.x * 0.5f, authPos.y + 116.0f), ImGui::GetColorU32(ImVec4(0.940f, 0.930f, 0.980f, 1.0f)), title);

    char profileLine[96] = {};
    std::snprintf(profileLine, sizeof(profileLine), "Signed in as: %s", g_ProfileName);
    const ImVec2 profileSize = ImGui::CalcTextSize(profileLine);
    drawList->AddText(ImVec2(centerX - profileSize.x * 0.5f, authPos.y + 142.0f), ImGui::GetColorU32(kMutedText), profileLine);

    const ImVec2 cardSize(authSize.x - 48.0f, 106.0f);
    const ImVec2 cardMin(centerX - cardSize.x * 0.5f, authPos.y + 178.0f);
    const ImVec2 cardMax(cardMin.x + cardSize.x, cardMin.y + cardSize.y);
    DrawSectionCardShadow(drawList, cardMin, cardMax, kSectionRounding);
    drawList->AddRectFilled(cardMin, cardMax, ImGui::GetColorU32(ImVec4(0.070f, 0.068f, 0.084f, 0.91f)), kSectionRounding);
    drawList->AddRect(cardMin, cardMax, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.11f)), kSectionRounding, 0, 1.0f);
    drawList->AddRect(ImVec2(cardMin.x + 2.0f, cardMin.y + 2.0f), ImVec2(cardMax.x - 2.0f, cardMax.y - 2.0f), AccentU32(0.08f), kSectionRounding - 1.0f, 0, 1.0f);

    const std::string lastSession = FormatElapsedSince(GetLastSessionTimestamp(g_ProfileName));
    const int savedConfigs = CountSavedConfigFiles();
    char savedConfigText[32] = {};
    std::snprintf(savedConfigText, sizeof(savedConfigText), "%d", savedConfigs);

    DrawWelcomeInfoRow(drawList, ImVec2(cardMin.x + 20.0f, cardMin.y + 18.0f), "Last session:", lastSession.c_str(), false);
    DrawWelcomeInfoRow(drawList, ImVec2(cardMin.x + 20.0f, cardMin.y + 45.0f), "Saved configs:", savedConfigText, false);
    DrawWelcomeInfoRow(drawList, ImVec2(cardMin.x + 20.0f, cardMin.y + 72.0f), "Current theme:", "Accent", true);

    const ImVec2 buttonSize(190.0f, 36.0f);
    ImGui::SetCursorScreenPos(ImVec2(centerX - buttonSize.x * 0.5f, authPos.y + 324.0f));
    const bool clicked = GradientButton("Continue to Panel", buttonSize);
    if (clicked)
    {
        const ConfigSaveResult result = UpdateLastSessionTimestamp(g_ProfileName, static_cast<std::int64_t>(std::time(nullptr)));
        if (!result.success)
        {
            ShowToast("Failed to update session timestamp", ToastType::Warning);
        }
    }

    (void)alpha;
    return clicked;
}

void DrawVisualsLivePreview(const VisualsConfig& state)
{
    const ImVec2 previewSize(200.0f, 240.0f);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float centeredX = ImGui::GetCursorPosX() + (availableWidth - previewSize.x) * 0.5f;

    if (availableWidth > previewSize.x)
    {
        ImGui::SetCursorPosX(centeredX);
    }

    const ImVec2 previewMin = ImGui::GetCursorScreenPos();
    const ImVec2 previewMax(previewMin.x + previewSize.x, previewMin.y + previewSize.y);
    ImGui::InvisibleButton("##VisualsLivePreview", previewSize);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(previewMin, previewMax, true);

    drawList->AddRectFilledMultiColor(
        previewMin,
        previewMax,
        ImGui::GetColorU32(ImVec4(0.036f, 0.037f, 0.048f, 1.0f)),
        ImGui::GetColorU32(ImVec4(0.044f, 0.042f, 0.058f, 1.0f)),
        ImGui::GetColorU32(ImVec4(0.022f, 0.023f, 0.030f, 1.0f)),
        ImGui::GetColorU32(ImVec4(0.030f, 0.031f, 0.040f, 1.0f)));

    const ImVec2 sceneCenter(previewMin.x + previewSize.x * 0.5f, previewMin.y + previewSize.y * 0.56f);
    const ImU32 gridColor = ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.08f));
    const ImU32 groundColor = ImGui::GetColorU32(ImVec4(0.210f, 0.195f, 0.255f, 0.40f));
    drawList->AddLine(ImVec2(previewMin.x + 22.0f, previewMax.y - 35.0f), ImVec2(previewMax.x - 22.0f, previewMax.y - 35.0f), groundColor, 1.0f);
    drawList->AddLine(ImVec2(previewMin.x + 34.0f, previewMax.y - 23.0f), ImVec2(previewMax.x - 34.0f, previewMax.y - 23.0f), ImGui::GetColorU32(ImVec4(0.130f, 0.120f, 0.160f, 0.28f)), 1.0f);
    drawList->AddCircleFilled(ImVec2(previewMin.x + 52.0f, previewMin.y + 44.0f), 40.0f, ImGui::GetColorU32(WithAlpha(g_AccentColor, 0.035f)), 48);
    drawList->AddCircleFilled(ImVec2(previewMax.x - 44.0f, previewMin.y + 72.0f), 30.0f, ImGui::GetColorU32(WithAlpha(ImVec4(state.glowColor[0], state.glowColor[1], state.glowColor[2], 1.0f), 0.028f)), 48);
    drawList->AddLine(ImVec2(previewMin.x + 18.0f, previewMin.y + 64.0f), ImVec2(previewMax.x - 18.0f, previewMin.y + 64.0f), gridColor, 1.0f);
    drawList->AddLine(ImVec2(previewMin.x + 18.0f, previewMin.y + 118.0f), ImVec2(previewMax.x - 18.0f, previewMin.y + 118.0f), gridColor, 1.0f);

    const ImVec4 chams(state.chamsColor[0], state.chamsColor[1], state.chamsColor[2], state.chamsColor[3]);
    const ImVec4 glow(state.glowColor[0], state.glowColor[1], state.glowColor[2], state.glowColor[3]);
    const ImU32 bodyColor = ImGui::GetColorU32(WithAlpha(chams, 0.72f * chams.w));
    const ImU32 bodyShade = ImGui::GetColorU32(WithAlpha(ScaleColor(chams, 0.62f), 0.82f * chams.w));
    const ImU32 bodyEdge = ImGui::GetColorU32(WithAlpha(ScaleColor(chams, 1.20f), 0.88f * chams.w));
    const ImU32 glowLine = ImGui::GetColorU32(WithAlpha(glow, 0.78f * glow.w));

    const ImVec2 headCenter(sceneCenter.x, sceneCenter.y - 72.0f);
    const float headRadius = 18.0f;
    const ImVec2 torsoMin(sceneCenter.x - 24.0f, sceneCenter.y - 48.0f);
    const ImVec2 torsoMax(sceneCenter.x + 24.0f, sceneCenter.y + 24.0f);
    const ImVec2 silhouetteMin(sceneCenter.x - 43.0f, sceneCenter.y - 94.0f);
    const ImVec2 silhouetteMax(sceneCenter.x + 43.0f, sceneCenter.y + 75.0f);

    for (int layer = 3; layer >= 1; --layer)
    {
        const float spread = static_cast<float>(layer) * 5.0f;
        const float alpha = 0.055f * static_cast<float>(4 - layer) * glow.w;
        const ImU32 glowSoft = ImGui::GetColorU32(WithAlpha(glow, alpha));
        drawList->AddRect(ImVec2(silhouetteMin.x - spread, silhouetteMin.y - spread), ImVec2(silhouetteMax.x + spread, silhouetteMax.y + spread), glowSoft, 18.0f, 0, 2.0f);
        drawList->AddCircle(headCenter, headRadius + spread * 0.55f, glowSoft, 36, 2.0f);
    }

    drawList->AddCircleFilled(headCenter, headRadius, bodyColor, 40);
    drawList->AddRectFilled(torsoMin, torsoMax, bodyColor, 11.0f);
    drawList->AddRectFilled(ImVec2(sceneCenter.x - 41.0f, sceneCenter.y - 38.0f), ImVec2(sceneCenter.x - 25.0f, sceneCenter.y + 20.0f), bodyShade, 8.0f);
    drawList->AddRectFilled(ImVec2(sceneCenter.x + 25.0f, sceneCenter.y - 38.0f), ImVec2(sceneCenter.x + 41.0f, sceneCenter.y + 20.0f), bodyShade, 8.0f);
    drawList->AddRectFilled(ImVec2(sceneCenter.x - 22.0f, sceneCenter.y + 20.0f), ImVec2(sceneCenter.x - 5.0f, sceneCenter.y + 74.0f), bodyShade, 7.0f);
    drawList->AddRectFilled(ImVec2(sceneCenter.x + 5.0f, sceneCenter.y + 20.0f), ImVec2(sceneCenter.x + 22.0f, sceneCenter.y + 74.0f), bodyShade, 7.0f);
    drawList->AddCircle(headCenter, headRadius, bodyEdge, 40, 1.3f);
    drawList->AddRect(torsoMin, torsoMax, bodyEdge, 11.0f, 0, 1.2f);

    if (state.skeletonEsp)
    {
        const ImU32 skeletonColor = ImGui::GetColorU32(ImVec4(0.945f, 0.935f, 0.980f, 0.92f));
        const ImVec2 neck(sceneCenter.x, sceneCenter.y - 50.0f);
        const ImVec2 pelvis(sceneCenter.x, sceneCenter.y + 24.0f);
        drawList->AddLine(headCenter, neck, skeletonColor, 1.4f);
        drawList->AddLine(neck, pelvis, skeletonColor, 1.4f);
        drawList->AddLine(ImVec2(sceneCenter.x - 34.0f, sceneCenter.y - 32.0f), ImVec2(sceneCenter.x + 34.0f, sceneCenter.y - 32.0f), skeletonColor, 1.4f);
        drawList->AddLine(ImVec2(sceneCenter.x - 34.0f, sceneCenter.y - 32.0f), ImVec2(sceneCenter.x - 36.0f, sceneCenter.y + 18.0f), skeletonColor, 1.4f);
        drawList->AddLine(ImVec2(sceneCenter.x + 34.0f, sceneCenter.y - 32.0f), ImVec2(sceneCenter.x + 36.0f, sceneCenter.y + 18.0f), skeletonColor, 1.4f);
        drawList->AddLine(pelvis, ImVec2(sceneCenter.x - 13.0f, sceneCenter.y + 74.0f), skeletonColor, 1.4f);
        drawList->AddLine(pelvis, ImVec2(sceneCenter.x + 13.0f, sceneCenter.y + 74.0f), skeletonColor, 1.4f);
        drawList->AddCircleFilled(neck, 2.1f, skeletonColor, 10);
        drawList->AddCircleFilled(pelvis, 2.1f, skeletonColor, 10);
    }

    if (state.boxEsp)
    {
        drawList->AddRect(silhouetteMin, silhouetteMax, glowLine, 2.0f, 0, 1.4f);
        drawList->AddRect(ImVec2(silhouetteMin.x - 1.0f, silhouetteMin.y - 1.0f), ImVec2(silhouetteMax.x + 1.0f, silhouetteMax.y + 1.0f), ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.30f)), 2.0f, 0, 1.0f);
    }

    if (state.healthBar)
    {
        const ImVec2 barMin(sceneCenter.x - 34.0f, silhouetteMin.y - 13.0f);
        const ImVec2 barMax(sceneCenter.x + 34.0f, silhouetteMin.y - 8.0f);
        drawList->AddRectFilled(barMin, barMax, ImGui::GetColorU32(ImVec4(0.055f, 0.055f, 0.064f, 0.95f)), 2.0f);
        drawList->AddRectFilled(barMin, ImVec2(barMin.x + (barMax.x - barMin.x) * 0.75f, barMax.y), ImGui::GetColorU32(ImVec4(0.180f, 0.820f, 0.360f, 1.0f)), 2.0f);
    }

    if (state.nameTags)
    {
        const char* name = "Player_01";
        const ImVec2 nameSize = ImGui::CalcTextSize(name);
        drawList->AddText(ImVec2(sceneCenter.x - nameSize.x * 0.5f, silhouetteMin.y - 31.0f), ImGui::GetColorU32(ImVec4(0.900f, 0.895f, 0.940f, 1.0f)), name);
    }

    if (state.distanceText)
    {
        const char* distance = "42m";
        drawList->AddText(ImVec2(silhouetteMax.x + 9.0f, sceneCenter.y + 26.0f), ImGui::GetColorU32(kMutedText), distance);
    }

    drawList->AddRect(previewMin, previewMax, ImGui::GetColorU32(kSectionBorder), 8.0f, 0, 1.0f);
    drawList->PopClipRect();
}

void AimTab()
{
    AimbotConfig& state = g_ConfigState.aimbot;
    constexpr const char* priorities[] = { "Closest", "Lowest HP", "Random" };
    constexpr const char* bones[] = { "Head", "Chest", "Auto" };

    SectionBegin("General");
    ToggleRow("Enabled", &state.enabled, "Would enable the mock aim-assist panel state.");
    ToggleRow("Draw FOV Circle", &state.drawFovCircle, "Would show a preview circle for the configured field of view.");
    KeybindDisplay("Aim Key", "MOUSE4", "Displays the mock activation key for documentation only.");
    SectionEnd();

    SectionBegin("Targeting");
    SliderFloatRow("FOV", &state.fov, 10.0f, 180.0f, "%.0f", "Would adjust the preview field-of-view radius.");
    SliderFloatRow("Smoothness", &state.smoothness, 1.0f, 20.0f, "%.1f", "Would tune mock movement smoothing in the UI.");
    ComboRow("Target Priority", &state.targetPriority, priorities, 3, "Would choose how targets are prioritized in a real implementation.");
    ComboRow("Aim Bone", &state.aimBone, bones, 3, "Would choose a preferred mock target bone.");
    SectionEnd();
}

void VisualsTab()
{
    VisualsConfig& state = g_ConfigState.visuals;

    SectionBegin("Overlays");
    const float overlayWidth = ImGui::GetContentRegionAvail().x;
    constexpr float previewWidth = 200.0f;
    constexpr float columnGap = 18.0f;
    if (overlayWidth >= previewWidth + 230.0f)
    {
        const float controlsWidth = overlayWidth - previewWidth - columnGap;
        ImGui::Columns(2, "##VisualsPreviewColumns", false);
        ImGui::SetColumnWidth(0, controlsWidth);
    }

    ToggleRow("Box ESP", &state.boxEsp, "Would draw mock bounding boxes in the overlay preview.");
    ToggleRow("Skeleton ESP", &state.skeletonEsp, "Would draw mock skeleton lines in a real visual overlay.");
    ToggleRow("Health Bar", &state.healthBar, "Would display a mock health bar beside preview targets.");
    ToggleRow("Name Tags", &state.nameTags, "Would display mock player names for UI demonstration.");
    ToggleRow("Distance Text", &state.distanceText, "Would show mock distance labels in the overlay.");
    ToggleRow("Snaplines", &state.snaplines, "Would draw mock lines from the screen edge to preview targets.");
    ToggleRow("Visibility Check", &state.visibilityCheck, "Would color mock visuals based on visibility state.");

    if (overlayWidth >= previewWidth + 230.0f)
    {
        ImGui::NextColumn();
        if (g_FontSmall != nullptr)
        {
            ImGui::PushFont(g_FontSmall);
        }
        ImGui::TextColored(kMutedText, "Live Preview");
        if (g_FontSmall != nullptr)
        {
            ImGui::PopFont();
        }
        ImGui::Spacing();
        DrawVisualsLivePreview(state);
        ImGui::Columns(1);
    }
    else
    {
        ImGui::Spacing();
        if (g_FontSmall != nullptr)
        {
            ImGui::PushFont(g_FontSmall);
        }
        ImGui::TextColored(kMutedText, "Live Preview");
        if (g_FontSmall != nullptr)
        {
            ImGui::PopFont();
        }
        ImGui::Spacing();
        DrawVisualsLivePreview(state);
    }
    SectionEnd();

    SectionBegin("Materials");
    ColorRow("Chams", state.chamsColor.data(), "Would configure the mock chams material color.");
    ColorRow("Glow", state.glowColor.data(), "Would configure the mock glow effect color.");
    SectionEnd();
}

void MiscTab()
{
    MiscConfig& state = g_ConfigState.misc;

    SectionBegin("Automation");
    ToggleRow("Auto Accept", &state.autoAccept, "Would automatically accept a matchmaking prompt in a real client.");
    SectionEnd();

    SectionBegin("Status");
    ImGui::TextUnformatted("Session Timer: 00:12:34");
    Tooltip("Static mock session timer for the portfolio UI.");
    ImGui::TextUnformatted("Build Info: EazyE HEX mock v0.4.0");
    Tooltip("Static build label for documentation.");
    SectionEnd();
}

void ConfigTab()
{
    RenderAppearanceControls();
    RenderHotkeyControls();

    SectionBegin("Profiles");
    SaveConfigButton();
    ImGui::SameLine();
    LoadConfigButton();
    ImGui::SameLine();
    DeleteConfigButton();
    PopupButton("Import", "Import Config Popup", "Config import preview only. No file I/O was performed.", "Settings imported", ToastType::Success);
    ImGui::SameLine();
    PopupButton("Export", "Export Config Popup", "Config export preview only. No file I/O was performed.", "Settings exported", ToastType::Success);
    SectionEnd();
}

void RenderSkinsTab()
{
    SkinsConfig& state = g_ConfigState.skins;
    constexpr const char* weaponSkins[] = { "Default", "Crimson", "Graphite", "Neon" };
    constexpr const char* knifeSkins[] = { "Default", "Karambit", "Bayonet", "Butterfly" };
    constexpr const char* gloveSkins[] = { "Default", "Sport", "Driver", "Moto" };

    SectionBegin("Inventory");
    ComboRow("Weapon Skin Changer", &state.weaponSkin, weaponSkins, 4, "Would select a mock weapon finish preset.");
    ComboRow("Knife Changer", &state.knifeSkin, knifeSkins, 4, "Would select a mock knife model preset.");
    ComboRow("Glove Changer", &state.gloveSkin, gloveSkins, 4, "Would select a mock glove model preset.");
    SectionEnd();
}

void RenderWorldTab()
{
    WorldConfig& state = g_ConfigState.world;

    SectionBegin("Environment");
    ToggleRow("Fullbright", &state.fullbright, "Would brighten the mock environment preview.");
    ToggleRow("No Fog", &state.noFog, "Would remove mock fog effects from the preview.");
    ToggleRow("Wireframe Mode", &state.wireframeMode, "Would display mock world geometry as wireframes.");
    ToggleRow("Remove Grass", &state.removeGrass, "Would hide mock foliage in a real visual module.");
    ColorRow("Custom Sky", state.skyColor.data(), "Would choose the mock sky tint.");
    SectionEnd();
}

void RenderMovementTab()
{
    MovementConfig& state = g_ConfigState.movement;

    SectionBegin("Movement");
    ToggleRow("Bunny Hop", &state.bunnyHop, "UI-only placeholder for automated jump timing.");
    ToggleRow("Auto Strafe", &state.autoStrafe, "UI-only placeholder for air-strafe assistance.");
    SliderFloatRow("Speed Multiplier", &state.speedMultiplier, 0.5f, 3.0f, "%.1fx", "Would scale a mock movement speed value.");
    ToggleRow("No Fall Damage", &state.noFallDamage, "UI-only placeholder for fall-damage prevention.");
    SectionEnd();
}

void RenderPlayerTab()
{
    PlayerConfig& state = g_ConfigState.player;

    SectionBegin("Camera");
    ToggleRow("Third Person Toggle", &state.thirdPersonToggle, "Would switch to a mock third-person camera preview.");
    SliderFloatRow("Custom FOV", &state.customFov, 60.0f, 140.0f, "%.0f", "Would adjust the mock player camera field of view.");
    ToggleRow("Anti-AFK", &state.antiAfk, "UI-only placeholder for anti-idle behavior.");
    SectionEnd();

    SectionBegin("Spectators");
    ImGui::TextColored(kMutedText, "Static placeholder list");
    ImGui::BulletText("northstar");
    ImGui::BulletText("violet");
    ImGui::BulletText("delta");
    Tooltip("Displays static mock spectator names only.");
    SectionEnd();
}

void RenderCategoryContent(Category selected)
{
    switch (selected)
    {
    case Category::Aimbot:
        AimTab();
        break;
    case Category::Visuals:
        VisualsTab();
        break;
    case Category::Skins:
        RenderSkinsTab();
        break;
    case Category::World:
        RenderWorldTab();
        break;
    case Category::Movement:
        RenderMovementTab();
        break;
    case Category::Player:
        RenderPlayerTab();
        break;
    case Category::Misc:
        MiscTab();
        break;
    case Category::Config:
        ConfigTab();
        break;
    default:
        break;
    }
}

void RenderMainPanelTabs()
{
    ApplyAccentThemeColors();

    static Category selected = Category::Aimbot;
    static Category previousSelected = Category::Aimbot;
    static double categorySwitchStart = -1.0;

    const ImVec2 panelPos = ImGui::GetCursorScreenPos();
    const ImVec2 panelSize = ImGui::GetContentRegionAvail();
    constexpr float topAccentBandHeight = 28.0f;
    const float bodyHeight = panelSize.y > topAccentBandHeight ? panelSize.y - topAccentBandHeight : panelSize.y;
    const ImVec2 bodyPos(panelPos.x, panelPos.y + topAccentBandHeight);
    const ImVec2 bodySize(panelSize.x, bodyHeight);
    DrawLayeredPanelShadow(
        ImGui::GetWindowDrawList(),
        panelPos,
        ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
        kOuterWindowRounding);
    DrawAcrylicPanelSurface(
        ImGui::GetWindowDrawList(),
        panelPos,
        ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
        kOuterWindowRounding);
    DrawPremiumTopStrip(
        ImGui::GetWindowDrawList(),
        panelPos,
        ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y));

    ImGui::SetCursorScreenPos(bodyPos);
    const Category selectedBeforeSidebar = selected;
    RenderSidebar(&selected);
    if (selected != selectedBeforeSidebar)
    {
        previousSelected = selectedBeforeSidebar;
        categorySwitchStart = ImGui::GetTime();
    }
    ImGui::SameLine();

    const ImVec2 contentPos = ImGui::GetCursorScreenPos();
    const ImVec2 contentSize = ImGui::GetContentRegionAvail();
    DrawSidebarSeparation(ImGui::GetWindowDrawList(), ImVec2(contentPos.x - ImGui::GetStyle().ItemSpacing.x * 0.5f, bodyPos.y), bodySize.y);
    DrawSoftShadow(
        ImGui::GetWindowDrawList(),
        contentPos,
        ImVec2(contentPos.x + contentSize.x, contentPos.y + contentSize.y),
        kSectionRounding);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.055f, 0.064f, 0.86f));
    ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), true);
    DrawContentBackgroundDepth(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y));
    DrawAnimatedGradientMesh(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        0.75f);
    DrawAnimatedTechPattern(
        ImGui::GetWindowDrawList(),
        ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        0.85f);
    RenderLiveStatsBar();
    if (g_FontSection != nullptr)
    {
        ImGui::PushFont(g_FontSection);
    }
    ImGui::TextColored(g_AccentColor, "%s", CategoryTitle(selected));
    if (g_FontSection != nullptr)
    {
        ImGui::PopFont();
    }
    ImGui::SameLine();
    if (g_FontSmall != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
    }
    ImGui::TextColored(kMutedText, "  Settings");
    if (g_FontSmall != nullptr)
    {
        ImGui::PopFont();
    }
    ImGui::Spacing();

    const double now = ImGui::GetTime();
    const float transitionProgress = categorySwitchStart >= 0.0
        ? Clamp01(static_cast<float>((now - categorySwitchStart) / 0.15))
        : 1.0f;
    if (transitionProgress >= 1.0f)
    {
        previousSelected = selected;
        categorySwitchStart = -1.0;
    }

    const bool transitionActive = transitionProgress < 1.0f;
    const bool showPrevious = transitionActive && transitionProgress < 0.5f;
    const float halfProgress = showPrevious
        ? EaseOutCubic(transitionProgress * 2.0f)
        : EaseOutCubic((transitionProgress - 0.5f) * 2.0f);
    const float contentAlpha = transitionActive
        ? (showPrevious ? 1.0f - halfProgress : halfProgress)
        : 1.0f;
    const float slideOffset = transitionActive
        ? (showPrevious ? -14.0f * halfProgress : 14.0f * (1.0f - halfProgress))
        : 0.0f;
    const Category contentCategory = showPrevious ? previousSelected : selected;
    const ImVec2 contentCursor = ImGui::GetCursorPos();
    ImGui::SetCursorPosX(contentCursor.x + slideOffset);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * contentAlpha);
    if (transitionActive)
    {
        ImGui::BeginDisabled(true);
    }
    RenderCategoryContent(contentCategory);
    if (transitionActive)
    {
        ImGui::EndDisabled();
    }
    ImGui::PopStyleVar();

    ImGui::EndChild();
    ImGui::PopStyleColor();
    RenderToastStack(panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y));
}
