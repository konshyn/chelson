#pragma once

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <chrono>

#include "../../WindowsIncludes.hpp"

namespace graphics
{
    constexpr uint8_t g_NumFrames{ 3 };


    // DirectX12.cpp
    extern bool g_EnableDebugLayer;
    extern ComPtr<ID3D12Device2> g_Device;
    extern ComPtr<IDXGIAdapter4> g_DXGIAdapter4;
    extern ComPtr<ID3D12CommandQueue> g_DirectCommandQueue;

    // SwapChain.cpp
    extern uint32_t g_ClientWidth;
    extern uint32_t g_ClientHeight;
    extern UINT g_CurrentBackBufferIndex;
    void AcquireBackbuffer(ComPtr<ID3D12GraphicsCommandList> commandList);
    void ClearCurrentBackbuffer(ComPtr<ID3D12GraphicsCommandList> commandList, FLOAT *color);
    void Present(ComPtr<ID3D12GraphicsCommandList> commandList);

    ComPtr<ID3D12Fence> CreateFence(UINT64 initialValut = 0);
    HANDLE CreateEventHandle(LPCWSTR name = nullptr);
    uint64_t Signal(ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
}