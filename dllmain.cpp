#include "src/Main.hpp"

int WINAPI InitializeHook() {

  if (!InitializeData()) {
    return 1;
  }

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourAttach(&(PVOID &)PresentFunc, IDXGISwapChainPresentNew);
  DetourTransactionCommit();

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourAttach(&(PVOID &)ResizeBuffersFunc, IDXGISwapChainResizeBuffersNew);
  DetourTransactionCommit();

  while (true) {
    Sleep(60000);
  }
  return 0;
}

int WINAPI ReleaseHook() {

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID &)PresentFunc, IDXGISwapChainPresentNew);
  DetourTransactionCommit();

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID &)ResizeBuffersFunc, IDXGISwapChainResizeBuffersNew);
  DetourTransactionCommit();

  g_Device->Release();
  g_Context->Release();
  g_RenderTargetView->Release();
  g_SwapChain->Release();

  return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {

  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    HookDllHandle = hModule;
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitializeHook, NULL, NULL,
                 NULL);
  } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ReleaseHook, NULL, NULL,
                 NULL);
  }
  return TRUE;
}
