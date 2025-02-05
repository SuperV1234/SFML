#pragma once

#include "Aliases.hpp"
#include "Countdown.hpp"
#include "LoopingTimer.hpp"
#include "TextShakeEffect.hpp"

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] CatType : sf::base::U8
{
    Normal = 0u,
    Uni    = 1u,
    Devil  = 2u,
    Witch  = 3u,
    Astro  = 4u,

    Wizard = 5u,

    Count
};

////////////////////////////////////////////////////////////
inline constexpr auto nCatTypes = static_cast<sf::base::SizeT>(CatType::Count);

////////////////////////////////////////////////////////////
[[nodiscard]] inline constexpr bool isUniqueCatType(const CatType catType) noexcept
{
    return catType == CatType::Wizard;
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] Cat
{
    sf::Vector2f position;
    sf::Vector2f rangeOffset;

    LoopingTimer wobbleTimer;
    LoopingTimer cooldownTimer;

    sf::Vector2f pawPosition;
    sf::Angle    pawRotation;

    float mainOpacity = 255.f;
    float pawOpacity  = 255.f;

    Countdown inspiredCountdown;

    SizeT nameIdx;

    TextShakeEffect textStatusShakeEffect;

    sf::base::U32 hits = 0u;

    CatType type;

    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] AstroState
    {
        float startX;

        float velocityX     = 0.f;
        float particleTimer = 0.f;

        bool wrapped = false;
    };

    sf::base::Optional<AstroState> astroState;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        textStatusShakeEffect.update(deltaTime);
        (void)wobbleTimer.updateAndLoop(deltaTime * 0.002f, sf::base::tau);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline sf::Vector2f getDrawPosition() const
    {
        return position + sf::Vector2f{0.f, std::sin(wobbleTimer.value * 2.f) * 7.5f};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool updateCooldown(const float maxCooldown, const float deltaTime)
    {
        const float cooldownSpeed = inspiredCountdown.updateAndIsActive(deltaTime) ? 2.f : 1.f;
        return cooldownTimer.updateAndStop(deltaTime * cooldownSpeed, maxCooldown);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getRadius() const noexcept
    {
        return 64.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool isCloseToStartX() const noexcept
    {
        SFML_BASE_ASSERT(type == CatType::Astro);
        SFML_BASE_ASSERT(astroState.hasValue());

        return astroState->wrapped && sf::base::fabs(position.x - astroState->startX) < 400.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool isAstroAndInFlight() const noexcept
    {
        return type == CatType::Astro && astroState.hasValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool hasUniqueType() const noexcept
    {
        return isUniqueCatType(type);
    }
};

////////////////////////////////////////////////////////////
[[nodiscard]] inline Cat makeCat(const CatType catType, const sf::Vector2f& position, const sf::Vector2f rangeOffset, const SizeT nameIdx)
{
    return Cat{.position              = position,
               .rangeOffset           = rangeOffset,
               .wobbleTimer           = {},
               .cooldownTimer         = {},
               .pawPosition           = position,
               .pawRotation           = sf::radians(0.f),
               .inspiredCountdown     = {},
               .nameIdx               = nameIdx,
               .textStatusShakeEffect = {},
               .type                  = catType,
               .astroState            = {}};
}
