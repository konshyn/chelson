#pragma once

#include "WindowsIncludes.hpp"
#include <cstdint>

#include "Scene.hpp"

class Application
{
// public API
public:
    static Application & Instance();
    bool Initialize(HINSTANCE hInstance);
    void Finitialize();
    void Run();
    HWND GetHWND() { return m_hwnd; };

private:
    bool m_isInitialize{false};
    HWND m_hwnd{};
    RECT m_windowRect{};

    uint32_t m_windowWidth{};
    uint32_t m_windowHeight{};

    Scene scene;

    bool m_vsync{ true };
    bool m_allowTearing{ false };
    bool m_fullscreen{ false };

public:
    Application(const Application &) = delete;
    Application & operator=(const Application &) = delete;

private:
    Application() = default;
    ~Application() = default;

private:
    void KeyDownEvent(WPARAM wParam);
    void ResizeWindowEvent();

    void CreateWindow(HINSTANCE hInst);

    void Update();

    void SetVSync(bool vsync);
    bool IsVSync();
    void ToggleVSync();

    void SetFullscreen(bool fullscreen);
    bool IsFullscreen();
    void ToggleFullscreen();

    void ToggleTearing();

public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};