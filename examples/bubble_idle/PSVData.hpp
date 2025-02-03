#pragma once

#include "GrowthFactors.hpp"

#include "SFML/Base/SizeT.hpp"

////////////////////////////////////////////////////////////
struct [[nodiscard]] PSVData
{
    const sf::base::SizeT nMaxPurchases;
    const GrowthFactors   cost;
    const GrowthFactors   value;
};
