#include "AssetTypes.hpp"
#include <cassert>
#include "../Mathematics/Mathematics.hpp"
#include <vector>
#include <cstring>

using Vec3 = DirectX::XMFLOAT3;

RawMeshDataSoA LoadTriangleScene()
{
    RawMeshDataSoA mesh{};

    std::vector<Vec3> positions
    {
        Vec3(0.0f, 0.0f, 1.0f), 
        Vec3(1.0f, 0.0f, 1.0f), 
        Vec3(0.0f, 1.0f, 1.0f)
    };

    size_t size = positions.size() * sizeof(Vec3);
    mesh.Positions.Data.reset(new uint8_t[size]);
    std::memcpy(mesh.Positions.Data.get(), positions.data(), size);

    mesh.Positions.Stride = sizeof(Vec3);

    mesh.PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    return mesh;
}

RawMeshDataSoA LoadSponzaScene()
{
    assert(false);
    return{};
}