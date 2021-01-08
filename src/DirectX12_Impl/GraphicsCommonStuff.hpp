#pragma once

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>

#include "../WindowsIncludes.hpp"

// GraphicsDirectX12.cpp
extern bool g_EnableDebugLayer;
extern ComPtr<ID3D12Device2> g_Device;
extern ComPtr<IDXGIAdapter4> g_dxgiAdapter4;

// GraphicsSwapChain.cpp
extern uint32_t g_ClientWidth;
extern uint32_t g_ClientHeight;

constexpr uint8_t g_NumFrames{ 3 };