#pragma once

#include "ParticleData.hpp"
#include "RNG.hpp"

#include "SFML/Graphics/Text.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] TextParticle
{
    char         buffer[16];
    ParticleData data;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        data.update(deltaTime);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToText(sf::Text& text) const
    {
        text.setString(buffer); // TODO P1: should find a way to assign directly to text buffer

        data.applyToTransformable(text);
        text.origin = text.getLocalBounds().size / 2.f;

        text.setFillColor(text.getFillColor().withAlpha(data.opacityAsAlpha()));
        text.setOutlineColor(text.getOutlineColor().withAlpha(data.opacityAsAlpha()));
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline TextParticle makeTextParticle(const sf::Vector2f position, const int combo)
{
    return {.buffer = {},
            .data   = {.position      = {position.x, position.y - 10.f},
                       .velocity      = getRndVector2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.425f,
                       .scale         = sf::base::clamp(1.f + 0.1f * static_cast<float>(combo + 1) / 1.75f, 1.f, 3.0f),
                       .accelerationY = 0.0042f,
                       .opacity       = 1.f,
                       .opacityDecay  = 0.00175f,
                       .rotation      = 0.f,
                       .torque        = getRndFloat(-0.002f, 0.002f)}};
}
