#pragma once

#include "SFML/Graphics/Color.hpp"

#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr sf::Color hueColor(const float hue, const sf::base::U8 alpha)
{
    return sf::Color{1, 1, static_cast<sf::base::U8>(hue / 360.f * 255.f), alpha};
}
