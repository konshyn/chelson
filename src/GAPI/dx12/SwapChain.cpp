#include <cassert>

#include "../GraphicsAPI.hpp"
#include "GraphicsCommonStuff.hpp"

#include "../Helpers.hpp"

namespace
{
    bool g_IsSupportTearing{false};
    HWND g_HWND{};
    ComPtr<IDXGISwapChain4> g_SwapChain;
    ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
}

namespace
{
    bool CheckTearingSupport()
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

    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
    {
        auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; i < g_NumFrames; ++i) {
            ComPtr<ID3D12Resource> backBuffer;
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

            device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

            g_BackBuffers[i] = backBuffer;

            rtvHandle.Offset(rtvDescriptorSize);
        }
    }
}

namespace graphics
{
    void CreateSwapChain(HWND hwnd, int width, int height)
    {
        assert(g_Device);
        g_HWND = hwnd;
        g_ClientWidth = width;
        g_ClientHeight = height;
        g_IsSupportTearing = CheckTearingSupport();

        ComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;

        if (g_EnableDebugLayer) {
            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }

        ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = g_NumFrames;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = g_IsSupportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<ID3D12CommandQueue> commandQueue = CreateCommandQueue();

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
            commandQueue.Get(),
            g_HWND,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(g_HWND, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain1.As(&g_SwapChain));
    }

    void ResizeSwapChain(int width, int height)
    {
        if (g_ClientWidth != width || g_ClientHeight != height) {
            // Don't allow 0 size swap chain back buffers.
            g_ClientWidth = std::max(1, width);
            g_ClientHeight = std::max(1, height);

            g_Viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(g_ClientWidth), static_cast<float>(g_ClientHeight) };
            g_ScissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(g_ClientWidth), static_cast<LONG>(g_ClientHeight) };

            // Flush the GPU queue to make sure the swap chain's back buffers
            // are not being referenced by an in-flight command list.
            Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);

            for (int i = 0; i < g_NumFrames; ++i) {
                // Any references to the back buffers must be released
                // before the swap chain can be resized.
                g_BackBuffers[i].Reset();
                g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
            }

            DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
            ThrowIfFailed(g_SwapChain->GetDesc(&swapChainDesc));
            ThrowIfFailed(g_SwapChain->ResizeBuffers(g_NumFrames, g_ClientWidth, g_ClientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

            g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

            UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);
        }
    
    }

    bool IsSupportTearing()
    {
        return g_IsSupportTearing;
    
    }

    void GetFreeBackBuffer(ComPtr<ID3D12GraphicsCommandList>)
    {
    
    }

    void PrepareForPresent(ComPtr<ID3D12GraphicsCommandList>)
    {
    
    }

    void Present()
    {
    
    }
}