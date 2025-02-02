#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Audio/Listener.hpp"

#include "SFML/System/Angle.hpp"
#include "SFML/System/Vector3.hpp"

#ifndef __FLT_MAX__
#include <cfloat>
#endif


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
struct ma_sound;


namespace sf::priv
{
////////////////////////////////////////////////////////////
struct SavedSettings
{
    static inline constexpr float floatMax =
#ifndef __FLT_MAX__
        FLT_MAX
#else
        __FLT_MAX__
#endif
        ;

    float    pitch{1.f};
    float    pan{0.f};
    float    volume{1.f};
    bool     spatializationEnabled{true};
    Vector3f position{0.f, 0.f, 0.f};
    Vector3f direction{0.f, 0.f, -1.f};
    float    directionalAttenuationFactor{1.f};
    Vector3f velocity{0.f, 0.f, 0.f};
    float    dopplerFactor{1.f};
    int      positioning{0 /* ma_positioning_absolute */};
    float    minDistance{1.f};
    float    maxDistance{floatMax};
    float    minGain{0.f};
    float    maxGain{1.f};
    float    rollOff{1.f};
    bool     playing{false};
    bool     looping{false};

    Listener::Cone cone{radians(6.283185f), radians(6.283185f), 1.f};

    ////////////////////////////////////////////////////////////
    explicit SavedSettings() = default;

    ////////////////////////////////////////////////////////////
    explicit SavedSettings(const ma_sound& sound);

    ////////////////////////////////////////////////////////////
    void applyOnto(ma_sound& sound) const;
};

} // namespace sf::priv
