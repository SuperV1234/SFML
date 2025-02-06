#pragma once

#include "Stats.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Profile
{
    float masterVolume          = 100.f;
    float musicVolume           = 100.f;
    bool  playAudioInBackground = true;
    bool  playComboEndSound     = true;
    float minimapScale          = 20.f;
    float hudScale              = 1.f;
    bool  tipsEnabled           = true;

    Stats statsLifetime;
};
