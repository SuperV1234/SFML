#pragma once

#include "Cat.hpp"

#include "SFML/Graphics/Color.hpp"

namespace CatConstants
{
////////////////////////////////////////////////////////////
inline constexpr sf::Color colors[nCatTypes]{
    {192u, 147u, 109u}, // Normal
    {129u, 251u, 191u}, // Uni
    {191u, 61u, 61u},   // Devil
    {90u, 155u, 48u},   // Witch
    sf::Color::White,   // Astro

    {123u, 108u, 191u}, // Wizard
};

////////////////////////////////////////////////////////////
inline constexpr const char* actionNames[nCatTypes]{
    "Pops",    // Normal
    "Shines",  // Uni
    "IEDs",    // Devil
    "Hexes",   // Witch
    "Flights", // Astro

    "Spells", // Wizard
};

////////////////////////////////////////////////////////////
static inline constexpr float baseCooldowns[nCatTypes]{
    1000.f,  // Normal
    3000.f,  // Uni
    7000.f,  // Devil
    2000.f,  // Witch
    10000.f, // Astro

    1.f, // Wizard
};

////////////////////////////////////////////////////////////
static inline constexpr float baseRanges[nCatTypes]{
    96.f,  // Normal
    64.f,  // Uni
    48.f,  // Devil
    256.f, // Witch
    48.f,  // Astro

    384.f, // Wizard
};

} // namespace CatConstants
