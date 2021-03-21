#pragma once

#include <Common/IApplication.hpp>

class Application final: public IApplication
{
// public API
public:
    Application();
    ~Application();

    bool Init(Systems systems) override;
    bool Finish() override;
    void Update() override;
    void WindowSizeChanged() override;

private:
    Systems m_systems;
};