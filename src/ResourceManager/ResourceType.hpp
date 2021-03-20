#pragma once

#include <memory>
#include <vector>
#include <string>

namespace Resources::CPU
{    
    struct RawData
    {
        std::unique_ptr<uint8_t[]> Data;
    };

    struct SponzaShape
    {
        struct Shape
        {
            std::vector<float> positions;
            std::vector<float> normals;
            std::vector<unsigned int> indicies;
            std::string name;
        };

        std::vector<Shape> shapes;
    };
};