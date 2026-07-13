#pragma once

extern int g_ToggleHotkey;
extern bool g_IsCapturingToggleHotkey;

const char* GetToggleHotkeyName();
void RenderMainPanelTabs();
void AimTab();
void VisualsTab();
void MiscTab();
void ConfigTab();
