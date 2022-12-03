#pragma once
#include "../framework.h"

// 导出Imgui的回调窗口函数
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam);

class Dx11Hook
{

public:
    // 本Dll的Handle
    static HINSTANCE HookDllHandle;

    // 判断是否已初始化
    static bool IS_INITIALIZED;

    // 目标窗口: Imgui将要渲染的窗口
    static HWND WindowTarget;

    // 目标窗口的原回调函数
    static WNDPROC WindowProc;

    // 设备、交换链、上下文、呈现视图指针
    static ID3D11Device *g_Device;
    static ID3D11DeviceContext *g_Context;
    static ID3D11RenderTargetView *g_RenderTargetView;
    static IDXGISwapChain *g_SwapChain;

    // Present函数声明
    typedef HRESULT(WINAPI *IDXGISwapChainPresent)(IDXGISwapChain *SwapChain, UINT SyncInterval, UINT Flags);
    // ResizeBuffers函数声明
    typedef HRESULT(WINAPI *IDXGISwapChainResizeBuffers)(
        IDXGISwapChain *SwapChain,
        UINT BufferCount,
        UINT Width,
        UINT Height,
        DXGI_FORMAT NewFormat,
        UINT SwapChainFlags);

    // 原Present函数的指针
    static IDXGISwapChainPresent PresentFunc;
    // 原ResizeBuffers函数指针
    static IDXGISwapChainResizeBuffers ResizeBuffersFunc;

    // 目标窗口的新回调函数
    static LRESULT __stdcall WindowProcNew(const HWND hWnd, UINT uMsg, WPARAM wParam,
                                           LPARAM lParam)
    {

        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        return CallWindowProc(WindowProc, hWnd, uMsg, wParam, lParam);
    }

    // 创建设备、交换链，获取回调函数
    static bool InitializeData()
    {
        // 设置交换链数据
        DXGI_SWAP_CHAIN_DESC scd;
        // 用 0 填充交换链的内存
        ZeroMemory(&scd, sizeof(scd));
        // 分辨率宽度: 设为 0 则交换链会自动获取分辨率宽度
        scd.BufferDesc.Width = NULL;
        // 分辨率高度: 设为 0 则交换链会自动获取分辨率宽度
        scd.BufferDesc.Height = NULL;
        // 刷新率的分子
        scd.BufferDesc.RefreshRate.Numerator = 60;
        // 刷新率的分母
        scd.BufferDesc.RefreshRate.Denominator = 1;
        // 资源数据格式
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        // 扫描方式
        scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        // 缩放模式
        scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        // 多重采样每个像素的样本数量为 1
        scd.SampleDesc.Count = 1;
        // 多重采样质量等级为 0
        scd.SampleDesc.Quality = 0;

        // 使用图面或资源作为输出目标
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        // 缓冲区数量为 2
        scd.BufferCount = 2;

        // 输出窗口的句柄为前台窗口: 不得为 NULL
        scd.OutputWindow = GetForegroundWindow();
        // 指定输出是否处于窗口模式
        scd.Windowed = true;
        // 交换效果
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        // 标志
        scd.Flags = NULL;

        // 创建D3D11设备和交换链
        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &scd, &g_SwapChain, &g_Device, NULL, NULL)))
        {
            // 获取SwapChain虚函数表
            void **pp_SwapChainVTable = *reinterpret_cast<void ***>(g_SwapChain);

            // 获取原Prensent函数
            PresentFunc = (IDXGISwapChainPresent)pp_SwapChainVTable[8];
            ResizeBuffersFunc = (IDXGISwapChainResizeBuffers)pp_SwapChainVTable[13];

            return true;
        }
        return false;
    }

    // Present的新函数
    static HRESULT WINAPI IDXGISwapChainPresentNew(IDXGISwapChain *pSwapChain,
                                                   UINT SyncInterval, UINT Flags)
    {
        // 如果未初始化则运行代码
        if (!IS_INITIALIZED)
        {
            // 获取 Device
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device),
                                                (void **)&g_Device)))
            {
                // 获取即时Context
                g_Device->GetImmediateContext(&g_Context);
                // 定义中转交换链
                DXGI_SWAP_CHAIN_DESC scd;
                // 用 0 填充交换链的内存
                ZeroMemory(&scd, sizeof(scd));
                // 获取于上文已经初始化的交换链
                pSwapChain->GetDesc(&scd);
                // 设置目标窗口
                WindowTarget = scd.OutputWindow;
                // 后台缓冲区指针
                ID3D11Texture2D *pBackBuffer;
                // 获取后台缓冲区的数据
                pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                      (LPVOID *)&pBackBuffer);
                // 创建呈现视图并将后台缓冲区数据指向呈现视图
                g_Device->CreateRenderTargetView(pBackBuffer, NULL,
                                                 &g_RenderTargetView);
                // 释放后台缓冲区指针
                pBackBuffer->Release();
                // Imgui创建上下文
                ImGui::CreateContext();
                // Imgui创建IO
                ImGuiIO &io = ImGui::GetIO();
                // 设置中文字体
                io.Fonts->AddFontFromFileTTF("C:\\windows\\fonts\\simhei.ttf", 20.0f,
                                             NULL, io.Fonts->GetGlyphRangesChineseFull());
                // Imgui设置后端不要改变鼠标光标的形状和可见性
                io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

                // Imgui初始化
                ImGui_ImplWin32_Init(WindowTarget);
                ImGui_ImplDX11_Init(g_Device, g_Context);

                // 设置目标窗口的回调函数为新窗口回调函数
                // 返回目标窗口的回调函数句柄
                WindowProc =
                    (WNDPROC)SetWindowLongPtr(WindowTarget, GWLP_WNDPROC, (LONG_PTR)WindowProcNew);

                // 已初始化
                IS_INITIALIZED = true;
            }
            else
                return PresentFunc(pSwapChain, SyncInterval, Flags);
        }

        // Imgui新Frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 显示Imgui的Demo窗口
        ImGui::ShowDemoWindow();

        // Imgui结束Frame
        ImGui::EndFrame();

        // Imgui准备用于渲染的数据
        ImGui::Render();

        // 将上下文绑定到呈现视图区
        g_Context->OMSetRenderTargets(1, &g_RenderTargetView, NULL);

        // Imgui渲染
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // 执行并返回到原Present函数
        return PresentFunc(pSwapChain, SyncInterval, Flags);
    }

    // ResizeBuffersNew的新函数
    static HRESULT WINAPI IDXGISwapChainResizeBuffersNew(IDXGISwapChain *SwapChain,
                                                         UINT BufferCount,
                                                         UINT Width,
                                                         UINT Height,
                                                         DXGI_FORMAT NewFormat,
                                                         UINT SwapChainFlags)
    {

        //                                无注解
        if (g_RenderTargetView)
        {
            g_Context->OMSetRenderTargets(0, 0, 0);
            g_RenderTargetView->Release();
        }

        HRESULT ReurnResizeBuffersOriginalFun = ResizeBuffersFunc(SwapChain, BufferCount,
                                                                  Width,
                                                                  Height,
                                                                  NewFormat,
                                                                  SwapChainFlags);

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
};
