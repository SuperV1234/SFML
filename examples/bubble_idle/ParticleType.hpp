#pragma once

#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] ParticleType : sf::base::U8
{
    Bubble   = 0u,
    Star     = 1u,
    Fire     = 2u,
    Hex      = 3u,
    Shrine   = 4u,
    Cursor   = 5u,
    Cog      = 6u,
    ByteCoin = 7u,

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
