#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/JoystickIdentification.hpp"
#include "SFML/Window/Stub/StubJoystickImpl.hpp"

#include "SFML/System/Err.hpp"


namespace sf::priv
{
////////////////////////////////////////////////////////////
void StubJoystickImpl::initialize()
{
    // err() << "Joystick API not implemented";
}


////////////////////////////////////////////////////////////
void StubJoystickImpl::cleanup()
{
    // err() << "Joystick API not implemented";
}


////////////////////////////////////////////////////////////
bool StubJoystickImpl::isConnected(unsigned int /* index */)
{
    // err() << "Joystick API not implemented";
    return false;
}


////////////////////////////////////////////////////////////
bool StubJoystickImpl::open(unsigned int /* index */)
{
    // err() << "Joystick API not implemented";
    return false;
}


////////////////////////////////////////////////////////////
void StubJoystickImpl::close()
{
    // err() << "Joystick API not implemented";
}


////////////////////////////////////////////////////////////
JoystickCapabilities StubJoystickImpl::getCapabilities() const
{
    // err() << "Joystick API not implemented";
    return {};
}


////////////////////////////////////////////////////////////
const JoystickIdentification& StubJoystickImpl::getIdentification() const
{
    // err() << "Joystick API not implemented";

    static JoystickIdentification result;
    return result;
}


////////////////////////////////////////////////////////////
JoystickState StubJoystickImpl::update()
{
    // err() << "Joystick API not implemented";
    return {};
}

} // namespace sf::priv
