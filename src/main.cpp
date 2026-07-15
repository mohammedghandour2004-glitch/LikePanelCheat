#include <tchar.h>
#include <windows.h>
#include <d3d11.h>

#include <cmath>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "ConfigManager.h"
#include "PanelUI.h"
#include "resource.h"
#include "Theme.h"

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static ID3D11Texture2D* g_layeredStagingTexture = nullptr;
static HDC g_layeredMemoryDc = nullptr;
static HBITMAP g_layeredBitmap = nullptr;
static void* g_layeredBitmapBits = nullptr;
static int g_layeredBitmapWidth = 0;
static int g_layeredBitmapHeight = 0;
static bool g_SwapChainOccluded = false;
static bool g_IsDraggingWindow = false;
static RECT g_PanelDragAreaScreen = {};
static POINT g_DragStartCursor = {};
static POINT g_DragStartWindow = {};
static UINT g_ResizeWidth = 0;
static UINT g_ResizeHeight = 0;
static bool g_UseRegisteredHotkey = false;
static bool g_PreviousToggleKeyDown = false;
static bool g_FocusLossPending = false;
static constexpr int kAuthWindowWidth = 360;
static constexpr int kAuthWindowHeight = 440;
static constexpr int kWindowWidth = 760;
static constexpr int kWindowHeight = 520;
static constexpr int kWindowCornerRadius = 16;
static constexpr int kToggleHotkeyId = 0xE22F;
static constexpr double kSplashDuration = 2.0;
static constexpr float kAnimFast = 0.10f;
static constexpr float kAnimMedium = 0.20f;
static constexpr float kAnimStandard = 0.25f;
static constexpr float kAnimSlow = 0.35f;
static constexpr double kSplashFadeDuration = kAnimSlow;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void EndWindowDrag(HWND hwnd);
static void ApplyWindowIcons(HWND hwnd, HINSTANCE hInstance);

enum class PanelVisibilityState
{
    Visible,
    Hiding,
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
    return PanelEaseOut(value);
}

static float EaseInOutCubic(float value)
{
    return PanelEaseInOut(value);
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

static HICON LoadAppIcon(HINSTANCE hInstance, int width, int height)
{
    return static_cast<HICON>(LoadImageA(
        hInstance,
        MAKEINTRESOURCEA(IDI_ICON1),
        IMAGE_ICON,
        width,
        height,
        LR_DEFAULTCOLOR | LR_SHARED));
}

static void ApplyWindowIcons(HWND hwnd, HINSTANCE hInstance)
{
    HICON bigIcon = LoadAppIcon(hInstance, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    HICON smallIcon = LoadAppIcon(hInstance, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));

    if (bigIcon == nullptr)
    {
        bigIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON1));
    }
    if (smallIcon == nullptr)
    {
        smallIcon = bigIcon;
    }

    if (bigIcon != nullptr)
    {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(bigIcon));
    }
    if (smallIcon != nullptr)
    {
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
    }
}

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView != nullptr)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

static void CleanupLayeredFrameResources()
{
    if (g_layeredStagingTexture != nullptr)
    {
        g_layeredStagingTexture->Release();
        g_layeredStagingTexture = nullptr;
    }
    if (g_layeredBitmap != nullptr)
    {
        DeleteObject(g_layeredBitmap);
        g_layeredBitmap = nullptr;
    }
    if (g_layeredMemoryDc != nullptr)
    {
        DeleteDC(g_layeredMemoryDc);
        g_layeredMemoryDc = nullptr;
    }
    g_layeredBitmapBits = nullptr;
    g_layeredBitmapWidth = 0;
    g_layeredBitmapHeight = 0;
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

static float RoundedCornerCoverage(int x, int y, int width, int height)
{
    const float radius = static_cast<float>(kWindowCornerRadius);
    const float px = static_cast<float>(x) + 0.5f;
    const float py = static_cast<float>(y) + 0.5f;
    float cx = px;
    float cy = py;

    if (px < radius)
    {
        cx = radius;
    }
    else if (px > static_cast<float>(width) - radius)
    {
        cx = static_cast<float>(width) - radius;
    }

    if (py < radius)
    {
        cy = radius;
    }
    else if (py > static_cast<float>(height) - radius)
    {
        cy = static_cast<float>(height) - radius;
    }

    if (cx == px && cy == py)
    {
        return 1.0f;
    }

    const float dx = px - cx;
    const float dy = py - cy;
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance <= radius - 0.5f)
    {
        return 1.0f;
    }
    if (distance >= radius + 0.5f)
    {
        return 0.0f;
    }
    return radius + 0.5f - distance;
}

static bool EnsureLayeredFrameResources(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (g_layeredStagingTexture != nullptr &&
        g_layeredBitmap != nullptr &&
        g_layeredMemoryDc != nullptr &&
        g_layeredBitmapBits != nullptr &&
        g_layeredBitmapWidth == width &&
        g_layeredBitmapHeight == height)
    {
        return true;
    }

    CleanupLayeredFrameResources();

    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = static_cast<UINT>(width);
    stagingDesc.Height = static_cast<UINT>(height);
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (g_pd3dDevice == nullptr || FAILED(g_pd3dDevice->CreateTexture2D(&stagingDesc, nullptr, &g_layeredStagingTexture)))
    {
        CleanupLayeredFrameResources();
        return false;
    }

    HDC screenDc = GetDC(nullptr);
    if (screenDc == nullptr)
    {
        CleanupLayeredFrameResources();
        return false;
    }
    g_layeredMemoryDc = CreateCompatibleDC(screenDc);
    ReleaseDC(nullptr, screenDc);
    if (g_layeredMemoryDc == nullptr)
    {
        CleanupLayeredFrameResources();
        return false;
    }

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    g_layeredBitmap = CreateDIBSection(g_layeredMemoryDc, &bitmapInfo, DIB_RGB_COLORS, &g_layeredBitmapBits, nullptr, 0);
    if (g_layeredBitmap == nullptr || g_layeredBitmapBits == nullptr)
    {
        CleanupLayeredFrameResources();
        return false;
    }

    SelectObject(g_layeredMemoryDc, g_layeredBitmap);
    g_layeredBitmapWidth = width;
    g_layeredBitmapHeight = height;
    return true;
}

static bool UpdateLayeredWindowFromBackBuffer(HWND hwnd)
{
    if (g_pSwapChain == nullptr || g_pd3dDeviceContext == nullptr)
    {
        return false;
    }

    RECT windowRect = {};
    if (!GetWindowRect(hwnd, &windowRect))
    {
        return false;
    }
    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    if (!EnsureLayeredFrameResources(width, height))
    {
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    if (FAILED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
    {
        return false;
    }

    g_pd3dDeviceContext->CopyResource(g_layeredStagingTexture, backBuffer);
    backBuffer->Release();

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(g_pd3dDeviceContext->Map(g_layeredStagingTexture, 0, D3D11_MAP_READ, 0, &mapped)))
    {
        return false;
    }

    unsigned char* destination = static_cast<unsigned char*>(g_layeredBitmapBits);
    const unsigned char* source = static_cast<const unsigned char*>(mapped.pData);
    constexpr int kBytesPerPixel = 4;
    for (int y = 0; y < height; ++y)
    {
        const unsigned char* sourceRow = source + static_cast<size_t>(mapped.RowPitch) * static_cast<size_t>(y);
        unsigned char* destinationRow = destination + static_cast<size_t>(width) * kBytesPerPixel * static_cast<size_t>(y);
        for (int x = 0; x < width; ++x)
        {
            const unsigned char* src = sourceRow + x * kBytesPerPixel;
            unsigned char* dst = destinationRow + x * kBytesPerPixel;
            const float coverage = RoundedCornerCoverage(x, y, width, height);
            dst[0] = static_cast<unsigned char>(static_cast<float>(src[0]) * coverage + 0.5f);
            dst[1] = static_cast<unsigned char>(static_cast<float>(src[1]) * coverage + 0.5f);
            dst[2] = static_cast<unsigned char>(static_cast<float>(src[2]) * coverage + 0.5f);
            dst[3] = static_cast<unsigned char>(static_cast<float>(src[3]) * coverage + 0.5f);
        }
    }
    g_pd3dDeviceContext->Unmap(g_layeredStagingTexture, 0);

    HDC screenDc = GetDC(nullptr);
    if (screenDc == nullptr)
    {
        return false;
    }

    POINT destinationPoint = { windowRect.left, windowRect.top };
    SIZE destinationSize = { width, height };
    POINT sourcePoint = { 0, 0 };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    const BOOL updated = UpdateLayeredWindow(hwnd, screenDc, &destinationPoint, &destinationSize, g_layeredMemoryDc, &sourcePoint, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screenDc);
    return updated != FALSE;
}

static void CleanupDeviceD3D()
{
    CleanupLayeredFrameResources();
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
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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

static POINT CalculateCenteredWindowPosition(int windowWidth, int windowHeight)
{
    RECT workArea = {};
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0))
    {
        workArea.left = 0;
        workArea.top = 0;
        workArea.right = GetSystemMetrics(SM_CXSCREEN);
        workArea.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    const int workAreaWidth = workArea.right - workArea.left;
    const int workAreaHeight = workArea.bottom - workArea.top;
    POINT position = {};
    position.x = workArea.left + (workAreaWidth - windowWidth) / 2;
    position.y = workArea.top + (workAreaHeight - windowHeight) / 2;
    return position;
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
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            g_FocusLossPending = true;
        }
        break;
    case WM_KILLFOCUS:
        g_FocusLossPending = true;
        break;
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
    wc.hIcon = LoadAppIcon(hInstance, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wc.hIconSm = LoadAppIcon(hInstance, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    wc.lpszClassName = "EazyE HEX Overlay";
    RegisterClassExA(&wc);

    const POINT centeredPosition = CalculateCenteredWindowPosition(kAuthWindowWidth, kAuthWindowHeight);
    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        wc.lpszClassName,
        "EazyE HEX",
        WS_POPUP,
        centeredPosition.x,
        centeredPosition.y,
        kAuthWindowWidth,
        kAuthWindowHeight,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);
    ApplyWindowIcons(hwnd, hInstance);

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
    SetPanelD3DDevice(g_pd3dDevice);
    (void)LoadConfigFromFile("Default");
    int registeredHotkey = 0;
    UpdateHotkeyRegistration(hwnd, registeredHotkey);

    bool done = false;
    PanelVisibilityState visibilityState = PanelVisibilityState::Visible;
    double visibilityTransitionStart = ImGui::GetTime();
    const double appStartTime = ImGui::GetTime();
    bool mainWindowSizeApplied = false;
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

        if (g_FocusLossPending)
        {
            g_FocusLossPending = false;
            if (IsAutoHideOnFocusLossEnabled() &&
                (visibilityState == PanelVisibilityState::Visible || visibilityState == PanelVisibilityState::Showing))
            {
                RequestPanelToggle(hwnd, visibilityState, visibilityTransitionStart);
            }
        }
        const double frameStartTime = ImGui::GetTime();

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
            const UINT resizeWidth = g_ResizeWidth;
            const UINT resizeHeight = g_ResizeHeight;
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, resizeWidth, resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = 0;
            g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ApplyAccentThemeColors();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        UpdateAutoConfigSave();

        static double panelOpenStart = -1.0;
        static double welcomeExitStart = -1.0;
        static bool welcomeAccepted = false;
        const double now = ImGui::GetTime();
        const double splashDuration = PanelAnimationDuration(kSplashDuration);
        const double splashFadeDuration = PanelAnimationDuration(kSplashFadeDuration);
        const double welcomeExitDuration = PanelAnimationDuration(kAnimSlow);
        const double splashElapsed = now - appStartTime;
        const bool splashOnly = splashElapsed < splashDuration;
        const bool splashFading = splashElapsed >= splashDuration && splashElapsed < splashDuration + splashFadeDuration;
        const bool splashComplete = splashElapsed >= splashDuration + splashFadeDuration;
        const bool welcomeExiting = welcomeExitStart >= 0.0 && now - welcomeExitStart < welcomeExitDuration;
        if (!welcomeAccepted && welcomeExitStart >= 0.0 && panelOpenStart < 0.0)
        {
            panelOpenStart = welcomeExitStart;
        }
        if (!welcomeAccepted && welcomeExitStart >= 0.0 && now - welcomeExitStart >= welcomeExitDuration)
        {
            welcomeAccepted = true;
        }
        const bool renderWelcome = splashComplete && !welcomeAccepted;

        float visibilityAlpha = 1.0f;
        float visibilityScale = 1.0f;
        bool renderPanel = !splashOnly && !splashFading && (welcomeAccepted || welcomeExiting);

        if (visibilityState == PanelVisibilityState::Hiding)
        {
            const float hideT = Clamp01(static_cast<float>((now - visibilityTransitionStart) / PanelAnimationDuration(kAnimMedium)));
            const float eased = EaseInOutCubic(hideT);
            visibilityAlpha = 1.0f - eased;
            visibilityScale = 1.0f - (0.04f * eased);
            if (hideT >= 1.0f)
            {
                visibilityState = PanelVisibilityState::Hidden;
                ShowWindow(hwnd, SW_HIDE);
                g_PanelDragAreaScreen = {};
                renderPanel = false;
            }
        }
        else if (visibilityState == PanelVisibilityState::Hidden)
        {
            renderPanel = false;
        }
        else if (visibilityState == PanelVisibilityState::Showing)
        {
            const float showT = EaseOutCubic(static_cast<float>((now - visibilityTransitionStart) / PanelAnimationDuration(kAnimMedium)));
            visibilityAlpha = showT;
            visibilityScale = 0.94f + (0.06f * showT);
            if (showT >= 1.0f)
            {
                visibilityState = PanelVisibilityState::Visible;
                visibilityAlpha = 1.0f;
                visibilityScale = 1.0f;
            }
        }

        const float openT = panelOpenStart >= 0.0
            ? EaseOutCubic(static_cast<float>((now - panelOpenStart) / PanelAnimationDuration(kAnimStandard)))
            : 0.0f;
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

        if (renderWelcome)
        {
            const float welcomeFadeOut = welcomeExiting
                ? 1.0f - EaseOutCubic(static_cast<float>((now - welcomeExitStart) / welcomeExitDuration))
                : 1.0f;
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(mainWindowSizeApplied ? kWindowWidth : kAuthWindowWidth), static_cast<float>(mainWindowSizeApplied ? kWindowHeight : kAuthWindowHeight)), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * welcomeFadeOut * visibilityAlpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::Begin(
                "EazyE HEX Welcome",
                nullptr,
                ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse);
            UpdatePanelDragArea(hwnd);
            if (!welcomeExiting && RenderWelcomeScreen(welcomeFadeOut))
            {
                const POINT mainPosition = CalculateCenteredWindowPosition(kWindowWidth, kWindowHeight);
                SetWindowPos(
                    hwnd,
                    nullptr,
                    mainPosition.x,
                    mainPosition.y,
                    kWindowWidth,
                    kWindowHeight,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                g_ResizeWidth = kWindowWidth;
                g_ResizeHeight = kWindowHeight;
                mainWindowSizeApplied = true;
                welcomeExitStart = now;
            }
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(3);
        }

        if (splashOnly || splashFading)
        {
            const float splashAlpha = splashFading
                ? 1.0f - EaseOutCubic(static_cast<float>((splashElapsed - splashDuration) / splashFadeDuration))
                : 1.0f;
            const float progress = static_cast<float>(splashElapsed / splashDuration);
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kAuthWindowWidth), static_cast<float>(kAuthWindowHeight)), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::Begin(
                "EazyE HEX Boot Splash",
                nullptr,
                ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoInputs);
            RenderBootSplash(progress, splashAlpha);
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }

        ImGui::Render();

        const float clearColorWithAlpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColorWithAlpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        UpdateLayeredWindowFromBackBuffer(hwnd);
        const double frameElapsed = ImGui::GetTime() - frameStartTime;
        constexpr double kTargetFrameSeconds = 1.0 / 60.0;
        if (frameElapsed < kTargetFrameSeconds)
        {
            const DWORD sleepMs = static_cast<DWORD>((kTargetFrameSeconds - frameElapsed) * 1000.0);
            if (sleepMs > 0)
            {
                Sleep(sleepMs);
            }
        }
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
