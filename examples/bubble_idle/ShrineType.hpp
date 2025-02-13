#pragma once

#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
enum class ShrineType : sf::base::U8
{
    Voodoo        = 0u,
    Magic         = 1u,
    Clicking      = 2u,
    Automation    = 3u,
    Repulsion     = 4u,
    Attraction    = 5u,
    Chaos         = 6u,
    Transmutation = 7u,
    Victory       = 8u,

    Count
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr auto asIdx(const ShrineType catType) noexcept
{
    return static_cast<sf::base::SizeT>(catType);
}

////////////////////////////////////////////////////////////
enum : sf::base::SizeT
{
    nShrineTypes = asIdx(ShrineType::Count)
};
