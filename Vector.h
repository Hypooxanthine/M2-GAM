#pragma once

#include <array>
#include <cmath>
#include <ostream>

template <typename T>
struct Vector3
{
    union
    {
        struct
        {
            T x, y, z;
        };

        struct
        {
            T r, g, b;
        };

        std::array<T, 3> data;
    };

    T& operator[](size_t i)
    {
        return data[i];
    }

    const T& operator[](size_t i) const
    {
        return data[i];
    }
    
};

template <typename T>
Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

template <typename T>
Vector3<T> operator-(const Vector3<T>& a, const Vector3<T>& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

template <typename T>
Vector3<T> operator*(const Vector3<T>& a, T b)
{
    return { a.x * b, a.y * b, a.z * b };
}

template <typename T>
Vector3<T> operator/(const Vector3<T>& a, const Vector3<T>& b)
{
    return { a.x / b.x, a.y / b.y, a.z / b.z };
}

template <typename T>
Vector3<T> operator/(const Vector3<T>& a, T b)
{
    return { a.x / b, a.y / b, a.z / b };
}

template <typename T>
Vector3<T> operator*(T a, const Vector3<T>& b)
{
    return { a * b.x, a * b.y, a * b.z };
}

template <typename T>
Vector3<T> operator/(T a, const Vector3<T>& b)
{
    return { a / b.x, a / b.y, a / b.z };
}

template <typename T>
T dot(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T>
Vector3<T> cross(const Vector3<T>& a, const Vector3<T>& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

template <typename T>
T length2(const Vector3<T>& a)
{
    return dot(a, a);
}

template <typename T>
T length(const Vector3<T>& a)
{
    return std::sqrt(length2(a));
}

template <typename T>
Vector3<T> normalized(const Vector3<T>& a)
{
    return a / lenght(a);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vector3<T>& vec)
{
    os << '(' << vec[0] << ',' << vec[1] << ',' << vec[2] << ')';
    return os;
}

using Vector3i = Vector3<int>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;
using Vector3ull = Vector3<size_t>;

