#pragma once

#include "BubbleType.hpp"

#include "SFML/System/Vector2.hpp"


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
