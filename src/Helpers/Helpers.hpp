#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT

#include <exception>

#include <cassert>

#define PRAGMA_STR1(x)  #x
#define PRAGMA_STR2(x)  PRAGMA_STR1 (x)
#define NOTE(x)  message (__FILE__ "(" PRAGMA_STR2(__LINE__) ") :- NOTE - " #x)
#define NOTE_wARG(x,y)  message (__FILE__ "(" PRAGMA_STR2(__LINE__) ") : - NOTE - " #x PRAGMA_STR2(y))

#pragma NOTE(Hello)

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) {
        throw std::exception();
    }
}