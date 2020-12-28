#include <cassert>
#include "Scene.hpp"

void FillTriangleScene(Scene &scene)
{
    scene = {};
    scene.mesh.vertices.emplace_back(0.0f, 0.0f, 1.0f);
    scene.mesh.vertices.emplace_back(1.0f, 0.0f, 1.0f);
    scene.mesh.vertices.emplace_back(0.0f, 1.0f, 1.0f);
}

void FillSponzaScene(Scene &scene)
{
    assert(false);
}