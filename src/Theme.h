#pragma once

#include "imgui.h"

extern ImVec4 g_AccentColor;
extern ImFont* g_FontRegular;
extern ImFont* g_FontSection;
extern ImFont* g_FontSmall;

void ApplyTheme();
void ApplyAccentThemeColors();
ImVec4 GetAccentHoverColor();
bool ToggleSwitch(const char* label, bool* value);
void RenderEazyEWatermark();
