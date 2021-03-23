#include "Win32System.hpp"
#include "IApplication.hpp"
#include "ApplicationSettings.hpp"

#include <Helpers/Helpers.hpp>

#include <algorithm>

static constexpr int NUM_FRAMES = 3;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Win32OS
{
    Win32System::Win32System()
        : m_hwnd{0}
    {
        // Init in LogSystem
        // Etc
    }

    Win32System::~Win32System()
    {
        Finish();
    }

    bool Win32System::Init(DescWin32& desc)
    {
        if (!createWindow(desc)) {
            return false;
        }

        if (!initDX12Subsystem(desc)) {
            return false;
        }

        if (!initEventSubsystem(desc)) {
            return false;
        }

        ::ShowWindow(m_hwnd, SW_SHOWDEFAULT);

        m_isInitialized = true;

        return true;
    }

    void Win32System::Run(IApplication &app)
    {
        Systems systems{this, &m_dx12, &m_eventSubsystem};
        app.Init(systems);

        MSG msg{};
        while (msg.message != WM_QUIT) {
            if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }

            app.Update();
        }

        app.Finish();
    }

    bool Win32System::Finish()
    {
        if (m_isInitialized) {
            m_dx12.Finish();
            m_eventSubsystem.Finish();
            m_isInitialized = false;
        }
        return true;
    }

    bool Win32System::IsInitialized()
    {
        return m_isInitialized;
    }

    HWND Win32System::GetHWND()
    {
        return m_hwnd;
    }

    bool Win32System::IsTearingSupport()
    {
        m_isTearingSupport;
    }
    
    bool Win32System::IsFullscreen()
    {
        return m_isFullscreen;
    }

    void Win32System::GetWindowSize(int &width, int &height)
    {
        width = m_windowWidth;
        height = m_windowHeight;
    }

    bool Win32System::checkTearingSupport()
    {
        BOOL allowTearing = FALSE;

        // Rather than create the DXGI 1.5 factory interface directly, we create the
        // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
        // graphics debugging tools which will not support the 1.5 factory interface 
        // until a future update.
        ComPtr<IDXGIFactory4> factory4;
        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))) {
            ComPtr<IDXGIFactory5> factory5;
            if (SUCCEEDED(factory4.As(&factory5))) {
                if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
                    allowTearing = FALSE;
                }
            }
        }

        return allowTearing == TRUE;
    }

    bool Win32System::createSwapChain(DescWin32& desc)
    {
        m_isTearingSupport = checkTearingSupport();

        ComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;

        if (desc.DX12SubsystemDebug) {
            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }

        ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_windowWidth;
        swapChainDesc.Height = m_windowHeight;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = NUM_FRAMES;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = m_isTearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
            m_dx12.GetDirectCommandQueue().Get(),
            m_hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain1.As(&g_SwapChain));

        // Create Descriptor Heap
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = NUM_FRAMES;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        ThrowIfFailed(g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_RTVDescriptorHeap)));
        g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateRenderTargetViews();

        graphics::WaitForGPU();

        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
        return true;
    }

    bool Win32System::createWindow(DescWin32& desc)
    {
        auto registerWindowClass = [&desc]() -> void {
            // Register a window class for creating our render window with.
            WNDCLASSEXW windowClass = {};

            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = &WndProc;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = desc.hInst;
            windowClass.hIcon = ::LoadIconW(desc.hInst, NULL);
            windowClass.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
            windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            windowClass.lpszMenuName = NULL;
            windowClass.lpszClassName = desc.windowTitle;
            windowClass.hIconSm = ::LoadIconW(desc.hInst, NULL);

            static ATOM atom = ::RegisterClassExW(&windowClass);
        };

        registerWindowClass();

        int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
        int desiredWidth = screenWidth * 3 / 4;
        int desiredHeight = screenHeight * 3 / 4;

        RECT windowRect = { 0, 0, static_cast<LONG>(desiredWidth), static_cast<LONG>(desiredHeight) };
        ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        m_windowWidth = windowRect.right - windowRect.left;
        m_windowHeight = windowRect.bottom - windowRect.top;

        // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
        int windowX = std::max<int>(0, (screenWidth - m_windowWidth) / 2);
        int windowY = std::max<int>(0, (screenHeight - m_windowHeight) / 2);

        m_hwnd = ::CreateWindowExW(
            NULL,
            desc.windowTitle,
            desc.windowTitle,
            WS_OVERLAPPEDWINDOW,
            windowX,
            windowY,
            m_windowWidth,
            m_windowHeight,
            NULL,
            NULL,
            desc.hInst,
            nullptr
        );

        return true;
    }

    bool Win32System::initDX12Subsystem(DescWin32& desc)
    {
        if (!m_dx12.Init()) {
            return false;
        }

        return true;
    }

    bool Win32System::initEventSubsystem(DescWin32& desc)
    {
        if (!m_eventSubsystem.Init()) {
            return false;
        }

        return true;    
    }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        //if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        //{
        //    WaitForLastSubmittedFrame();
        //    ImGui_ImplDX12_InvalidateDeviceObjects();
        //    CleanupRenderTarget();
        //    ResizeSwapChain(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
        //    CreateRenderTarget();
        //    ImGui_ImplDX12_CreateDeviceObjects();
        //}
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}