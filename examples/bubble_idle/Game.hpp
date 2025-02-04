#pragma once

#include "Aliases.hpp"
#include "Bubble.hpp"
#include "Cat.hpp"
#include "Constants.hpp"
#include "Milestones.hpp"
#include "PSVDataConstants.hpp"
#include "PurchasableScalingValue.hpp"
#include "Stats.hpp"

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Algorithm.hpp"

#include <imgui.h>

#include <climits>
#include <cstdio>


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
        return gameScreenSize.x * static_cast<float>(mapLimitIncreases + 1u);
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
