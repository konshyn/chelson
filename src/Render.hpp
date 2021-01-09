#pragma once

enum class ELightingModel
{
    Default = 0,
    Lambertian = Default,
    PBR,
    Cartoon
};

enum class ERenderTechnique
{
    Default = 0,
    Forward = Default,
    Deffered,
    ForwardPlus
};

class Scene;
namespace render
{
    void InitRender(ERenderTechnique renderTechnique, ELightingModel lightModel);
    void SetScene(Scene *scene);
    void Render();
    void RequestExit();
}
