#include "GraphicsCommonStuff.hpp"


#include <DirectXMath.h>
#include <algorithm>
#include <chrono>
#include <iostream>

#include "../GraphicsAPI.hpp"
#include "../Helpers.hpp"

#include "../Scene.hpp"

// D3D12 extension library.
#include "d3dx12.h"

namespace //global render state
{
    ComPtr<ID3D12CommandQueue> g_CommandQueue;

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
}

namespace 
{
////////////////////////////////////////
    void CreateAdapter()
    {
        ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
        if (g_EnableDebugLayer) {
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
                ThrowIfFailed(dxgiAdapter1.As(&g_dxgiAdapter4));
            }
        }
    }

    void CreateDevice()
    {
        assert(g_dxgiAdapter4);
        ThrowIfFailed(D3D12CreateDevice(g_dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_Device)));

        // Enable debug messages in debug mode.
        if (g_EnableDebugLayer) {
            ComPtr<ID3D12InfoQueue> pInfoQueue;
            if (SUCCEEDED(g_Device.As(&pInfoQueue))) {
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



    void EnableDebugLayer()
    {
        ComPtr<ID3D12Debug> debugInterface;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
        g_EnableDebugLayer = true;
    }
} // namespace


namespace graphics
{
    void InitGPU(bool enableDebugLayer /* = false */)
    {
        if (enableDebugLayer) {
            EnableDebugLayer();
        }

        CreateAdapter();
        CreateDevice();

        g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

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
    }
    
    void RequestExit()
    {
        Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
    }


}