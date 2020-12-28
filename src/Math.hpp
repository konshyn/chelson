#pragma once

template<typename T>
struct Vec3
{
    Vec3(T tx, T ty, T tz) : x{tx}, y{ty}, z{tz} {};
    T x{};
    T y{};
    T z{};
};

using Vec3f = Vec3<float>;
