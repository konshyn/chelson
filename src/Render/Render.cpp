#include "../Render.hpp"
#include "../Scene.hpp"

#include <cassert>

#include "../GAPI/dx12/RenderPass.hpp"

#include <memory>

#include <iostream>

namespace 
{
    std::unique_ptr<graphics::RenderPass> g_RenderPass;
    Scene *g_Scene{nullptr};
}

namespace render
{
    void InitRender(ERenderTechnique renderTechnique, ELightingModel lightModel)
    {
        assert(renderTechnique == ERenderTechnique::Forward);
        assert(lightModel == ELightingModel::Lambertian);
        g_RenderPass.reset(new graphics::RenderPass(renderTechnique, lightModel));
    }
    
    void SetScene(Scene *scene)
    {
        assert(scene);
        g_Scene = scene;
    }

    void Render()
    {
        static int i = 0;
        ++i;
        wchar_t buffer[500];
        //swprintf_s(buffer, 500, L"i = %d\n", i);
        //OutputDebugString(buffer);
        g_RenderPass->Render(g_Scene);
        graphics::Present();
    }

    void RequestExit()
    {
        SetScene(nullptr);
    }
}
