#pragma once

#include "SFML/Audio/AudioContext.hpp"
#include "SFML/Audio/Music.hpp"
#include "SFML/Audio/PlaybackDevice.hpp"
#include "SFML/Audio/Sound.hpp"
#include "SFML/Audio/SoundBuffer.hpp"

#include "SFML/System/Path.hpp"

#include "SFML/Base/Algorithm.hpp"

#include <vector>


////////////////////////////////////////////////////////////
struct Sounds
{
    ////////////////////////////////////////////////////////////
    struct LoadedSound : private sf::SoundBuffer, public sf::Sound // TODO P2: eww
    {
        ////////////////////////////////////////////////////////////
        explicit LoadedSound(const sf::Path& filename) :
        sf::SoundBuffer(sf::SoundBuffer::loadFromFile("resources/" / filename).value()),
        sf::Sound(static_cast<const sf::SoundBuffer&>(*this))
        {
        }

        ////////////////////////////////////////////////////////////
        LoadedSound(const LoadedSound&) = delete;
        LoadedSound(LoadedSound&&)      = delete;

        ////////////////////////////////////////////////////////////
        const sf::Sound& asSound() const
        {
            return *this;
        }

        ////////////////////////////////////////////////////////////
        const sf::SoundBuffer& asBuffer() const
        {
            return *this;
        }
    };

    ////////////////////////////////////////////////////////////
    LoadedSound pop{"pop.ogg"};
    LoadedSound reversePop{"reversePop.ogg"};
    LoadedSound shine{"shine.ogg"};
    LoadedSound click{"click2.ogg"};
    LoadedSound byteMeow{"bytemeow.ogg"};
    LoadedSound grab{"grab.ogg"};
    LoadedSound drop{"drop.ogg"};
    LoadedSound scratch{"scratch.ogg"};
    LoadedSound buy{"buy.ogg"};
    LoadedSound explosion{"explosion.ogg"};
    LoadedSound makeBomb{"makebomb.ogg"};
    LoadedSound hex{"hex.ogg"};
    LoadedSound byteSpeak{"bytespeak.ogg"};
    LoadedSound prestige{"prestige.ogg"};
    LoadedSound launch{"launch.ogg"};
    LoadedSound rocket{"rocket.ogg"};
    LoadedSound earthquake{"earthquake.ogg"};
    LoadedSound earthquakeFast{"earthquakefast.ogg"};
    LoadedSound woosh{"woosh.ogg"};
    LoadedSound cast0{"cast0.ogg"};

    ////////////////////////////////////////////////////////////
    std::vector<sf::Sound> soundsBeingPlayed;

    ////////////////////////////////////////////////////////////
    explicit Sounds()
    {
        const auto setupWorldSound = [&](auto& sound, const float attenuationMult = 1.f)
        {
            sound.setAttenuation(0.0025f * attenuationMult);
            sound.setSpatializationEnabled(true);
        };

        const auto setupUISound = [&](auto& sound)
        {
            sound.setAttenuation(0.f);
            sound.setSpatializationEnabled(false);
        };

        setupWorldSound(pop);
        setupWorldSound(reversePop);
        setupWorldSound(shine);
        setupWorldSound(explosion);
        setupWorldSound(makeBomb);
        setupWorldSound(hex);
        setupWorldSound(launch);
        setupWorldSound(rocket);
        setupWorldSound(earthquake, /* attenuationMult */ 0.1f);
        setupWorldSound(earthquakeFast, /* attenuationMult */ 0.1f);
        setupWorldSound(woosh, /* attenuationMult */ 0.1f);
        setupWorldSound(cast0, /* attenuationMult */ 0.1f);

        setupUISound(click);
        setupUISound(byteMeow);
        setupUISound(grab);
        setupUISound(drop);
        setupUISound(scratch);
        setupUISound(buy);
        setupUISound(byteSpeak);
        setupUISound(prestige);

        scratch.setVolume(35.f);
        buy.setVolume(75.f);
        explosion.setVolume(75.f);
    }

    ////////////////////////////////////////////////////////////
    Sounds(const Sounds&) = delete;
    Sounds(Sounds&&)      = delete;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isPlayingPooled(const LoadedSound& ls) const
    {
        return sf::base::anyOf( //
            soundsBeingPlayed.begin(),
            soundsBeingPlayed.end(),
            [&ls](const sf::Sound& sound)
            { return sound.getStatus() == sf::Sound::Status::Playing && &sound.getBuffer() == &ls.asBuffer(); });
    }

    ////////////////////////////////////////////////////////////
    bool playPooled(sf::PlaybackDevice& playbackDevice, const LoadedSound& ls, const bool overlap)
    {
        // TODO P2 (lib): improve in library

        if (!overlap && isPlayingPooled(ls))
            return false;

        const auto it = sf::base::findIf( //
            soundsBeingPlayed.begin(),
            soundsBeingPlayed.end(),
            [](const sf::Sound& sound) { return sound.getStatus() == sf::Sound::Status::Stopped; });

        if (it != soundsBeingPlayed.end())
        {
            *it = ls.asSound(); // assigment does not reallocate `m_impl`
            it->play(playbackDevice);

            return true;
        }

        // TODO P2 (lib): to sf base, also not needed
        // std::erase_if(soundsBeingPlayed,
        //               [](const sf::Sound& sound) { return sound.getStatus() == sf::Sound::Status::Stopped; });

        soundsBeingPlayed.emplace_back(ls.asSound()).play(playbackDevice);
        return true;
    }
};
