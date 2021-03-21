#include "ResourceManager.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <external/ObjLoader/ObjLoader.h>

using namespace Resources::CPU;

bool Resources::CPU::LoadSponzaShape(SponzaShape &sponza)
{
    sponza = SponzaShape{};
    objl::Loader objLoader;

    bool loadout = objLoader.LoadFile("assets/sponza/sponza.obj");

    if (loadout) {

        for (int i = 0; i < objLoader.LoadedMeshes.size(); i++) {
            // Copy one of the loaded meshes to be our current mesh
            objl::Mesh curMesh = objLoader.LoadedMeshes[i];
            SponzaShape::Shape curShape;
            curShape.name = curMesh.MeshName;
            for (int j = 0; j < curMesh.Vertices.size(); ++j) {
                curShape.positions.push_back(curMesh.Vertices[j].Position.X);
                curShape.positions.push_back(curMesh.Vertices[j].Position.Y);
                curShape.positions.push_back(curMesh.Vertices[j].Position.Z);

                curShape.normals.push_back(curMesh.Vertices[j].Normal.X);
                curShape.normals.push_back(curMesh.Vertices[j].Normal.Y);
                curShape.normals.push_back(curMesh.Vertices[j].Normal.Z);
            }

            for (int j = 0; j < curMesh.Indices.size(); ++j) {
                curShape.indicies.push_back(curMesh.Indices[j]);
            }

            sponza.shapes.push_back(curShape);
        }
    }

    return loadout;
}