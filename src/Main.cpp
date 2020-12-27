#include "Application.hpp"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Application &app = Application::Instance();

    bool res = app.Initialize(hInstance);

    if (!res) {
        return S_FALSE;
    }

    app.Run();

    app.Finitialize();

    return S_OK;
}