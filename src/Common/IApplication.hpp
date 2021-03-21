#pragma once

#include "Win32System.hpp"
#include "DX12Subsystem.hpp"
#include "EventSubsystem.hpp"


//class Win32OS::Win32System;
//class DX12S::DX12Subsystem;
//class EventS::EventSubsystem;
struct Systems
{
    Win32OS::Win32System* win32;
    DX12S::DX12Subsystem* dx12;
    EventS::EventSubsystem* events;
};

class IApplication
{
public:  
    virtual bool Init(Systems systems) = 0;
    virtual bool Finish() = 0;
    virtual void Update() = 0;
    virtual void WindowSizeChanged() = 0;
};