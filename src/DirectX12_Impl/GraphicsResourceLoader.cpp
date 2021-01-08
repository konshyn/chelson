#pragma once

#include <cassert>

#include "../GraphicsAPI.hpp"
#include "../BulkDataContainer.hpp"

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

    BulkDataContainer<VertexBuffer> g_VertexBuffers;
    BulkDataContainer<IndexBuffer> g_IndexBuffers;
    BulkDataContainer<RenderItem> g_RenderItems;
}

namespace graphics
{
    // creating resources
    VertexBufferID CreateVertexBuffer(const uint8_t* data, size_t sizeInBytes, size_t strideInBytes)
    {
    
    }

    IndexBufferID CreateIndexBuffer(const uint8_t* data, size_t numIndicies, DXGI_FORMAT indexFormat)
    {

    }

    uint32_t CreateRenderItem(VertexPositionDesc& desc)
    {

    }

    uint32_t CreateRenderItem(VertexPositionTextureDesc& desc) { assert(false); }
    uint32_t CreateRenderItem(VertexPositionNormalDesc& desc) { assert(false); }
    uint32_t CreateRenderItem(VertexPositionTextureNormalDesc& desc) { assert(false); }
    uint32_t CreateRenderItem(VertexPositionTextureNormalTangentBitangentDesc& desc) { assert(false); }
}