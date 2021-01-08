#include "D3D12StuffCreator.hpp"
#include "GraphicsCommonStuff.hpp"
#include "../Helpers.hpp"
#include "../WindowsIncludes.hpp"

#include "d3dx12.h"

#include <cassert>

namespace dx12
{
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    {
        assert(g_Device);
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;

        ThrowIfFailed(g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

        return descriptorHeap;
    }

    ComPtr<ID3D12CommandQueue> CreateCommandQueue()
    {
        assert(g_Device);
        ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        ThrowIfFailed(g_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

        return d3d12CommandQueue;
    }

    ComPtr<ID3D12RootSignature> CreateRootSignature()
    {
        assert(g_Device);

        ComPtr<ID3D12RootSignature> rootSignature;
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        return rootSignature;
    }

    ComPtr<ID3D12Resource> CreateBuffer(const uint8_t* data, size_t sizeInBytes)
    {
        assert(g_Device);
        assert(data);

        ComPtr<ID3D12Resource> buffer;

        CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&buffer)));

        UINT8* pBufferDataBegin;
        //CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(buffer->Map(0, nullptr, reinterpret_cast<void**>(&pBufferDataBegin)));
        memcpy(pBufferDataBegin, data, sizeInBytes);
        buffer->Unmap(0, nullptr);
    }

    ComPtr<ID3D12PipelineState> CreatePipelineStateObject(
        ComPtr<ID3D12RootSignature> &rootSignature,
        ComPtr<ID3DBlob> vertexShader,
        ComPtr<ID3DBlob> pixelShader)
    {
        assert(g_Device);

        ComPtr<ID3D12PipelineState> pipelineState;
        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());

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
        ThrowIfFailed(g_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

        return pipelineState;
    }

    ComPtr<ID3DBlob> LoadShaderFromFile(LPCWSTR fileName)
    {
        assert(g_Device);
        UINT compileFlags{0};
        if (g_EnableDebugLayer) {
            compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        }
        ID3DBlob* error{ nullptr };

        ID3DBlob* shader = nullptr;
        try {
            ThrowIfFailed(D3DCompileFromFile(fileName, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &, &error));
            ThrowIfFailed(D3DCompileFromFile(fileName, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &g_PixelShader, &error));
        }
        catch (...) {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
    }


    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
    {
        assert(g_Device);
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ThrowIfFailed(g_Device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

        return commandAllocator;
    }

    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        assert(g_Device);
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ThrowIfFailed(g_Device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

        ThrowIfFailed(commandList->Close());

        return commandList;
    }
}