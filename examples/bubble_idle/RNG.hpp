#pragma once

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Assume.hpp"

#include <random>


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline std::minstd_rand0& getRandomEngine()
{
    static std::minstd_rand0 randomEngine(std::random_device{}());
    return randomEngine;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline float getRndFloat(const float min, const float max)
{
    SFML_BASE_ASSERT(min <= max);
    SFML_BASE_ASSUME(min <= max);

    return std::uniform_real_distribution<float>{min, max}(getRandomEngine());
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getRndVector2f(const sf::Vector2f mins,
                                                                                   const sf::Vector2f maxs)
{
    return {getRndFloat(mins.x, maxs.x), getRndFloat(mins.y, maxs.y)};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getRndVector2f(const sf::Vector2f maxs)
{
    return {getRndFloat(0.f, maxs.x), getRndFloat(0.f, maxs.y)};
}
