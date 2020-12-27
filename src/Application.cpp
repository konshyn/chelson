#include <algorithm>
#include <cassert>
#include <chrono>

#include "Application.hpp"
#include "Renderer.hpp"

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Application& app = Application::Instance();

    switch (message)
    {
        //case WM_SYSKEYDOWN:
    case WM_KEYDOWN: app.KeyDownEvent(wParam); break;
    case WM_SYSCHAR: break;
    case WM_SIZE: app.ResizeWindowEvent(); break;
    case WM_DESTROY: ::PostQuitMessage(0); break;
    default:
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}

void Application::CreateWindow(HINSTANCE hInst)
{
    const wchar_t *windowClassName = L"Chelson";

    auto registerWindowClass = [hInst, windowClassName] () -> void {
        // Register a window class for creating our render window with.
        WNDCLASSEXW windowClass = {};

        windowClass.cbSize = sizeof(WNDCLASSEX);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = &Application::WindowProc;
        windowClass.cbClsExtra = 0;
        windowClass.cbWndExtra = 0;
        windowClass.hInstance = hInst;
        windowClass.hIcon = ::LoadIconW(hInst, NULL);
        windowClass.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
        windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        windowClass.lpszMenuName = NULL;
        windowClass.lpszClassName = windowClassName;
        windowClass.hIconSm = ::LoadIconW(hInst, NULL);

        static ATOM atom = ::RegisterClassExW(&windowClass);
        //assert(atom > 0);
    }; 

    registerWindowClass();
        
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
    int desiredWidth = screenWidth * 2 / 3;
    int desiredHeight = screenHeight * 2 / 3;

    RECT windowRect = { 0, 0, static_cast<LONG>(desiredWidth), static_cast<LONG>(desiredHeight) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    m_windowWidth = windowRect.right - windowRect.left;
    m_windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int windowX = std::max<int>(0, (screenWidth - m_windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - m_windowHeight) / 2);
       
    m_hwnd = ::CreateWindowExW(
        NULL,
        windowClassName,
        windowClassName,
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        m_windowWidth,
        m_windowHeight,
        NULL,
        NULL,
        hInst,
        nullptr
    );

    assert(m_hwnd && "Failed to create window");
}

void Application::KeyDownEvent(WPARAM wParam)
{
    switch (wParam)
    {
    case 'V':
        renderer::ToggleVSync();
        break;
    //case 'T':
    //    renderer::ToggleTearing();
    //    break;
    case 'F': 
        renderer::ToggleFullscreen(); 
        break;
    case VK_ESCAPE: 
        ::PostQuitMessage(0); 
        break;
    default: 
        break;
    }
}

void Application::ResizeWindowEvent()
{
    RECT clientRect = {};
    ::GetClientRect(m_hwnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    renderer::ResizeSwapChain(width, height);

    //m_swapChain.Resize(width, height);
}

Application & Application::Instance()
{
    static Application app;
    return app;
}

bool Application::Initialize(HINSTANCE hInstance)
{
    bool result = true;
    
    CreateWindow(hInstance);
    assert(m_hwnd && "Failed to create window");

    renderer::Initialize(m_hwnd, false);

    m_isInitialize = result;

    ::ShowWindow(m_hwnd, SW_SHOW);

    return m_isInitialize;
}

void Application::Finitialize()
{
    
}

void Application::Run()
{
    assert(m_isInitialize && "Application is not initialized");

    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        Update();
        renderer::Render();
    }

    renderer::RequestExit();
}


void Application::Update()
{
    static uint64_t frameCounter = 0;
    static double elapsedSeconds = 0.0;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();

    frameCounter++;
    auto t1 = clock.now();
    auto deltaTime = t1 - t0;
    t0 = t1;

    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0) {
        wchar_t buffer[500];
        auto fps = frameCounter / elapsedSeconds;
        swprintf_s(buffer, 500, L"FPS: %f\n", fps);
        OutputDebugString(buffer);

        frameCounter = 0;
        elapsedSeconds = 0.0;
    }
}