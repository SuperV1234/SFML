#pragma once

#include "Countdown.hpp"
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

    Wizard  = 5u,
    Mouse   = 6u,
    Engi    = 7u,
    Repulso = 8u,

    Count
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr auto asIdx(const CatType type) noexcept
{
    return static_cast<sf::base::SizeT>(type);
}

////////////////////////////////////////////////////////////
enum : sf::base::SizeT
{
    nCatTypes = asIdx(CatType::Count)
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr bool isUniqueCatType(const CatType catType) noexcept
{
    SFML_BASE_ASSERT(catType < CatType::Count);
    return catType >= CatType::Wizard;
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] Cat
{
    sf::Vector2f position;
    sf::Vector2f rangeOffset; // TODO P1: hardcode per cat type

    float     wobbleRadians;
    Countdown cooldown;

    sf::Vector2f pawPosition;
    sf::Angle    pawRotation;

    float mainOpacity = 255.f;
    float pawOpacity  = 255.f;

    float hue = 0.f;

    Countdown inspiredCountdown;
    Countdown boostCountdown;

    sf::base::SizeT nameIdx;

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
        wobbleRadians = sf::base::fmod(wobbleRadians + deltaTime * 0.002f, sf::base::tau);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline sf::Vector2f getDrawPosition() const
    {
        return position + sf::Vector2f{0.f, std::sin(wobbleRadians * 2.f) * 7.5f};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool updateCooldown(const float deltaTime)
    {
        const float ispiredMult   = inspiredCountdown.updateAndIsActive(deltaTime) ? 2.f : 1.f;
        const float boostMult     = boostCountdown.updateAndIsActive(deltaTime) ? 2.f : 1.f;
        const float cooldownSpeed = ispiredMult * boostMult;

        return cooldown.updateAndStop(deltaTime * cooldownSpeed) == CountdownStatusStop::AlreadyFinished;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline float getRadius() const noexcept
    {
        return 64.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline float getRadiusSquared() const
    {
        const float radius = getRadius();
        return radius * radius;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline bool isCloseToStartX() const noexcept
    {
        SFML_BASE_ASSERT(type == CatType::Astro);
        SFML_BASE_ASSERT(astroState.hasValue());

        return astroState->wrapped && sf::base::fabs(position.x - astroState->startX) < 400.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline bool isAstroAndInFlight() const noexcept
    {
        return type == CatType::Astro && astroState.hasValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline bool hasUniqueType() const noexcept
    {
        return isUniqueCatType(type);
    }
};
