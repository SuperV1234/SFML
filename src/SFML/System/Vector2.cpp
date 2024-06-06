////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#include <SFML/System/Vector2.hpp>

#include <cassert>
#include <cmath>

namespace
{

// Named differently from the one in 'Vector3.cpp' to support unity builds.

// clang-format off
template <typename> constexpr bool isVec2FloatingPoint              = false;
template <>         constexpr bool isVec2FloatingPoint<float>       = true;
template <>         constexpr bool isVec2FloatingPoint<double>      = true;
template <>         constexpr bool isVec2FloatingPoint<long double> = true;
// clang-format on

} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
template <typename T>
Vector2<T> Vector2<T>::normalized() const
{
    static_assert(isVec2FloatingPoint<T>, "Vector2::normalized() is only supported for floating point types");

    assert(*this != Vector2<T>() && "Vector2::normalized() cannot normalize a zero vector");
    return (*this) / length();
}


////////////////////////////////////////////////////////////
template <typename T>
Angle Vector2<T>::angleTo(const Vector2<T>& rhs) const
{
    static_assert(isVec2FloatingPoint<T>, "Vector2::angleTo() is only supported for floating point types");

    assert(*this != Vector2<T>() && "Vector2::angleTo() cannot calculate angle from a zero vector");
    assert(rhs != Vector2<T>() && "Vector2::angleTo() cannot calculate angle to a zero vector");
    return radians(static_cast<float>(std::atan2(cross(rhs), dot(rhs))));
}


////////////////////////////////////////////////////////////
template <typename T>
Angle Vector2<T>::angle() const
{
    static_assert(isVec2FloatingPoint<T>, "Vector2::angle() is only supported for floating point types");

    assert(*this != Vector2<T>() && "Vector2::angle() cannot calculate angle from a zero vector");
    return radians(static_cast<float>(std::atan2(y, x)));
}


////////////////////////////////////////////////////////////
template <typename T>
Vector2<T> Vector2<T>::rotatedBy(Angle phi) const
{
    static_assert(isVec2FloatingPoint<T>, "Vector2::rotatedBy() is only supported for floating point types");

    // No zero vector assert, because rotating a zero vector is well-defined (yields always itself)
    T cos = std::cos(static_cast<T>(phi.asRadians()));
    T sin = std::sin(static_cast<T>(phi.asRadians()));

    // Don't manipulate x and y separately, otherwise they're overwritten too early
    return Vector2<T>(cos * x - sin * y, sin * x + cos * y);
}


////////////////////////////////////////////////////////////
template <typename T>
Vector2<T> Vector2<T>::projectedOnto(const Vector2<T>& axis) const
{
    static_assert(isVec2FloatingPoint<T>, "Vector2::projectedOnto() is only supported for floating point types");

    assert(axis != Vector2<T>() && "Vector2::projectedOnto() cannot project onto a zero vector");
    return dot(axis) / axis.lengthSq() * axis;
}


////////////////////////////////////////////////////////////
template <typename T>
Vector2<T>::Vector2(T r, Angle phi) :
x(r * static_cast<T>(std::cos(phi.asRadians()))),
y(r * static_cast<T>(std::sin(phi.asRadians())))
{
    static_assert(isVec2FloatingPoint<T>, "Vector2::Vector2(T, Angle) is only supported for floating point types");
}


////////////////////////////////////////////////////////////
template <typename T>
T Vector2<T>::length() const
{
    static_assert(isVec2FloatingPoint<T>, "Vector2::length() is only supported for floating point types");

    // don't use std::hypot because of slow performance
    return std::sqrt(x * x + y * y);
}

} // namespace sf


////////////////////////////////////////////////////////////
// Explicit instantiation definitions
////////////////////////////////////////////////////////////

template class sf::Vector2<float>;
template class sf::Vector2<double>;
template class sf::Vector2<long double>;

#define SFML_INSTANTIATE_VECTOR2_BASIC_MEMBER_FUNCTIONS(type)                \
    template /*                   */ sf::Vector2<type>::Vector2();           \
    template /*                   */ sf::Vector2<type>::Vector2(type, type); \
    template const sf::Vector2<type> sf::Vector2<type>::UnitX;               \
    template const sf::Vector2<type> sf::Vector2<type>::UnitY;

#define SFML_INSTANTIATE_VECTOR2_INTEGRAL_MEMBER_FUNCTIONS(type)                      \
    template type              sf::Vector2<type>::lengthSq() const;                   \
    template sf::Vector2<type> sf::Vector2<type>::perpendicular() const;              \
    template type              sf::Vector2<type>::dot(const Vector2& rhs) const;      \
    template type              sf::Vector2<type>::cross(const Vector2& rhs) const;    \
    template sf::Vector2<type> sf::Vector2<type>::cwiseMul(const Vector2& rhs) const; \
    template sf::Vector2<type> sf::Vector2<type>::cwiseDiv(const Vector2& rhs) const;

SFML_INSTANTIATE_VECTOR2_BASIC_MEMBER_FUNCTIONS(bool)
SFML_INSTANTIATE_VECTOR2_BASIC_MEMBER_FUNCTIONS(int)
SFML_INSTANTIATE_VECTOR2_BASIC_MEMBER_FUNCTIONS(unsigned int)

SFML_INSTANTIATE_VECTOR2_INTEGRAL_MEMBER_FUNCTIONS(int)
SFML_INSTANTIATE_VECTOR2_INTEGRAL_MEMBER_FUNCTIONS(unsigned int)
