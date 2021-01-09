#include "Common.hpp"
#include "../../GraphicsInterlayer.hpp"
#include "RenderPass.hpp"
#include "../../Scene.hpp"
#include "d3dx12.hpp"

#include <d3dcompiler.h>

#include <cassert>

namespace
{
    ComPtr<ID3D12GraphicsCommandList> g_CommandList;
    ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];

    ComPtr<ID3D12RootSignature> g_RootSignature;

    CD3DX12_VIEWPORT g_Viewport{};
    CD3DX12_RECT g_ScissorRect{};
    ComPtr<ID3D12PipelineState> g_PipelineState;

    ComPtr<ID3DBlob> g_VertexShader;
    ComPtr<ID3DBlob> g_PixelShader;
}

namespace
{
    void CreateRootSignature()
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(graphics::g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature)));
    }

    void LoadShaders()
    {
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        ID3DBlob* error{ nullptr };

        ID3DBlob* shader = nullptr;

        try {
            //HRESULT hr = D3DCompileFromFile(L"flat_color.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &g_VertexShader, &error);
            //if (FAILED(hr)) {
            //    std::cout << static_cast<const char *>(error->GetBufferPointer()) << std::endl;
            //}
            ThrowIfFailed(D3DCompileFromFile(L"flat_color.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &g_VertexShader, &error));
            ThrowIfFailed(D3DCompileFromFile(L"flat_color.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &g_PixelShader, &error));
        }
        catch (...) {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
    }

    void CreatePipelineStateObject()
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
        ThrowIfFailed(graphics::g_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_PipelineState)));
    }
}

namespace graphics
{
    RenderPass::RenderPass(ERenderTechnique, ELightingModel)
    {
        assert(g_Device);
        ComPtr<ID3D12GraphicsCommandList> g_CommandList;
        ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];

        for (int i = 0; i < g_NumFrames; ++i) {
            ThrowIfFailed(graphics::g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocators[i])));
        }

        ThrowIfFailed(g_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&g_CommandList)));

        ThrowIfFailed(g_CommandList->Close());

        g_Viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(g_ClientWidth), static_cast<float>(g_ClientHeight) };
        g_ScissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(g_ClientWidth), static_cast<LONG>(g_ClientHeight) };

        CreateRootSignature();
        LoadShaders();
        CreatePipelineStateObject();
    }

    void RenderPass::Render(Scene *scene)
    {
        auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
        commandAllocator->Reset();
        g_CommandList->Reset(commandAllocator.Get(), nullptr);

        AcquireBackbuffer(g_CommandList);

        

        // Clear the render target.
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                backBuffer.Get(),
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            g_CommandList->ResourceBarrier(1, &barrier);

            FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                g_CurrentBackBufferIndex, g_RTVDescriptorSize);

            g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        }

        // Present
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                backBuffer.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
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

            g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

            WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
        }
    }
}