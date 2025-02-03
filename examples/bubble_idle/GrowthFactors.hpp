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
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::pure, gnu::always_inline]] inline float computeGrowth(const GrowthFactors& factors, const float n)
{
    return ((factors.initial + n * factors.multiplicative) * std::pow(factors.exponential, n) + factors.linear * n +
            factors.flat) *
           factors.finalMult;
}
