#pragma once

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Assume.hpp"
#include "SFML/Base/Constants.hpp"

#include <cmath>


////////////////////////////////////////////////////////////
using EasingFn = float (*)(const float);

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr float easeIdentity(const float x)
{
    SFML_BASE_ASSERT(x >= 0.f && x <= 1.f);
    SFML_BASE_ASSUME(x >= 0.f && x <= 1.f);

    return x;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr float easeInOutCubic(const float x)
{
    SFML_BASE_ASSERT(x >= 0.f && x <= 1.f);
    SFML_BASE_ASSUME(x >= 0.f && x <= 1.f);

    return x < 0.5f ? 4.f * x * x * x : 1.f - std::pow(-2.f * x + 2.f, 3.f) / 2.f;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr float easeInOutSine(const float x)
{
    SFML_BASE_ASSERT(x >= 0.f && x <= 1.f);
    SFML_BASE_ASSUME(x >= 0.f && x <= 1.f);

    return -(std::cos(sf::base::pi * x) - 1.f) / 2.f;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr float easeInOutQuint(const float x)
{
    SFML_BASE_ASSERT(x >= 0.f && x <= 1.f);
    SFML_BASE_ASSUME(x >= 0.f && x <= 1.f);

    return x < 0.5f ? 16.f * x * x * x * x * x : 1.f - std::pow(-2.f * x + 2.f, 5.f) / 2.f;
}
