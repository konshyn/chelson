#pragma once

class Application;

class Subsystem
{
public:
    virtual void Initialize(const Application &app) = 0;
    virtual void Finitialize() = 0;
    virtual void Suspend() = 0;
    virtual void Resume() = 0;
    virtual void RequestExit() = 0;
};