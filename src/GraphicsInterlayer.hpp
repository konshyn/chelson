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
    void InitGraphicsInterlayer(bool enableDebugLayer = false);
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

#pragma once

#include <cstdint>

using VertexBufferID = uint32_t;
using IndexBufferID = uint32_t;

struct VertexPositionDesc
{
    VertexBufferID Positions;
    IndexBufferID Indecies;
};

struct VertexPositionTextureDesc
{
    VertexBufferID Positions;
    VertexBufferID TexCoordinates;
    IndexBufferID Indecies;
};

struct VertexPositionNormalDesc
{
    VertexBufferID Positions;
    VertexBufferID Normals;
    IndexBufferID Indecies;
};

struct VertexPositionTextureNormalDesc
{
    VertexBufferID Positions;
    VertexBufferID TexCoordinates;
    VertexBufferID Normals;
    IndexBufferID Indecies;
};

struct VertexPositionTextureNormalTangentBitangentDesc
{
    VertexBufferID Positions;
    VertexBufferID TexCoordinates;
    VertexBufferID Normals;
    VertexBufferID Tangent;
    VertexBufferID Bitangent;
    IndexBufferID Indecies;
};