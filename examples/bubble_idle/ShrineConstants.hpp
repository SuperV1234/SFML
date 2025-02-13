#pragma once

#include "ExactArray.hpp"
#include "ShrineType.hpp"


////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    const char*,
    shrineNames,
    nShrineTypes,
    {
        "Shrine Of Voodoo",
        "Shrine Of Magic",
        "Shrine Of Clicking",
        "Shrine Of Automation",
        "Shrine Of Repulsion",
        "Shrine Of Attraction",
        "Shrine Of Chaos",
        "Shrine Of Transmutation",
        "Shrine Of Victory",
    });

////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    float,
    shrineHues,
    nShrineTypes,
    {
        30.f,   // Voodoo
        -139.f, // Magic
        130.f,  // Clicking
        -15.f,  // Automation
        180.f,  // Repulsion
        -80.f,  // Attraction
        0.f,    // Chaos
        0.f,    // Transmutation
        0.f,    // Victory
    });
