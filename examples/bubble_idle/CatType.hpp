#pragma once

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] CatType : sf::base::U8
{
    Normal = 0u,
    Uni    = 1u,
    Devil  = 2u,
    Astro  = 3u,

    Witch    = 4u,
    Wizard   = 5u,
    Mouse    = 6u,
    Engi     = 7u,
    Repulso  = 8u,
    Attracto = 9u,

    Count
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr auto asIdx(const CatType type) noexcept
{
    return static_cast<sf::base::SizeT>(type);
}

////////////////////////////////////////////////////////////
enum : sf::base::SizeT
{
    nCatTypes = asIdx(CatType::Count)
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr bool isUniqueCatType(const CatType catType) noexcept
{
    SFML_BASE_ASSERT(catType < CatType::Count);
    return catType >= CatType::Witch;
}
