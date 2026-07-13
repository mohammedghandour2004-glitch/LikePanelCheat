#include <tchar.h>
#include <windows.h>
#include <d3d11.h>

#include <cstdio>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "PanelUI.h"
#include "Theme.h"

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static bool g_SwapChainOccluded = false;
static bool g_IsDraggingWindow = false;
static RECT g_PanelDragAreaScreen = {};
static POINT g_DragStartCursor = {};
static POINT g_DragStartWindow = {};
static UINT g_ResizeWidth = 0;
static UINT g_ResizeHeight = 0;
static bool g_UseRegisteredHotkey = false;
static bool g_PreviousToggleKeyDown = false;
static constexpr int kWindowWidth = 760;
static constexpr int kWindowHeight = 520;
static constexpr int kToggleHotkeyId = 0xE22F;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void EndWindowDrag(HWND hwnd);

enum class PanelVisibilityState
{
    Visible,
    Hiding,
    HiddenMessage,
    Hidden,
    Showing
};

static float Clamp01(float value)
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

static float EaseOutCubic(float value)
{
    const float t = 1.0f - Clamp01(value);
    return 1.0f - t * t * t;
}

static float EaseInCubic(float value)
{
    const float t = Clamp01(value);
    return t * t * t;
}

static void RequestPanelToggle(HWND hwnd, PanelVisibilityState& visibilityState, double& transitionStart)
{
    const double now = ImGui::GetTime();
    if (visibilityState == PanelVisibilityState::Visible || visibilityState == PanelVisibilityState::Showing)
    {
        EndWindowDrag(hwnd);
        visibilityState = PanelVisibilityState::Hiding;
        transitionStart = now;
    }
    else
    {
        visibilityState = PanelVisibilityState::Showing;
        transitionStart = now;
        ShowWindow(hwnd, SW_SHOW);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

static void UpdateHotkeyRegistration(HWND hwnd, int& registeredHotkey)
{
    if (registeredHotkey == g_ToggleHotkey)
    {
        return;
    }

    if (g_UseRegisteredHotkey)
    {
        UnregisterHotKey(hwnd, kToggleHotkeyId);
        g_UseRegisteredHotkey = false;
    }

    registeredHotkey = g_ToggleHotkey;
    g_PreviousToggleKeyDown = false;
    if (!g_IsCapturingToggleHotkey)
    {
        g_UseRegisteredHotkey = RegisterHotKey(hwnd, kToggleHotkeyId, MOD_NOREPEAT, static_cast<UINT>(registeredHotkey)) != FALSE;
    }
}

static void RenderPanelHiddenMessage()
{
    const ImVec2 windowSize(static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight));
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(
        "EazyE HEX Hidden Hint",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoInputs);

    char message[96] = {};
    std::snprintf(message, sizeof(message), "Panel Hidden - Press %s to restore", GetToggleHotkeyName());
    const ImVec2 textSize = ImGui::CalcTextSize(message);
    const ImVec2 cardSize(textSize.x + 42.0f, textSize.y + 26.0f);
    const ImVec2 cardMin((windowSize.x - cardSize.x) * 0.5f, (windowSize.y - cardSize.y) * 0.5f);
    const ImVec2 cardMax(cardMin.x + cardSize.x, cardMin.y + cardSize.y);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (int i = 0; i < 3; ++i)
    {
        const float spread = 3.0f + static_cast<float>(i) * 3.0f;
        drawList->AddRectFilled(
            ImVec2(cardMin.x + 4.0f - spread, cardMin.y + 5.0f - spread),
            ImVec2(cardMax.x + 4.0f + spread, cardMax.y + 5.0f + spread),
            ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.18f - static_cast<float>(i) * 0.045f)),
            9.0f + spread);
    }
    drawList->AddRectFilled(cardMin, cardMax, ImGui::GetColorU32(ImVec4(0.060f, 0.060f, 0.072f, 0.94f)), 8.0f);
    drawList->AddRect(cardMin, cardMax, ImGui::GetColorU32(ImVec4(g_AccentColor.x, g_AccentColor.y, g_AccentColor.z, 0.45f)), 8.0f, 0, 1.0f);
    drawList->AddText(ImVec2(cardMin.x + 21.0f, cardMin.y + 13.0f), ImGui::GetColorU32(ImVec4(0.880f, 0.875f, 0.920f, 1.0f)), message);

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView != nullptr)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

static bool CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    if (g_pSwapChain == nullptr || FAILED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
    {
        return false;
    }

    const HRESULT hr = g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);
    backBuffer->Release();
    return SUCCEEDED(hr);
}

static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain != nullptr)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext != nullptr)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice != nullptr)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

static bool CreateDeviceD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    const D3D_FEATURE_LEVEL featureLevelArray[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            createDeviceFlags,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &featureLevel,
            &g_pd3dDeviceContext);
    }
    if (FAILED(hr) || !CreateRenderTarget())
    {
        CleanupDeviceD3D();
        return false;
    }

    return true;
}

static bool IsPointInRect(const RECT& rect, POINT point)
{
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

static void EndWindowDrag(HWND hwnd)
{
    g_IsDraggingWindow = false;
    if (GetCapture() == hwnd)
    {
        ReleaseCapture();
    }
}

static void BeginWindowDrag(HWND hwnd)
{
    POINT cursor = {};
    RECT windowRect = {};
    if (!GetCursorPos(&cursor) || !GetWindowRect(hwnd, &windowRect) || !IsPointInRect(g_PanelDragAreaScreen, cursor))
    {
        return;
    }

    g_IsDraggingWindow = true;
    g_DragStartCursor = cursor;
    g_DragStartWindow = { windowRect.left, windowRect.top };
    SetCapture(hwnd);
}

static void UpdateWindowDrag(HWND hwnd)
{
    if (!g_IsDraggingWindow)
    {
        return;
    }

    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0)
    {
        EndWindowDrag(hwnd);
        return;
    }

    POINT cursor = {};
    if (!GetCursorPos(&cursor))
    {
        EndWindowDrag(hwnd);
        return;
    }

    const int newWindowX = g_DragStartWindow.x + (cursor.x - g_DragStartCursor.x);
    const int newWindowY = g_DragStartWindow.y + (cursor.y - g_DragStartCursor.y);
    SetWindowPos(
        hwnd,
        nullptr,
        newWindowX,
        newWindowY,
        0,
        0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static void UpdatePanelDragArea(HWND hwnd)
{
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float dragHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2.0f;

    RECT windowRect = {};
    if (!GetWindowRect(hwnd, &windowRect))
    {
        g_PanelDragAreaScreen = {};
        return;
    }

    g_PanelDragAreaScreen.left = windowRect.left + static_cast<LONG>(windowPos.x);
    g_PanelDragAreaScreen.top = windowRect.top + static_cast<LONG>(windowPos.y);
    g_PanelDragAreaScreen.right = windowRect.left + static_cast<LONG>(windowPos.x + windowSize.x);
    g_PanelDragAreaScreen.bottom = windowRect.top + static_cast<LONG>(windowPos.y + dragHeight);
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        POINT cursor = {};
        if (GetCursorPos(&cursor) && IsPointInRect(g_PanelDragAreaScreen, cursor))
        {
            BeginWindowDrag(hWnd);
            return 0;
        }
        break;
    }
    case WM_LBUTTONUP:
    case WM_CANCELMODE:
    case WM_CAPTURECHANGED:
        EndWindowDrag(hWnd);
        break;
    default:
        break;
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            return 0;
        }
        g_ResizeWidth = static_cast<UINT>(LOWORD(lParam));
        g_ResizeHeight = static_cast<UINT>(HIWORD(lParam));
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;
    case WM_DESTROY:
        EndWindowDrag(hWnd);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "EazyE HEX Overlay";
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        wc.lpszClassName,
        "EazyE HEX",
        WS_POPUP,
        0,
        0,
        kWindowWidth,
        kWindowHeight,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    if (!CreateDeviceD3D(hwnd))
    {
        DestroyWindow(hwnd);
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef EAZYE_IMGUI_HAS_DOCKING
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    ApplyTheme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    int registeredHotkey = 0;
    UpdateHotkeyRegistration(hwnd, registeredHotkey);

    bool done = false;
    PanelVisibilityState visibilityState = PanelVisibilityState::Visible;
    double visibilityTransitionStart = ImGui::GetTime();
    while (!done)
    {
        UpdateHotkeyRegistration(hwnd, registeredHotkey);

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            if (msg.message == WM_HOTKEY && msg.wParam == kToggleHotkeyId)
            {
                if (!g_IsCapturingToggleHotkey)
                {
                    RequestPanelToggle(hwnd, visibilityState, visibilityTransitionStart);
                }
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                done = true;
            }
        }

        if (done)
        {
            break;
        }

        if (!g_UseRegisteredHotkey && !g_IsCapturingToggleHotkey)
        {
            const bool toggleKeyDown = (GetAsyncKeyState(g_ToggleHotkey) & 0x8000) != 0;
            if (toggleKeyDown && !g_PreviousToggleKeyDown)
            {
                RequestPanelToggle(hwnd, visibilityState, visibilityTransitionStart);
            }
            g_PreviousToggleKeyDown = toggleKeyDown;
        }
        else if (g_IsCapturingToggleHotkey)
        {
            g_PreviousToggleKeyDown = false;
        }

        UpdateWindowDrag(hwnd);

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = 0;
            g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ApplyAccentThemeColors();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static const double panelOpenStart = ImGui::GetTime();
        const double now = ImGui::GetTime();
        float visibilityAlpha = 1.0f;
        float visibilityScale = 1.0f;
        bool renderPanel = true;
        bool renderHiddenMessage = false;

        if (visibilityState == PanelVisibilityState::Hiding)
        {
            const float hideT = Clamp01(static_cast<float>((now - visibilityTransitionStart) / 0.15));
            const float eased = EaseInCubic(hideT);
            visibilityAlpha = 1.0f - eased;
            visibilityScale = 1.0f - (0.04f * eased);
            if (hideT >= 1.0f)
            {
                visibilityState = PanelVisibilityState::HiddenMessage;
                visibilityTransitionStart = now;
                renderPanel = false;
                renderHiddenMessage = true;
            }
        }
        else if (visibilityState == PanelVisibilityState::HiddenMessage)
        {
            renderPanel = false;
            renderHiddenMessage = true;
            if (now - visibilityTransitionStart >= 1.5)
            {
                visibilityState = PanelVisibilityState::Hidden;
                ShowWindow(hwnd, SW_HIDE);
                g_PanelDragAreaScreen = {};
                renderHiddenMessage = false;
            }
        }
        else if (visibilityState == PanelVisibilityState::Hidden)
        {
            renderPanel = false;
        }
        else if (visibilityState == PanelVisibilityState::Showing)
        {
            const float showT = EaseOutCubic(static_cast<float>((now - visibilityTransitionStart) / 0.15));
            visibilityAlpha = showT;
            visibilityScale = 0.94f + (0.06f * showT);
            if (showT >= 1.0f)
            {
                visibilityState = PanelVisibilityState::Visible;
                visibilityAlpha = 1.0f;
                visibilityScale = 1.0f;
            }
        }

        const float openT = EaseOutCubic(static_cast<float>((now - panelOpenStart) / 0.25));
        if (renderPanel)
        {
            const float panelScale = (0.90f + (0.10f * openT)) * visibilityScale;
            const ImVec2 panelSize(static_cast<float>(kWindowWidth) * panelScale, static_cast<float>(kWindowHeight) * panelScale);
            ImGui::SetNextWindowPos(
                ImVec2((static_cast<float>(kWindowWidth) - panelSize.x) * 0.5f, (static_cast<float>(kWindowHeight) - panelSize.y) * 0.5f),
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
            const ImGuiWindowFlags panelFlags =
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * openT * visibilityAlpha);
            ImGui::Begin("EazyE HEX", nullptr, panelFlags);
            UpdatePanelDragArea(hwnd);
            RenderMainPanelTabs();
            ImGui::End();
            ImGui::PopStyleVar();
        }
        else
        {
            g_PanelDragAreaScreen = {};
        }

        if (renderHiddenMessage)
        {
            RenderPanelHiddenMessage();
        }

        ImGui::Render();

        const float clearColorWithAlpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColorWithAlpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        const HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_UseRegisteredHotkey)
    {
        UnregisterHotKey(hwnd, kToggleHotkeyId);
    }
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);

    return 0;
}
