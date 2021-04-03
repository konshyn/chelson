#include "Editor.hpp"

#include <utility>
#include <iostream>

Editor::Editor()
{

}

Editor::~Editor()
{

}

bool Editor::Init(Systems systems)
{
    m_systems = std::move(systems);
    m_systems.dx12->CreateSwapChain();


    return true;
}

bool Editor::Finish()
{
    return true;
}

void Editor::Update()
{
    std::cout << "Editor::Update" << std::endl;
}

void Editor::WindowSizeChanged()
{
    int width, height;
    m_systems.win32->GetWindowSize(width, height);
}
