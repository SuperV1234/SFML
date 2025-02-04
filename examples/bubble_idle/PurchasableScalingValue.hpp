#pragma once

#include "Aliases.hpp"
#include "PSVData.hpp"

#include "SFML/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] PurchasableScalingValue
{
    const PSVData*  data; // TODO: eww
    sf::base::SizeT nPurchases = 0u;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float nextCost() const
    {
        return data->cost.computeGrowth(static_cast<float>(nPurchases));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float currentValue() const
    {
        return data->value.computeGrowth(static_cast<float>(nPurchases));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] auto maxSubsequentPurchases(const MoneyType money, float globalMultiplier) const
    {
        struct Result
        {
            sf::base::SizeT times    = 0u;
            MoneyType       maxCost  = 0u;
            MoneyType       nextCost = 0u;
        } result;

        MoneyType cumulative = 0;

        for (sf::base::SizeT i = nPurchases; i < data->nMaxPurchases; ++i)
        {
            const auto currentCost = static_cast<MoneyType>(
                data->cost.computeGrowth(static_cast<float>(i)) * globalMultiplier);

            // Check if we can afford to buy the next upgrade
            if (cumulative + currentCost > money)
                break;

            // Track cumulative cost and update results
            cumulative += currentCost;
            ++result.times;
            result.maxCost = cumulative;

            // Calculate the cumulative cost for the next potential purchase
            const auto nextCostCandidate = static_cast<MoneyType>(
                data->cost.computeGrowth(static_cast<float>(i + 1)) * globalMultiplier);
            result.nextCost = cumulative + nextCostCandidate;
        }

        // Handle edge case: no purchases possible, but next cost exists
        if (result.times == 0 && nPurchases < data->nMaxPurchases)
        {
            result.nextCost = static_cast<MoneyType>(
                data->cost.computeGrowth(static_cast<float>(nPurchases)) * globalMultiplier);
        }

        return result;
    }
};
