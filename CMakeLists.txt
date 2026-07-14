cmake_minimum_required(VERSION 3.20)

project(EazyE_HEX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "EazyE HEX is a Windows-only DirectX 11 overlay mockup.")
endif()

set(IMGUI_DIR "" CACHE PATH "Path to a local Dear ImGui checkout. If empty, imgui-master or external/imgui is used when present, otherwise FetchContent downloads it.")

if(IMGUI_DIR)
    set(IMGUI_ROOT "${IMGUI_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui-master/imgui.cpp")
    set(IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/imgui-master")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp")
    set(IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
else()
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
    )
    FetchContent_MakeAvailable(imgui)
    set(IMGUI_ROOT "${imgui_SOURCE_DIR}")
endif()

set(IMGUI_REQUIRED_FILES
    "${IMGUI_ROOT}/imgui.h"
    "${IMGUI_ROOT}/imgui.cpp"
    "${IMGUI_ROOT}/imgui_draw.cpp"
    "${IMGUI_ROOT}/imgui_tables.cpp"
    "${IMGUI_ROOT}/imgui_widgets.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_win32.h"
    "${IMGUI_ROOT}/backends/imgui_impl_win32.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_dx11.h"
    "${IMGUI_ROOT}/backends/imgui_impl_dx11.cpp"
)

foreach(IMGUI_REQUIRED_FILE IN LISTS IMGUI_REQUIRED_FILES)
    if(NOT EXISTS "${IMGUI_REQUIRED_FILE}")
        message(FATAL_ERROR "Required Dear ImGui file not found: ${IMGUI_REQUIRED_FILE}")
    endif()
endforeach()

file(STRINGS "${IMGUI_ROOT}/imgui.h" IMGUI_DOCKING_FLAG_LINES REGEX "ImGuiConfigFlags_DockingEnable|ImGuiCol_DockingPreview")
if(IMGUI_DOCKING_FLAG_LINES)
    set(EAZYE_IMGUI_HAS_DOCKING ON)
else()
    set(EAZYE_IMGUI_HAS_DOCKING OFF)
    message(WARNING "The selected Dear ImGui checkout does not expose docking symbols. Docking will be skipped for this build. Use Dear ImGui's docking branch to enable it.")
endif()

add_library(imgui STATIC
    "${IMGUI_ROOT}/imgui.cpp"
    "${IMGUI_ROOT}/imgui_draw.cpp"
    "${IMGUI_ROOT}/imgui_tables.cpp"
    "${IMGUI_ROOT}/imgui_widgets.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_win32.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_dx11.cpp"
)

target_include_directories(imgui PUBLIC
    "${IMGUI_ROOT}"
    "${IMGUI_ROOT}/backends"
)

if(MINGW)
    target_compile_definitions(imgui PUBLIC
        IMGUI_DISABLE_TIME_FUNCTIONS
        IMGUI_DISABLE_WIN32_FUNCTIONS
        IMGUI_IMPL_WIN32_DISABLE_ALPHA_COMPOSITING
        IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    )
endif()

add_executable(EazyE_HEX WIN32
    src/ConfigManager.cpp
    src/ConfigManager.h
    src/ImageLoader.cpp
    src/ImageLoader.h
    src/main.cpp
    src/PanelUI.cpp
    src/PanelUI.h
    src/Theme.cpp
    src/Theme.h
)

target_include_directories(EazyE_HEX PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/json"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb"
)

target_link_libraries(EazyE_HEX PRIVATE
    imgui
    d3d11
    d3dcompiler
    dxgi
    gdi32
    comdlg32
)

if(MINGW)
    target_link_options(EazyE_HEX PRIVATE
        -static-libgcc
        -static-libstdc++
        -static
    )
endif()

target_compile_definitions(EazyE_HEX PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(EAZYE_IMGUI_HAS_DOCKING)
    target_compile_definitions(EazyE_HEX PRIVATE EAZYE_IMGUI_HAS_DOCKING=1)
endif()
