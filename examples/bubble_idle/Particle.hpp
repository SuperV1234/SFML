#pragma once

#include "ParticleData.hpp"
#include "ParticleType.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Particle
{
    ParticleData data;
    sf::base::U8 hueByte = 0u;
    ParticleType type;
};
