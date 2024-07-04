////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio/AudioDevice.hpp>
#include <SFML/Audio/AudioResource.hpp>

#include <iostream>
#include <mutex>
#include <optional>


namespace
{
////////////////////////////////////////////////////////////
struct DeviceState
{
    std::mutex                           mutex;
    std::optional<sf::priv::AudioDevice> device;
    unsigned int                         referenceCounter{};

    DeviceState()
    {
        std::cout << "DeviceState::DeviceState()\n";
    }

    ~DeviceState()
    {
        std::cout << "DeviceState::~DeviceState()\n";
    }
};

////////////////////////////////////////////////////////////
DeviceState& getDeviceState()
{
    static DeviceState deviceState;
    return deviceState;
}

} // namespace

namespace sf
{
////////////////////////////////////////////////////////////
AudioResource::AudioResource()
{
    auto& [mutex, device, referenceCounter] = getDeviceState();
    const std::lock_guard guard{mutex};

    std::cout << "AudioResource::AudioResource() | rc = " << referenceCounter + 1 << "\n";

    if (referenceCounter++ == 0u)
        device.emplace();
}

////////////////////////////////////////////////////////////
AudioResource::~AudioResource()
{
    auto& [mutex, device, referenceCounter] = getDeviceState();
    const std::lock_guard guard{mutex};

    std::cout << "~AudioResource() | rc = " << referenceCounter - 1 << "\n";

    if (--referenceCounter == 0u)
        device.reset();
}

////////////////////////////////////////////////////////////
AudioResource::AudioResource(const AudioResource&) : AudioResource{}
{
}

////////////////////////////////////////////////////////////
AudioResource::AudioResource(AudioResource&&) noexcept : AudioResource{}
{
}

} // namespace sf
