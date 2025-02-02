#include "SFML/Audio/SoundSource.hpp"

#include "SFML/Audio/PlaybackDevice.hpp"

#include <Doctest.hpp>

#include <AudioUtil.hpp>
#include <CommonTraits.hpp>
#include <SystemUtil.hpp>

namespace
{
class SoundSource : public sf::SoundSource
{
    void play(sf::PlaybackDevice&) override
    {
    }

    void pause() override
    {
    }

    void stop() override
    {
    }

    [[nodiscard]] void* getSound() const override
    {
        return {};
    }

public:
    [[nodiscard]] Status getStatus() const override
    {
        return {};
    }
};
} // namespace

TEST_CASE("[Audio] sf::SoundSource" * doctest::skip(skipAudioDeviceTests))
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!SFML_BASE_IS_CONSTRUCTIBLE(sf::SoundSource));
        STATIC_CHECK(!SFML_BASE_IS_COPY_CONSTRUCTIBLE(sf::SoundSource));
        STATIC_CHECK(SFML_BASE_IS_COPY_ASSIGNABLE(sf::SoundSource));
        STATIC_CHECK(!SFML_BASE_IS_MOVE_CONSTRUCTIBLE(sf::SoundSource));
        STATIC_CHECK(SFML_BASE_IS_MOVE_ASSIGNABLE(sf::SoundSource));
        STATIC_CHECK(SFML_BASE_IS_NOTHROW_MOVE_ASSIGNABLE(sf::SoundSource));
        STATIC_CHECK(SFML_BASE_HAS_VIRTUAL_DESTRUCTOR(sf::SoundSource));
    }

    SECTION("Construction")
    {
        const SoundSource soundSource;
        CHECK(soundSource.getPitch() == 1.f);
        CHECK(soundSource.getPan() == 0);
        CHECK(soundSource.getVolume() == 0);
        CHECK(!soundSource.isSpatializationEnabled());
        CHECK(soundSource.getPosition() == sf::Vector3f());
        CHECK(soundSource.getDirection() == sf::Vector3f());
        CHECK(soundSource.getCone().innerAngle == sf::degrees(0));
        CHECK(soundSource.getCone().innerAngle == sf::degrees(0));
        CHECK(soundSource.getCone().outerGain == 0);
        CHECK(soundSource.getVelocity() == sf::Vector3f());
        CHECK(soundSource.getDopplerFactor() == 0);
        CHECK(soundSource.getDirectionalAttenuationFactor() == 0);
        CHECK(!soundSource.isRelativeToListener());
        CHECK(soundSource.getMinDistance() == 0);
        CHECK(soundSource.getMaxDistance() == 0);
        CHECK(soundSource.getMinGain() == 0);
        CHECK(soundSource.getMaxGain() == 0);
        CHECK(soundSource.getAttenuation() == 0);
        CHECK(soundSource.getStatus() == sf::SoundSource::Status::Stopped);
    }

    SECTION("Copy semantics")
    {
        const SoundSource soundSource;

        SECTION("Construction")
        {
            const SoundSource soundSourceCopy(soundSource); // NOLINT(performance-unnecessary-copy-initialization)
            CHECK(soundSourceCopy.getPitch() == 0);
            CHECK(soundSourceCopy.getPan() == 0);
            CHECK(soundSourceCopy.getVolume() == 0);
            CHECK(!soundSourceCopy.isSpatializationEnabled());
            CHECK(soundSourceCopy.getPosition() == sf::Vector3f());
            CHECK(soundSourceCopy.getDirection() == sf::Vector3f());
            CHECK(soundSourceCopy.getCone().innerAngle == sf::degrees(0));
            CHECK(soundSourceCopy.getCone().innerAngle == sf::degrees(0));
            CHECK(soundSourceCopy.getCone().outerGain == 0);
            CHECK(soundSourceCopy.getVelocity() == sf::Vector3f());
            CHECK(soundSourceCopy.getDopplerFactor() == 0);
            CHECK(soundSourceCopy.getDirectionalAttenuationFactor() == 0);
            CHECK(!soundSourceCopy.isRelativeToListener());
            CHECK(soundSourceCopy.getMinDistance() == 0);
            CHECK(soundSourceCopy.getMaxDistance() == 0);
            CHECK(soundSourceCopy.getMinGain() == 0);
            CHECK(soundSourceCopy.getMaxGain() == 0);
            CHECK(soundSourceCopy.getAttenuation() == 0);
            CHECK(soundSourceCopy.getStatus() == sf::SoundSource::Status::Stopped);
        }

        SECTION("Assignment")
        {
            SoundSource soundSourceCopy;
            soundSourceCopy = soundSource;
            CHECK(soundSourceCopy.getPitch() == 0);
            CHECK(soundSourceCopy.getPan() == 0);
            CHECK(soundSourceCopy.getVolume() == 0);
            CHECK(!soundSourceCopy.isSpatializationEnabled());
            CHECK(soundSourceCopy.getPosition() == sf::Vector3f());
            CHECK(soundSourceCopy.getDirection() == sf::Vector3f());
            CHECK(soundSourceCopy.getCone().innerAngle == sf::degrees(0));
            CHECK(soundSourceCopy.getCone().innerAngle == sf::degrees(0));
            CHECK(soundSourceCopy.getCone().outerGain == 0);
            CHECK(soundSourceCopy.getVelocity() == sf::Vector3f());
            CHECK(soundSourceCopy.getDopplerFactor() == 0);
            CHECK(soundSourceCopy.getDirectionalAttenuationFactor() == 0);
            CHECK(!soundSourceCopy.isRelativeToListener());
            CHECK(soundSourceCopy.getMinDistance() == 0);
            CHECK(soundSourceCopy.getMaxDistance() == 0);
            CHECK(soundSourceCopy.getMinGain() == 0);
            CHECK(soundSourceCopy.getMaxGain() == 0);
            CHECK(soundSourceCopy.getAttenuation() == 0);
            CHECK(soundSourceCopy.getStatus() == sf::SoundSource::Status::Stopped);
        }
    }

    SECTION("Set/get pitch")
    {
        SoundSource soundSource;
        soundSource.setPitch(42);
        CHECK(soundSource.getPitch() == 0);
    }

    SECTION("Set/get pan")
    {
        SoundSource soundSource;
        soundSource.setPan(1);
        CHECK(soundSource.getPan() == 0);
    }

    SECTION("Set/get volume")
    {
        SoundSource soundSource;
        soundSource.setVolume(50.f);
        CHECK(soundSource.getVolume() == 50.f);
    }

    SECTION("Set/get spatialization enabled")
    {
        SoundSource soundSource;
        soundSource.setSpatializationEnabled(true);
        CHECK(!soundSource.isSpatializationEnabled());
    }

    SECTION("Set/get position")
    {
        SoundSource soundSource;
        soundSource.setPosition({1, 2, 3});
        CHECK(soundSource.getPosition() == sf::Vector3f());
    }

    SECTION("Set/get direction")
    {
        SoundSource soundSource;
        soundSource.setDirection({4, 5, 6});
        CHECK(soundSource.getDirection() == sf::Vector3f());
    }

    SECTION("Set/get cone")
    {
        SoundSource soundSource;
        soundSource.setCone({sf::radians(1), sf::radians(2), 3});
        CHECK(soundSource.getCone().innerAngle == sf::degrees(0));
        CHECK(soundSource.getCone().outerAngle == sf::degrees(0));
        CHECK(soundSource.getCone().outerGain == 0);
    }

    SECTION("Set/get velocity")
    {
        SoundSource soundSource;
        soundSource.setVelocity({7, 8, 9});
        CHECK(soundSource.getVelocity() == sf::Vector3f());
    }

    SECTION("Set/get doppler factor")
    {
        SoundSource soundSource;
        soundSource.setDopplerFactor(1);
        CHECK(soundSource.getDopplerFactor() == 0);
    }

    SECTION("Set/get directional attenuation factor")
    {
        SoundSource soundSource;
        soundSource.setDirectionalAttenuationFactor(1);
        CHECK(soundSource.getDirectionalAttenuationFactor() == 0);
    }

    SECTION("Set/get relative to listener")
    {
        SoundSource soundSource;
        soundSource.setRelativeToListener(true);
        CHECK(!soundSource.isRelativeToListener());
    }

    SECTION("Set/get min distance")
    {
        SoundSource soundSource;
        soundSource.setMinDistance(12.34f);
        CHECK(soundSource.getMinDistance() == 0);
    }

    SECTION("Set/get max distance")
    {
        SoundSource soundSource;
        soundSource.setMaxDistance(12.34f);
        CHECK(soundSource.getMaxDistance() == 0);
    }

    SECTION("Set/get min gain")
    {
        SoundSource soundSource;
        soundSource.setMinGain(12.34f);
        CHECK(soundSource.getMinGain() == 0);
    }

    SECTION("Set/get max gain")
    {
        SoundSource soundSource;
        soundSource.setMaxGain(12.34f);
        CHECK(soundSource.getMaxGain() == 0);
    }

    SECTION("Set/get attenuation")
    {
        SoundSource soundSource;
        soundSource.setAttenuation(10);
        CHECK(soundSource.getAttenuation() == 0);
    }
}
