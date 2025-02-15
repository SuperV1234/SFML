#pragma once

#include "AssertAssume.hpp"

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Constants.hpp"
#include "SFML/Base/Math/Cos.hpp"
#include "SFML/Base/Math/Sin.hpp"
#include "SFML/Base/Math/Sqrt.hpp"

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
    template <typename T>
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline T getI(const T min, const T max)
    {
        ASSERT_AND_ASSUME(min <= max);
        return std::uniform_int_distribution<T>{min, max}(m_engine);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline float getF(const float min, const float max)
    {
        ASSERT_AND_ASSUME(min <= max);
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

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getPointInCircle(const sf::Vector2f center,
                                                                                         const float        radius)
    {
        const float angle    = getF(0.f, sf::base::tau);
        const float distance = radius * sf::base::sqrt(getF(0.f, 1.f));

        // Compute the point's coordinates using polar-to-Cartesian conversion.
        return {center.x + distance * sf::base::cos(angle), center.y + distance * sf::base::sin(angle)};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getRandomDirection()
    {
        const float angle = getF(0.f, sf::base::tau);
        return {sf::base::cos(angle), sf::base::sin(angle)};
    }
};
