#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Network/Packet.hpp"
#include "SFML/Network/SocketImpl.hpp"

#include "SFML/System/String.hpp"
#include "SFML/System/Utils.hpp"

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Memcpy.hpp"
#include "SFML/Base/Strlen.hpp"

#include <string>
#include <vector>

#include <cwchar>

namespace sf
{
////////////////////////////////////////////////////////////
struct Packet::Impl
{
    std::vector<std::byte> data;          //!< Data stored in the packet
    std::size_t            readPos{};     //!< Current reading position in the packet
    std::size_t            sendPos{};     //!< Current send position in the packet (for handling partial sends)
    bool                   isValid{true}; //!< Reading state of the packet
};


////////////////////////////////////////////////////////////
Packet::Packet() = default;


////////////////////////////////////////////////////////////
Packet::~Packet() = default;


////////////////////////////////////////////////////////////
Packet::Packet(const Packet&) = default;


////////////////////////////////////////////////////////////
Packet& Packet::operator=(const Packet&) = default;


////////////////////////////////////////////////////////////
Packet::Packet(Packet&&) noexcept = default;


////////////////////////////////////////////////////////////
Packet& Packet::operator=(Packet&&) noexcept = default;


////////////////////////////////////////////////////////////
void Packet::append(const void* data, std::size_t sizeInBytes)
{
    if (data && (sizeInBytes > 0))
    {
        const auto* begin = reinterpret_cast<const std::byte*>(data);
        const auto* end   = begin + sizeInBytes;
        m_impl->data.insert(m_impl->data.end(), begin, end);
    }
}


////////////////////////////////////////////////////////////
std::size_t Packet::getReadPosition() const
{
    return m_impl->readPos;
}


////////////////////////////////////////////////////////////
void Packet::clear()
{
    m_impl->data.clear();
    m_impl->readPos = 0;
    m_impl->isValid = true;
}


////////////////////////////////////////////////////////////
const void* Packet::getData() const
{
    return !m_impl->data.empty() ? m_impl->data.data() : nullptr;
}


////////////////////////////////////////////////////////////
std::size_t Packet::getDataSize() const
{
    return m_impl->data.size();
}


////////////////////////////////////////////////////////////
bool Packet::endOfPacket() const
{
    return m_impl->readPos >= m_impl->data.size();
}


////////////////////////////////////////////////////////////
Packet::operator bool() const
{
    return m_impl->isValid;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(bool& data)
{
    std::uint8_t value = 0;
    if (*this >> value)
        data = (value != 0);

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::int8_t& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::uint8_t& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::int16_t& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        data = static_cast<std::int16_t>(priv::SocketImpl::ntohs(static_cast<std::uint16_t>(data)));
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::uint16_t& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        data = priv::SocketImpl::ntohs(data);
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::int32_t& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        data = static_cast<std::int32_t>(priv::SocketImpl::ntohl(static_cast<std::uint32_t>(data)));
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::uint32_t& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        data = priv::SocketImpl::ntohl(data);
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::int64_t& data)
{
    if (checkSize(sizeof(data)))
    {
        // Since ntohll is not available everywhere, we have to convert
        // to network byte order (big endian) manually
        std::byte bytes[sizeof(data)];
        SFML_BASE_MEMCPY(bytes, &m_impl->data[m_impl->readPos], sizeof(data));

        data = priv::byteSequenceToInteger<
            std::int64_t>(bytes[7], bytes[6], bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]);

        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::uint64_t& data)
{
    if (checkSize(sizeof(data)))
    {
        // Since ntohll is not available everywhere, we have to convert
        // to network byte order (big endian) manually
        std::byte bytes[sizeof(data)]{};
        SFML_BASE_MEMCPY(bytes, &m_impl->data[m_impl->readPos], sizeof(data));

        data = priv::byteSequenceToInteger<
            std::uint64_t>(bytes[7], bytes[6], bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]);

        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(float& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(double& data)
{
    if (checkSize(sizeof(data)))
    {
        SFML_BASE_MEMCPY(&data, &m_impl->data[m_impl->readPos], sizeof(data));
        m_impl->readPos += sizeof(data);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(char* data)
{
    SFML_BASE_ASSERT(data && "Packet::operator>> Data must not be null");

    // First extract string length
    std::uint32_t length = 0;
    *this >> length;

    if ((length > 0) && checkSize(length))
    {
        // Then extract characters
        SFML_BASE_MEMCPY(data, &m_impl->data[m_impl->readPos], length);
        data[length] = '\0';

        // Update reading position
        m_impl->readPos += length;
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::string& data)
{
    // First extract string length
    std::uint32_t length = 0;
    *this >> length;

    data.clear();
    if ((length > 0) && checkSize(length))
    {
        // Then extract characters
        data.assign(reinterpret_cast<char*>(&m_impl->data[m_impl->readPos]), length);

        // Update reading position
        m_impl->readPos += length;
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(wchar_t* data)
{
    SFML_BASE_ASSERT(data && "Packet::operator>> Data must not be null");

    // First extract string length
    std::uint32_t length = 0;
    *this >> length;

    if ((length > 0) && checkSize(length * sizeof(std::uint32_t)))
    {
        // Then extract characters
        for (std::uint32_t i = 0; i < length; ++i)
        {
            std::uint32_t character = 0;
            *this >> character;
            data[i] = static_cast<wchar_t>(character);
        }
        data[length] = L'\0';
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::wstring& data)
{
    // First extract string length
    std::uint32_t length = 0;
    *this >> length;

    data.clear();
    if ((length > 0) && checkSize(length * sizeof(std::uint32_t)))
    {
        // Then extract characters
        for (std::uint32_t i = 0; i < length; ++i)
        {
            std::uint32_t character = 0;
            *this >> character;
            data += static_cast<wchar_t>(character);
        }
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(String& data)
{
    // First extract the string length
    std::uint32_t length = 0;
    *this >> length;

    data.clear();
    if ((length > 0) && checkSize(length * sizeof(std::uint32_t)))
    {
        // Then extract characters
        for (std::uint32_t i = 0; i < length; ++i)
        {
            std::uint32_t character = 0;
            *this >> character;
            data += static_cast<char32_t>(character);
        }
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(bool data)
{
    *this << static_cast<std::uint8_t>(data);
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::int8_t data)
{
    append(&data, sizeof(data));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::uint8_t data)
{
    append(&data, sizeof(data));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::int16_t data)
{
    auto toWrite = static_cast<std::int16_t>(priv::SocketImpl::htons(static_cast<std::uint16_t>(data)));
    append(&toWrite, sizeof(toWrite));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::uint16_t data)
{
    std::uint16_t toWrite = priv::SocketImpl::htons(data);
    append(&toWrite, sizeof(toWrite));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::int32_t data)
{
    auto toWrite = static_cast<std::int32_t>(priv::SocketImpl::htonl(static_cast<std::uint32_t>(data)));
    append(&toWrite, sizeof(toWrite));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::uint32_t data)
{
    std::uint32_t toWrite = priv::SocketImpl::htonl(data);
    append(&toWrite, sizeof(toWrite));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::int64_t data)
{
    // Since htonll is not available everywhere, we have to convert
    // to network byte order (big endian) manually

    std::uint8_t toWrite[] = {static_cast<std::uint8_t>((data >> 56) & 0xFF),
                              static_cast<std::uint8_t>((data >> 48) & 0xFF),
                              static_cast<std::uint8_t>((data >> 40) & 0xFF),
                              static_cast<std::uint8_t>((data >> 32) & 0xFF),
                              static_cast<std::uint8_t>((data >> 24) & 0xFF),
                              static_cast<std::uint8_t>((data >> 16) & 0xFF),
                              static_cast<std::uint8_t>((data >> 8) & 0xFF),
                              static_cast<std::uint8_t>((data) & 0xFF)};

    append(&toWrite, sizeof(toWrite));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(std::uint64_t data)
{
    // Since htonll is not available everywhere, we have to convert
    // to network byte order (big endian) manually

    std::uint8_t toWrite[] = {static_cast<std::uint8_t>((data >> 56) & 0xFF),
                              static_cast<std::uint8_t>((data >> 48) & 0xFF),
                              static_cast<std::uint8_t>((data >> 40) & 0xFF),
                              static_cast<std::uint8_t>((data >> 32) & 0xFF),
                              static_cast<std::uint8_t>((data >> 24) & 0xFF),
                              static_cast<std::uint8_t>((data >> 16) & 0xFF),
                              static_cast<std::uint8_t>((data >> 8) & 0xFF),
                              static_cast<std::uint8_t>((data) & 0xFF)};

    append(&toWrite, sizeof(toWrite));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(float data)
{
    append(&data, sizeof(data));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(double data)
{
    append(&data, sizeof(data));
    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const char* data)
{
    SFML_BASE_ASSERT(data && "Packet::operator<< Data must not be null");

    // First insert string length
    const auto length = static_cast<std::uint32_t>(SFML_BASE_STRLEN(data));
    *this << length;

    // Then insert characters
    append(data, length * sizeof(char));

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const std::string& data)
{
    // First insert string length
    const auto length = static_cast<std::uint32_t>(data.size());
    *this << length;

    // Then insert characters
    if (length > 0)
        append(data.c_str(), length * sizeof(std::string::value_type));

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const wchar_t* data)
{
    SFML_BASE_ASSERT(data && "Packet::operator<< Data must not be null");

    // First insert string length
    const auto length = static_cast<std::uint32_t>(std::wcslen(data));
    *this << length;

    // Then insert characters
    for (const wchar_t* c = data; *c != L'\0'; ++c)
        *this << static_cast<std::uint32_t>(*c);

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const std::wstring& data)
{
    // First insert string length
    const auto length = static_cast<std::uint32_t>(data.size());
    *this << length;

    // Then insert characters
    if (length > 0)
    {
        for (const wchar_t c : data)
            *this << static_cast<std::uint32_t>(c);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const String& data)
{
    // First insert the string length
    const auto length = static_cast<std::uint32_t>(data.getSize());
    *this << length;

    // Then insert characters
    if (length > 0)
    {
        for (const unsigned int datum : data)
            *this << datum;
    }

    return *this;
}


////////////////////////////////////////////////////////////
bool Packet::checkSize(std::size_t size)
{
    m_impl->isValid = m_impl->isValid && (m_impl->readPos + size <= m_impl->data.size());

    return m_impl->isValid;
}


////////////////////////////////////////////////////////////
std::size_t& Packet::getSendPos()
{
    return m_impl->sendPos;
}


////////////////////////////////////////////////////////////
const void* Packet::onSend(std::size_t& size)
{
    size = getDataSize();
    return getData();
}


////////////////////////////////////////////////////////////
void Packet::onReceive(const void* data, std::size_t size)
{
    append(data, size);
}

} // namespace sf
