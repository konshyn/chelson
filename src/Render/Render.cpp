#include "../Render.hpp"
#include "../Scene.hpp"

#include <cassert>

#include "../GAPI/dx12/RenderPass.hpp"

#include <memory>

namespace 
{
    std::unique_ptr<RenderPass> g_RenderPass;
    Scene *g_Scene{nullptr};
}

namespace render
{
    void InitRender(ERenderTechnique renderTechnique, ELightingModel lightModel)
    {
        assert(renderTechnique == ERenderTechnique::Forward);
        assert(lightModel == ELightingModel::Lambertian);
        g_RenderPass.reset(new RenderPass(renderTechnique, lightModel));
    }
    
    void SetScene(Scene *scene)
    {
        assert(scene);
        g_Scene = scene;
    }

    void Render()
    {
        g_RenderPass->Render(g_Scene);
    }

    void RequestExit()
    {
        SetScene(nullptr);
    }

}
