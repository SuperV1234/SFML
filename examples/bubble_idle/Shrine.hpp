#pragma once

#include "Aliases.hpp"
#include "Countdown.hpp"
#include "LoopingTimer.hpp"
#include "TextShakeEffect.hpp"

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Optional.hpp"
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
inline constexpr auto nShrineTypes = static_cast<sf::base::SizeT>(ShrineType::Count);

////////////////////////////////////////////////////////////
inline constexpr const char* shrineNames[nShrineTypes]{
    "Shrine Of Magic",
    "Shrine Of Clicking",
    "Shrine Of Automation",
    "Shrine Of Repulsion",
    "Shrine Of Attraction",
    "Shrine Of Decay",
    "Shrine Of Chaos",
    "Shrine Of Transmutation",
    "Shrine Of Victory",
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Shrine
{
    sf::Vector2f position;

    LoopingTimer wobbleTimer{};

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
        (void)wobbleTimer.updateAndLoop(deltaTime * 0.002f + getDeathProgress() * 0.2f, sf::base::tau);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline sf::Vector2f getDrawPosition() const
    {
        const sf::Vector2f wobbleOffset{std::cos(wobbleTimer.value) * (7.5f + getDeathProgress() * 128.f),
                                        std::sin(wobbleTimer.value) * (14.f + getDeathProgress() * 128.f)};

        return position + getActivationProgress() * wobbleOffset;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getRange() const
    {
        if (!isActive())
            return 0.f;

        return 256.f * (1.f - getDeathProgress());
    }
};
