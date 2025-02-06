#pragma once

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Assume.hpp"
#include "SFML/Base/Math/Exp.hpp"


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr float remap(
    const float x,
    const float oldMin,
    const float oldMax,
    const float newMin,
    const float newMax)
{
    SFML_BASE_ASSERT(oldMax != oldMin);
    SFML_BASE_ASSUME(oldMax != oldMin);

    return newMin + ((x - oldMin) / (oldMax - oldMin)) * (newMax - newMin);
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr float exponentialApproach(
    const float current,
    const float target,
    const float deltaTimeMs,
    const float speed)
{
    SFML_BASE_ASSERT(speed >= 0.f);
    SFML_BASE_ASSUME(speed >= 0.f);

    const float factor = 1.f - sf::base::exp(-deltaTimeMs / speed);
    return current + (target - current) * factor;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr sf::Vector2f exponentialApproach(
    const sf::Vector2f current,
    const sf::Vector2f target,
    const float        deltaTimeMs,
    const float        speed)
{
    // TODO P2: this is axis independent, should be fixed

    return {exponentialApproach(current.x, target.x, deltaTimeMs, speed),
            exponentialApproach(current.y, target.y, deltaTimeMs, speed)};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr float wrapHue(const float hue)
{
    const auto val = hue - static_cast<float>(static_cast<int>(hue / 360.f)) * 360.f;
    return val >= 0.f ? val : val + 360.f;
}
