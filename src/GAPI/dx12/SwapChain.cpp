#include <cassert>

#include "../../GraphicsInterlayer.hpp"
#include "Common.hpp"

#include "../../Helpers.hpp"

#include "d3dx12.hpp"

namespace
{
    bool g_IsSupportTearing{false};
    bool g_AllowTearing{false};
    bool g_Vsync{true};
    HWND g_HWND{};
    ComPtr<IDXGISwapChain4> g_SwapChain;
    ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
    ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
    UINT g_RTVDescriptorSize;

    ComPtr<ID3D12Fence> g_Fence;
    uint64_t g_FenceValue = 0;
    uint64_t g_FrameFenceValues[g_NumFrames]{};
    HANDLE g_FenceEvent;
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

    void UpdateRenderTargetViews()
    {
        assert(graphics::g_Device);
        assert(g_SwapChain);
        assert(g_RTVDescriptorHeap);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; i < g_NumFrames; ++i) {
            ComPtr<ID3D12Resource> backBuffer;
            ThrowIfFailed(g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

            g_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

            g_BackBuffers[i] = backBuffer;

            rtvHandle.Offset(g_RTVDescriptorSize);
        }
    }

    void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
    {
        uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
        WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
    }
}

namespace graphics
{
    void InitSwapChain(HWND hwnd, int width, int height)
    {
        assert(g_Device);
        assert(g_DirectCommandQueue);
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

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
            g_DirectCommandQueue.Get(),
            g_HWND,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(g_HWND, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain1.As(&g_SwapChain));

        // Create Descriptor Heap
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = g_NumFrames;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        ThrowIfFailed(g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_RTVDescriptorHeap)));
        g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateRenderTargetViews();

        g_Fence = CreateFence();
        g_FenceEvent = CreateEventHandle(L"SwapChainEvent");
    }

    void ResizeSwapChain(int width, int height)
    {
        if (g_ClientWidth != width || g_ClientHeight != height) {
            // Don't allow 0 size swap chain back buffers.
            g_ClientWidth = std::max(1, width);
            g_ClientHeight = std::max(1, height);

            // Flush the GPU queue to make sure the swap chain's back buffers
            // are not being referenced by an in-flight command list.
            Flush();
            
            UINT currentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
            for (int i = 0; i < g_NumFrames; ++i) {
                // Any references to the back buffers must be released
                // before the swap chain can be resized.
                g_BackBuffers[i].Reset();
                g_FrameFenceValues[i] = g_FrameFenceValues[currentBackBufferIndex];
            }

            DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
            ThrowIfFailed(g_SwapChain->GetDesc(&swapChainDesc));
            ThrowIfFailed(g_SwapChain->ResizeBuffers(g_NumFrames, g_ClientWidth, g_ClientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

            UpdateRenderTargetViews();
        }
    
    }

    void Present(ComPtr<ID3D12GraphicsCommandList> commandList)
    {
        g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);

        UINT syncInterval = g_Vsync ? 1 : 0;
        UINT presentFlags = g_AllowTearing && !g_Vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));

        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

        WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
    
    }

    void AcquireBackbuffer(ComPtr<ID3D12GraphicsCommandList> commandList)
    {
        WaitForFenceValue(g_Fence, g_FrameFenceValues[g_SwapChain->GetCurrentBackBufferIndex()], g_FenceEvent);
        auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ResourceBarrier(1, &barrier);
    }

    void ReleaseBackbuffer(ComPtr<ID3D12GraphicsCommandList> commandList)
    {
        auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);
    }

    void ClearCurrentBackbuffer(ComPtr<ID3D12GraphicsCommandList> commandList, FLOAT *color)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_CurrentBackBufferIndex, g_RTVDescriptorSize);
        commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
    }


    bool IsSupportTearing()
    {
        return g_IsSupportTearing;
    }

    void AllowTearing(bool value)
    {
        g_AllowTearing = value;
    }

    void EnableVsync(bool value)
    {
        g_Vsync = value;
    }
}