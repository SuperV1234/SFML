#pragma once

#include "Aliases.hpp"
#include "Bubble.hpp"
#include "Cat.hpp"
#include "Constants.hpp"
#include "Countdown.hpp"
#include "LoopingTimer.hpp"
#include "PSVDataConstants.hpp"
#include "ParticleData.hpp"
#include "PurchasableScalingValue.hpp"
#include "RNG.hpp"
#include "TextShakeEffect.hpp"

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
#include "SFML/Base/Optional.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <limits>

#include <climits>
#include <cstdio>


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
