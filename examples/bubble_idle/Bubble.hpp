#pragma once

#include "Aliases.hpp"

#include "SFML/Graphics/Sprite.hpp"

#include "SFML/System/Vector2.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] BubbleType : U8
{
    Normal = 0u,
    Star   = 1u,
    Bomb   = 2u,

    Count
};

////////////////////////////////////////////////////////////
inline constexpr auto nBubbleTypes = static_cast<sf::base::SizeT>(BubbleType::Count);

////////////////////////////////////////////////////////////
struct [[nodiscard]] Bubble
{
    sf::Vector2f position;
    sf::Vector2f velocity;

    float scale;
    float rotation;

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
    [[nodiscard, gnu::always_inline]] inline float getRadius() const
    {
        return 256.f * scale;
    }
};
