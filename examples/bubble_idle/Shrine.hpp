#pragma once

#include "Aliases.hpp"
#include "Countdown.hpp"
#include "MathUtils.hpp"
#include "ShrineConstants.hpp"
#include "ShrineType.hpp"
#include "TextShakeEffect.hpp"

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Constants.hpp"
#include "SFML/Base/Math/Cos.hpp"
#include "SFML/Base/Math/Sin.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Shrine
{
    sf::Vector2f position;

    float wobbleRadians = 0.f;

    float mainOpacity = 255.f;

    OptionalTargetedCountdown tcActivation;
    OptionalTargetedCountdown tcDeath;

    TextShakeEffect textStatusShakeEffect;

    MoneyType collectedReward = 0u;

    ShrineType type;

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
        textStatusShakeEffect.update(deltaTime);
        wobbleRadians = sf::base::fmod(wobbleRadians + deltaTime * 0.002f + getDeathProgress() * 0.2f, sf::base::tau);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline sf::Vector2f getDrawPosition() const
    {
        const sf::Vector2f wobbleOffset{sf::base::cos(wobbleRadians) * (7.5f + getDeathProgress() * 128.f),
                                        sf::base::sin(wobbleRadians) * (14.f + getDeathProgress() * 128.f)};

        return position + getActivationProgress() * wobbleOffset;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getRange() const
    {
        if (!isActive())
            return 0.f;

        return 256.f * (1.f - getDeathProgress());
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] inline bool isInRange(const sf::Vector2f point) const
    {
        return isActive() && (point - position).length() < getRange();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getHue() const
    {
        return wrapHue(shrineHues[static_cast<U8>(type)]);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getRadius() const noexcept
    {
        return 64.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline float getRadiusSquared() const
    {
        const float radius = getRadius();
        return radius * radius;
    }
};
