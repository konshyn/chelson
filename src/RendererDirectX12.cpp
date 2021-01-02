#include "Renderer.hpp"
#include "Helpers.hpp"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>


// D3D12 extension library.
#include "d3dx12.h"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "Scene.hpp"

using namespace renderer;

namespace //global render state
{
    bool g_IsInitialized{false};
    bool g_EnableDebugLayer{false};
    bool g_VSync{true};
    bool g_TearingSupported{ false };
    bool g_Fullscreen{ false };

    constexpr uint8_t g_NumFrames{3};

    HWND g_HWND{};
    uint32_t g_ClientWidth{};
    uint32_t g_ClientHeight{};
    // Window rectangle (used to toggle fullscreen state).
    RECT g_WindowRect{};
    
    ComPtr<ID3D12Device2> g_Device;
    ComPtr<ID3D12CommandQueue> g_CommandQueue;
    ComPtr<IDXGISwapChain4> g_SwapChain;
    ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
    ComPtr<ID3D12GraphicsCommandList> g_CommandList;
    ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
    ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
    UINT g_RTVDescriptorSize;
    UINT g_CurrentBackBufferIndex;

    ComPtr<ID3D12Fence> g_Fence;
    uint64_t g_FenceValue = 0;
    uint64_t g_FrameFenceValues[g_NumFrames] {};
    HANDLE g_FenceEvent;

    ComPtr<ID3D12RootSignature> g_RootSignature;

    CD3DX12_VIEWPORT g_Viewport{};
    CD3DX12_RECT g_ScissorRect{};
    ComPtr<ID3D12PipelineState> g_PipelineState;

    ComPtr<ID3DBlob> g_VertexShader;
    ComPtr<ID3DBlob> g_PixelShader;

    ComPtr<ID3D12Resource> g_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView;


////////////////////////////////////////
    ComPtr<IDXGIAdapter4> GetAdapter()
    {
        ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
        if (g_EnableDebugLayer) {
            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }

        ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        ComPtr<IDXGIAdapter1> dxgiAdapter1;
        ComPtr<IDXGIAdapter4> dxgiAdapter4;

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
                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }

        return dxgiAdapter4;
    }

    ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
    {
        ComPtr<ID3D12Device2> d3d12Device2;
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

        // Enable debug messages in debug mode.
        if (g_EnableDebugLayer) {
            ComPtr<ID3D12InfoQueue> pInfoQueue;
            if (SUCCEEDED(d3d12Device2.As(&pInfoQueue))) {
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

        return d3d12Device2;
    }

    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

        return d3d12CommandQueue;
    }

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
                if (FAILED(factory5->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
                    allowTearing = FALSE;
                }
            }
        }

        return allowTearing == TRUE;
    }

    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
    {
        ComPtr<IDXGISwapChain4> dxgiSwapChain4;
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
        swapChainDesc.BufferCount = bufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
            commandQueue.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

        return dxgiSwapChain4;
    }

    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    {
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;

        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

        return descriptorHeap;
    }

    void CreateRootSignature(ComPtr<ID3D12Device2> device)
    {
        //// Individual uniforms
        //D3D12_DESCRIPTOR_RANGE1 ranges[1];
        //ranges[0].BaseShaderRegister = 0;
        //ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        //ranges[0].NumDescriptors = 1;
        //ranges[0].RegisterSpace = 0;
        //ranges[0].OffsetInDescriptorsFromTableStart = 0;
        //ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

        ////Groups of Uniforms
        //D3D12_ROOT_PARAMETER1 rootParameters[1];
        //rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        //rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        //rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
        //rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

        //// Overall layout
        //D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        //rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        //rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        //rootSignatureDesc.Desc_1_1.NumParameters = 1;
        //rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
        //rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
        //rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

        //ID3DBlob* signature;
        //ID3DBlob* error;

        //// Create the root signature
        //ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
        //ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature)));
        //g_RootSignature->SetName(L"Hello Triangle Root Signature");


        //if (signature) {
        //    signature->Release();
        //    signature = nullptr;
        //}

        {
            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
            ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature)));
        }
    }

    void CreateVertexBuffer(ComPtr<ID3D12Device2> device)
    {
        // Define the geometry for a triangle.
        std::vector<DirectX::XMFLOAT3> triangleVertices;
        triangleVertices.emplace_back(0.0f, 0.0f, 0.5f);
        triangleVertices.emplace_back(1.0f, 0.0f, 0.5f);
        triangleVertices.emplace_back(0.0f, 1.0f, 0.5f);

        const UINT vertexBufferSize = sizeof(DirectX::XMFLOAT3) * 3;

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_UPLOAD};
        CD3DX12_RESOURCE_DESC resourceDesc =  CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_VertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(g_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices.data(), vertexBufferSize);
        g_VertexBuffer->Unmap(0, nullptr);
        //g_VertexBuffer->SetName(L"Vertex Buffer Triangle");

        // Initialize the vertex buffer view.
        g_VertexBufferView.BufferLocation = g_VertexBuffer->GetGPUVirtualAddress();
        g_VertexBufferView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
        g_VertexBufferView.SizeInBytes = vertexBufferSize;
    }

    void CreatePipelineStateObject(ComPtr<ID3D12Device2> device)
    {
        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = g_RootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(g_VertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(g_PixelShader.Get());

        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        rasterizerDesc.FrontCounterClockwise = TRUE;
        psoDesc.RasterizerState = rasterizerDesc;

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_PipelineState)));
     }

    void LoadShaders(ComPtr<ID3D12Device2> device)
    {
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        ID3DBlob* error{nullptr};

        ID3DBlob *shader = nullptr;
        try {
            //HRESULT hr = D3DCompileFromFile(L"flat_color.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &g_VertexShader, &error);
            //if (FAILED(hr)) {
            //    std::cout << static_cast<const char *>(error->GetBufferPointer()) << std::endl;
            //}
            ThrowIfFailed(D3DCompileFromFile(L"flat_color.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &g_VertexShader, &error));
            ThrowIfFailed(D3DCompileFromFile(L"flat_color.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &g_PixelShader, &error));
        } catch(...) {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }

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

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

        return commandAllocator;
    }

    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

        ThrowIfFailed(commandList->Close());

        return commandList;
    }

    ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
    {
        ComPtr<ID3D12Fence> fence;

        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        return fence;
    }

    HANDLE CreateEventHandle()
    {
        HANDLE fenceEvent;

        fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(fenceEvent && "Failed to create fence event.");

        return fenceEvent;
    }

    uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
    {
        uint64_t fenceValueForSignal = ++fenceValue;
        ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

        return fenceValueForSignal;
    }

    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max())
    {
        if (fence->GetCompletedValue() < fenceValue) {
            ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
            ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
        }
    }

    void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent) 
    {
        uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
        WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
    }

    void EnableDebugLayer()
    {
        ComPtr<ID3D12Debug> debugInterface;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
        g_EnableDebugLayer = true;
    }
} // namespace


namespace renderer
{
    void Initialize(HWND hwnd, bool enableDebugLayer /* = false */)
    {
        g_HWND = hwnd;
        if (enableDebugLayer) {
            EnableDebugLayer();
        }

        EnableDebugLayer();

        g_TearingSupported = CheckTearingSupport();

        // Initialize the global window rect variable.
        ::GetWindowRect(g_HWND, &g_WindowRect);

        ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter();

        g_Device = CreateDevice(dxgiAdapter4);

        g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

        g_SwapChain = CreateSwapChain(g_HWND, g_CommandQueue, g_ClientWidth, g_ClientHeight, g_NumFrames);

        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

        g_RTVDescriptorHeap = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_NumFrames);
        g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);

        for (int i = 0; i < g_NumFrames; ++i) {
            g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        }
        g_CommandList = CreateCommandList(g_Device, g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

        g_Fence = CreateFence(g_Device);
        g_FenceEvent = CreateEventHandle();

        g_Viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(g_ClientWidth), static_cast<float>(g_ClientHeight) };
        g_ScissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(g_ClientWidth), static_cast<LONG>(g_ClientHeight) };

        CreateRootSignature(g_Device);
        CreateVertexBuffer(g_Device);
        LoadShaders(g_Device);
        CreatePipelineStateObject(g_Device);
        //g_CommandList = CreateCommandList(g_Device, g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
        g_IsInitialized = true;
    }

    void ResizeSwapChain(int width, int height)
    {
        if (g_ClientWidth != width || g_ClientHeight != height) {
            // Don't allow 0 size swap chain back buffers.
            g_ClientWidth = std::max(1, width);
            g_ClientHeight = std::max(1, height);

            g_Viewport = CD3DX12_VIEWPORT{0.0f, 0.0f, static_cast<float>(g_ClientWidth), static_cast<float>(g_ClientHeight)};
            g_ScissorRect = CD3DX12_RECT{0, 0, static_cast<LONG>(g_ClientWidth), static_cast<LONG>(g_ClientHeight)};
            //g_ScissorRect = CD3DX12_RECT{0, 0, 100, 100};

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

    void SetVSync(bool vsync)
    {
        g_VSync = vsync;
    }

    bool IsVSync()
    {
        return g_VSync;
    }

    void ToggleVSync()
    {
        g_VSync = !g_VSync;
    }

    void SetFullscreen(bool fullscreen)
    {
        if (g_Fullscreen != fullscreen) {
            g_Fullscreen = fullscreen;

            if (g_Fullscreen) { // Switching to fullscreen.
                // Store the current window dimensions so they can be restored 
                // when switching out of fullscreen state.
                ::GetWindowRect(g_HWND, &g_WindowRect);

                // Set the window style to a borderless window so the client area fills
                // the entire screen.
                UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

                ::SetWindowLongW(g_HWND, GWL_STYLE, windowStyle);

                // Query the name of the nearest display device for the window.
                // This is required to set the fullscreen dimensions of the window
                // when using a multi-monitor setup.
                HMONITOR hMonitor = ::MonitorFromWindow(g_HWND, MONITOR_DEFAULTTONEAREST);
                MONITORINFOEX monitorInfo = {};
                monitorInfo.cbSize = sizeof(MONITORINFOEX);
                ::GetMonitorInfo(hMonitor, &monitorInfo);

                ::SetWindowPos(g_HWND, HWND_TOPMOST,
                    monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.top,
                    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE);

                ::ShowWindow(g_HWND, SW_MAXIMIZE);
            } else {
                // Restore all the window decorators.
                ::SetWindowLong(g_HWND, GWL_STYLE, WS_OVERLAPPEDWINDOW);

                ::SetWindowPos(g_HWND, HWND_NOTOPMOST,
                    g_WindowRect.left,
                    g_WindowRect.top,
                    g_WindowRect.right - g_WindowRect.left,
                    g_WindowRect.bottom - g_WindowRect.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE);

                ::ShowWindow(g_HWND, SW_NORMAL);
            }
        }
    }

    bool IsFullscreen()
    {
        return g_Fullscreen;
    }

    void ToggleFullscreen()
    {
        SetFullscreen(!g_Fullscreen);
    }

    void ToggleTearing()
    {
        g_TearingSupported = !g_TearingSupported;
    }

    void Render(Scene *scene)
    {
        auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
        auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];

        commandAllocator->Reset();
        g_CommandList->Reset(commandAllocator.Get(), nullptr);

        {

            g_CommandList->SetPipelineState(g_PipelineState.Get());
            // Set necessary state.
            g_CommandList->SetGraphicsRootSignature(g_RootSignature.Get());
            g_CommandList->RSSetViewports(1, &g_Viewport);
            g_CommandList->RSSetScissorRects(1, &g_ScissorRect);

            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            g_CommandList->ResourceBarrier(1, &barrier);

            FLOAT clearColor[] = { 0.05f, 0.15f, 0.25f, 1.0f };
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_CurrentBackBufferIndex, g_RTVDescriptorSize);
            g_CommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
            g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);


            // Record commands.
            g_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            g_CommandList->IASetVertexBuffers(0, 1, &g_VertexBufferView);
            g_CommandList->DrawInstanced(3, 1, 0, 0);
        }

        // Present
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            g_CommandList->ResourceBarrier(1, &barrier);

            ThrowIfFailed(g_CommandList->Close());

            ID3D12CommandList* const commandLists[] = {
                g_CommandList.Get()
            };

            g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

            g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);

            UINT syncInterval = g_VSync ? 1 : 0;
            UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
            ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));

            UINT prevBackbufferIndex = g_CurrentBackBufferIndex;
            g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

            WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
        }
    }

    void RequestExit()
    {
        Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
    }
}