#pragma once

#include "Win32Includes.hpp"
#include "DX12Subsystem.hpp"
#include "EventSubsystem.hpp"

class IApplication;
namespace Win32OS
{
    struct DescWin32
    {
        HINSTANCE hInst;
        const wchar_t *windowTitle{nullptr};
        bool DX12SubsystemDebug{true};
    };

    class Win32System 
    {
    public:
        Win32System();
        ~Win32System();

        bool Init(DescWin32 &desc);
        bool Finish();
        void Run(IApplication &app);
        bool IsInitialized();
        HWND GetHWND();

    public:
        bool IsTearingSupport();
        bool IsFullscreen();
        void GetWindowSize(int &width, int &height);
        
    private:
        bool createWindow(DescWin32 &desc);
        bool createSwapChain(DescWin32 &desc);
        bool checkTearingSupport();
        bool initDX12Subsystem(DescWin32 &desc);
        bool initEventSubsystem(DescWin32 &desc);

        bool m_isInitialized{false};
        bool m_isFullscreen{false};
        
        DX12S::DX12Subsystem m_dx12;
        EventS::EventSubsystem m_eventSubsystem;

        HWND m_hwnd;
        int m_windowWidth{1280};
        int m_windowHeight{720};
        bool m_isTearingSupport{false};
    };
};