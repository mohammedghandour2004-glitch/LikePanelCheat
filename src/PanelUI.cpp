#include "PanelUI.h"

#include "Theme.h"
#include "imgui.h"

#include <cmath>
#include <cstdio>

namespace
{
constexpr float kSidebarWidth = 180.0f;
constexpr ImVec4 kSidebarBg = ImVec4(0.060f, 0.060f, 0.070f, 1.0f);
constexpr ImVec4 kSectionBg = ImVec4(0.075f, 0.075f, 0.086f, 1.0f);
constexpr ImVec4 kSectionBorder = ImVec4(0.170f, 0.150f, 0.220f, 1.0f);
constexpr ImVec4 kMutedText = ImVec4(0.580f, 0.580f, 0.640f, 1.0f);

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

struct CategoryItem
{
    Category id;
    const char* label;
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

ImU32 AccentU32(float alpha = 1.0f)
{
    return ImGui::GetColorU32(WithAlpha(g_AccentColor, alpha));
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
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##gradient-button", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const float target = hovered || active ? 1.0f : 0.0f;
    const float speed = 8.5f * ImGui::GetIO().DeltaTime;
    *hoverT += (target - *hoverT) * (speed > 1.0f ? 1.0f : speed);

    const float brighten = 1.0f + (*hoverT * 0.15f) + (active ? 0.08f : 0.0f);
    const ImVec4 top = ScaleColor(g_AccentColor, 0.46f * brighten);
    const ImVec4 bottom = ScaleColor(g_AccentColor, 0.26f * brighten);
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilledMultiColor(
        pos,
        max,
        ImGui::GetColorU32(top),
        ImGui::GetColorU32(top),
        ImGui::GetColorU32(bottom),
        ImGui::GetColorU32(bottom));
    drawList->AddRect(pos, max, ImGui::GetColorU32(GetAccentHoverColor()), 6.0f, 0, 1.0f);
    if (hovered || active)
    {
        drawList->AddRect(ImVec2(pos.x - 1.0f, pos.y - 1.0f), ImVec2(max.x + 1.0f, max.y + 1.0f), AccentU32(0.24f), 7.0f, 0, 2.0f);
    }

    const ImVec2 textSize = ImGui::CalcTextSize(label);
    drawList->AddText(
        ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f),
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

void SliderFloatRow(const char* label, float* value, float minValue, float maxValue, const char* format, const char* tooltip)
{
    ImGui::SliderFloat(label, value, minValue, maxValue, format);
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const float ratio = (*value - minValue) / (maxValue - minValue);
        const float knobX = min.x + (max.x - min.x) * (ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio));
        const ImVec2 center(knobX, (min.y + max.y) * 0.5f);
        ImGui::GetWindowDrawList()->AddCircleFilled(center, 7.0f, AccentU32(ImGui::IsItemActive() ? 0.28f : 0.18f), 24);
    }
    Tooltip(tooltip);
}

void ColorRow(const char* label, float color[4], const char* tooltip)
{
    ImGui::ColorEdit4(label, color, ImGuiColorEditFlags_NoInputs);
    Tooltip(tooltip);
}

void ComboRow(const char* label, int* selected, const char* const* items, int itemCount, const char* tooltip)
{
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::BeginCombo(label, items[*selected]))
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
    Tooltip(tooltip);
}

void KeybindDisplay(const char* label, const char* keyName, const char* tooltip)
{
    ImGui::TextUnformatted(label);
    ImGui::SameLine(170.0f);
    ImGui::Button(keyName, ImVec2(96.0f, 0.0f));
    Tooltip(tooltip);
}

void SectionBegin(const char* title)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, kSectionBg);
    ImGui::PushStyleColor(ImGuiCol_Border, kSectionBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
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
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::Spacing();
}

void DrawSidebarIcon(ImDrawList* drawList, Category category, ImVec2 center, ImU32 color)
{
    switch (category)
    {
    case Category::Aimbot:
        drawList->AddCircle(center, 6.2f, color, 24, 1.8f);
        drawList->AddCircleFilled(center, 1.7f, color, 12);
        drawList->AddLine(ImVec2(center.x - 9.0f, center.y), ImVec2(center.x - 4.5f, center.y), color, 1.4f);
        drawList->AddLine(ImVec2(center.x + 4.5f, center.y), ImVec2(center.x + 9.0f, center.y), color, 1.4f);
        drawList->AddLine(ImVec2(center.x, center.y - 9.0f), ImVec2(center.x, center.y - 4.5f), color, 1.4f);
        drawList->AddLine(ImVec2(center.x, center.y + 4.5f), ImVec2(center.x, center.y + 9.0f), color, 1.4f);
        break;
    case Category::Visuals:
        drawList->AddBezierCubic(ImVec2(center.x - 9.0f, center.y), ImVec2(center.x - 4.5f, center.y - 6.0f), ImVec2(center.x + 4.5f, center.y - 6.0f), ImVec2(center.x + 9.0f, center.y), color, 1.6f);
        drawList->AddBezierCubic(ImVec2(center.x - 9.0f, center.y), ImVec2(center.x - 4.5f, center.y + 6.0f), ImVec2(center.x + 4.5f, center.y + 6.0f), ImVec2(center.x + 9.0f, center.y), color, 1.6f);
        drawList->AddCircleFilled(center, 3.0f, color, 16);
        break;
    case Category::Skins:
        drawList->AddRect(ImVec2(center.x - 7.0f, center.y - 7.0f), ImVec2(center.x + 7.0f, center.y + 7.0f), color, 3.0f, 0, 1.5f);
        drawList->AddLine(ImVec2(center.x - 7.0f, center.y - 1.0f), ImVec2(center.x + 7.0f, center.y - 1.0f), color, 1.5f);
        drawList->AddLine(ImVec2(center.x - 1.0f, center.y - 7.0f), ImVec2(center.x - 1.0f, center.y + 7.0f), color, 1.5f);
        break;
    case Category::World:
        drawList->AddCircle(center, 7.0f, color, 24, 1.6f);
        drawList->AddLine(ImVec2(center.x - 7.0f, center.y), ImVec2(center.x + 7.0f, center.y), color, 1.2f);
        drawList->AddBezierCubic(ImVec2(center.x, center.y - 7.0f), ImVec2(center.x - 4.0f, center.y - 2.0f), ImVec2(center.x - 4.0f, center.y + 2.0f), ImVec2(center.x, center.y + 7.0f), color, 1.2f);
        drawList->AddBezierCubic(ImVec2(center.x, center.y - 7.0f), ImVec2(center.x + 4.0f, center.y - 2.0f), ImVec2(center.x + 4.0f, center.y + 2.0f), ImVec2(center.x, center.y + 7.0f), color, 1.2f);
        break;
    case Category::Movement:
        drawList->AddTriangleFilled(ImVec2(center.x - 6.5f, center.y - 7.0f), ImVec2(center.x - 6.5f, center.y + 7.0f), ImVec2(center.x + 8.0f, center.y), color);
        break;
    case Category::Player:
        drawList->AddCircle(center, 7.0f, color, 24, 1.5f);
        drawList->AddCircleFilled(ImVec2(center.x, center.y - 2.5f), 2.4f, color, 12);
        drawList->AddBezierCubic(ImVec2(center.x - 5.0f, center.y + 5.0f), ImVec2(center.x - 2.5f, center.y + 1.5f), ImVec2(center.x + 2.5f, center.y + 1.5f), ImVec2(center.x + 5.0f, center.y + 5.0f), color, 1.5f);
        break;
    case Category::Misc:
        drawList->AddCircleFilled(ImVec2(center.x - 5.0f, center.y), 2.0f, color, 12);
        drawList->AddCircleFilled(center, 2.0f, color, 12);
        drawList->AddCircleFilled(ImVec2(center.x + 5.0f, center.y), 2.0f, color, 12);
        break;
    case Category::Config:
        drawList->AddCircle(center, 5.0f, color, 18, 1.6f);
        for (int i = 0; i < 8; ++i)
        {
            const float angle = (3.14159265f * 2.0f * static_cast<float>(i)) / 8.0f;
            const ImVec2 inner(center.x + cosf(angle) * 7.0f, center.y + sinf(angle) * 7.0f);
            const ImVec2 outer(center.x + cosf(angle) * 9.0f, center.y + sinf(angle) * 9.0f);
            drawList->AddLine(inner, outer, color, 1.3f);
        }
        break;
    default:
        drawList->AddCircleFilled(center, 4.0f, color, 16);
        break;
    }
}

void SidebarButton(const CategoryItem& item, Category* selected)
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
    const ImU32 textColor = ImGui::GetColorU32(isSelected ? g_AccentColor : (hovered ? ImVec4(0.780f, 0.760f, 0.840f, 1.0f) : kMutedText));
    const ImU32 iconColor = ImGui::GetColorU32(isSelected ? g_AccentColor : (hovered ? ImVec4(0.580f, 0.560f, 0.640f, 1.0f) : ImVec4(0.360f, 0.340f, 0.420f, 1.0f)));
    const ImVec2 rowMax(pos.x + rowSize.x, pos.y + rowSize.y);

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
    }
    else if (hovered)
    {
        drawList->AddRectFilled(pos, rowMax, ImGui::GetColorU32(ImVec4(0.105f, 0.105f, 0.125f, 1.0f)), 7.0f);
    }

    if (isSelected)
    {
        drawList->AddRectFilled(pos, ImVec2(pos.x + 4.0f, pos.y + rowSize.y), ImGui::GetColorU32(g_AccentColor), 2.0f);
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

void RenderSidebar(Category* selected)
{
    constexpr float headerHeight = 72.0f;
    constexpr float footerHeight = 82.0f;
    static char profileName[32] = "Default";

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kSidebarBg);
    DrawSoftShadow(
        ImGui::GetWindowDrawList(),
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + kSidebarWidth, ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y),
        8.0f);
    ImGui::BeginChild("Sidebar", ImVec2(kSidebarWidth, 0.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::BeginChild("SidebarHeader", ImVec2(0.0f, headerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderBrandHeader();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    GradientSeparator();
    ImGui::EndChild();

    const float sidebarRemainingHeight = ImGui::GetContentRegionAvail().y;
    const float middleHeight = sidebarRemainingHeight > footerHeight
        ? sidebarRemainingHeight - footerHeight
        : 0.0f;

    ImGui::BeginChild("SidebarCategories", ImVec2(0.0f, middleHeight), false);
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    for (const CategoryItem& item : kCategories)
    {
        SidebarButton(item, selected);
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
    }

    ImGui::EndChild();

    ImGui::BeginChild("SidebarFooter", ImVec2(0.0f, footerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    GradientSeparator();
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
    ImGui::InputText("##ProfileName", profileName, sizeof(profileName));
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
    if (g_FontSmall != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
    }
    ImGui::TextColored(kMutedText, "FPS: 240");
    if (g_FontSmall != nullptr)
    {
        ImGui::PopFont();
    }
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void RenderSearchBar()
{
    static char searchText[128] = {};
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##SearchSettings", "Search settings...", searchText, sizeof(searchText));
    ImGui::Spacing();
}

void PopupButton(const char* label, const char* popupId, const char* popupText)
{
    if (GradientButton(label, ImVec2(118.0f, 28.0f)))
    {
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
}

void AimTab()
{
    static bool enabled = false;
    static bool drawFovCircle = true;
    static float fov = 90.0f;
    static float smoothness = 6.0f;
    static int targetPriority = 0;
    static int aimBone = 0;
    constexpr const char* priorities[] = { "Closest", "Lowest HP", "Random" };
    constexpr const char* bones[] = { "Head", "Chest", "Auto" };

    SectionBegin("General");
    ToggleRow("Enabled", &enabled, "Would enable the mock aim-assist panel state.");
    ToggleRow("Draw FOV Circle", &drawFovCircle, "Would show a preview circle for the configured field of view.");
    KeybindDisplay("Aim Key", "MOUSE4", "Displays the mock activation key for documentation only.");
    SectionEnd();

    SectionBegin("Targeting");
    SliderFloatRow("FOV", &fov, 10.0f, 180.0f, "%.0f", "Would adjust the preview field-of-view radius.");
    SliderFloatRow("Smoothness", &smoothness, 1.0f, 20.0f, "%.1f", "Would tune mock movement smoothing in the UI.");
    ComboRow("Target Priority", &targetPriority, priorities, 3, "Would choose how targets are prioritized in a real implementation.");
    ComboRow("Aim Bone", &aimBone, bones, 3, "Would choose a preferred mock target bone.");
    SectionEnd();
}

void VisualsTab()
{
    static bool boxEsp = true;
    static bool skeletonEsp = false;
    static bool healthBar = true;
    static bool nameTags = false;
    static bool distanceText = true;
    static bool snaplines = false;
    static bool visibilityCheck = true;
    static float chamsColor[4] = {0.320f, 0.520f, 0.950f, 1.0f};
    static float glowColor[4] = {0.180f, 0.800f, 0.443f, 1.0f};

    SectionBegin("Overlays");
    ToggleRow("Box ESP", &boxEsp, "Would draw mock bounding boxes in the overlay preview.");
    ToggleRow("Skeleton ESP", &skeletonEsp, "Would draw mock skeleton lines in a real visual overlay.");
    ToggleRow("Health Bar", &healthBar, "Would display a mock health bar beside preview targets.");
    ToggleRow("Name Tags", &nameTags, "Would display mock player names for UI demonstration.");
    ToggleRow("Distance Text", &distanceText, "Would show mock distance labels in the overlay.");
    ToggleRow("Snaplines", &snaplines, "Would draw mock lines from the screen edge to preview targets.");
    ToggleRow("Visibility Check", &visibilityCheck, "Would color mock visuals based on visibility state.");
    SectionEnd();

    SectionBegin("Materials");
    ColorRow("Chams", chamsColor, "Would configure the mock chams material color.");
    ColorRow("Glow", glowColor, "Would configure the mock glow effect color.");
    SectionEnd();
}

void MiscTab()
{
    static bool autoAccept = false;

    SectionBegin("Automation");
    ToggleRow("Auto Accept", &autoAccept, "Would automatically accept a matchmaking prompt in a real client.");
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

    SectionBegin("Profiles");
    PopupButton("Save Config", "Save Config Popup", "Config save preview only. No file I/O was performed.");
    ImGui::SameLine();
    PopupButton("Load Config", "Load Config Popup", "Config load preview only. No file I/O was performed.");
    ImGui::SameLine();
    PopupButton("Delete Config", "Delete Config Popup", "Config delete preview only. No file I/O was performed.");
    PopupButton("Import", "Import Config Popup", "Config import preview only. No file I/O was performed.");
    ImGui::SameLine();
    PopupButton("Export", "Export Config Popup", "Config export preview only. No file I/O was performed.");
    SectionEnd();
}

void RenderSkinsTab()
{
    static int weaponSkin = 0;
    static int knifeSkin = 0;
    static int gloveSkin = 0;
    constexpr const char* weaponSkins[] = { "Default", "Crimson", "Graphite", "Neon" };
    constexpr const char* knifeSkins[] = { "Default", "Karambit", "Bayonet", "Butterfly" };
    constexpr const char* gloveSkins[] = { "Default", "Sport", "Driver", "Moto" };

    SectionBegin("Inventory");
    ComboRow("Weapon Skin Changer", &weaponSkin, weaponSkins, 4, "Would select a mock weapon finish preset.");
    ComboRow("Knife Changer", &knifeSkin, knifeSkins, 4, "Would select a mock knife model preset.");
    ComboRow("Glove Changer", &gloveSkin, gloveSkins, 4, "Would select a mock glove model preset.");
    SectionEnd();
}

void RenderWorldTab()
{
    static bool fullbright = false;
    static bool noFog = true;
    static bool wireframeMode = false;
    static bool removeGrass = false;
    static float skyColor[4] = {0.400f, 0.620f, 1.000f, 1.0f};

    SectionBegin("Environment");
    ToggleRow("Fullbright", &fullbright, "Would brighten the mock environment preview.");
    ToggleRow("No Fog", &noFog, "Would remove mock fog effects from the preview.");
    ToggleRow("Wireframe Mode", &wireframeMode, "Would display mock world geometry as wireframes.");
    ToggleRow("Remove Grass", &removeGrass, "Would hide mock foliage in a real visual module.");
    ColorRow("Custom Sky", skyColor, "Would choose the mock sky tint.");
    SectionEnd();
}

void RenderMovementTab()
{
    static bool bunnyHop = false;
    static bool autoStrafe = true;
    static bool noFallDamage = false;
    static float speedMultiplier = 1.0f;

    SectionBegin("Movement");
    ToggleRow("Bunny Hop", &bunnyHop, "UI-only placeholder for automated jump timing.");
    ToggleRow("Auto Strafe", &autoStrafe, "UI-only placeholder for air-strafe assistance.");
    SliderFloatRow("Speed Multiplier", &speedMultiplier, 0.5f, 3.0f, "%.1fx", "Would scale a mock movement speed value.");
    ToggleRow("No Fall Damage", &noFallDamage, "UI-only placeholder for fall-damage prevention.");
    SectionEnd();
}

void RenderPlayerTab()
{
    static bool thirdPersonToggle = false;
    static bool antiAfk = true;
    static float customFov = 100.0f;

    SectionBegin("Camera");
    ToggleRow("Third Person Toggle", &thirdPersonToggle, "Would switch to a mock third-person camera preview.");
    SliderFloatRow("Custom FOV", &customFov, 60.0f, 140.0f, "%.0f", "Would adjust the mock player camera field of view.");
    ToggleRow("Anti-AFK", &antiAfk, "UI-only placeholder for anti-idle behavior.");
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
    static Category selected = Category::Aimbot;

    RenderSidebar(&selected);
    ImGui::SameLine();

    const ImVec2 contentPos = ImGui::GetCursorScreenPos();
    const ImVec2 contentSize = ImGui::GetContentRegionAvail();
    DrawSoftShadow(
        ImGui::GetWindowDrawList(),
        contentPos,
        ImVec2(contentPos.x + contentSize.x, contentPos.y + contentSize.y),
        8.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.055f, 0.064f, 1.0f));
    ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), true);
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
    RenderSearchBar();
    RenderCategoryContent(selected);
    ImGui::EndChild();
    ImGui::PopStyleColor();
}
