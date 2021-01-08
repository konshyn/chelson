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