#pragma once

#include "Common.hpp"
#include "../../GraphicsInterlayer.hpp"
#include "../../Render.hpp"
#include "../../Helpers.hpp"

struct Scene;
namespace graphics
{
    class RenderPass
    {
    public:
        RenderPass(ERenderTechnique, ELightingModel);
        ~RenderPass() = default;

        void Render(Scene* scene);

    private:
    };
}