#pragma once

#include "RNG.hpp"

#include "SFML/Graphics/Text.hpp"

#include "SFML/Base/Algorithm.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] TextShakeEffect
{
    float grow  = 0.f;
    float angle = 0.f;

    ////////////////////////////////////////////////////////////
    void bump(const float strength)
    {
        grow  = strength;
        angle = getRndFloat(-grow * 0.2f, grow * 0.2f);
    }

    ////////////////////////////////////////////////////////////
    void update(const float deltaTimeMs)
    {
        if (grow > 0.f)
            grow -= deltaTimeMs * 0.0165f;

        if (angle != 0.f)
        {
            const float sign = angle > 0.f ? 1.f : -1.f;
            angle -= sign * deltaTimeMs * 0.00565f;

            if (sign * angle < 0.f)
                angle = 0.f;
        }

        grow  = sf::base::clamp(grow, 0.f, 5.f);
        angle = sf::base::clamp(angle, -0.5f, 0.5f);
    }

    ////////////////////////////////////////////////////////////
    void applyToText(sf::Text& text) const
    {
        text.scale    = {1.f + grow * 0.2f, 1.f + grow * 0.2f};
        text.rotation = sf::radians(angle);
    }
};
