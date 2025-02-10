#pragma once

#include "Aliases.hpp"
#include "Bubble.hpp"
#include "Cat.hpp"
#include "CatConstants.hpp"
#include "Constants.hpp"
#include "ExactArray.hpp"
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
    PurchasableScalingValue psvStarpawPercentage{&PSVDataConstants::starpawPercentage};
    PurchasableScalingValue psvMewltiplierMult{&PSVDataConstants::mewltiplierMult};

    EXACT_ARRAY(PurchasableScalingValue,
                psvPerCatType,
                nCatTypes,
                {
                    {&PSVDataConstants::catNormal},
                    {&PSVDataConstants::catUni},
                    {&PSVDataConstants::catDevil},
                    {&PSVDataConstants::catWitch},
                    {&PSVDataConstants::catAstro},

                    {&PSVDataConstants::catWizard},  // TODO P1: unused?
                    {&PSVDataConstants::catMouse},   // TODO P1: unused?
                    {&PSVDataConstants::catEngi},    // TODO P1: unused?
                    {&PSVDataConstants::catRepulso}, // TODO P1: unused?
                });

    EXACT_ARRAY(PurchasableScalingValue,
                psvCooldownMultsPerCatType,
                nCatTypes,
                {
                    {&PSVDataConstants::catNormalCooldownMult},
                    {&PSVDataConstants::catUniCooldownMult},
                    {&PSVDataConstants::catDevilCooldownMult},
                    {&PSVDataConstants::catWitchCooldownMult},
                    {&PSVDataConstants::catAstroCooldownMult},

                    {&PSVDataConstants::catWizardCooldownMult},
                    {&PSVDataConstants::catMouseCooldownMult},
                    {&PSVDataConstants::catEngiCooldownMult},
                    {&PSVDataConstants::catRepulsoCooldownMult},
                });

    EXACT_ARRAY(PurchasableScalingValue,
                psvRangeDivsPerCatType,
                nCatTypes,
                {
                    {&PSVDataConstants::catNormalRangeDiv},
                    {&PSVDataConstants::catUniRangeDiv},
                    {&PSVDataConstants::catDevilRangeDiv},
                    {&PSVDataConstants::catWitchRangeDiv},
                    {&PSVDataConstants::catAstroRangeDiv},

                    {&PSVDataConstants::catWizardRangeDiv},
                    {&PSVDataConstants::catMouseRangeDiv},
                    {&PSVDataConstants::catEngiRangeDiv},
                    {&PSVDataConstants::catRepulsoRangeDiv},
                });

    //
    // Permanent PSV instances
    PurchasableScalingValue psvPPMultiPopRange{&PSVDataConstants::multiPopRange};
    PurchasableScalingValue psvPPInspireDurationMult{&PSVDataConstants::inspireDurationMult};
    PurchasableScalingValue psvPPManaCooldownMult{&PSVDataConstants::manaCooldownMult};
    PurchasableScalingValue psvPPManaMaxMult{&PSVDataConstants::manaMaxMult};
    PurchasableScalingValue psvPPMouseCatGlobalBonusMult{&PSVDataConstants::mouseCatGlobalBonusMult};
    PurchasableScalingValue psvPPEngiCatGlobalBonusMult{&PSVDataConstants::engiCatGlobalBonusMult};
    PurchasableScalingValue psvPPRepulsoCatConverterChance{&PSVDataConstants::repulsoCatConverterChance};

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
    // Magic
    float     manaTimer       = 0.f;
    ManaType  mana            = 0u;
    bool      absorbingWisdom = false;
    MoneyType wisdom          = 0u;
    float     arcaneAuraTimer = 0.f;

    //
    // Mouse cat
    int       mouseCatCombo = 0u;
    Countdown mouseCatComboCountdown;

    //
    // Permanent purchases
    bool multiPopPurchased            = false;
    bool smartCatsPurchased           = false;
    bool geniusCatsPurchased          = false; // if true, `smartCatsPurchased` must be true
    bool windPurchased                = false;
    bool astroCatInspirePurchased     = false;
    bool starpawConversionIgnoreBombs = false;
    bool repulsoCatFilterPurchased    = false;
    bool repulsoCatConverterPurchased = false;

    //
    // Permanent purchases settings
    bool multiPopEnabled               = false;
    bool windEnabled                   = false;
    bool geniusCatIgnoreNormalBubbles  = false;
    bool geniusCatIgnoreStarBubbles    = false;
    bool geniusCatIgnoreBombBubbles    = false;
    bool repulsoCatIgnoreNormalBubbles = false;
    bool repulsoCatIgnoreStarBubbles   = false;
    bool repulsoCatIgnoreBombBubbles   = false;
    bool repulsoCatConverterEnabled    = false;

    //
    // Object state
    std::vector<Bubble> bubbles;
    std::vector<Cat>    cats;
    std::vector<Shrine> shrines;

    //
    // Shrines
    SizeT nShrinesCompleted = 0u;

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
    [[nodiscard]] PrestigePointsType calculatePrestigePointReward(const PrestigePointsType prestigeCount) const
    {
        const auto currentPrestigeLevel = static_cast<PrestigePointsType>(psvBubbleValue.currentValue());
        const auto targetPrestigeLevel  = currentPrestigeLevel + prestigeCount;

        PrestigePointsType pointsToAdd = 0u;

        for (auto i = currentPrestigeLevel; i < targetPrestigeLevel; ++i)
            pointsToAdd += static_cast<PrestigePointsType>(std::pow(2.f, static_cast<float>(i)));

        return pointsToAdd;
    }

    ////////////////////////////////////////////////////////////
    void onPrestige(const PrestigePointsType prestigePointReward)
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
        prestigePoints += prestigePointReward;

        comboPurchased = false;
        mapPurchased   = false;

        manaTimer       = 0.f;
        mana            = 0u;
        absorbingWisdom = false;
        wisdom          = 0u;
        arcaneAuraTimer = 0.f;

        mouseCatCombo                = 0.f;
        mouseCatComboCountdown.value = 0.f;

        windEnabled = false;

        // bubbles, cats, and shrines are cleaned in the game loop

        nShrinesCompleted = 0u;

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
        constexpr EXACT_ARRAY(MoneyType,
                              baseRewards,
                              nBubbleTypes,
                              {
                                  1u,  // Normal
                                  25u, // Star
                                  1u,  // Bomb
                              });

        return baseRewards[asIdx(type)] * static_cast<MoneyType>(psvBubbleValue.currentValue() + 1.f);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline MoneyType getComputedRequiredRewardByShrineType(const ShrineType type) const
    {
        constexpr EXACT_ARRAY(MoneyType,
                              baseRequiredRewards,
                              nShrineTypes,
                              {
                                  1'000,         // Magic // TODO P0: decide on values, implement all magic spells
                                  10'000,        // Clicking
                                  50'000,        // Automation
                                  150'000,       // Repulsion
                                  500'000,       // Attraction
                                  1'500'000,     // Decay
                                  25'000'000,    // Chaos
                                  500'000'000,   // Transmutation
                                  1'000'000'000, // Victory
                              });

        return static_cast<MoneyType>(
            static_cast<float>(baseRequiredRewards[asIdx(type)]) * getComputedGlobalCostMultiplier());
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedCooldownByCatType(const CatType catType) const
    {
        return CatConstants::baseCooldowns[asIdx(catType)] * psvCooldownMultsPerCatType[asIdx(catType)].currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedRangeByCatType(const CatType catType) const
    {
        const auto result = CatConstants::baseRanges[asIdx(catType)] /
                            psvRangeDivsPerCatType[asIdx(catType)].currentValue();

        if (catType == CatType::Wizard && absorbingWisdom)
            return result / 2.f;

        return result;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline float getComputedSquaredRangeByCatType(const CatType catType) const
    {
        const float range = getComputedRangeByCatType(catType);
        return range * range;
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
        return psvPerCatType[asIdx(type)].nPurchases;
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
