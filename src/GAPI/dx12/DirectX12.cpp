#include "Common.hpp"

#include <DirectXMath.h>
#include <algorithm>
#include <chrono>
#include <iostream>

#include "../../GraphicsInterlayer.hpp"
#include "../../Helpers.hpp"

//// D3D12 extension library.
//#include "d3dx12.hpp"

namespace graphics
{
    bool g_EnableDebugLayer;
    ComPtr<ID3D12Device2> g_Device;
    ComPtr<IDXGIAdapter4> g_DXGIAdapter4;
    ComPtr<ID3D12CommandQueue> g_DirectCommandQueue;
}

namespace
{
    ComPtr<ID3D12Fence> g_Fence;
    HANDLE g_FenceEvent;
    uint64_t g_CurrentFenceValue{0};
}

namespace 
{
////////////////////////////////////////
    void CreateAdapter()
    {
        ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
        if (graphics::g_EnableDebugLayer) {
            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }

        ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        ComPtr<IDXGIAdapter1> dxgiAdapter1;

        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            // Check to see if the adapter can create a D3D12 device without actually 
            // creating it. The adapter with the largest dedicated video memory
            // is favored.
            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ThrowIfFailed(dxgiAdapter1.As(&graphics::g_DXGIAdapter4));
            }
        }
    }

    void CreateDevice()
    {
        assert(graphics::g_DXGIAdapter4);
        ThrowIfFailed(D3D12CreateDevice(graphics::g_DXGIAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&graphics::g_Device)));

        // Enable debug messages in debug mode.
        if (graphics::g_EnableDebugLayer) {
            ComPtr<ID3D12InfoQueue> pInfoQueue;
            if (SUCCEEDED(graphics::g_Device.As(&pInfoQueue))) {
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

                // Suppress whole categories of messages
                //D3D12_MESSAGE_CATEGORY Categories[] = {};

                // Suppress messages based on their severity level
                D3D12_MESSAGE_SEVERITY Severities[] = {
                    D3D12_MESSAGE_SEVERITY_INFO
                };

                // Suppress individual messages by their ID
                D3D12_MESSAGE_ID DenyIds[] = {
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
                    D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
                };

                D3D12_INFO_QUEUE_FILTER NewFilter = {};
                //NewFilter.DenyList.NumCategories = _countof(Categories);
                //NewFilter.DenyList.pCategoryList = Categories;
                NewFilter.DenyList.NumSeverities = _countof(Severities);
                NewFilter.DenyList.pSeverityList = Severities;
                NewFilter.DenyList.NumIDs = _countof(DenyIds);
                NewFilter.DenyList.pIDList = DenyIds;

                ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
            }
        }
    }

    void EnableDebugLayer()
    {
        ComPtr<ID3D12Debug> debugInterface;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
        graphics::g_EnableDebugLayer = true;
    }
} // namespace


namespace graphics
{
    void InitGraphicsInterlayer(bool enableDebugLayer /* = false */)
    {
        if (enableDebugLayer) {
            EnableDebugLayer();
        }

        CreateAdapter();
        CreateDevice();

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        ThrowIfFailed(g_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_DirectCommandQueue)));

        g_Fence = CreateFence();
        g_FenceEvent = CreateEventHandle(L"GlobalFenceEvent");
        g_CurrentFenceValue = g_Fence->GetCompletedValue();
    }
    
    void RequestExit()
    {
        WaitForGPU();
        // TODO clean up
    }

    void WaitForGPU()
    {
        WaitForFenceValue(Signal());
    }

    uint64_t Signal()
    {
        g_CurrentFenceValue++;
        ThrowIfFailed(g_DirectCommandQueue->Signal(g_Fence.Get(), g_CurrentFenceValue));
        return g_CurrentFenceValue;
    }

    void WaitForFenceValue(uint64_t fenceValue, std::chrono::milliseconds duration /* = std::chrono::milliseconds::max() */)
    {
        if (g_Fence->GetCompletedValue() < fenceValue) {
            ThrowIfFailed(g_Fence->SetEventOnCompletion(fenceValue, g_FenceEvent));
            ::WaitForSingleObject(g_FenceEvent, static_cast<DWORD>(duration.count()));
        }
    }
}