#pragma once

#include "GrowthFactors.hpp"
#include "PSVData.hpp"
#include "PSVDataConstants.hpp"

#include "SFML/ImGui/ImGui.hpp"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/DrawableBatch.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Text.hpp"

#include "SFML/Window/Keyboard.hpp"

#include "SFML/System/Angle.hpp"
#include "SFML/System/Rect.hpp"
#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Assume.hpp"
#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Math/Exp.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/SizeT.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <limits>
#include <random>

#include <climits>
#include <cstdio>


////////////////////////////////////////////////////////////
using sf::base::SizeT;
using sf::base::U64;
using sf::base::U8;

////////////////////////////////////////////////////////////
using MoneyType = U64;

////////////////////////////////////////////////////////////
constexpr sf::Vector2f resolution{1366.f, 768.f};
constexpr auto         resolutionUInt = resolution.toVector2u();

////////////////////////////////////////////////////////////
constexpr sf::Vector2f boundaries{1366.f * 10.f, 768.f};

////////////////////////////////////////////////////////////
constexpr sf::Color colorBlueOutline{50u, 84u, 135u};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline std::minstd_rand0& getRandomEngine()
{
    static std::minstd_rand0 randomEngine(std::random_device{}());
    return randomEngine;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline float getRndFloat(const float min, const float max)
{
    SFML_BASE_ASSERT(min <= max);
    SFML_BASE_ASSUME(min <= max);

    return std::uniform_real_distribution<float>{min, max}(getRandomEngine());
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getRndVector2f(const sf::Vector2f mins,
                                                                                   const sf::Vector2f maxs)
{
    return {getRndFloat(mins.x, maxs.x), getRndFloat(mins.y, maxs.y)};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getRndVector2f(const sf::Vector2f maxs)
{
    return {getRndFloat(0.f, maxs.x), getRndFloat(0.f, maxs.y)};
}

////////////////////////////////////////////////////////////
[[nodiscard]] auto getShuffledCatNames(auto&& randomEngine)
{
    std::array names{"Gorgonzola", "Provolino",  "Pistacchietto", "Ricottina",  "Mozzarellina",  "Tiramisu",
                     "Cannolino",  "Biscottino", "Cannolina",     "Biscottina", "Pistacchietta", "Provolina",
                     "Arancino",   "Limoncello", "Ciabatta",      "Focaccina",  "Amaretto",      "Pallino",
                     "Birillo",    "Trottola",   "Baffo",         "Poldo",      "Fuffi",         "Birba",
                     "Ciccio",     "Pippo",      "Tappo",         "Briciola",   "Braciola",      "Pulce",
                     "Dante",      "Bolla",      "Fragolina",     "Luppolo",    "Sirena",        "Polvere",
                     "Stellina",   "Lunetta",    "Briciolo",      "Fiammetta",  "Nuvoletta",     "Scintilla",
                     "Piuma",      "Fulmine",    "Arcobaleno",    "Stelluccia", "Lucciola",      "Pepita",
                     "Fiocco",     "Girandola",  "Bombetta",      "Fusillo",    "Cicciobello",   "Palloncino",
                     "Joe Biden",  "Trump",      "Obama",         "De Luca",    "Salvini",       "Renzi",
                     "Nutella",    "Vespa",      "Mandolino",     "Ferrari",    "Pavarotti",     "Espresso",
                     "Sir",        "Nocciolina", "Fluffy",        "Costanzo",   "Mozart",        "DB",
                     "Soniuccia",  "Pupi",       "Pupetta",       "Genitore 1", "Genitore 2",    "Stonks",
                     "Carotina",   "Waffle",     "Pancake",       "Muffin",     "Cupcake",       "Donut",
                     "Jinx",       "Miao",       "Arnold",        "Granita",    "Leone",         "Pangocciolo"};

    std::shuffle(names.begin(), names.end(), randomEngine);
    return names;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const, gnu::always_inline]] inline constexpr float remap(
    const float x,
    const float oldMin,
    const float oldMax,
    const float newMin,
    const float newMax)
{
    SFML_BASE_ASSERT(oldMax != oldMin);
    SFML_BASE_ASSUME(oldMax != oldMin);

    return newMin + ((x - oldMin) / (oldMax - oldMin)) * (newMax - newMin);
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const, gnu::always_inline]] inline constexpr float exponentialApproach(
    const float current,
    const float target,
    const float deltaTimeMs,
    const float speed)
{
    SFML_BASE_ASSERT(speed >= 0.f);
    SFML_BASE_ASSUME(speed >= 0.f);

    const float factor = 1.f - sf::base::exp(-deltaTimeMs / speed);
    return current + (target - current) * factor;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const, gnu::always_inline]] inline constexpr sf::Vector2f exponentialApproach(
    const sf::Vector2f current,
    const sf::Vector2f target,
    const float        deltaTimeMs,
    const float        speed)
{
    // TODO: this is axis independent

    return {exponentialApproach(current.x, target.x, deltaTimeMs, speed),
            exponentialApproach(current.y, target.y, deltaTimeMs, speed)};
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] Countdown
{
    float value = 0.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndLoop(const float deltaTimeMs, const float startingValue)
    {
        if (!updateAndStop(deltaTimeMs))
            return false;

        value = startingValue;
        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndStop(const float deltaTimeMs)
    {
        if (value > 0.f)
        {
            value -= deltaTimeMs;
            return false;
        }

        value = 0.f;
        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndIsActive(const float deltaTimeMs)
    {
        return !updateAndStop(deltaTimeMs);
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] TargetedCountdown : Countdown
{
    float startingValue;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void reset()
    {
        Countdown::value = startingValue;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndLoop(const float deltaTimeMs)
    {
        return Countdown::updateAndLoop(deltaTimeMs, startingValue);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndStop(const float deltaTimeMs)
    {
        return Countdown::updateAndStop(deltaTimeMs);
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] LoopingTimer
{
    float value = 0.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndLoop(const float deltaTimeMs, const float target)
    {
        if (!updateAndStop(deltaTimeMs, target))
            return false;

        value = 0.f;
        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndStop(const float deltaTimeMs, const float target)
    {
        if (value < target)
        {
            value += deltaTimeMs;
            return false;
        }

        value = target;
        return true;
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] TargetedLoopingTimer : LoopingTimer
{
    float target;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndLoop(const float deltaTimeMs)
    {
        return LoopingTimer::updateAndLoop(deltaTimeMs, target);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndStop(const float deltaTimeMs)
    {
        return LoopingTimer::updateAndStop(deltaTimeMs, target);
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] TextShakeEffect
{
    float grow  = 0.f;
    float angle = 0.f;

    ////////////////////////////////////////////////////////////
    void bump(const float strength)
    {
        grow  = strength;
        angle = getRndFloat(-grow * 0.2f, grow * 0.2f);
    }

    ////////////////////////////////////////////////////////////
    void update(const float deltaTimeMs)
    {
        if (grow > 0.f)
            grow -= deltaTimeMs * 0.0165f;

        if (angle != 0.f)
        {
            const float sign = angle > 0.f ? 1.f : -1.f;
            angle -= sign * deltaTimeMs * 0.00565f;

            if (sign * angle < 0.f)
                angle = 0.f;
        }

        grow  = sf::base::clamp(grow, 0.f, 5.f);
        angle = sf::base::clamp(angle, -0.5f, 0.5f);
    }

    ////////////////////////////////////////////////////////////
    void applyToText(sf::Text& text) const
    {
        text.scale    = {1.f + grow * 0.2f, 1.f + grow * 0.2f};
        text.rotation = sf::radians(angle);
    }
};

////////////////////////////////////////////////////////////
enum class [[nodiscard]] BubbleType : U8
{
    Normal = 0u,
    Star   = 1u,
    Bomb   = 2u
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Bubble
{
    sf::Vector2f position;
    sf::Vector2f velocity;

    float scale;
    float radius;
    float rotation;

    BubbleType type;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        position += velocity * deltaTime;
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToSprite(sf::Sprite& sprite) const
    {
        sprite.position = position;
        sprite.scale    = {scale, scale};
        sprite.rotation = sf::radians(rotation);
    }
};

////////////////////////////////////////////////////////////
enum class [[nodiscard]] ParticleType : U8
{
    Bubble = 0u,
    Star   = 1u,
    Fire   = 2u,
    Hex    = 3u,
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] ParticleData
{
    sf::Vector2f position;
    sf::Vector2f velocity;

    float scale;
    float accelerationY;

    float opacity;
    float opacityDecay;

    float rotation;
    float torque;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        velocity.y += accelerationY * deltaTime;
        position += velocity * deltaTime;

        rotation += torque * deltaTime;

        opacity = sf::base::clamp(opacity - opacityDecay * deltaTime, 0.f, 1.f);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToTransformable(auto& transformable) const
    {
        transformable.position = position;
        transformable.scale    = {scale, scale};
        transformable.rotation = sf::radians(rotation);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure, gnu::always_inline]] inline U8 opacityAsAlpha() const
    {
        return static_cast<U8>(opacity * 255.f);
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Particle
{
    ParticleData data;
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
        sprite.color.a = data.opacityAsAlpha();
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline Particle makeParticle(
    const sf::Vector2f position,
    const ParticleType particleType,
    const float        scaleMult,
    const float        speedMult,
    const float        opacity = 1.f)
{
    return {.data = {.position      = position,
                     .velocity      = getRndVector2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * speedMult,
                     .scale         = getRndFloat(0.08f, 0.27f) * scaleMult,
                     .accelerationY = 0.002f,
                     .opacity       = opacity,
                     .opacityDecay  = getRndFloat(0.00025f, 0.0015f),
                     .rotation      = getRndFloat(0.f, sf::base::tau),
                     .torque        = getRndFloat(-0.002f, 0.002f)},
            .type = particleType};
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] TextParticle
{
    char         buffer[16];
    ParticleData data;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        data.update(deltaTime);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToText(sf::Text& text) const
    {
        text.setString(buffer); // TODO P1: should find a way to assign directly to text buffer

        data.applyToTransformable(text);
        text.origin = text.getLocalBounds().size / 2.f;

        text.setFillColor(text.getFillColor().withAlpha(data.opacityAsAlpha()));
        text.setOutlineColor(text.getOutlineColor().withAlpha(data.opacityAsAlpha()));
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline TextParticle makeTextParticle(const sf::Vector2f position, const int combo)
{
    return {.buffer = {},
            .data   = {.position      = {position.x, position.y - 10.f},
                       .velocity      = getRndVector2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.425f,
                       .scale         = sf::base::clamp(1.f + 0.1f * static_cast<float>(combo + 1) / 1.75f, 1.f, 3.0f),
                       .accelerationY = 0.0042f,
                       .opacity       = 1.f,
                       .opacityDecay  = 0.00175f,
                       .rotation      = 0.f,
                       .torque        = getRndFloat(-0.002f, 0.002f)}};
}

////////////////////////////////////////////////////////////
enum class [[nodiscard]] CatType : U8
{
    Normal = 0u,
    Uni    = 1u,
    Devil  = 2u,
    Witch  = 3u,
    Astro  = 4u,

    Count
};

////////////////////////////////////////////////////////////
constexpr auto nCatTypes = static_cast<SizeT>(CatType::Count);

////////////////////////////////////////////////////////////
struct [[nodiscard]] Cat
{
    CatType type;

    sf::Vector2f position;
    sf::Vector2f rangeOffset;

    LoopingTimer wobbleTimer;
    LoopingTimer cooldownTimer;

    sf::Vector2f drawPosition;

    sf::Vector2f pawPosition;
    sf::Angle    pawRotation;

    float mainOpacity = 255.f;
    float pawOpacity  = 255.f;

    Countdown inspiredCountdown;

    SizeT nameIdx;

    TextShakeEffect textStatusShakeEffect;

    int hits = 0;

    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] AstroState
    {
        float startX;

        float velocityX     = 0.f;
        bool  wrapped       = false;
        float particleTimer = 0.f;
    };

    sf::base::Optional<AstroState> astroState;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        textStatusShakeEffect.update(deltaTime);

        (void)wobbleTimer.updateAndLoop(deltaTime * 0.002f, sf::base::tau);

        drawPosition.x = position.x;
        drawPosition.y = position.y + std::sin(wobbleTimer.value * 2.f) * 7.5f;
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
};

////////////////////////////////////////////////////////////
[[nodiscard]] inline Cat makeCat(const CatType catType, const sf::Vector2f& position, const sf::Vector2f rangeOffset, const SizeT nameIdx)
{
    return Cat{.type                  = catType,
               .position              = position,
               .rangeOffset           = rangeOffset,
               .wobbleTimer           = {},
               .cooldownTimer         = {},
               .drawPosition          = position,
               .pawPosition           = position,
               .pawRotation           = sf::radians(0.f),
               .inspiredCountdown     = {},
               .nameIdx               = nameIdx,
               .textStatusShakeEffect = {},
               .astroState            = {}};
}


////////////////////////////////////////////////////////////
struct [[nodiscard]] PurchasableScalingValue
{
    const PSVData* data; // TODO: eww
    SizeT          nPurchases = 0u;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float nextCost() const
    {
        return computeGrowth(data->cost, static_cast<float>(nPurchases));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float currentValue() const
    {
        return computeGrowth(data->value, static_cast<float>(nPurchases));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] auto maxSubsequentPurchases(const MoneyType money, float globalMultiplier) const
    {
        struct Result
        {
            SizeT     times    = 0u;
            MoneyType maxCost  = 0u;
            MoneyType nextCost = 0u;
        } result;

        MoneyType cumulative = 0;

        for (SizeT i = nPurchases; i < data->nMaxPurchases; ++i)
        {
            const auto currentCost = static_cast<MoneyType>(
                computeGrowth(data->cost, static_cast<float>(i)) * globalMultiplier);

            // Check if we can afford to buy the next upgrade
            if (cumulative + currentCost > money)
                break;

            // Track cumulative cost and update results
            cumulative += currentCost;
            ++result.times;
            result.maxCost = cumulative;

            // Calculate the cumulative cost for the next potential purchase
            const auto nextCostCandidate = static_cast<MoneyType>(
                computeGrowth(data->cost, static_cast<float>(i + 1)) * globalMultiplier);
            result.nextCost = cumulative + nextCostCandidate;
        }

        // Handle edge case: no purchases possible, but next cost exists
        if (result.times == 0 && nPurchases < data->nMaxPurchases)
        {
            result.nextCost = static_cast<MoneyType>(
                computeGrowth(data->cost, static_cast<float>(nPurchases)) * globalMultiplier);
        }

        return result;
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Stats
{
    U64 secondsPlayed            = 0u;
    U64 bubblesPopped            = 0u;
    U64 bubblesPoppedRevenue     = 0u;
    U64 bubblesHandPopped        = 0u;
    U64 bubblesHandPoppedRevenue = 0u;
    U64 explosionRevenue         = 0u;
    U64 flightRevenue            = 0u;
};

////////////////////////////////////////////////////////////
static constexpr auto maxU64 = std::numeric_limits<U64>::max();

////////////////////////////////////////////////////////////
struct [[nodiscard]] Milestones
{
    U64 firstCat      = maxU64;
    U64 firstUnicat   = maxU64;
    U64 firstDevilcat = maxU64;
    U64 firstAstrocat = maxU64;

    U64 fiveCats      = maxU64;
    U64 fiveUnicats   = maxU64;
    U64 fiveDevilcats = maxU64;
    U64 fiveAstrocats = maxU64;

    U64 tenCats      = maxU64;
    U64 tenUnicats   = maxU64;
    U64 tenDevilcats = maxU64;
    U64 tenAstrocats = maxU64;

    U64 prestigeLevel1  = maxU64;
    U64 prestigeLevel2  = maxU64;
    U64 prestigeLevel3  = maxU64;
    U64 prestigeLevel4  = maxU64;
    U64 prestigeLevel5  = maxU64;
    U64 prestigeLevel10 = maxU64;
    U64 prestigeLevel15 = maxU64;
    U64 prestigeLevel20 = maxU64;

    U64 revenue10000      = maxU64;
    U64 revenue100000     = maxU64;
    U64 revenue1000000    = maxU64;
    U64 revenue10000000   = maxU64;
    U64 revenue100000000  = maxU64;
    U64 revenue1000000000 = maxU64;
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Profile
{
    float masterVolume          = 100.f;
    float musicVolume           = 100.f;
    bool  playAudioInBackground = true;
    bool  playComboEndSound     = true;
    float minimapScale          = 20.f;
    bool  tipsEnabled           = true;

    Stats statsLifetime;
};

////////////////////////////////////////////////////////////
struct Game
{
    //
    // PSV instances
    PurchasableScalingValue psvComboStartTime{&PSVDataConstants::comboStartTime};
    PurchasableScalingValue psvBubbleCount{&PSVDataConstants::bubbleCount};
    PurchasableScalingValue psvBubbleValue{&PSVDataConstants::bubbleValue};
    PurchasableScalingValue psvExplosionRadiusMult{&PSVDataConstants::explosionRadiusMult};

    PurchasableScalingValue psvCooldownMultsPerCatType[nCatTypes]{{&PSVDataConstants::catNormalCooldownMult},
                                                                  {&PSVDataConstants::catUniCooldownMult},
                                                                  {&PSVDataConstants::catDevilCooldownMult},
                                                                  {&PSVDataConstants::catWitchCooldownMult},
                                                                  {&PSVDataConstants::catAstroCooldownMult}};

    PurchasableScalingValue psvRangeDivsPerCatType[nCatTypes]{{&PSVDataConstants::catNormalRangeDiv},
                                                              {&PSVDataConstants::catUniRangeDiv},
                                                              {&PSVDataConstants::catDevilRangeDiv},
                                                              {&PSVDataConstants::catWitchRangeDiv},
                                                              {&PSVDataConstants::catAstroRangeDiv}};

    //
    // Permanent PSV instances
    PurchasableScalingValue psvMultiPopRange{&PSVDataConstants::multiPopRange};
    PurchasableScalingValue psvInspireDurationMult{&PSVDataConstants::inspireDurationMult};

    //
    // Currencies
    MoneyType money = 0u;

    //
    // Permanent currencies
    U64 prestigePoints = 0u;

    //
    // Purchases
    bool comboPurchased    = false;
    bool mapPurchased      = false;
    U64  mapLimitIncreases = 0u;

    //
    // Permanent purchases
    bool multiPopPurchased        = false;
    bool smartCatsPurchased       = false;
    bool geniusCatsPurchased      = false;
    bool windPurchased            = false;
    bool astroCatInspirePurchased = false;

    //
    // Permanent purchases settings
    bool multiPopEnabled              = false;
    bool windEnabled                  = false;
    bool geniusCatIgnoreNormalBubbles = false;
    bool geniusCatIgnoreStarBubbles   = false;
    bool geniusCatIgnoreBombBubbles   = false;

    //
    // Object state
    std::vector<Bubble> bubbles;
    std::vector<Cat>    cats;

    //
    // Statistics
    Stats      statsTotal;
    Stats      statsSession;
    Milestones milestones;

    //
    // Other flags
    bool prestigeTipShown = false;

    ////////////////////////////////////////////////////////////
    void onPrestige(const U64 prestigeCount)
    {
        psvComboStartTime.nPurchases      = 0u;
        psvBubbleCount.nPurchases         = 0u;
        psvExplosionRadiusMult.nPurchases = 0u;

        for (auto& psv : psvCooldownMultsPerCatType)
            psv.nPurchases = 0u;

        for (auto& psv : psvRangeDivsPerCatType)
            psv.nPurchases = 0u;

        money = 0u;
        prestigePoints += prestigeCount;

        comboPurchased    = false;
        mapPurchased      = false;
        mapLimitIncreases = 0u;

        windEnabled = false;

        statsSession = {};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getMapLimit() const
    {
        return resolution.x * static_cast<float>(mapLimitIncreases + 1u);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr float getBaseCooldownByCatType(const CatType type) const
    {
        constexpr float baseCooldowns[nCatTypes]{
            1000.f, // Normal
            3000.f, // Uni
            7000.f, // Devil
            2000.f, // Witch
            10000.f // Astro
        };

        return baseCooldowns[static_cast<U8>(type)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr float getBaseRangeByCatType(const CatType type) const
    {
        constexpr float baseRanges[nCatTypes]{
            96.f,  // Normal
            64.f,  // Uni
            48.f,  // Devil
            256.f, // Witch
            48.f   // Astro
        };

        return baseRanges[static_cast<U8>(type)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline PurchasableScalingValue& getCooldownMultPSVByCatType(const CatType catType)
    {
        return psvCooldownMultsPerCatType[static_cast<U8>(catType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline PurchasableScalingValue& getRangeDivPSVByCatType(const CatType catType)
    {
        return psvRangeDivsPerCatType[static_cast<U8>(catType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedCooldownByCatType(const CatType catType)
    {
        return getBaseCooldownByCatType(catType) * getCooldownMultPSVByCatType(catType).currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedRangeByCatType(const CatType catType)
    {
        return getBaseRangeByCatType(catType) / getRangeDivPSVByCatType(catType).currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedBombExplosionRadius() const
    {
        return 200.f * psvExplosionRadiusMult.currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedInspirationDuration() const
    {
        return 1500.f * psvInspireDurationMult.currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getComputedMultiPopRange() const
    {
        return psvMultiPopRange.currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SizeT getCatCountByType(const CatType type) const
    {
        return sf::base::countIf(cats.begin(), cats.end(), [type](const Cat& cat) { return cat.type == type; });
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getComputedGlobalCostMultiplier() const
    {
        // TODO: optimize/cache the counts
        const auto nCatNormal = getCatCountByType(CatType::Normal);
        const auto nCatUni    = getCatCountByType(CatType::Uni);
        const auto nCatDevil  = getCatCountByType(CatType::Devil);
        const auto nCatWitch  = getCatCountByType(CatType::Witch);
        const auto nCatAstro  = getCatCountByType(CatType::Astro);

        // [ 0.25, 0.25 + 0.125, 0.25 + 0.125 + 0.0625, ... ]
        const auto geomSum = [](auto n)
        { return static_cast<float>(n) <= 0.f ? 0.f : 0.5f * (1.f - std::pow(0.5f, static_cast<float>(n) + 1.f)); };

        return 1.f +                                            //
               (geomSum(psvComboStartTime.nPurchases) * 0.1f) + //
               (geomSum(psvBubbleCount.nPurchases) * 0.5f) +    //
               (geomSum(psvBubbleValue.nPurchases) * 0.75f) +   //
               (geomSum(nCatNormal) * 0.35f) +                  //
               (geomSum(nCatUni) * 0.5f) +                      //
               (geomSum(nCatDevil) * 0.75f) +                   //
               (geomSum(nCatWitch) * 0.75f) +                   //
               (geomSum(nCatAstro) * 0.75f);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isBubbleValueUnlocked() const // also unlocks prestige
    {
        const auto nCatUni = getCatCountByType(CatType::Uni);
        return psvBubbleValue.nPurchases > 0 || (psvBubbleCount.nPurchases > 0 && nCatUni >= 3);
    }
};
