#pragma once

#include <Common/IApplication.hpp>

class Editor final: public IApplication
{
// public API
public:
    Editor();
    ~Editor();

    bool Init(Systems systems) override;
    bool Finish() override;
    void Update() override;
    void WindowSizeChanged() override;
private:
    bool createSwapChain();

private:
    Systems m_systems;
};