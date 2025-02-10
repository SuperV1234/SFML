#pragma once

#include "HueColor.hpp"
#include "ParticleData.hpp"
#include "RNG.hpp"

#include "SFML/Graphics/Sprite.hpp"

#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] ParticleType : sf::base::U8
{
    Bubble = 0u,
    Star   = 1u,
    Fire   = 2u,
    Hex    = 3u,
    Shrine = 4u,
    Cursor = 5u,
    Cog    = 6u,

    Count
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr auto asIdx(const ParticleType type) noexcept
{
    return static_cast<sf::base::SizeT>(type);
}

////////////////////////////////////////////////////////////
inline constexpr auto nParticleTypes = asIdx(ParticleType::Count);

////////////////////////////////////////////////////////////
struct [[nodiscard]] Particle
{
    ParticleData data;
    float        hue = 0.f;

    ParticleType type;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        data.update(deltaTime);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToSprite(sf::Sprite& sprite) const
    {
        data.applyToTransformable(sprite);
        sprite.color = hueColor(hue, data.opacityAsAlpha());
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline Particle makeParticle(
    RNG&               rng,
    const sf::Vector2f position,
    const ParticleType particleType,
    const float        scaleMult,
    const float        speedMult,
    const float        opacity = 1.f,
    const float        hue     = 0.f)
{
    return {.data = {.position      = position,
                     .velocity      = rng.getVec2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * speedMult,
                     .scale         = rng.getF(0.08f, 0.27f) * scaleMult,
                     .accelerationY = 0.002f,
                     .opacity       = opacity,
                     .opacityDecay  = rng.getF(0.00025f, 0.0015f),
                     .rotation      = rng.getF(0.f, sf::base::tau),
                     .torque        = rng.getF(-0.002f, 0.002f)},
            .hue  = hue,
            .type = particleType};
}
