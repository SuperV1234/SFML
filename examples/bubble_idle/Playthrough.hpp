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

#include <imgui.h>

#include <climits>
#include <cstdio>


////////////////////////////////////////////////////////////
struct Playthrough
{
    //
    // PSV instances
    PurchasableScalingValue psvComboStartTime{&PSVDataConstants::comboStartTime};
    PurchasableScalingValue psvMapExtension{&PSVDataConstants::mapExtension};
    PurchasableScalingValue psvShrineActivation{&PSVDataConstants::shrineActivation};
    PurchasableScalingValue psvBubbleCount{&PSVDataConstants::bubbleCount};
    PurchasableScalingValue psvBubbleValue{&PSVDataConstants::bubbleValue}; // prestige
    PurchasableScalingValue psvExplosionRadiusMult{&PSVDataConstants::explosionRadiusMult};

    PurchasableScalingValue psvPerCatType[nCatTypes]{{&PSVDataConstants::catNormal},
                                                     {&PSVDataConstants::catUni},
                                                     {&PSVDataConstants::catDevil},
                                                     {&PSVDataConstants::catWitch},
                                                     {&PSVDataConstants::catAstro}};

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
    PrestigePointsType prestigePoints = 0u;

    //
    // Purchases
    bool comboPurchased = false;
    bool mapPurchased   = false;

    //
    // Permanent purchases
    bool multiPopPurchased        = false;
    bool smartCatsPurchased       = false;
    bool geniusCatsPurchased      = false; // if true, `smartCatsPurchased` must be true
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
    std::vector<Shrine> shrines;

    //
    // Statistics
    Stats      statsTotal;
    Stats      statsSession;
    Milestones milestones;

    //
    // Other flags
    bool prestigeTipShown = false;

    ////////////////////////////////////////////////////////////
    void onPrestige(const PrestigePointsType prestigeCount)
    {
        psvComboStartTime.nPurchases      = 0u;
        psvMapExtension.nPurchases        = 0u;
        psvBubbleCount.nPurchases         = 0u;
        psvExplosionRadiusMult.nPurchases = 0u;

        for (auto& psv : psvPerCatType)
            psv.nPurchases = 0u;

        for (auto& psv : psvCooldownMultsPerCatType)
            psv.nPurchases = 0u;

        for (auto& psv : psvRangeDivsPerCatType)
            psv.nPurchases = 0u;

        money = 0u;
        prestigePoints += prestigeCount;

        comboPurchased = false;
        mapPurchased   = false;

        windEnabled = false;

        // bubbles, cats, and shrines are cleaned in the game loop

        statsSession = {};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline SizeT getMapLimitIncreases() const
    {
        return static_cast<SizeT>(mapPurchased) + psvMapExtension.nPurchases;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getMapLimit() const
    {
        return gameScreenSize.x * static_cast<float>(getMapLimitIncreases() + 1u);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr MoneyType getBaseRewardByBubbleType(const BubbleType type) const
    {
        constexpr MoneyType baseRewards[nBubbleTypes]{
            1u,  // Normal
            25u, // Star
            1u,  // Bomb
        };

        return baseRewards[static_cast<U8>(type)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr MoneyType getComputedRewardByBubbleType(const BubbleType type) const
    {
        return getBaseRewardByBubbleType(type) * static_cast<MoneyType>(psvBubbleValue.currentValue() + 1.f);
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
    [[nodiscard, gnu::always_inline]] inline PurchasableScalingValue& getPSVByCatType(const CatType catType)
    {
        return psvPerCatType[static_cast<U8>(catType)];
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
        return psvPerCatType[static_cast<U8>(type)].nPurchases;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getComputedGlobalCostMultiplier() const
    {
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
