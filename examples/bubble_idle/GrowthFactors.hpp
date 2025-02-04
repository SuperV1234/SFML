#pragma once

#include <cmath>


////////////////////////////////////////////////////////////
struct [[nodiscard]] GrowthFactors
{
    float initial;
    float linear         = 0.f;
    float multiplicative = 0.f;
    float exponential    = 1.f;
    float flat           = 0.f;
    float finalMult      = 1.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure, gnu::always_inline]] inline float computeGrowth(const float n) const
    {
        return ((initial + n * multiplicative) * std::pow(exponential, n) + linear * n + flat) * finalMult;
    }
};
