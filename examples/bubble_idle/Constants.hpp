#pragma once

#include "SFML/Graphics/Color.hpp"

#include "SFML/System/Vector2.hpp"


////////////////////////////////////////////////////////////
constexpr sf::Vector2f resolution{1366.f, 768.f};
constexpr auto         resolutionUInt = resolution.toVector2u();

////////////////////////////////////////////////////////////
constexpr sf::Vector2f gameScreenSize{1366.f, 768.f};
constexpr auto         gameScreenSizeUInt = gameScreenSize.toVector2u();

////////////////////////////////////////////////////////////
constexpr sf::Vector2f boundaries{1366.f * 10.f, 768.f};

////////////////////////////////////////////////////////////
constexpr sf::Color colorBlueOutline{50u, 84u, 135u};
constexpr sf::Color colorRedOutline{135u, 50u, 50u};
