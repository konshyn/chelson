#pragma once

#include <memory>

#include "WindowsIncludes.hpp"

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
    void RequestExit();
    void Render();
}