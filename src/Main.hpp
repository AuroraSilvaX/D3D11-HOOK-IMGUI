#pragma once
#include "Impl.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam);

typedef HRESULT(WINAPI *IDXGISwapChainPresent)(IDXGISwapChain *SwapChain,
                                               UINT SyncInterval, UINT Flags);

typedef HRESULT(WINAPI *IDXGISwapChainResizeBuffers)(IDXGISwapChain *SwapChain,
                                                     UINT BufferCount,
                                                     UINT Width, UINT Height,
                                                     DXGI_FORMAT NewFormat,
                                                     UINT SwapChainFlags);

IDXGISwapChainPresent PresentFunc;
IDXGISwapChainResizeBuffers ResizeBuffersFunc;

ID3D11Device *g_Device;
ID3D11DeviceContext *g_Context;
ID3D11RenderTargetView *g_RenderTargetView;
IDXGISwapChain *g_SwapChain;

bool IS_INITIALIZED;

HINSTANCE HookDllHandle;

DXGI_SWAP_CHAIN_DESC scd;

HWND WindowTarget;
WNDPROC WindowProc;

LRESULT WINAPI WindowProcNew(const HWND hWnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam) {

  if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    return true;

  return CallWindowProc(WindowProc, hWnd, uMsg, wParam, lParam);
}

bool InitializeData() {

  ZeroMemory(&scd, sizeof(scd));
  scd.BufferDesc.Width = NULL;
  scd.BufferDesc.Height = NULL;
  scd.BufferDesc.RefreshRate.Numerator = 60;
  scd.BufferDesc.RefreshRate.Denominator = 1;
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  scd.SampleDesc.Count = 1;
  scd.SampleDesc.Quality = 0;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.BufferCount = 2;
  scd.OutputWindow = GetForegroundWindow();
  scd.Windowed = true;
  scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  scd.Flags = NULL;

  if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(
          NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
          D3D11_SDK_VERSION, &scd, &g_SwapChain, &g_Device, NULL, NULL))) {

    void **pp_SwapChainVTable = *reinterpret_cast<void ***>(g_SwapChain);
    PresentFunc = (IDXGISwapChainPresent)pp_SwapChainVTable[8];
    ResizeBuffersFunc = (IDXGISwapChainResizeBuffers)pp_SwapChainVTable[13];
    return true;
  }
  return false;
}

HRESULT WINAPI IDXGISwapChainPresentNew(IDXGISwapChain *pSwapChain,
                                        UINT SyncInterval, UINT Flags) {

  if (!IS_INITIALIZED) {
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device),
                                        (void **)&g_Device))) {

      g_Device->GetImmediateContext(&g_Context);
      pSwapChain->GetDesc(&scd);
      WindowTarget = scd.OutputWindow;
      ID3D11Texture2D *pBackBuffer;
      pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                            (LPVOID *)&pBackBuffer);
      g_Device->CreateRenderTargetView(pBackBuffer, NULL, &g_RenderTargetView);
      pBackBuffer->Release();

      ImGui::CreateContext();
      ImGuiIO &io = ImGui::GetIO();
      io.IniFilename = NULL;
      io.Fonts->AddFontFromFileTTF("C:\\windows\\fonts\\simhei.ttf", 20.0f,
                                   NULL, io.Fonts->GetGlyphRangesChineseFull());
      io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

      ImGui_ImplWin32_Init(WindowTarget);
      ImGui_ImplDX11_Init(g_Device, g_Context);

      WindowProc = (WNDPROC)SetWindowLongPtr(WindowTarget, GWLP_WNDPROC,
                                             (LONG_PTR)WindowProcNew);

      IS_INITIALIZED = true;
    } else
      return PresentFunc(pSwapChain, SyncInterval, Flags);
  }

  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

  ImGui::EndFrame();
  ImGui::Render();

  g_Context->OMSetRenderTargets(1, &g_RenderTargetView, NULL);

  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  return PresentFunc(pSwapChain, SyncInterval, Flags);
}

HRESULT WINAPI IDXGISwapChainResizeBuffersNew(IDXGISwapChain *SwapChain,
                                              UINT BufferCount, UINT Width,
                                              UINT Height,
                                              DXGI_FORMAT NewFormat,
                                              UINT SwapChainFlags) {

  if (g_RenderTargetView) {
    g_Context->OMSetRenderTargets(0, 0, 0);
    g_RenderTargetView->Release();
  }

  HRESULT ReurnResizeBuffersOriginalFun = ResizeBuffersFunc(
      SwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

  ID3D11Texture2D *pBuffer;
  SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&pBuffer);
  g_Device->CreateRenderTargetView(pBuffer, NULL, &g_RenderTargetView);
  pBuffer->Release();
  g_Context->OMSetRenderTargets(1, &g_RenderTargetView, NULL);

  D3D11_VIEWPORT vp;
  vp.Width = Width;
  vp.Height = Height;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  g_Context->RSSetViewports(1, &vp);

  return ReurnResizeBuffersOriginalFun;
}
