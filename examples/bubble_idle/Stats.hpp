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
};
