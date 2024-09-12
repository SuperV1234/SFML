#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/VideoMode.hpp"
#include "SFML/Window/VideoModeUtils.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Span.hpp"


namespace sf
{
////////////////////////////////////////////////////////////
bool VideoMode::isValid() const
{
    const base::Span<const VideoMode> modes = VideoModeUtils::getFullscreenModes();

    return base::find(modes.begin(), modes.end(), *this) != modes.end();
}


////////////////////////////////////////////////////////////
bool operator==(const VideoMode& left, const VideoMode& right)
{
    return (left.size == right.size) && (left.bitsPerPixel == right.bitsPerPixel);
}


////////////////////////////////////////////////////////////
bool operator!=(const VideoMode& left, const VideoMode& right)
{
    return !(left == right);
}


////////////////////////////////////////////////////////////
bool operator<(const VideoMode& left, const VideoMode& right)
{
    if (left.bitsPerPixel == right.bitsPerPixel)
    {
        if (left.size.x == right.size.x)
        {
            return left.size.y < right.size.y;
        }

        return left.size.x < right.size.x;
    }

    return left.bitsPerPixel < right.bitsPerPixel;
}


////////////////////////////////////////////////////////////
bool operator>(const VideoMode& left, const VideoMode& right)
{
    return right < left;
}


////////////////////////////////////////////////////////////
bool operator<=(const VideoMode& left, const VideoMode& right)
{
    return !(right < left);
}


////////////////////////////////////////////////////////////
bool operator>=(const VideoMode& left, const VideoMode& right)
{
    return !(left < right);
}

} // namespace sf
