#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <fstream>
#include <string>

#include <d3d11.h>
#include "src/Detours/detours.h"
#include "src/ImGui/imgui.h"
#include "src/ImGui/imgui_impl_dx11.h"
#include "src/ImGui/imgui_impl_win32.h"

#pragma comment(lib, "d3d11.lib")

#ifdef _WIN32
#pragma comment(lib, "Detours.x64.lib")
#else
#pragma comment(lib, "Detours.x86.lib")
#endif
