#pragma once

#include "SFML/Graphics/Color.hpp"

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Assume.hpp"
#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr sf::Color hueColor(const float hue, const sf::base::U8 alpha)
{
    SFML_BASE_ASSERT(hue >= 0.f && hue <= 360.f);
    SFML_BASE_ASSUME(hue >= 0.f && hue <= 360.f);

    // R=1 and G=1 is used in the shader as a signal to make the color hue-based
    return sf::Color{1, 1, static_cast<sf::base::U8>(hue / 360.f * 255.f), alpha};
}
