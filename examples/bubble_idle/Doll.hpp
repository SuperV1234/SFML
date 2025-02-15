#pragma once

#include "CatType.hpp"
#include "Countdown.hpp"

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Constants.hpp"
#include "SFML/Base/Math/Cos.hpp"
#include "SFML/Base/Math/Sin.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Doll
{
    sf::Vector2f position;
    float        wobbleRadians;
    float        buffPower;
    float        hue = 0.f;
    CatType      catType;

    TargetedCountdown         tcActivation;
    OptionalTargetedCountdown tcDeath;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool isActive() const
    {
        return tcActivation.isDone();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getActivationProgress() const
    {
        return tcActivation.getProgress();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getDeathProgress() const
    {
        return tcDeath.getProgress();
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        wobbleRadians = sf::base::fmod(wobbleRadians + deltaTime * 0.002f, sf::base::tau);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline sf::Vector2f getDrawPosition() const
    {
        const sf::Vector2f wobbleOffset{sf::base::cos(wobbleRadians) * (7.5f + getDeathProgress() * 128.f),
                                        sf::base::sin(wobbleRadians) * (14.f + getDeathProgress() * 128.f)};

        return position + getActivationProgress() * wobbleOffset;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline float getRadius() const noexcept
    {
        return 32.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline float getRadiusSquared() const
    {
        const float radius = getRadius();
        return radius * radius;
    }
};
