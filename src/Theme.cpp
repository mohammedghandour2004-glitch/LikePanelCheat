#include "Theme.h"

#include <windows.h>

#include <cstdio>

#include "ConfigManager.h"
#include "imgui_internal.h"

namespace
{
constexpr ImVec4 kBackground = ImVec4(0.051f, 0.051f, 0.059f, 1.0f); // #0D0D0F
constexpr ImVec4 kPanel = ImVec4(0.075f, 0.075f, 0.086f, 1.0f);
constexpr ImVec4 kPanelHover = ImVec4(0.105f, 0.105f, 0.125f, 1.0f);
constexpr ImVec4 kText = ImVec4(0.925f, 0.925f, 0.950f, 1.0f);
constexpr ImVec4 kTextMuted = ImVec4(0.580f, 0.580f, 0.640f, 1.0f);
constexpr float kAnimFast = 0.10f;
constexpr float kAnimMedium = 0.20f;
constexpr float kAnimStandard = 0.25f;

bool FileExists(const char* path)
{
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool LoadFontIfExists(ImGuiIO& io, const char* path, float size)
{
    if (!FileExists(path))
    {
        return false;
    }

    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 2;
    config.PixelSnapH = true;
    return io.Fonts->AddFontFromFileTTF(path, size, &config) != nullptr;
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

float EaseInOutCubic(float value)
{
    const float t = Clamp01(value);
    return t < 0.5f
        ? 4.0f * t * t * t
        : 1.0f - ImPow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}

float Approach(float current, float target, float responseSeconds)
{
    const float dt = ImGui::GetIO().DeltaTime;
    const float speed = g_ConfigState.panel.animationSpeed < 0.5f ? 0.5f : (g_ConfigState.panel.animationSpeed > 2.0f ? 2.0f : g_ConfigState.panel.animationSpeed);
    const float adjustedResponse = responseSeconds / speed;
    const float step = adjustedResponse > 0.0f ? EaseOutCubic(dt / adjustedResponse) : 1.0f;
    return current + (target - current) * step;
}
}

ImVec4 g_AccentColor = ImVec4(0.486f, 0.227f, 0.929f, 1.0f); // #7C3AED
ImFont* g_FontRegular = nullptr;
ImFont* g_FontSection = nullptr;
ImFont* g_FontSmall = nullptr;

ImVec4 GetAccentHoverColor()
{
    return ImVec4(
        ImMin(g_AccentColor.x + 0.095f, 1.0f),
        ImMin(g_AccentColor.y + 0.118f, 1.0f),
        ImMin(g_AccentColor.z + 0.036f, 1.0f),
        g_AccentColor.w);
}

void ApplyAccentThemeColors()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    const ImVec4 accentHover = GetAccentHoverColor();

    colors[ImGuiCol_ScrollbarGrabActive] = g_AccentColor;
    colors[ImGuiCol_CheckMark] = g_AccentColor;
    colors[ImGuiCol_SliderGrab] = g_AccentColor;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_Button] = ImVec4(g_AccentColor.x * 0.32f, g_AccentColor.y * 0.32f, g_AccentColor.z * 0.32f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(g_AccentColor.x * 0.48f, g_AccentColor.y * 0.48f, g_AccentColor.z * 0.48f, 1.0f);
    colors[ImGuiCol_ButtonActive] = g_AccentColor;
    colors[ImGuiCol_Header] = ImVec4(g_AccentColor.x * 0.34f, g_AccentColor.y * 0.34f, g_AccentColor.z * 0.34f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(g_AccentColor.x * 0.52f, g_AccentColor.y * 0.52f, g_AccentColor.z * 0.52f, 1.0f);
    colors[ImGuiCol_HeaderActive] = g_AccentColor;
    colors[ImGuiCol_SeparatorHovered] = accentHover;
    colors[ImGuiCol_SeparatorActive] = g_AccentColor;
    colors[ImGuiCol_ResizeGrip] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.250f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.600f);
    colors[ImGuiCol_ResizeGripActive] = g_AccentColor;
    colors[ImGuiCol_TabHovered] = ImVec4(g_AccentColor.x * 0.52f, g_AccentColor.y * 0.52f, g_AccentColor.z * 0.52f, 1.0f);
#ifdef EAZYE_IMGUI_HAS_DOCKING
    colors[ImGuiCol_DockingPreview] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.700f);
#endif
    colors[ImGuiCol_PlotLines] = g_AccentColor;
    colors[ImGuiCol_PlotLinesHovered] = accentHover;
    colors[ImGuiCol_PlotHistogram] = g_AccentColor;
    colors[ImGuiCol_PlotHistogramHovered] = accentHover;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.350f);
    colors[ImGuiCol_DragDropTarget] = accentHover;
    colors[ImGuiCol_NavHighlight] = g_AccentColor;
}

void ApplyTheme()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.Fonts->Fonts.empty())
    {
        char windowsDirectory[MAX_PATH] = {};
        char segoePath[MAX_PATH] = {};
        char segoeBoldPath[MAX_PATH] = {};

        const bool hasWindowsDirectory = GetWindowsDirectoryA(windowsDirectory, MAX_PATH) > 0;
        if (hasWindowsDirectory)
        {
            std::snprintf(segoePath, sizeof(segoePath), "%s\\Fonts\\segoeui.ttf", windowsDirectory);
            std::snprintf(segoeBoldPath, sizeof(segoeBoldPath), "%s\\Fonts\\segoeuib.ttf", windowsDirectory);
        }

        g_FontRegular = LoadFontIfExists(io, "assets/fonts/Inter-Regular.ttf", 15.0f)
            ? io.Fonts->Fonts.back()
            : nullptr;
        if (g_FontRegular == nullptr && hasWindowsDirectory && LoadFontIfExists(io, segoePath, 15.0f))
        {
            g_FontRegular = io.Fonts->Fonts.back();
        }

        if (hasWindowsDirectory && LoadFontIfExists(io, segoeBoldPath, 16.0f))
        {
            g_FontSection = io.Fonts->Fonts.back();
        }
        if (hasWindowsDirectory && LoadFontIfExists(io, segoePath, 13.0f))
        {
            g_FontSmall = io.Fonts->Fonts.back();
        }

        if (g_FontRegular == nullptr)
        {
            g_FontRegular = io.Fonts->AddFontDefault();
        }
        if (g_FontSection == nullptr)
        {
            g_FontSection = g_FontRegular;
        }
        if (g_FontSmall == nullptr)
        {
            g_FontSmall = g_FontRegular;
        }
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(14.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize = 12.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = kText;
    colors[ImGuiCol_TextDisabled] = kTextMuted;
    colors[ImGuiCol_WindowBg] = kBackground;
    colors[ImGuiCol_ChildBg] = kBackground;
    colors[ImGuiCol_PopupBg] = kPanel;
    colors[ImGuiCol_Border] = ImVec4(0.170f, 0.150f, 0.220f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = kPanel;
    colors[ImGuiCol_FrameBgHovered] = kPanelHover;
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.150f, 0.105f, 0.240f, 1.0f);
    colors[ImGuiCol_TitleBg] = kBackground;
    colors[ImGuiCol_TitleBgActive] = kBackground;
    colors[ImGuiCol_TitleBgCollapsed] = kBackground;
    colors[ImGuiCol_MenuBarBg] = kPanel;
    colors[ImGuiCol_ScrollbarBg] = kBackground;
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.220f, 0.200f, 0.280f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.250f, 0.410f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = g_AccentColor;
    colors[ImGuiCol_CheckMark] = g_AccentColor;
    colors[ImGuiCol_SliderGrab] = g_AccentColor;
    colors[ImGuiCol_SliderGrabActive] = GetAccentHoverColor();
    colors[ImGuiCol_Button] = ImVec4(0.155f, 0.110f, 0.245f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.220f, 0.150f, 0.360f, 1.0f);
    colors[ImGuiCol_ButtonActive] = g_AccentColor;
    colors[ImGuiCol_Header] = ImVec4(0.160f, 0.110f, 0.260f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.240f, 0.160f, 0.410f, 1.0f);
    colors[ImGuiCol_HeaderActive] = g_AccentColor;
    colors[ImGuiCol_Separator] = ImVec4(0.190f, 0.165f, 0.250f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = GetAccentHoverColor();
    colors[ImGuiCol_SeparatorActive] = g_AccentColor;
    colors[ImGuiCol_ResizeGrip] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.250f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.600f);
    colors[ImGuiCol_ResizeGripActive] = g_AccentColor;
    colors[ImGuiCol_Tab] = kPanel;
    colors[ImGuiCol_TabHovered] = ImVec4(0.240f, 0.160f, 0.410f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.155f, 0.110f, 0.245f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = kPanel;
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.120f, 0.095f, 0.170f, 1.0f);
#ifdef EAZYE_IMGUI_HAS_DOCKING
    colors[ImGuiCol_DockingPreview] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.700f);
    colors[ImGuiCol_DockingEmptyBg] = kBackground;
#endif
    colors[ImGuiCol_PlotLines] = g_AccentColor;
    colors[ImGuiCol_PlotLinesHovered] = GetAccentHoverColor();
    colors[ImGuiCol_PlotHistogram] = g_AccentColor;
    colors[ImGuiCol_PlotHistogramHovered] = GetAccentHoverColor();
    colors[ImGuiCol_TableHeaderBg] = kPanel;
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.190f, 0.165f, 0.250f, 1.0f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.130f, 0.115f, 0.160f, 1.0f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.025f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.350f);
    colors[ImGuiCol_DragDropTarget] = GetAccentHoverColor();
    colors[ImGuiCol_NavHighlight] = g_AccentColor;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.650f);
    ApplyAccentThemeColors();
}

bool ToggleSwitch(const char* label, bool* value)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float height = ImGui::GetFrameHeight();
    const float width = height * 1.85f;
    const float radius = height * 0.50f;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect switchRect(pos, ImVec2(pos.x + width, pos.y + height));
    const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
    const ImRect totalRect(
        pos,
        ImVec2(pos.x + width + (labelSize.x > 0.0f ? style.ItemInnerSpacing.x + labelSize.x : 0.0f),
               pos.y + ImMax(height, labelSize.y)));

    ImGui::ItemSize(totalRect, style.FramePadding.y);
    if (!ImGui::ItemAdd(totalRect, id))
    {
        return false;
    }

    bool hovered = false;
    bool held = false;
    const bool pressed = ImGui::ButtonBehavior(totalRect, id, &hovered, &held);
    if (pressed)
    {
        *value = !*value;
        if (*value)
        {
            window->StateStorage.SetFloat(id + 3, static_cast<float>(ImGui::GetTime()));
        }
        ImGui::MarkItemEdited(id);
    }

    float* hoverT = window->StateStorage.GetFloatRef(id + 4, 0.0f);
    float* pressT = window->StateStorage.GetFloatRef(id + 5, 0.0f);
    *hoverT = Approach(*hoverT, hovered || held ? 1.0f : 0.0f, kAnimFast);
    *pressT = Approach(*pressT, held ? 1.0f : 0.0f, held ? kAnimFast * 0.35f : kAnimFast);

    float* knobT = window->StateStorage.GetFloatRef(id + 1, *value ? 1.0f : 0.0f);
    *knobT = Approach(*knobT, *value ? 1.0f : 0.0f, kAnimMedium);
    const float t = *knobT;
    const float drawScale = 1.0f + *hoverT * 0.020f - *pressT * 0.050f;
    const ImVec2 switchCenter((switchRect.Min.x + switchRect.Max.x) * 0.5f, (switchRect.Min.y + switchRect.Max.y) * 0.5f);
    const ImVec2 switchHalf((switchRect.Max.x - switchRect.Min.x) * 0.5f * drawScale, (switchRect.Max.y - switchRect.Min.y) * 0.5f * drawScale);
    const ImRect drawSwitchRect(
        ImVec2(switchCenter.x - switchHalf.x, switchCenter.y - switchHalf.y),
        ImVec2(switchCenter.x + switchHalf.x, switchCenter.y + switchHalf.y));
    const float drawRadius = radius * drawScale;

    const ImU32 trackColor = ImGui::GetColorU32(*value
        ? (hovered ? GetAccentHoverColor() : g_AccentColor)
        : (hovered ? ImVec4(0.240f, 0.240f, 0.280f, 1.0f) : ImVec4(0.160f, 0.160f, 0.190f, 1.0f)));
    const ImU32 knobColor = ImGui::GetColorU32(ImVec4(0.940f, 0.940f, 0.970f, 1.0f));

    if (*hoverT > 0.001f || *pressT > 0.001f)
    {
        window->DrawList->AddRect(
            ImVec2(drawSwitchRect.Min.x - 2.0f, drawSwitchRect.Min.y - 2.0f),
            ImVec2(drawSwitchRect.Max.x + 2.0f, drawSwitchRect.Max.y + 2.0f),
            ImGui::GetColorU32(ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, *hoverT * 0.24f + *pressT * 0.12f)),
            drawRadius + 2.0f,
            0,
            2.0f);
        window->DrawList->AddRect(
            ImVec2(drawSwitchRect.Min.x - 5.0f, drawSwitchRect.Min.y - 5.0f),
            ImVec2(drawSwitchRect.Max.x + 5.0f, drawSwitchRect.Max.y + 5.0f),
            ImGui::GetColorU32(ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, *hoverT * 0.09f)),
            drawRadius + 5.0f,
            0,
            1.0f);
    }

    window->DrawList->AddRectFilled(drawSwitchRect.Min, drawSwitchRect.Max, trackColor, drawRadius);
    if (*value)
    {
        window->DrawList->AddRect(
            ImVec2(drawSwitchRect.Min.x + 1.0f, drawSwitchRect.Min.y + 1.0f),
            ImVec2(drawSwitchRect.Max.x - 1.0f, drawSwitchRect.Max.y - 1.0f),
            ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.20f)),
            drawRadius,
            0,
            1.0f);
        window->DrawList->AddRect(
            ImVec2(drawSwitchRect.Min.x + 3.0f, drawSwitchRect.Min.y + 3.0f),
            ImVec2(drawSwitchRect.Max.x - 3.0f, drawSwitchRect.Max.y - 3.0f),
            ImGui::GetColorU32(ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.30f)),
            drawRadius - 2.0f,
            0,
            1.0f);
    }

    const float knobRadius = drawRadius - 3.0f;
    const float knobX = ImLerp(drawSwitchRect.Min.x + drawRadius, drawSwitchRect.Max.x - drawRadius, t);
    const ImVec2 knobCenter(knobX, drawSwitchRect.Min.y + drawRadius);
    const float burstStart = window->StateStorage.GetFloat(id + 3, -10.0f);
    const float burstAge = static_cast<float>(ImGui::GetTime()) - burstStart;
    if (burstAge >= 0.0f && burstAge < kAnimStandard)
    {
        const float burstT = EaseInOutCubic(burstAge / kAnimStandard);
        const float burstAlpha = (1.0f - burstT) * 0.34f;
        window->DrawList->AddCircleFilled(
            knobCenter,
            knobRadius + 8.0f + 14.0f * EaseOutCubic(burstT),
            ImGui::GetColorU32(ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, burstAlpha)),
            40);
    }
    if (hovered || held || *value)
    {
        const float glowAlpha = held ? 0.30f : (hovered ? 0.22f : 0.14f);
        window->DrawList->AddCircleFilled(knobCenter, knobRadius + 5.0f, ImGui::GetColorU32(ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, glowAlpha)), 32);
    }
    window->DrawList->AddCircleFilled(knobCenter, knobRadius, knobColor, 32);

    if (labelSize.x > 0.0f)
    {
        ImGui::RenderText(
            ImVec2(switchRect.Max.x + style.ItemInnerSpacing.x,
                   switchRect.Min.y + (height - labelSize.y) * 0.5f),
            label);
    }

    return pressed;
}

void RenderEazyEWatermark()
{
    ImGui::SetCursorPos(ImVec2(14.0f, 10.0f));
    ImGui::TextColored(g_AccentColor, "EazyE HEX");
    ImGui::Spacing();
}
