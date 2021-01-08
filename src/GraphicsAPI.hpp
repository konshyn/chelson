#pragma once

#include <memory>
#include "WindowsIncludes.hpp"
#include "GraphicsVertexTypes.hpp"

#include <d3d12.h>

struct Scene;
class ID3D12Resource;
class ID3D12GraphicsCommandList;

namespace graphics
{
    // GraphicsDirectX12.cpp
    void InitGPU(bool enableDebugLayer = false);
    void RequestExit();
    //----------------------------------

    // Renderer.cpp
    void InitRenderer();
    void Render(Scene* scene);
    void Flush();
    //----------------------------------

    // GraphicsSwapChain.cpp
    void CreateSwapChain(HWND hwnd, int width, int height);
    void ResizeSwapChain(int width, int height);
    bool IsSupportTearing();
    void GetFreeBackBuffer(ComPtr<ID3D12GraphicsCommandList>);
    void PrepareForPresent(ComPtr<ID3D12GraphicsCommandList>);
    void Present();
    //----------------------------------

    // GraphicsResourceLoader.cpp
    VertexBufferID CreateVertexBuffer(const uint8_t *data, size_t sizeInBytes, size_t strideInBytes);
    IndexBufferID CreateIndexBuffer(const uint8_t *data, size_t numIndicies, DXGI_FORMAT indexFormat);
    uint32_t CreateRenderItem(VertexPositionDesc&);
    uint32_t CreateRenderItem(VertexPositionTextureDesc&);
    uint32_t CreateRenderItem(VertexPositionNormalDesc&);
    uint32_t CreateRenderItem(VertexPositionTextureNormalDesc&);
    uint32_t CreateRenderItem(VertexPositionTextureNormalTangentBitangentDesc&);
    //----------------------------------
}