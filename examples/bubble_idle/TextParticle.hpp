#pragma once

#include "ParticleData.hpp"

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
