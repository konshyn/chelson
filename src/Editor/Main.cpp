#include <Common/Win32System.hpp>
#include "Application.hpp"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Win32OS::Win32System win32;
    Win32OS::DescWin32 desc;
    Application app;
    
    desc.DX12SubsystemDebug = true;
    desc.hInst = hInstance;
    desc.windowTitle = L"Chelson Editor";

    win32.Init(desc);
    win32.Run(app);
    win32.Finish();


    return S_OK;
}