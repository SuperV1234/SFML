#include "SFML/Audio/SoundSource.hpp"

#include "SFML/Audio/PlaybackDevice.hpp"

#include <Doctest.hpp>

#include <AudioUtil.hpp>
#include <CommonTraits.hpp>
#include <SystemUtil.hpp>

#include <cfloat>
#include <climits>


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

#define SFML_TEST_CHECK_DEFAULT_VALUES(xSoundSource)                      \
    CHECK((xSoundSource).getPitch() == 1.f);                              \
    CHECK((xSoundSource).getPan() == 0.f);                                \
    CHECK((xSoundSource).getVolume() == 100.f);                           \
    CHECK((xSoundSource).isSpatializationEnabled());                      \
    CHECK((xSoundSource).getPosition() == sf::Vector3{0.f, 0.f, 0.f});    \
    CHECK((xSoundSource).getDirection() == sf::Vector3f{0.f, 0.f, -1.f}); \
    CHECK((xSoundSource).getCone().innerAngle == sf::radians(6.283185f)); \
    CHECK((xSoundSource).getCone().outerAngle == sf::radians(6.283185f)); \
    CHECK((xSoundSource).getCone().outerGain == 1.f);                     \
    CHECK((xSoundSource).getVelocity() == sf::Vector3f{});                \
    CHECK((xSoundSource).getDopplerFactor() == 1.f);                      \
    CHECK((xSoundSource).getDirectionalAttenuationFactor() == 1.f);       \
    CHECK(!(xSoundSource).isRelativeToListener());                        \
    CHECK((xSoundSource).getMinDistance() == 1.f);                        \
    CHECK((xSoundSource).getMaxDistance() == FLT_MAX);                    \
    CHECK((xSoundSource).getMinGain() == 0);                              \
    CHECK((xSoundSource).getMaxGain() == 1.f);                            \
    CHECK((xSoundSource).getAttenuation() == 1.f);                        \
    CHECK((xSoundSource).getStatus() == sf::SoundSource::Status::Stopped);

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
        SFML_TEST_CHECK_DEFAULT_VALUES(soundSource);
    }

    SECTION("Copy semantics")
    {
        const SoundSource soundSource;

        SECTION("Construction")
        {
            const SoundSource soundSourceCopy(soundSource); // NOLINT(performance-unnecessary-copy-initialization)
            SFML_TEST_CHECK_DEFAULT_VALUES(soundSourceCopy);
        }

        SECTION("Assignment")
        {
            SoundSource soundSourceCopy;
            soundSourceCopy = soundSource;
            SFML_TEST_CHECK_DEFAULT_VALUES(soundSourceCopy);
        }
    }

    SECTION("Set/get pitch")
    {
        SoundSource soundSource;
        soundSource.setPitch(42.f);
        CHECK(soundSource.getPitch() == 42.f);
    }

    SECTION("Set/get pan")
    {
        SoundSource soundSource;

        soundSource.setPan(1.f);
        CHECK(soundSource.getPan() == 1.f);
    }

    SECTION("Set/get volume")
    {
        SoundSource soundSource;

        soundSource.setVolume(0.f);
        CHECK(soundSource.getVolume() == 0.f);

        soundSource.setVolume(50.f);
        CHECK(soundSource.getVolume() == 50.f);

        soundSource.setVolume(100.f);
        CHECK(soundSource.getVolume() == 100.f);
    }

    SECTION("Set/get spatialization enabled")
    {
        SoundSource soundSource;

        soundSource.setSpatializationEnabled(true);
        CHECK(soundSource.isSpatializationEnabled());
    }

    SECTION("Set/get position")
    {
        SoundSource soundSource;

        soundSource.setPosition({1, 2, 3});
        CHECK(soundSource.getPosition() == sf::Vector3f{1, 2, 3});
    }

    SECTION("Set/get direction")
    {
        SoundSource soundSource;

        soundSource.setDirection({4, 5, 6});
        CHECK(soundSource.getDirection() == sf::Vector3f{4, 5, 6});
    }

    SECTION("Set/get cone")
    {
        SoundSource soundSource;

        soundSource.setCone({sf::radians(1), sf::radians(2), 3});
        CHECK(soundSource.getCone().innerAngle == sf::radians(1));
        CHECK(soundSource.getCone().outerAngle == sf::radians(2));
        CHECK(soundSource.getCone().outerGain == 3);
    }

    SECTION("Set/get velocity")
    {
        SoundSource soundSource;

        soundSource.setVelocity({7, 8, 9});
        CHECK(soundSource.getVelocity() == sf::Vector3f{7, 8, 9});
    }

    SECTION("Set/get doppler factor")
    {
        SoundSource soundSource;

        soundSource.setDopplerFactor(1);
        CHECK(soundSource.getDopplerFactor() == 1);
    }

    SECTION("Set/get directional attenuation factor")
    {
        SoundSource soundSource;

        soundSource.setDirectionalAttenuationFactor(1);
        CHECK(soundSource.getDirectionalAttenuationFactor() == 1);
    }

    SECTION("Set/get relative to listener")
    {
        SoundSource soundSource;

        soundSource.setRelativeToListener(true);
        CHECK(soundSource.isRelativeToListener());
    }

    SECTION("Set/get min distance")
    {
        SoundSource soundSource;

        soundSource.setMinDistance(12.34f);
        CHECK(soundSource.getMinDistance() == 12.34f);
    }

    SECTION("Set/get max distance")
    {
        SoundSource soundSource;

        soundSource.setMaxDistance(12.34f);
        CHECK(soundSource.getMaxDistance() == 12.34f);
    }

    SECTION("Set/get min gain")
    {
        SoundSource soundSource;

        soundSource.setMinGain(12.34f);
        CHECK(soundSource.getMinGain() == 12.34f);
    }

    SECTION("Set/get max gain")
    {
        SoundSource soundSource;
        soundSource.setMaxGain(12.34f);
        CHECK(soundSource.getMaxGain() == 12.34f);
    }

    SECTION("Set/get attenuation")
    {
        SoundSource soundSource;

        soundSource.setAttenuation(10);
        CHECK(soundSource.getAttenuation() == 10);
    }
}
