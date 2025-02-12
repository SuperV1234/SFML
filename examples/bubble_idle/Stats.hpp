#pragma once

#include "Bubble.hpp"
#include "PSVDataConstants.hpp"

#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Stats
{
    sf::base::U64 secondsPlayed                                           = 0u;
    sf::base::U64 nBubblesPoppedByType[nBubbleTypes]                      = {};
    sf::base::U64 revenueByType[nBubbleTypes]                             = {};
    sf::base::U64 nBubblesHandPoppedByType[nBubbleTypes]                  = {};
    sf::base::U64 revenueHandByType[nBubbleTypes]                         = {};
    sf::base::U64 explosionRevenue                                        = 0u;
    sf::base::U64 flightRevenue                                           = 0u;
    sf::base::U64 highestStarBubblePopCombo                               = 0u;
    sf::base::U64 nAbsorbedStarBubbles                                    = 0u;
    sf::base::U64 nSpellCasts[PSVDataConstants::spellCount.nMaxPurchases] = {};
    sf::base::U64 nMaintenances                                           = 0u;
    sf::base::U64 highestSimultaneousMaintenances                         = 0u;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getTotalNBubblesPopped() const noexcept
    {
        sf::base::U64 acc = 0u;

        for (const auto x : nBubblesPoppedByType)
            acc += x;

        return acc;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getTotalRevenue() const noexcept
    {
        sf::base::U64 acc = 0u;

        for (const auto x : revenueByType)
            acc += x;

        return acc;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getTotalNBubblesHandPopped() const noexcept
    {
        sf::base::U64 acc = 0u;

        for (const auto x : nBubblesHandPoppedByType)
            acc += x;

        return acc;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getTotalRevenueHand() const noexcept
    {
        sf::base::U64 acc = 0u;

        for (const auto x : revenueHandByType)
            acc += x;

        return acc;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getTotalNBubblesCatPopped() const noexcept
    {
        return getTotalNBubblesPopped() - getTotalNBubblesHandPopped();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getTotalRevenueCat() const noexcept
    {
        return getTotalRevenue() - getTotalRevenueHand();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getNBubblesPopped(const BubbleType bubbleType) const noexcept
    {
        return nBubblesPoppedByType[asIdx(bubbleType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getRevenue(const BubbleType bubbleType) const noexcept
    {
        return revenueByType[asIdx(bubbleType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getNBubblesHandPopped(const BubbleType bubbleType) const noexcept
    {
        return nBubblesHandPoppedByType[asIdx(bubbleType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getRevenueHand(const BubbleType bubbleType) const noexcept
    {
        return revenueHandByType[asIdx(bubbleType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getNBubblesCatPopped(const BubbleType bubbleType) const noexcept
    {
        return getNBubblesPopped(bubbleType) - getNBubblesHandPopped(bubbleType);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] sf::base::U64 getRevenueCat(const BubbleType bubbleType) const noexcept
    {
        return getRevenue(bubbleType) - getRevenueHand(bubbleType);
    }
};
