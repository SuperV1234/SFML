#pragma once

#include "ParticleData.hpp"

#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] ParticleType : sf::base::U8
{
    Bubble = 0u,
    Star   = 1u,
    Fire   = 2u,
    Hex    = 3u,
    Shrine = 4u,
    Cursor = 5u,
    Cog    = 6u,

    Count
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr auto asIdx(const ParticleType type) noexcept
{
    return static_cast<sf::base::SizeT>(type);
}

////////////////////////////////////////////////////////////
enum : sf::base::SizeT
{
    nParticleTypes = asIdx(ParticleType::Count)
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Particle
{
    ParticleData data;
    float        hue = 0.f;
    ParticleType type;
};
