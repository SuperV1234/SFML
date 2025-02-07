#pragma once

#include "Aliases.hpp"
#include "Bubble.hpp"
#include "Cat.hpp"
#include "CatConstants.hpp"
#include "Constants.hpp"
#include "Milestones.hpp"
#include "PSVDataConstants.hpp"
#include "PurchasableScalingValue.hpp"
#include "RNG.hpp"
#include "Shrine.hpp"
#include "Stats.hpp"

#include "SFML/System/Vector2.hpp"

#include <vector>

#include <cmath>


////////////////////////////////////////////////////////////
struct Playthrough
{
    //
    // RNG stuff
    RNG::SeedType seed;
    SizeT         nextCatName = 0u;

    //
    // PSV instances
    PurchasableScalingValue psvComboStartTime{&PSVDataConstants::comboStartTime};
    PurchasableScalingValue psvMapExtension{&PSVDataConstants::mapExtension};
    PurchasableScalingValue psvShrineActivation{&PSVDataConstants::shrineActivation};
    PurchasableScalingValue psvBubbleCount{&PSVDataConstants::bubbleCount};
    PurchasableScalingValue psvSpellCount{&PSVDataConstants::spellCount};
    PurchasableScalingValue psvBubbleValue{&PSVDataConstants::bubbleValue}; // also tracks prestige level
    PurchasableScalingValue psvExplosionRadiusMult{&PSVDataConstants::explosionRadiusMult};

    PurchasableScalingValue psvPerCatType[nCatTypes]{
        {&PSVDataConstants::catNormal},
        {&PSVDataConstants::catUni},
        {&PSVDataConstants::catDevil},
        {&PSVDataConstants::catWitch},
        {&PSVDataConstants::catAstro},
        {&PSVDataConstants::catWizard}, // TODO: unused?
    };

    PurchasableScalingValue psvCooldownMultsPerCatType[nCatTypes]{
        {&PSVDataConstants::catNormalCooldownMult},
        {&PSVDataConstants::catUniCooldownMult},
        {&PSVDataConstants::catDevilCooldownMult},
        {&PSVDataConstants::catWitchCooldownMult},
        {&PSVDataConstants::catAstroCooldownMult},
        {&PSVDataConstants::catWizardCooldownMult}, // TODO: unused? or change to PP
    };

    PurchasableScalingValue psvRangeDivsPerCatType[nCatTypes]{
        {&PSVDataConstants::catNormalRangeDiv},
        {&PSVDataConstants::catUniRangeDiv},
        {&PSVDataConstants::catDevilRangeDiv},
        {&PSVDataConstants::catWitchRangeDiv},
        {&PSVDataConstants::catAstroRangeDiv},
        {&PSVDataConstants::catWizardRangeDiv}, // TODO: unused? or change to PP
    };

    //
    // Permanent PSV instances
    PurchasableScalingValue psvPPMultiPopRange{&PSVDataConstants::multiPopRange};
    PurchasableScalingValue psvPPInspireDurationMult{&PSVDataConstants::inspireDurationMult};
    PurchasableScalingValue psvPPManaCooldownMult{&PSVDataConstants::manaCooldownMult};
    PurchasableScalingValue psvPPManaMaxMult{&PSVDataConstants::manaMaxMult};

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
    // Shrine rewards
    bool      magicUnlocked   = false;
    float     manaTimer       = 0.f;
    ManaType  mana            = 0u;
    bool      absorbingWisdom = false;
    MoneyType wisdom          = 0u;

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
    bool shrinesSpawned   = false;

    ////////////////////////////////////////////////////////////
    void spawnAllShrinesIfNeeded()
    {
        if (shrinesSpawned)
            return;

        shrinesSpawned = true;

        for (SizeT i = 0u; i < 9u; ++i)
            shrines.push_back({
                .position              = gameScreenSize / 2.f + sf::Vector2f{gameScreenSize.x * (i + 1u), 0.f},
                .tcActivation          = {},
                .tcDeath               = {},
                .textStatusShakeEffect = {},
                .type                  = static_cast<ShrineType>(i),
            });
    }

    ////////////////////////////////////////////////////////////
    void onPrestige(const PrestigePointsType prestigeCount)
    {
        psvComboStartTime.nPurchases      = 0u;
        psvMapExtension.nPurchases        = 0u;
        psvShrineActivation.nPurchases    = 0u;
        psvBubbleCount.nPurchases         = 0u;
        psvSpellCount.nPurchases          = 0u;
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

        magicUnlocked   = false;
        manaTimer       = 0.f;
        mana            = 0u;
        absorbingWisdom = false;
        wisdom          = 0u;

        windEnabled = false;

        // bubbles, cats, and shrines are cleaned in the game loop

        statsSession = {};

        shrinesSpawned = false;
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
    [[nodiscard]] constexpr MoneyType getComputedRewardByBubbleType(const BubbleType type) const
    {
        constexpr MoneyType baseRewards[nBubbleTypes]{
            1u,  // Normal
            25u, // Star
            1u,  // Bomb
        };

        return baseRewards[static_cast<U8>(type)] * static_cast<MoneyType>(psvBubbleValue.currentValue() + 1.f);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline MoneyType getComputedRequiredRewardByShrineType(const ShrineType type) const
    {
        constexpr MoneyType baseRequiredRewards[nShrineTypes]{
            1'0 /*00*/,    // Magic // TODO P0: decide on values, implement all magic spells
            1'000,         // Clicking
            1'000,         // Automation
            10'000,        // Repulsion
            100'000,       // Attraction
            1'000'000,     // Decay
            10'000'000,    // Chaos
            100'000'000,   // Transmutation
            1'000'000'000, // Victory
        };

        return static_cast<MoneyType>(
            static_cast<float>(baseRequiredRewards[static_cast<U8>(type)]) * getComputedGlobalCostMultiplier());
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
        return CatConstants::baseCooldowns[static_cast<U8>(catType)] * getCooldownMultPSVByCatType(catType).currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedRangeByCatType(const CatType catType)
    {
        return CatConstants::baseRanges[static_cast<U8>(catType)] / getRangeDivPSVByCatType(catType).currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedBombExplosionRadius() const
    {
        return 200.f * psvExplosionRadiusMult.currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedInspirationDuration() const
    {
        return 1500.f * psvPPInspireDurationMult.currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedManaCooldown() const
    {
        return 10000.f * psvPPManaCooldownMult.currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline ManaType getComputedMaxMana() const
    {
        return static_cast<ManaType>(20.f * psvPPManaMaxMult.currentValue());
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getComputedMultiPopRange() const
    {
        return psvPPMultiPopRange.currentValue();
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
