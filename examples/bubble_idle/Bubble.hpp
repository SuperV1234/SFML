#pragma once

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] BubbleType : sf::base::U8
{
    Normal = 0u,
    Star   = 1u,
    Bomb   = 2u,

    Count
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr auto asIdx(const BubbleType type) noexcept
{
    return static_cast<sf::base::SizeT>(type);
}

////////////////////////////////////////////////////////////
enum : sf::base::SizeT
{
    nBubbleTypes = asIdx(BubbleType::Count)
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Bubble
{
    sf::Vector2f position;
    sf::Vector2f velocity;

    float radius; // TODO P2: if these three members are turned into U8 struct size goes from 32 to 20
    float rotation;
    float hueMod;

    BubbleType type;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline float getRadiusSquared() const
    {
        return radius * radius;
    }
};
