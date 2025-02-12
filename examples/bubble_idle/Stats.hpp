#pragma once

#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Stats
{
    sf::base::U64 secondsPlayed            = 0u;
    sf::base::U64 bubblesPopped            = 0u;
    sf::base::U64 bubblesPoppedRevenue     = 0u;
    sf::base::U64 bubblesHandPopped        = 0u;
    sf::base::U64 bubblesHandPoppedRevenue = 0u;
    sf::base::U64 explosionRevenue         = 0u;
    sf::base::U64 flightRevenue            = 0u;

    [[nodiscard]] sf::base::U64 getBubblesCatPopped() const noexcept
    {
        return bubblesPopped - bubblesHandPopped;
    }

    [[nodiscard]] sf::base::U64 getBubblesCatPoppedRevenue() const noexcept
    {
        return bubblesPoppedRevenue - bubblesHandPoppedRevenue;
    }
};
