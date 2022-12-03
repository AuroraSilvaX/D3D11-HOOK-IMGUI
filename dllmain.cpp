#include "framework.h"
#include "src/impl.hpp"

bool Dx11Hook::IS_INITIALIZED = false;
WNDPROC Dx11Hook::WindowProc = NULL;
HINSTANCE Dx11Hook::HookDllHandle = NULL;
HWND Dx11Hook::WindowTarget = NULL;
ID3D11Device *Dx11Hook::g_Device = NULL;
ID3D11DeviceContext *Dx11Hook::g_Context = NULL;
ID3D11RenderTargetView *Dx11Hook::g_RenderTargetView = NULL;
IDXGISwapChain *Dx11Hook::g_SwapChain = NULL;
Dx11Hook::IDXGISwapChainPresent Dx11Hook::PresentFunc = NULL;
Dx11Hook::IDXGISwapChainResizeBuffers Dx11Hook::ResizeBuffersFunc = NULL;

// 安装Hook
static int WINAPI InitializeHook()
{

    // 如果没获取到present函数则返回
    if (!Dx11Hook::InitializeData())
    {
        return 1;
    }

    // 替换原Present函数为新Present函数
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID &)Dx11Hook::PresentFunc, Dx11Hook::IDXGISwapChainPresentNew);
    DetourTransactionCommit();

    // 替换原ResizeBuffers函数为新ResizeBuffers函数
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID &)Dx11Hook::ResizeBuffersFunc, Dx11Hook::IDXGISwapChainResizeBuffersNew);
    DetourTransactionCommit();

    // 循环Sleep以防线程退出
    while (true)
    {
        Sleep(60000);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{

    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        Dx11Hook::HookDllHandle = hModule;
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitializeHook, NULL, NULL, NULL);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
    }
    return TRUE;
}
