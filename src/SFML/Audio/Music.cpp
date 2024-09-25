#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Audio/ChannelMap.hpp"
#include "SFML/Audio/InputSoundFile.hpp"
#include "SFML/Audio/Music.hpp"

#include "SFML/System/Err.hpp"
#include "SFML/System/Time.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/TrivialVector.hpp"
#include "SFML/Base/UniquePtr.hpp"

#include <mutex>


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard]] sf::Time samplesToTime(unsigned int sampleRate, unsigned int channelCount, sf::base::U64 samples)
{
    auto position = sf::Time::Zero;

    // Make sure we don't divide by 0
    if (sampleRate != 0 && channelCount != 0)
        position = sf::microseconds(static_cast<sf::base::I64>((samples * 1000000) / (channelCount * sampleRate)));

    return position;
}

////////////////////////////////////////////////////////////
[[nodiscard]] sf::base::U64 timeToSamples(unsigned int sampleRate, unsigned int channelCount, sf::Time position)
{
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
    // We refactor it to keep sf::base::I64 as the data type throughout the whole operation.
    return ((static_cast<sf::base::U64>(position.asMicroseconds()) * sampleRate * channelCount) + 500000) / 1000000;
}

} // namespace

namespace sf
{
////////////////////////////////////////////////////////////
struct Music::Impl
{
    InputSoundFile                 file;     //!< Input sound file
    base::TrivialVector<base::I16> samples;  //!< Temporary buffer of samples
    std::recursive_mutex           mutex;    //!< Mutex protecting the data
    Span<base::U64>                loopSpan; //!< Loop Range Specifier

    explicit Impl(InputSoundFile&& theFile) :
    file(SFML_BASE_MOVE(theFile)),

    // Resize the internal buffer so that it can contain 1 second of audio samples
    samples(file.getSampleRate() * file.getChannelCount()),

    // Compute the music source positions
    loopSpan{0u, file.getSampleCount()}
    {
    }
};


////////////////////////////////////////////////////////////
Music::Music(base::PassKey<Music>&&, InputSoundFile&& file) : m_impl(base::makeUnique<Impl>(SFML_BASE_MOVE(file)))
{
    SoundStream::initialize(m_impl->file.getChannelCount(), m_impl->file.getSampleRate(), m_impl->file.getChannelMap());
}


////////////////////////////////////////////////////////////
Music::~Music()
{
    // We must stop before destroying the file
    if (m_impl != nullptr)
    {
        stop();
    }
}


////////////////////////////////////////////////////////////
Music::Music(Music&&) noexcept = default;


////////////////////////////////////////////////////////////
Music& Music::operator=(Music&&) noexcept = default;


////////////////////////////////////////////////////////////
base::Optional<Music> Music::tryOpenFromInputSoundFile(base::Optional<InputSoundFile>&& optFile, const char* errorContext)
{
    if (!optFile.hasValue())
    {
        priv::err() << "Failed to open music from " << errorContext;
        return base::nullOpt;
    }

    return base::makeOptional<Music>(base::PassKey<Music>{}, SFML_BASE_MOVE(*optFile));
}


////////////////////////////////////////////////////////////
base::Optional<Music> Music::openFromFile(const Path& filename)
{
    return tryOpenFromInputSoundFile(InputSoundFile::openFromFile(filename), "file");
}


////////////////////////////////////////////////////////////
base::Optional<Music> Music::openFromMemory(const void* data, base::SizeT sizeInBytes)
{
    return tryOpenFromInputSoundFile(InputSoundFile::openFromMemory(data, sizeInBytes), "memory");
}


////////////////////////////////////////////////////////////
base::Optional<Music> Music::openFromStream(InputStream& stream)
{
    return tryOpenFromInputSoundFile(InputSoundFile::openFromStream(stream), "stream");
}


////////////////////////////////////////////////////////////
Time Music::getDuration() const
{
    return m_impl->file.getDuration();
}


////////////////////////////////////////////////////////////
unsigned int Music::getChannelCount() const
{
    return m_impl->file.getChannelCount();
}


////////////////////////////////////////////////////////////
unsigned int Music::getSampleRate() const
{
    return m_impl->file.getSampleRate();
}


////////////////////////////////////////////////////////////
ChannelMap Music::getChannelMap() const
{
    return m_impl->file.getChannelMap();
}


////////////////////////////////////////////////////////////
[[nodiscard]] base::U64 Music::getSampleCount() const
{
    return m_impl->file.getSampleCount();
}


////////////////////////////////////////////////////////////
bool Music::onGetData(SoundStream::Chunk& data)
{
    const std::lock_guard lock(m_impl->mutex);

    base::SizeT     toFill        = m_impl->samples.size();
    base::U64       currentOffset = m_impl->file.getSampleOffset();
    const base::U64 loopEnd       = m_impl->loopSpan.offset + m_impl->loopSpan.length;

    // If the loop end is enabled and imminent, request less data.
    // This will trip an "onLoop()" call from the underlying SoundStream,
    // and we can then take action.
    if (isLooping() && (m_impl->loopSpan.length != 0) && (currentOffset <= loopEnd) && (currentOffset + toFill > loopEnd))
        toFill = static_cast<base::SizeT>(loopEnd - currentOffset);

    // Fill the chunk parameters
    data.samples     = m_impl->samples.data();
    data.sampleCount = static_cast<base::SizeT>(m_impl->file.read(m_impl->samples.data(), toFill));
    currentOffset += data.sampleCount;

    // Check if we have stopped obtaining samples or reached either the EOF or the loop end point
    return (data.sampleCount != 0) && (currentOffset < m_impl->file.getSampleCount()) &&
           (currentOffset != loopEnd || m_impl->loopSpan.length == 0);
}


////////////////////////////////////////////////////////////
void Music::onSeek(Time timeOffset)
{
    const std::lock_guard lock(m_impl->mutex);
    m_impl->file.seek(timeOffset);
}


////////////////////////////////////////////////////////////
base::Optional<base::U64> Music::onLoop()
{
    // Called by underlying SoundStream so we can determine where to loop.
    const std::lock_guard lock(m_impl->mutex);
    const base::U64       currentOffset = m_impl->file.getSampleOffset();

    if (isLooping() && (m_impl->loopSpan.length != 0) &&
        (currentOffset == m_impl->loopSpan.offset + m_impl->loopSpan.length))
    {
        // Looping is enabled, and either we're at the loop end, or we're at the EOF
        // when it's equivalent to the loop end (loop end takes priority). Send us to loop begin
        m_impl->file.seek(m_impl->loopSpan.offset);
        return base::makeOptional(m_impl->file.getSampleOffset());
    }

    if (isLooping() && (currentOffset >= m_impl->file.getSampleCount()))
    {
        // If we're at the EOF, reset to 0
        m_impl->file.seek(0);
        return base::makeOptional(base::U64{0});
    }

    return base::nullOpt;
}


////////////////////////////////////////////////////////////
Music::TimeSpan Music::getLoopPoints() const
{
    return TimeSpan{samplesToTime(getSampleRate(), getChannelCount(), m_impl->loopSpan.offset),
                    samplesToTime(getSampleRate(), getChannelCount(), m_impl->loopSpan.length)};
}


////////////////////////////////////////////////////////////
void Music::setLoopPoints(TimeSpan timePoints)
{
    Span<base::U64> samplePoints{timeToSamples(getSampleRate(), getChannelCount(), timePoints.offset),
                                 timeToSamples(getSampleRate(), getChannelCount(), timePoints.length)};

    // Check our state. This averts a divide-by-zero. GetChannelCount() is cheap enough to use often
    if (getChannelCount() == 0 || m_impl->file.getSampleCount() == 0)
    {
        priv::err() << "Music is not in a valid state to assign Loop Points.";
        return;
    }

    // Round up to the next even sample if needed
    samplePoints.offset += (getChannelCount() - 1);
    samplePoints.offset -= (samplePoints.offset % getChannelCount());
    samplePoints.length += (getChannelCount() - 1);
    samplePoints.length -= (samplePoints.length % getChannelCount());

    // Validate
    if (samplePoints.offset >= m_impl->file.getSampleCount())
    {
        priv::err() << "LoopPoints offset val must be in range [0, Duration).";
        return;
    }

    if (samplePoints.length == 0)
    {
        priv::err() << "LoopPoints length val must be nonzero.";
        return;
    }

    // Clamp End Point
    samplePoints.length = base::min(samplePoints.length, m_impl->file.getSampleCount() - samplePoints.offset);

    // If this change has no effect, we can return without touching anything
    if (samplePoints.offset == m_impl->loopSpan.offset && samplePoints.length == m_impl->loopSpan.length)
        return;

    // When we apply this change, we need to "reset" this instance and its buffer

    // Get old playing status and position
    const Status oldStatus = getStatus();
    const Time   oldPos    = getPlayingOffset();

    // Unload
    stop();

    // Set
    m_impl->loopSpan = samplePoints;

    // Restore
    if (oldPos != Time::Zero)
        setPlayingOffset(oldPos);

    // Resume
    if (oldStatus == Status::Playing)
    {
        SFML_BASE_ASSERT(m_lastPlaybackDevice != nullptr);
        play(*m_lastPlaybackDevice);
    }
}

} // namespace sf
