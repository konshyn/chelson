#pragma once

struct Camera;
struct Item;

struct Scene
{
    Camera *camera{nullptr};
    Item *item{nullptr};
};