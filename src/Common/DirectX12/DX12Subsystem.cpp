#include <Helpers/Helpers.hpp>
#include <Common/ConfigVars.hpp>

#include "DX12Subsystem.hpp"

static constexpr size_t NUM_BACKBUFFERS = 3;

namespace DX12S
{
    DX12Subsystem::DX12Subsystem()
    {}

    DX12Subsystem::~DX12Subsystem()
    {}

    void DX12Subsystem::createAdapter()
    {
        UINT createFactoryFlags = 0;
        if (CVar::NeedGraphicsDebugLayer) {
            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }

        ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));

        ComPtr<IDXGIAdapter1> dxgiAdapter1;

        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
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
                ThrowIfFailed(dxgiAdapter1.As(&m_dxgiAdapter4));
            }
        }
    }

    void DX12Subsystem::createDevice()
    {
        ThrowIfFailed(D3D12CreateDevice(m_dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

        // Enable debug messages in debug mode.
        if (CVar::NeedGraphicsDebugLayer) {
            ComPtr<ID3D12InfoQueue> pInfoQueue;
            if (SUCCEEDED(m_device.As(&pInfoQueue))) {
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

                //ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
            }
        }
    }

    void DX12Subsystem::enableGDL()
    {
        ComPtr<ID3D12Debug> debugInterface;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
    }

    bool DX12Subsystem::Init()
    {
        if (CVar::NeedGraphicsDebugLayer) {
            enableGDL();
        }

        createAdapter();
        createDevice();

        m_isInitialized = true;

        return true;
    }

    bool DX12Subsystem::Finish()
    {
        assert(m_isInitialized);
        return true;
    }

    // API
    ComPtr<ID3D12CommandQueue>& DX12Subsystem::GetDirectCommandQueue()
    {
        assert(m_isInitialized);
        if (!m_directCommandQueue) {
            D3D12_COMMAND_QUEUE_DESC desc;
            desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_directCommandQueue));
        }

        return m_directCommandQueue;
    }

    // API
    ComPtr<ID3D12CommandQueue>& DX12Subsystem::GetComputeCommandQueue()
    {
        assert(m_isInitialized);
        if (!m_computeCommandQueue) {
            D3D12_COMMAND_QUEUE_DESC desc;
            desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_computeCommandQueue));
        }

        return m_computeCommandQueue;
    }

    bool DX12Subsystem::checkTearingSupport()
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

    void DX12Subsystem::CreateSwapChain(HWND hwnd, UINT width, UINT height)
    {
        m_isTearingSupport = checkTearingSupport();

        ComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;

        if (CVar::NeedGraphicsDebugLayer) {
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
        swapChainDesc.BufferCount = NUM_BACKBUFFERS;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = m_isTearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        
        ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
            m_directCommandQueue.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &m_swapChain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

        // Create Descriptor Heap
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = NUM_BACKBUFFERS;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_RTVDescriptorHeap)));
        g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateRenderTargetViews();

        graphics::WaitForGPU();

        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
        return true;
        
    }
};