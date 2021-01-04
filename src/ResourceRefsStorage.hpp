#pragma once

#include "d3dx12.h"

#include "WindowsIncludes.hpp"

#include "C++STL.hpp"

struct ResourceRefsStorage
{
    std::deque<ComPtr<ID3D12Resource>> vertexBuffers;
    std::deque<ComPtr<ID3D12Resource>> vertexBuffers;
    std::deque<ComPtr<ID3D12Resource>> vertexBuffers;
    std::deque<ComPtr<ID3D12Resource>> vertexBuffers;
    
};