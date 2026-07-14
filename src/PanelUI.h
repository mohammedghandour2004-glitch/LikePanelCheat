#pragma once

struct ID3D11Device;

extern int g_ToggleHotkey;
extern bool g_IsCapturingToggleHotkey;

const char* GetToggleHotkeyName();
void SetPanelD3DDevice(ID3D11Device* device);
const char* GetProfilePicturePath();
bool LoadProfilePictureFromPath(const char* path);
void ClearProfilePicture();
void RenderBootSplash(float progress, float alpha);
bool RenderWelcomeScreen(float alpha);
void RenderMainPanelTabs();
void AimTab();
void VisualsTab();
void MiscTab();
void ConfigTab();
