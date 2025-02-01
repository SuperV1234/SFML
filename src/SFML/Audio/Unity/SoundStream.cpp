#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Audio/ChannelMap.hpp"
#include "SFML/Audio/EffectProcessor.hpp"
#include "SFML/Audio/MiniaudioUtils.hpp"
#include "SFML/Audio/PlaybackDevice.hpp"
#include "SFML/Audio/SoundStream.hpp"

#include "SFML/System/Err.hpp"
#include "SFML/System/Sleep.hpp"
#include "SFML/System/Time.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Memcpy.hpp"
#include "SFML/Base/Macros.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/TrivialVector.hpp"

#include <miniaudio.h>


namespace sf
{
struct SoundStream::Impl
{
    explicit Impl(SoundStream* theOwner) : owner(theOwner)
    {
    }

    void initialize()
    {
        SFML_BASE_ASSERT(soundBase.hasValue());

        if (!soundBase->initialize(&onEnd))
            priv::err() << "Failed to initialize SoundStream::Impl";

        // Because we are providing a custom data source, we have to provide the channel map ourselves
        if (channelMap.isEmpty())
        {
            soundBase->getSound().engineNode.spatializer.pChannelMapIn = nullptr;
            return;
        }

        soundBase->clearSoundChannelMap();

        for (const SoundChannel channel : channelMap)
            soundBase->addToSoundChannelMap(priv::MiniaudioUtils::soundChannelToMiniaudioChannel(channel));

        soundBase->refreshSoundChannelMap();
    }

    static void onEnd(void* userData, ma_sound* soundPtr)
    {
        // Seek back to the start of the sound when it finishes playing
        auto& impl     = *static_cast<Impl*>(userData);
        impl.streaming = true;
        impl.status    = Status::Stopped;

        if (const ma_result result = ma_sound_seek_to_pcm_frame(soundPtr, 0); result != MA_SUCCESS)
            priv::MiniaudioUtils::fail("seek sound to frame 0", result);
    }

    static ma_result read(ma_data_source* dataSource, void* framesOut, ma_uint64 frameCount, ma_uint64* framesRead)
    {
        auto& impl = *static_cast<Impl*>(dataSource);

        // Try to fill our buffer with new samples if the source is still willing to stream data
        if (impl.sampleBuffer.empty() && impl.streaming)
        {
            Chunk chunk;

            impl.streaming = impl.owner->onGetData(chunk);

            if (chunk.samples && chunk.sampleCount)
            {
                impl.sampleBuffer.clear();
                impl.sampleBuffer.reserve(chunk.sampleCount);
                impl.sampleBuffer.unsafeEmplaceRange(chunk.samples, chunk.sampleCount);

                impl.sampleBufferCursor = 0;
            }
        }

        // Push the samples to miniaudio
        if (!impl.sampleBuffer.empty())
        {
            // Determine how many frames we can read
            *framesRead = base::min(frameCount,
                                    static_cast<ma_uint64>(
                                        (impl.sampleBuffer.size() - impl.sampleBufferCursor) / impl.channelCount));

            const auto sampleCount = *framesRead * impl.channelCount;

            // Copy the samples to the output
            SFML_BASE_MEMCPY(framesOut,
                             impl.sampleBuffer.data() + impl.sampleBufferCursor,
                             static_cast<base::SizeT>(sampleCount) * sizeof(impl.sampleBuffer[0]));

            impl.sampleBufferCursor += static_cast<base::SizeT>(sampleCount);
            impl.samplesProcessed += sampleCount;

            if (impl.sampleBufferCursor >= impl.sampleBuffer.size())
            {
                impl.sampleBuffer.clear();
                impl.sampleBufferCursor = 0;

                // If we are looping and at the end of the loop, set the cursor back to the beginning of the loop
                if (!impl.streaming && impl.owner->isLooping())
                {
                    if (const base::Optional seekPositionAfterLoop = impl.owner->onLoop())
                    {
                        impl.streaming        = true;
                        impl.samplesProcessed = *seekPositionAfterLoop;
                    }
                }
            }
        }
        else
        {
            *framesRead = 0;
        }

        return MA_SUCCESS;
    }

    static ma_result seek(ma_data_source* dataSource, ma_uint64 frameIndex)
    {
        auto& impl = *static_cast<Impl*>(dataSource);

        impl.streaming = true;
        impl.sampleBuffer.clear();
        impl.sampleBufferCursor = 0;
        impl.samplesProcessed   = frameIndex * impl.channelCount;

        const Time offset = impl.sampleRate == 0
                                ? Time::Zero
                                : seconds(static_cast<float>(frameIndex) / static_cast<float>(impl.sampleRate));

        impl.owner->onSeek(offset);
        return MA_SUCCESS;
    }

    static ma_result getFormat(ma_data_source* dataSource,
                               ma_format*      format,
                               ma_uint32*      channels,
                               ma_uint32*      sampleRate,
                               ma_channel*,
                               base::SizeT)
    {
        const auto& impl = *static_cast<const Impl*>(dataSource);

        // If we don't have valid values yet, initialize with defaults so sound creation doesn't fail
        *format     = ma_format_s16;
        *channels   = impl.channelCount ? impl.channelCount : 1;
        *sampleRate = impl.sampleRate ? impl.sampleRate : 44100;

        return MA_SUCCESS;
    }

    static ma_result getCursor(ma_data_source* dataSource, ma_uint64* cursor)
    {
        auto& impl = *static_cast<Impl*>(dataSource);
        *cursor    = impl.channelCount ? impl.samplesProcessed / impl.channelCount : 0;

        return MA_SUCCESS;
    }

    static ma_result getLength(ma_data_source*, ma_uint64* length)
    {
        *length = 0;

        return MA_NOT_IMPLEMENTED;
    }

    static ma_result setLooping(ma_data_source* /* dataSource */, ma_bool32 /* looping */)
    {
        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    static inline constexpr ma_data_source_vtable vtable{read, seek, getFormat, getCursor, getLength, setLooping, /* flags */ 0};

    base::Optional<priv::MiniaudioUtils::SoundBase> soundBase; //!< Sound base, needs to be first member

    SoundStream*                   owner;                //!< Owning `SoundStream` object
    base::TrivialVector<base::I16> sampleBuffer;         //!< Our temporary sample buffer
    base::SizeT                    sampleBufferCursor{}; //!< The current read position in the temporary sample buffer
    base::U64                      samplesProcessed{};   //!< Number of samples processed since beginning of the stream
    unsigned int                   channelCount{};       //!< Number of channels (1 = mono, 2 = stereo, ...)
    unsigned int                   sampleRate{};         //!< Frequency (samples / second)
    ChannelMap                     channelMap;           //!< The map of position in sample frame to sound channel
    bool                           streaming{true};      //!< `true` if we are still streaming samples from the source
    SoundSource::Status            status{SoundSource::Status::Stopped}; //!< The status
};


////////////////////////////////////////////////////////////
SoundStream::SoundStream() : m_impl(base::makeUnique<Impl>(this))
{
}


////////////////////////////////////////////////////////////
SoundStream::~SoundStream() = default;


////////////////////////////////////////////////////////////
SoundStream::SoundStream(SoundStream&& rhs) noexcept : m_impl(SFML_BASE_MOVE(rhs.m_impl))
{
    // Update self-referential owner pointer.
    m_impl->owner = this;
}


////////////////////////////////////////////////////////////
SoundStream& SoundStream::operator=(SoundStream&& rhs) noexcept
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
void SoundStream::initialize(unsigned int channelCount, unsigned int sampleRate, const ChannelMap& channelMap)
{
    m_impl->channelCount     = channelCount;
    m_impl->sampleRate       = sampleRate;
    m_impl->channelMap       = channelMap;
    m_impl->samplesProcessed = 0;

    if (m_impl->soundBase.hasValue())
    {
        m_impl->soundBase->deinitialize();
        m_impl->initialize();

        SFML_BASE_ASSERT(m_impl->soundBase.hasValue());
        applyStoredSettings(m_impl->soundBase->getSound());
        setEffectProcessor(getEffectProcessor());
        setPlayingOffset(getPlayingOffset());
    }
}


////////////////////////////////////////////////////////////
void SoundStream::play(PlaybackDevice& playbackDevice)
{
    m_lastPlaybackDevice = &playbackDevice;

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
        priv::MiniaudioUtils::fail("start playing sound stream", result);
        return;
    }

    m_impl->status = Status::Playing;
}


////////////////////////////////////////////////////////////
void SoundStream::pause()
{
    if (!m_impl->soundBase.hasValue())
        return;

    if (const ma_result result = ma_sound_stop(&m_impl->soundBase->getSound()); result != MA_SUCCESS)
    {
        priv::MiniaudioUtils::fail("stop playing sound stream", result);
        return;
    }

    if (m_impl->status == Status::Playing)
        m_impl->status = Status::Paused;
}


////////////////////////////////////////////////////////////
void SoundStream::stop()
{
    if (!m_impl->soundBase.hasValue())
        return;

    if (const ma_result result = ma_sound_stop(&m_impl->soundBase->getSound()); result != MA_SUCCESS)
    {
        priv::MiniaudioUtils::fail("stop playing sound stream", result);
        return;
    }

    setPlayingOffset(Time::Zero);
    m_impl->status = Status::Stopped;
}


////////////////////////////////////////////////////////////
unsigned int SoundStream::getChannelCount() const
{
    return m_impl->channelCount;
}


////////////////////////////////////////////////////////////
unsigned int SoundStream::getSampleRate() const
{
    return m_impl->sampleRate;
}


////////////////////////////////////////////////////////////
ChannelMap SoundStream::getChannelMap() const
{
    return m_impl->channelMap;
}


////////////////////////////////////////////////////////////
SoundStream::Status SoundStream::getStatus() const
{
    return m_impl->status;
}


////////////////////////////////////////////////////////////
void SoundStream::setPlayingOffset(Time playingOffset)
{
    SoundSource::setPlayingOffset(playingOffset);

    if (!m_impl->soundBase.hasValue())
        return;

    if (m_impl->sampleRate == 0)
        return;

    if (m_impl->soundBase->getSound().pDataSource == nullptr || m_impl->soundBase->getSound().engineNode.pEngine == nullptr)
        return;

    const auto frameIndex = ma_uint64{priv::MiniaudioUtils::getFrameIndex(m_impl->soundBase->getSound(), playingOffset)};

    m_impl->streaming = true;
    m_impl->sampleBuffer.clear();
    m_impl->sampleBufferCursor = 0;
    m_impl->samplesProcessed   = frameIndex * m_impl->channelCount;

    onSeek(seconds(static_cast<float>(frameIndex) / static_cast<float>(m_impl->sampleRate)));
}


////////////////////////////////////////////////////////////
Time SoundStream::getPlayingOffset() const
{
    if (m_impl->channelCount == 0 || m_impl->sampleRate == 0)
        return Time{};

    return SoundSource::getPlayingOffset();
}


////////////////////////////////////////////////////////////
void SoundStream::setEffectProcessor(EffectProcessor effectProcessor)
{
    SoundSource::setEffectProcessor(effectProcessor);

    if (!m_impl->soundBase.hasValue())
        return;

    m_impl->soundBase->setAndConnectEffectProcessor(SFML_BASE_MOVE(effectProcessor));
}


////////////////////////////////////////////////////////////
base::Optional<base::U64> SoundStream::onLoop()
{
    onSeek(Time::Zero);
    return base::makeOptional(base::U64{0});
}


////////////////////////////////////////////////////////////
void* SoundStream::getSound() const
{
    if (!m_impl->soundBase.hasValue())
        return nullptr;

    return &m_impl->soundBase->getSound();
}

} // namespace sf
