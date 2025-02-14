#pragma once

#include "CatType.hpp"
#include "ExactArray.hpp"

#include "SFML/Graphics/Color.hpp"


namespace CatConstants
{
////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    sf::Color,
    colors,
    nCatTypes,
    {
        {192u, 147u, 109u}, // Normal
        {129u, 251u, 191u}, // Uni
        {191u, 61u, 61u},   // Devil
        sf::Color::White,   // Astro

        {90u, 155u, 48u},   // Witch
        {123u, 108u, 191u}, // Wizard
        {175u, 175u, 175u}, // Mouse
        {72u, 51u, 33u},    // Engi
        {132u, 132u, 132u}, // Repulso
        {132u, 132u, 132u}, // Attracto (TODO P1: change?)
    });


////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    const char*,
    actionNames,
    nCatTypes,
    {
        "Pops",    // Normal
        "Shines",  // Uni
        "IEDs",    // Devil
        "Flights", // Astro

        "Rituals",     // Witch
        "Spells",      // Wizard
        "Clicks",      // Mouse
        "Boosts",      // Engi
        "Conversions", // Repulso
        "TODO",        // Attracto
    });

////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    float,
    baseCooldowns,
    nCatTypes,
    {
        850.f,   // Normal
        3000.f,  // Uni
        7000.f,  // Devil
        10000.f, // Astro

        60000.f, // Witch
        1500.f,  // Wizard
        500.f,   // Mouse
        10000.f, // Engi
        5.f,     // Repulso
        5.f,     // Attracto (TODO P1: change?)
    });

////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    float,
    baseRanges,
    nCatTypes,
    {
        96.f, // Normal
        64.f, // Uni
        48.f, // Devil
        48.f, // Astro

        192.f, // Witch
        384.f, // Wizard
        128.f, // Mouse
        192.f, // Engi
        192.f, // Repulso
        192.f, // Attracto
    });

////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    sf::Vector2f,
    rangeOffsets,
    nCatTypes,
    {
        {0.f, 0.f},    // Normal
        {0.f, -100.f}, // Uni
        {0.f, 100.f},  // Devil
        {-64.f, 0.f},  // Astro

        {0.f, 0.f}, // Witch
        {0.f, 0.f}, // Wizard
        {0.f, 0.f}, // Mouse
        {0.f, 0.f}, // Engi
        {0.f, 0.f}, // Repulso
        {0.f, 0.f}, // Attracto
    });

} // namespace CatConstants
