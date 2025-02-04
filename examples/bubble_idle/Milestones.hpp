#pragma once

#include "SFML/Base/IntTypes.hpp"

#include <limits>


////////////////////////////////////////////////////////////
static inline constexpr auto maxU64 = std::numeric_limits<sf::base::U64>::max();

////////////////////////////////////////////////////////////
struct [[nodiscard]] Milestones
{
    sf::base::U64 firstCat      = maxU64;
    sf::base::U64 firstUnicat   = maxU64;
    sf::base::U64 firstDevilcat = maxU64;
    sf::base::U64 firstAstrocat = maxU64;

    sf::base::U64 fiveCats      = maxU64;
    sf::base::U64 fiveUnicats   = maxU64;
    sf::base::U64 fiveDevilcats = maxU64;
    sf::base::U64 fiveAstrocats = maxU64;

    sf::base::U64 tenCats      = maxU64;
    sf::base::U64 tenUnicats   = maxU64;
    sf::base::U64 tenDevilcats = maxU64;
    sf::base::U64 tenAstrocats = maxU64;

    sf::base::U64 prestigeLevel1  = maxU64;
    sf::base::U64 prestigeLevel2  = maxU64;
    sf::base::U64 prestigeLevel3  = maxU64;
    sf::base::U64 prestigeLevel4  = maxU64;
    sf::base::U64 prestigeLevel5  = maxU64;
    sf::base::U64 prestigeLevel10 = maxU64;
    sf::base::U64 prestigeLevel15 = maxU64;
    sf::base::U64 prestigeLevel20 = maxU64;

    sf::base::U64 revenue10000      = maxU64;
    sf::base::U64 revenue100000     = maxU64;
    sf::base::U64 revenue1000000    = maxU64;
    sf::base::U64 revenue10000000   = maxU64;
    sf::base::U64 revenue100000000  = maxU64;
    sf::base::U64 revenue1000000000 = maxU64;
};
