#pragma once

#include "WindowsIncludes.hpp"
#include <cstdint>

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

public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};