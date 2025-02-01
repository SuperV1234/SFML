#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

// TODO P1: consider passing buffer in on .play and not storing it

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Audio/EffectProcessor.hpp"
#include "SFML/Audio/MiniaudioUtils.hpp"
#include "SFML/Audio/PlaybackDevice.hpp"
#include "SFML/Audio/Sound.hpp"
#include "SFML/Audio/SoundBuffer.hpp"
#include "SFML/Audio/SoundSource.hpp"

#include "SFML/System/Err.hpp"
#include "SFML/System/Time.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Memcpy.hpp"
#include "SFML/Base/Macros.hpp"
#include "SFML/Base/UniquePtr.hpp"

#include <miniaudio.h>


namespace sf
{
struct Sound::Impl
{
    explicit Impl(Sound* theOwner) : owner(theOwner)
    {
    }

    void initialize()
    {
        SFML_BASE_ASSERT(soundBase.hasValue());

        if (!soundBase->initialize(&onEnd))
            priv::err() << "Failed to initialize Sound::Impl";

        // Because we are providing a custom data source, we have to provide the channel map ourselves
        if (buffer == nullptr || buffer->getChannelMap().isEmpty())
        {
            soundBase->getSound().engineNode.spatializer.pChannelMapIn = nullptr;
            return;
        }

        soundBase->clearSoundChannelMap();

        for (const SoundChannel channel : buffer->getChannelMap())
            soundBase->addToSoundChannelMap(priv::MiniaudioUtils::soundChannelToMiniaudioChannel(channel));

        soundBase->refreshSoundChannelMap();
    }

    static void onEnd(void* userData, ma_sound* soundPtr)
    {
        auto& impl  = *static_cast<Impl*>(userData);
        impl.status = Status::Stopped;

        // Seek back to the start of the sound when it finishes playing
        if (const ma_result result = ma_sound_seek_to_pcm_frame(soundPtr, 0); result != MA_SUCCESS)
            priv::MiniaudioUtils::fail("seek sound to frame 0", result);
    }

    static ma_result read(ma_data_source* dataSource, void* framesOut, ma_uint64 frameCount, ma_uint64* framesRead)
    {
        auto&       impl   = *static_cast<Impl*>(dataSource);
        const auto* buffer = impl.buffer;

        if (buffer == nullptr)
            return MA_NO_DATA_AVAILABLE;

        // Determine how many frames we can read
        *framesRead = base::min(frameCount,
                                static_cast<ma_uint64>((buffer->getSampleCount() - impl.cursor) / buffer->getChannelCount()));

        // Copy the samples to the output
        const auto sampleCount = *framesRead * buffer->getChannelCount();

        SFML_BASE_MEMCPY(framesOut,
                         buffer->getSamples() + impl.cursor,
                         static_cast<base::SizeT>(sampleCount) * sizeof(buffer->getSamples()[0]));

        impl.cursor += static_cast<base::SizeT>(sampleCount);

        // If we are looping and at the end of the sound, set the cursor back to the start
        if (impl.owner->isLooping() && (impl.cursor >= buffer->getSampleCount()))
            impl.cursor = 0;

        return MA_SUCCESS;
    }

    static ma_result seek(ma_data_source* dataSource, ma_uint64 frameIndex)
    {
        auto&       impl   = *static_cast<Impl*>(dataSource);
        const auto* buffer = impl.buffer;

        if (buffer == nullptr)
            return MA_NO_DATA_AVAILABLE;

        impl.cursor = static_cast<base::SizeT>(frameIndex * buffer->getChannelCount());

        return MA_SUCCESS;
    }

    static ma_result getFormat(ma_data_source* dataSource,
                               ma_format*      format,
                               ma_uint32*      channels,
                               ma_uint32*      sampleRate,
                               ma_channel*,
                               base::SizeT)
    {
        const auto& impl   = *static_cast<const Impl*>(dataSource);
        const auto* buffer = impl.buffer;

        // If we don't have valid values yet, initialize with defaults so sound creation doesn't fail
        *format     = ma_format_s16;
        *channels   = buffer && buffer->getChannelCount() ? buffer->getChannelCount() : 1;
        *sampleRate = buffer && buffer->getSampleRate() ? buffer->getSampleRate() : 44100;

        return MA_SUCCESS;
    }

    static ma_result getCursor(ma_data_source* dataSource, ma_uint64* cursor)
    {
        const auto& impl   = *static_cast<const Impl*>(dataSource);
        const auto* buffer = impl.buffer;

        if (buffer == nullptr)
            return MA_NO_DATA_AVAILABLE;

        *cursor = impl.cursor / buffer->getChannelCount();

        return MA_SUCCESS;
    }

    static ma_result getLength(ma_data_source* dataSource, ma_uint64* length)
    {
        const auto& impl   = *static_cast<const Impl*>(dataSource);
        const auto* buffer = impl.buffer;

        if (buffer == nullptr)
            return MA_NO_DATA_AVAILABLE;

        *length = buffer->getSampleCount() / buffer->getChannelCount();

        return MA_SUCCESS;
    }

    static ma_result setLooping(ma_data_source* /* dataSource */, ma_bool32 /* looping */)
    {
        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    static inline constexpr ma_data_source_vtable vtable{read, seek, getFormat, getCursor, getLength, setLooping, 0};

    base::Optional<priv::MiniaudioUtils::SoundBase> soundBase; //!< Sound base, needs to be first member
    Sound*                                          owner;     //!< Owning `Sound` object
    base::SizeT                                     cursor{};  //!< The current playing position
    const SoundBuffer*                              buffer{};  //!< Sound buffer bound to the source
    SoundSource::Status                             status{SoundSource::Status::Stopped}; //!< The status
};


////////////////////////////////////////////////////////////
Sound::Sound(const SoundBuffer& buffer) : m_impl(base::makeUnique<Impl>(this))
{
    setBuffer(buffer);

    SFML_UPDATE_LIFETIME_DEPENDANT(SoundBuffer, Sound, this, m_impl->buffer);
}


////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-redundant-member-init)
Sound::Sound(const Sound& rhs) : SoundSource(rhs), m_impl(base::makeUnique<Impl>(this))
{
    SoundSource::operator=(rhs);

    if (rhs.m_impl->buffer != nullptr)
        setBuffer(*rhs.m_impl->buffer);

    SFML_UPDATE_LIFETIME_DEPENDANT(SoundBuffer, Sound, this, m_impl->buffer);
}


////////////////////////////////////////////////////////////
Sound& Sound::operator=(const Sound& rhs)
{
    // Here we don't use the copy-and-swap idiom, because it would mess up
    // the list of sound instances contained in the buffers

    // Handle self-assignment here, as no copy-and-swap idiom is being used
    if (this == &rhs)
        return *this;

    // Delegate to base class, which copies all the sound attributes
    SoundSource::operator=(rhs);

    // Detach the sound instance from the previous buffer (if any)
    if (m_impl->buffer != nullptr)
    {
        stop();

        m_impl->buffer->detachSound(this);
        m_impl->buffer = nullptr;
    }

    // Copy the remaining sound attributes
    if (rhs.m_impl->buffer != nullptr)
        setBuffer(*rhs.m_impl->buffer);

    return *this;
}


////////////////////////////////////////////////////////////
Sound::Sound(Sound&& rhs) noexcept : m_impl(SFML_BASE_MOVE(rhs.m_impl))
{
    // Update self-referential owner pointer.
    m_impl->owner = this;
}


////////////////////////////////////////////////////////////
Sound& Sound::operator=(Sound&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_impl = SFML_BASE_MOVE(rhs.m_impl);

        // Update self-referential owner pointer.
        m_impl->owner = this;
    }

    return *this;
}


////////////////////////////////////////////////////////////
Sound::~Sound()
{
    stop();

    if (m_impl->buffer != nullptr)
        m_impl->buffer->detachSound(this);
}


////////////////////////////////////////////////////////////
void Sound::play(PlaybackDevice& playbackDevice)
{
    if (!m_impl->soundBase.hasValue())
    {
        m_impl->soundBase.emplace(playbackDevice, &Impl::vtable, [](void* ptr) { static_cast<Impl*>(ptr)->initialize(); });
        m_impl->initialize();

        SFML_BASE_ASSERT(m_impl->soundBase.hasValue());
        applyStoredSettings(m_impl->soundBase->getSound());
        setEffectProcessor(getEffectProcessor());
        setPlayingOffset(getPlayingOffset());
    }

    if (m_impl->status == Status::Playing)
        setPlayingOffset(Time::Zero);

    if (const ma_result result = ma_sound_start(&m_impl->soundBase->getSound()); result != MA_SUCCESS)
    {
        priv::MiniaudioUtils::fail("start playing sound", result);
        return;
    }

    m_impl->status = Status::Playing;
}


////////////////////////////////////////////////////////////
void Sound::pause()
{
    if (!m_impl->soundBase.hasValue())
        return;

    if (const ma_result result = ma_sound_stop(&m_impl->soundBase->getSound()); result != MA_SUCCESS)
    {
        priv::MiniaudioUtils::fail("pause playing sound", result);
        return;
    }

    if (m_impl->status == Status::Playing)
        m_impl->status = Status::Paused;
}


////////////////////////////////////////////////////////////
void Sound::stop()
{
    if (!m_impl->soundBase.hasValue())
        return;

    if (const ma_result result = ma_sound_stop(&m_impl->soundBase->getSound()); result != MA_SUCCESS)
    {
        priv::MiniaudioUtils::fail("stop playing sound", result);
        return;
    }

    setPlayingOffset(Time::Zero);
    m_impl->status = Status::Stopped;
}


////////////////////////////////////////////////////////////
void Sound::setBuffer(const SoundBuffer& buffer)
{
    // First detach from the previous buffer
    if (m_impl->buffer != nullptr)
    {
        stop();

        // Reset cursor
        m_impl->cursor = 0;
        m_impl->buffer->detachSound(this);
    }

    // Assign and use the new buffer
    m_impl->buffer = &buffer;
    m_impl->buffer->attachSound(this);

    if (m_impl->soundBase.hasValue())
    {
        m_impl->soundBase->deinitialize();
        m_impl->initialize();

        SFML_BASE_ASSERT(m_impl->soundBase.hasValue());
        applyStoredSettings(m_impl->soundBase->getSound());
        setEffectProcessor(getEffectProcessor());
        setPlayingOffset(getPlayingOffset());
    }

    SFML_UPDATE_LIFETIME_DEPENDANT(SoundBuffer, Sound, this, m_impl->buffer);
}

////////////////////////////////////////////////////////////
void Sound::setPlayingOffset(Time playingOffset)
{
    SoundSource::setPlayingOffset(playingOffset);

    if (!m_impl->soundBase.hasValue())
        return;

    if (m_impl->soundBase->getSound().pDataSource == nullptr || m_impl->soundBase->getSound().engineNode.pEngine == nullptr)
        return;

    const auto frameIndex = ma_uint64{priv::MiniaudioUtils::getFrameIndex(m_impl->soundBase->getSound(), playingOffset)};

    if (m_impl->buffer != nullptr)
        m_impl->cursor = static_cast<base::SizeT>(frameIndex * m_impl->buffer->getChannelCount());
}


////////////////////////////////////////////////////////////
void Sound::setEffectProcessor(EffectProcessor effectProcessor)
{
    SoundSource::setEffectProcessor(effectProcessor);

    if (!m_impl->soundBase.hasValue())
        return;

    m_impl->soundBase->setAndConnectEffectProcessor(SFML_BASE_MOVE(effectProcessor));
}


////////////////////////////////////////////////////////////
const SoundBuffer& Sound::getBuffer() const
{
    SFML_BASE_ASSERT(m_impl && "Sound::getBuffer() Cannot access unset buffer");
    return *m_impl->buffer;
}


////////////////////////////////////////////////////////////
Time Sound::getPlayingOffset() const
{
    if (m_impl->buffer == nullptr || m_impl->buffer->getChannelCount() == 0 || m_impl->buffer->getSampleRate() == 0)
        return Time{};

    return SoundSource::getPlayingOffset();
}


////////////////////////////////////////////////////////////
Sound::Status Sound::getStatus() const
{
    return m_impl->status;
}


////////////////////////////////////////////////////////////
void Sound::detachBuffer()
{
    // First stop the sound in case it is playing
    stop();

    // Detach the buffer
    if (m_impl->buffer != nullptr)
    {
        m_impl->buffer->detachSound(this);
        m_impl->buffer = nullptr;
    }
}


////////////////////////////////////////////////////////////
void* Sound::getSound() const
{
    if (!m_impl->soundBase.hasValue())
        return nullptr;

    return &m_impl->soundBase->getSound();
}

} // namespace sf
