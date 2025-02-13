#pragma once

#include "SFML/Graphics/Color.hpp"

#include "SFML/System/Vector2.hpp"


////////////////////////////////////////////////////////////
inline constexpr sf::Vector2f gameScreenSize{1366.f, 768.f};
inline constexpr auto         gameScreenSizeUInt = gameScreenSize.toVector2u();

////////////////////////////////////////////////////////////
inline constexpr sf::Vector2f boundaries{gameScreenSize.x * 10.f, gameScreenSize.y};

////////////////////////////////////////////////////////////
inline constexpr sf::Color colorBlueOutline{50u, 84u, 135u};
