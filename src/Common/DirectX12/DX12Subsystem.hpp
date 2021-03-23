#pragma once

#include <Common/Win32Includes.hpp>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "d3dx12.h"

namespace DX12S
{
    class DX12Subsystem
    {
    public:
        DX12Subsystem();
        ~DX12Subsystem();
        bool Init();
        bool Finish();

    public:
        ComPtr<ID3D12CommandQueue> & GetDirectCommandQueue();

    private:
        void createAdapter();
        void createDevice();
        void enableGDL();
        ComPtr<ID3D12Device2> m_device;
        ComPtr<IDXGIAdapter4> m_dxgiAdapter4;
        ComPtr<ID3D12CommandQueue> m_directCommandQueue;
        ComPtr<IDXGIFactory4> m_dxgiFactory;

        bool m_isInitialized{false};
    };
};