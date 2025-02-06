#pragma once

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Assume.hpp"

#include <random>


////////////////////////////////////////////////////////////
class [[nodiscard]] RNG
{
public:
    using SeedType = unsigned int;

private:
    SeedType          m_seed;
    std::minstd_rand0 m_engine;

public:
    ////////////////////////////////////////////////////////////
    explicit RNG(const SeedType seed) : m_seed{seed}, m_engine{seed}
    {
        m_engine.discard(1);
    }

    ////////////////////////////////////////////////////////////
    void reseed(const SeedType seed) noexcept
    {
        m_seed = seed;
        m_engine.seed(seed);
        m_engine.discard(1);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SeedType getSeed() const noexcept
    {
        return m_seed;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::minstd_rand0& getEngine() noexcept
    {
        return m_engine;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline float getF(const float min, const float max)
    {
        SFML_BASE_ASSERT(min <= max);
        SFML_BASE_ASSUME(min <= max);

        return std::uniform_real_distribution<float>{min, max}(m_engine);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getVec2f(const sf::Vector2f mins, const sf::Vector2f maxs)
    {
        return {getF(mins.x, maxs.x), getF(mins.y, maxs.y)};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getVec2f(const sf::Vector2f maxs)
    {
        return {getF(0.f, maxs.x), getF(0.f, maxs.y)};
    }
};
