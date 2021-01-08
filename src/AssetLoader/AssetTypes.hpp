#pragma once

#include <memory>

#include <d3d12.h>

struct RawVertexBuffer
{
    std::unique_ptr<uint8_t[]> Data;
    uint32_t Stride;
};

struct RawMeshDataSoA
{
    RawVertexBuffer Positions;
    RawVertexBuffer TextureCoordinates;
    RawVertexBuffer Normals;
    RawVertexBuffer Tangents;
    RawVertexBuffer Bitangents;
    RawVertexBuffer Indecies;
    DXGI_FORMAT IndexFormat{DXGI_FORMAT_UNKNOWN};
    D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology{D3D_PRIMITIVE_TOPOLOGY_UNDEFINED};
};