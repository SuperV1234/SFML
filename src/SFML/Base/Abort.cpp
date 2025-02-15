#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Base/Abort.hpp"

#include <cstdlib>


namespace sf::base
{
////////////////////////////////////////////////////////////
void abort() noexcept
{
    std::abort();
}

} // namespace sf::base
