#include "Common.hpp"

#include "../../Helpers.hpp"

#include <cassert>

namespace graphics
{
    ComPtr<ID3D12Fence> CreateFence(UINT64 initialValue /* = 0 */)
    {
        assert(g_Device);
        ComPtr<ID3D12Fence> fence;

        ThrowIfFailed(g_Device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        return fence;
    }

    HANDLE CreateEventHandle(LPCWSTR name /* = nullptr */)
    {
        HANDLE fenceEvent;

        fenceEvent = ::CreateEventW(NULL, FALSE, FALSE, name);
        assert(fenceEvent && "Failed to create fence event.");

        return fenceEvent;
    }

    uint64_t Signal(ComPtr<ID3D12Fence> fence, uint64_t &fenceValue)
    {
        uint64_t fenceValueForSignal = ++fenceValue;
        ThrowIfFailed(g_DirectCommandQueue->Signal(fence.Get(), fenceValueForSignal));

        return fenceValueForSignal;
    }

    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration /* = std::chrono::milliseconds::max() */)
    {
        if (fence->GetCompletedValue() < fenceValue) {
            ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
            ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
        }
    }
}