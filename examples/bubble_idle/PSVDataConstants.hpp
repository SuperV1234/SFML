#pragma once

#include "PSVData.hpp"


////////////////////////////////////////////////////////////
namespace PSVDataConstants
{

inline constexpr PSVData comboStartTime //
    {.nMaxPurchases = 20u,
     .cost          = {.initial = 35.f, .linear = 125.f, .exponential = 1.7f},
     .value         = {.initial = 0.55f, .linear = 0.04f, .exponential = 1.02f}};

inline constexpr PSVData mapExtension //
    {.nMaxPurchases = 8u, .cost = {.initial = 100.f, .exponential = 4.85f}, .value = {}};

inline constexpr PSVData shrineActivation //
    {.nMaxPurchases = 9u, .cost = {.initial = 200.f, .exponential = 4.85f}, .value = {}};

inline constexpr PSVData bubbleCount //
    {.nMaxPurchases = 30u,
     .cost          = {.initial = 75.f, .linear = 1500.f, .exponential = 2.5f},
     .value         = {.initial = 500.f, .linear = 325.f, .exponential = 1.01f}};

inline constexpr PSVData bubbleValue //
    {.nMaxPurchases = 19u,
     .cost          = {.initial = 2500.f, .linear = 2500.f, .exponential = 2.f},
     .value         = {.initial = 0.f, .linear = 1.f}};

inline constexpr PSVData explosionRadiusMult //
    {.nMaxPurchases = 10u, .cost = {.initial = 15000.f, .exponential = 1.5f}, .value = {.initial = 1.f, .linear = 0.1f}};

inline constexpr PSVData catNormal //
    {.nMaxPurchases = 64u, .cost = {.initial = 35.f, .exponential = 1.7f}, .value = {}};

inline constexpr PSVData catNormalCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 2000.f, .exponential = 1.68f, .flat = -1500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

inline constexpr PSVData catNormalRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 4000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

inline constexpr PSVData catUni //
    {.nMaxPurchases = 64u, .cost = {.initial = 250.f, .exponential = 1.75f}, .value = {}};

inline constexpr PSVData catUniCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 6000.f, .exponential = 1.68f, .flat = -2500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

inline constexpr PSVData catUniRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 4000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

inline constexpr PSVData catDevil //
    {.nMaxPurchases = 64u, .cost = {.initial = 15000.f, .exponential = 1.6f}, .value = {}};

inline constexpr PSVData catDevilCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 200'000.f, .exponential = 1.68f, .flat = 0.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

inline constexpr PSVData catDevilRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 150'000.f, .exponential = 1.85f, .flat = 0.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

// TODO test
inline constexpr PSVData catWitch //
    {.nMaxPurchases = 64u, .cost = {.initial = 1000000.f, .exponential = 1.5}, .value = {}};

// TODO test
inline constexpr PSVData catWitchCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 3'500'000.f, .exponential = 1.68f, .flat = -1500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

// TODO test
inline constexpr PSVData catWitchRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 5'500'000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

inline constexpr PSVData catAstro //
    {.nMaxPurchases = 64u, .cost = {.initial = 150000.f, .exponential = 1.5}, .value = {}};

inline constexpr PSVData catAstroCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 3'500'000.f, .exponential = 1.68f, .flat = -1500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

inline constexpr PSVData catAstroRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 5'500'000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

inline constexpr PSVData multiPopRange //
    {.nMaxPurchases = 24u,
     .cost          = {.initial = 1.f, .linear = 1.f, .exponential = 1.5f},
     .value         = {.initial = 64.0f, .linear = 8.0f}};

inline constexpr PSVData inspireDurationMult //
    {.nMaxPurchases = 20u,
     .cost          = {.initial = 1.f, .linear = 1.f, .exponential = 1.5f},
     .value         = {.initial = 1.f, .linear = 0.2f}};

} // namespace PSVDataConstants
