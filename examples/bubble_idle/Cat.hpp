#pragma once

#include "Aliases.hpp"
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

    Wizard = 5u,

    Count
};

////////////////////////////////////////////////////////////
inline constexpr auto nCatTypes = static_cast<sf::base::SizeT>(CatType::Count);

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] inline constexpr bool isUniqueCatType(const CatType catType) noexcept
{
    return catType == CatType::Wizard;
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] Cat
{
    sf::Vector2f position;
    sf::Vector2f rangeOffset;

    float     wobbleRadians;
    Countdown cooldown;

    sf::Vector2f pawPosition;
    sf::Angle    pawRotation;

    float mainOpacity = 255.f;
    float pawOpacity  = 255.f;

    float hue = 0.f;

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
        wobbleRadians = sf::base::fmod(wobbleRadians + deltaTime * 0.002f, sf::base::tau);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline sf::Vector2f getDrawPosition() const
    {
        return position + sf::Vector2f{0.f, std::sin(wobbleRadians * 2.f) * 7.5f};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool updateCooldown(const float deltaTime)
    {
        const float cooldownSpeed = inspiredCountdown.updateAndIsActive(deltaTime) ? 2.f : 1.f;
        return cooldown.updateAndStop(deltaTime * cooldownSpeed) == CountdownStatusStop::AlreadyFinished;
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
