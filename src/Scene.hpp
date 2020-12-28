#pragma once

#include "Camera.hpp"
#include "Mesh.hpp"

struct Scene
{
    Camera camera{};
    Mesh mesh{};
};

void FillTriangleScene(Scene &scene);
void FillSponzaScene(Scene &scene);