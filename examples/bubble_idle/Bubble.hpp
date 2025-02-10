#pragma once

#include "SFML/Graphics/Sprite.hpp"

#include "SFML/System/Angle.hpp"
#include "SFML/System/Vector2.hpp"


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

    float scale; // TODO P2: if these three members are turned into U8 struct size goes from 32 to 20
    float rotation;
    float hueMod;

    BubbleType type;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        position += velocity * deltaTime;
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToSprite(sf::Sprite& sprite) const
    {
        sprite.position = position;
        sprite.scale    = {scale, scale};
        sprite.rotation = sf::radians(rotation);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline float getRadius() const
    {
        return 256.f * scale;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline float getRadiusSquared() const
    {
        const float radius = getRadius();
        return radius * radius;
    }
};
