#include "Application.hpp"

#include <utility>
#include <iostream>

Application::Application()
{

}

Application::~Application()
{

}

bool Application::Init(Systems systems)
{
    m_systems = std::move(systems);

    return true;
}

bool Application::Finish()
{
    return true;
}

void Application::Update()
{
    std::cout << "Application::Update" << std::endl;
}

void Application::WindowSizeChanged()
{
    int width, height;
    m_systems.win32->GetWindowSize(width, height);
}
