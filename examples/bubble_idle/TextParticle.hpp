#pragma once

#include "ParticleData.hpp"

#include "SFML/Graphics/Text.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] TextParticle
{
    char         buffer[16];
    ParticleData data;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToText(sf::Text& text) const
    {
        text.setString(buffer); // TODO P2: (lib) should find a way to assign directly to text buffer

        data.applyToTransformable(text);
        text.origin = text.getLocalBounds().size / 2.f;

        text.setFillColor(text.getFillColor().withAlpha(data.opacityAsAlpha()));
        text.setOutlineColor(text.getOutlineColor().withAlpha(data.opacityAsAlpha()));
    }
};
