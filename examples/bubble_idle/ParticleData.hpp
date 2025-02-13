#pragma once

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] ParticleData
{
    sf::Vector2f position;
    sf::Vector2f velocity;

    float scale; // TODO P2: consider optimizing memory layout
    float accelerationY;

    float opacity;
    float opacityDecay;

    float rotation;
    float torque;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        velocity.y += accelerationY * deltaTime;
        position += velocity * deltaTime;

        rotation += torque * deltaTime;

        opacity = sf::base::clamp(opacity - opacityDecay * deltaTime, 0.f, 1.f);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToTransformable(auto& transformable) const
    {
        transformable.position = position;
        transformable.scale    = {scale, scale};
        transformable.rotation = sf::radians(rotation);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline sf::base::U8 opacityAsAlpha() const
    {
        return static_cast<sf::base::U8>(opacity * 255.f);
    }
};
