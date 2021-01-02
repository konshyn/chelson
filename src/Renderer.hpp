#pragma once

#include <memory>

#include "WindowsIncludes.hpp"

struct Scene;
namespace renderer
{
    void Initialize(HWND hwnd, bool enableDebugLayer = false);

    void ResizeSwapChain(int width, int height);

    void SetVSync(bool vsync);
    bool IsVSync();
    void ToggleVSync();

    void SetFullscreen(bool fullscreen);
    bool IsFullscreen();
    void ToggleFullscreen();

    void ToggleTearing();

    void RequestExit();
    void Render(Scene *scene);
}

