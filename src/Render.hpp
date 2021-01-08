#pragma once

enum class ELightingType
{
    Default = 0,
    Lambertian = Default,
    PBR,
    Cartoon
};

enum class ERenderPassType
{
    Default = 0,
    Forward = Default,
    Deffered,
    ForwardPlus
};

class Scene;
namespace render
{
    void InitRender(ERenderPassType rptype, ELightingType ltype);
    void SetScene(Scene *scene);
    void Render();
    void RequestExit();
}
