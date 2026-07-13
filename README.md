# EazyE HEX

Pure UI mockup project using Dear ImGui with the DirectX 11 backend on Windows.

This project creates a borderless, transparent, always-on-top overlay window titled `EazyE HEX`. It does not include game-hooking, memory reading, injection, or process interaction logic.

## Build with MinGW

Install CMake, MinGW, and Git, then run:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

By default, CMake uses `external/imgui` if present. If it is missing, CMake fetches Dear ImGui from the official repository using the `docking` branch.

To use an existing Dear ImGui checkout:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DIMGUI_DIR=C:\path\to\imgui
cmake --build build
```
