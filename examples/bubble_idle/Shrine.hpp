#pragma once

#include "Aliases.hpp"
#include "Countdown.hpp"
#include "ExactArray.hpp"
#include "MathUtils.hpp"
#include "TextShakeEffect.hpp"

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
enum class ShrineType : U8
{
    Magic         = 0u, // No special effects, unlocks magic menu
    Clicking      = 1u, // Buff click reward & nerf cat reward, unlocks clicking magic buff
    Automation    = 2u, // Buff cat reward & nerf click reward, unlocks cat magic buff
    Repulsion     = 3u, // Pushes bubbles away, unlocks TBD
    Attraction    = 4u, // Pulls bubbles in, unlocks TBD
    Decay         = 5u, // All special bubbles become hexed, unlocks TBD
    Chaos         = 6u, // Periodically turns bubbles into random ones, unlocks TBD
    Transmutation = 7u, // Changes cats behavior in range, unlocks TBD
    Victory       = 8u, // TBD special effect, unlocks game victory

    Count
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr auto asIdx(const ShrineType catType) noexcept
{
    return static_cast<sf::base::SizeT>(catType);
}

////////////////////////////////////////////////////////////
inline constexpr auto nShrineTypes = asIdx(ShrineType::Count);

////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    const char*,
    shrineNames,
    nShrineTypes,
    {
        "Shrine Of Magic",
        "Shrine Of Clicking",
        "Shrine Of Automation",
        "Shrine Of Repulsion",
        "Shrine Of Attraction",
        "Shrine Of Decay",
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
        -139.f, // Magic,
        130.f, // Clicking,
        -15.f, // Automation,
        180.f, // Repulsion,
        -80.f, // Attraction,
        0.f, // Decay,
        0.f, // Chaos,
        0.f, // Transmutation,
        0.f, // Victory,
    });

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
        const sf::Vector2f wobbleOffset{std::cos(wobbleRadians) * (7.5f + getDeathProgress() * 128.f),
                                        std::sin(wobbleRadians) * (14.f + getDeathProgress() * 128.f)};

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
