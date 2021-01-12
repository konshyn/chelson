#pragma once

#include <cassert>

#include "../../GraphicsInterlayer.hpp"

namespace
{
    struct VertexBuffer
    {
        ComPtr<ID3D12Resource> Resource;
        D3D12_VERTEX_BUFFER_VIEW View;
    };

    struct IndexBuffer
    {
        ComPtr<ID3D12Resource> Resource;
        D3D12_INDEX_BUFFER_VIEW View;
    };

    struct RenderItem
    {
    
    };
}

namespace graphics
{
    // creating resources
    VertexBufferID CreateVertexBuffer(const uint8_t* data, size_t sizeInBytes, size_t strideInBytes)
    {
        assert(false);
        return {};
    }

    IndexBufferID CreateIndexBuffer(const uint8_t* data, size_t numIndicies, DXGI_FORMAT indexFormat)
    {
        assert(false);
        return {};
    }

    uint32_t CreateRenderItem(VertexPositionDesc& desc)
    {
        assert(false);
        return {};
    }

    uint32_t CreateRenderItem(VertexPositionTextureDesc& desc) { assert(false); return{};}
    uint32_t CreateRenderItem(VertexPositionNormalDesc& desc) { assert(false);  return{};}
    uint32_t CreateRenderItem(VertexPositionTextureNormalDesc& desc) { assert(false);  return{};}
    uint32_t CreateRenderItem(VertexPositionTextureNormalTangentBitangentDesc& desc) { assert(false); return{};}
}