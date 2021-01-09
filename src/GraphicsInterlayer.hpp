#pragma once

#include <d3d12.h>
#include <cstdint>

#include "WindowsIncludes.hpp"

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


namespace graphics
{
    // GraphicsDirectX12.cpp
    void InitGraphicsInterlayer(bool enableDebugLayer = false);
    void RequestExit();
    //----------------------------------

    // GraphicsSwapChain.cpp
    void InitSwapChain(HWND hwnd, int width, int height);
    void ResizeSwapChain(int width, int height);
    bool IsSupportTearing();
    void AllowTearing(bool value);
    void EnableVsync(bool value);
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