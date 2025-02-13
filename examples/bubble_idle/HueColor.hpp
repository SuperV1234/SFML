#pragma once

#include "AssertAssume.hpp"

#include "SFML/Graphics/Color.hpp"

#include "SFML/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr bool isHueColor(const sf::Color color) noexcept
{
    return color.r == 1 && color.g == 1;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr sf::Color hueColor(const float hue, const sf::base::U8 alpha)
{
    ASSERT_AND_ASSUME(hue >= 0.f && hue <= 360.f);

    // R=1 and G=1 is used in the shader as a signal to make the color hue-based
    return sf::Color{1, 1, static_cast<sf::base::U8>(hue / 360.f * 255.f), alpha};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr sf::Color hueByteColor(const sf::base::U8 hueByte,
                                                                                      const sf::base::U8 alpha)
{
    // R=1 and G=1 is used in the shader as a signal to make the color hue-based
    return sf::Color{1, 1, hueByte, alpha};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr float extractHue(const sf::Color hueColor)
{
    return static_cast<float>(hueColor.g) / 255.f * 360.f;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr sf::base::U8 hueToByte(const float hue)
{
    ASSERT_AND_ASSUME(hue >= 0.f && hue <= 360.f);
    return static_cast<sf::base::U8>(hue / 360.f * 255.f);
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr float byteToHue(const sf::base::U8 byte)
{
    return static_cast<float>(byte) / 255.f * 360.f;
}
