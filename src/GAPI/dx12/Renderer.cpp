#include "GraphicsCommonStuff.hpp"

#include "../GraphicsAPI.hpp"

namespace
{

}

namespace graphics
{
    void InitRenderer()
    {

    }

    void Render(Scene* scene)
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

    void Flush()
    {
        uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
        WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
    }
}