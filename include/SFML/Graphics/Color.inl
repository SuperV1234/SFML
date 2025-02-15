#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/Color.hpp" // NOLINT(misc-header-include-cycle)


namespace sf
{
////////////////////////////////////////////////////////////
constexpr Color Color::fromRGBA(base::U32 color)
{
    return {static_cast<base::U8>((color & 0xff000000) >> 24),
            static_cast<base::U8>((color & 0x00ff0000) >> 16),
            static_cast<base::U8>((color & 0x0000ff00) >> 8),
            static_cast<base::U8>(color & 0x000000ff)};
}


////////////////////////////////////////////////////////////
constexpr Color Color::fromHSLA(float hue, float saturation, float lightness, base::U8 alpha)
{
    while (hue < 0.f)
        hue += 360.f;

    while (hue > 360.f)
        hue -= 360.f;

    const auto clampBetweenZeroAndOne = [](float value) -> float
    { return value < 0.f ? 0.f : (value > 1.f ? 1.f : value); };

    saturation = clampBetweenZeroAndOne(saturation);
    lightness  = clampBetweenZeroAndOne(lightness);

    // `maxChroma` and `minChroma` define the range for each color component
    // `maxChroma` is the upper bound, `minChroma` is the lower bound
    const float maxChroma = lightness < 0.5f ? lightness * (1 + saturation) : lightness + saturation - lightness * saturation;
    const float minChroma = 2 * lightness - maxChroma;

    const auto hueToRGB = [&](float normalizedHue) -> float
    {
        if (normalizedHue < 0.f)
            normalizedHue += 1.f;

        if (normalizedHue > 1.f)
            normalizedHue -= 1.f;

        if (normalizedHue < 1.0f / 6.0f)
            return minChroma + (maxChroma - minChroma) * 6.f * normalizedHue;

        if (normalizedHue < 1.0f / 2.0f)
            return maxChroma;

        if (normalizedHue < 2.0f / 3.0f)
            return minChroma + (maxChroma - minChroma) * (2.0f / 3.0f - normalizedHue) * 6.f;

        return minChroma;
    };

    const float normalizedHue = hue / 360.0f;

    // NOLINTBEGIN(bugprone-incorrect-roundings)
    return {static_cast<base::U8>(hueToRGB(normalizedHue + 1.0f / 3.0f) * 255.f + 0.5f),
            static_cast<base::U8>(hueToRGB(normalizedHue) * 255 + 0.5f),
            static_cast<base::U8>(hueToRGB(normalizedHue - 1.0f / 3.0f) * 255.f + 0.5f),
            alpha};
    // NOLINTEND(bugprone-incorrect-roundings)
}


////////////////////////////////////////////////////////////
constexpr base::U32 Color::toInteger() const
{
    return static_cast<base::U32>((r << 24) | (g << 16) | (b << 8) | a);
}


////////////////////////////////////////////////////////////
constexpr Color operator+(Color lhs, Color rhs)
{
    const auto clampedAdd = [](base::U8 l, base::U8 r) __attribute__((always_inline, flatten)) -> base::U8
    {
        const int intResult = int{l} + int{r};
        return static_cast<base::U8>(intResult < 255 ? intResult : 255);
    };

    return {clampedAdd(lhs.r, rhs.r), clampedAdd(lhs.g, rhs.g), clampedAdd(lhs.b, rhs.b), clampedAdd(lhs.a, rhs.a)};
}


////////////////////////////////////////////////////////////
constexpr Color operator-(Color lhs, Color rhs)
{
    const auto clampedSub = [](base::U8 l, base::U8 r) __attribute__((always_inline, flatten)) -> base::U8
    {
        const int intResult = int{l} - int{r};
        return static_cast<base::U8>(intResult > 0 ? intResult : 0);
    };

    return {clampedSub(lhs.r, rhs.r), clampedSub(lhs.g, rhs.g), clampedSub(lhs.b, rhs.b), clampedSub(lhs.a, rhs.a)};
}


////////////////////////////////////////////////////////////
constexpr Color operator*(Color lhs, Color rhs)
{
    const auto scaledMul = [](base::U8 l, base::U8 r) __attribute__((always_inline, flatten)) -> base::U8
    {
        const auto uint16Result = static_cast<base::U16>(base::U16{l} * base::U16{r});
        return static_cast<base::U8>(uint16Result / 255u);
    };

    return {scaledMul(lhs.r, rhs.r), scaledMul(lhs.g, rhs.g), scaledMul(lhs.b, rhs.b), scaledMul(lhs.a, rhs.a)};
}


////////////////////////////////////////////////////////////
constexpr Color& operator+=(Color& lhs, Color rhs)
{
    return lhs = lhs + rhs;
}


////////////////////////////////////////////////////////////
constexpr Color& operator-=(Color& lhs, Color rhs)
{
    return lhs = lhs - rhs;
}


////////////////////////////////////////////////////////////
constexpr Color& operator*=(Color& lhs, Color rhs)
{
    return lhs = lhs * rhs;
}


////////////////////////////////////////////////////////////
// Static member data
////////////////////////////////////////////////////////////

// Note: the 'inline' keyword here is technically not required, but VS2019 fails
// to compile with a bogus "multiple definition" error if not explicitly used.
inline constexpr Color Color::Black(0, 0, 0);
inline constexpr Color Color::White(255, 255, 255);
inline constexpr Color Color::Red(255, 0, 0);
inline constexpr Color Color::Green(0, 255, 0);
inline constexpr Color Color::Blue(0, 0, 255);
inline constexpr Color Color::Yellow(255, 255, 0);
inline constexpr Color Color::Magenta(255, 0, 255);
inline constexpr Color Color::Cyan(0, 255, 255);
inline constexpr Color Color::Transparent(0, 0, 0, 0);

inline constexpr Color Color::Brown{128u, 80u, 32u};
inline constexpr Color Color::Orange{255u, 128u, 0u};
inline constexpr Color Color::Pink{255u, 128u, 192u};
inline constexpr Color Color::BabyPink{255u, 192u, 224u};
inline constexpr Color Color::HotPink{255u, 0u, 192u};
inline constexpr Color Color::Salmon{255u, 128u, 128u};
inline constexpr Color Color::Violet{128u, 0u, 255u};
inline constexpr Color Color::Purple{64u, 0u, 128u};
inline constexpr Color Color::Peach{255u, 128u, 96u};
inline constexpr Color Color::Lime{128u, 255u, 0u};
inline constexpr Color Color::Mint{64u, 255u, 192u};
inline constexpr Color Color::Gray{128u, 128u, 128u};

inline constexpr Color Color::LightBlack{64u, 64u, 64u};
inline constexpr Color Color::LightBlue{128u, 128u, 255u};
inline constexpr Color Color::LightRed{255u, 128u, 128u};
inline constexpr Color Color::LightMagenta{255u, 128u, 255u};
inline constexpr Color Color::LightGreen{128u, 255u, 128u};
inline constexpr Color Color::LightCyan{128u, 255u, 255u};
inline constexpr Color Color::LightYellow{255u, 255u, 128u};
inline constexpr Color Color::LightBrown{192u, 128u, 64u};
inline constexpr Color Color::LightOrange{255u, 160u, 64u};
inline constexpr Color Color::LightPink{255u, 160u, 224u};
inline constexpr Color Color::LightBabyPink{255u, 208u, 232u};
inline constexpr Color Color::LightHotPink{255u, 96u, 224u};
inline constexpr Color Color::LightSalmon{255u, 160u, 160u};
inline constexpr Color Color::LightViolet{160u, 96u, 255u};
inline constexpr Color Color::LightPurple{128u, 64u, 192u};
inline constexpr Color Color::LightPeach{255u, 160u, 128u};
inline constexpr Color Color::LightLime{192u, 255u, 128u};
inline constexpr Color Color::LightMint{128u, 255u, 22u};
inline constexpr Color Color::LightGray{192u, 192u, 192u};

inline constexpr Color Color::DarkBlue{0u, 0u, 128u};
inline constexpr Color Color::DarkRed{128u, 0u, 0u};
inline constexpr Color Color::DarkMagenta{128u, 0u, 128u};
inline constexpr Color Color::DarkGreen{0u, 128u, 0u};
inline constexpr Color Color::DarkCyan{0u, 128u, 128u};
inline constexpr Color Color::DarkYellow{128u, 128u, 0u};
inline constexpr Color Color::DarkWhite{128u, 128u, 128u};
inline constexpr Color Color::DarkBrown{64u, 32u, 0u};
inline constexpr Color Color::DarkOrange{128u, 64u, 0u};
inline constexpr Color Color::DarkPink{128u, 64u, 96u};
inline constexpr Color Color::DarkBabyPink{160u, 96u, 128u};
inline constexpr Color Color::DarkHotPink{128u, 0u, 96u};
inline constexpr Color Color::DarkSalmon{128u, 64u, 64u};
inline constexpr Color Color::DarkViolet{64u, 0u, 128u};
inline constexpr Color Color::DarkPurple{32u, 0u, 64u};
inline constexpr Color Color::DarkPeach{128u, 64u, 48u};
inline constexpr Color Color::DarkLime{64u, 128u, 0u};
inline constexpr Color Color::DarkMint{32u, 128u, 96u};
inline constexpr Color Color::DarkGray{64u, 64u, 64u};

inline constexpr Color Color::VeryDarkBlue{0u, 0u, 64u};
inline constexpr Color Color::VeryDarkRed{64u, 0u, 0u};
inline constexpr Color Color::VeryDarkMagenta{64u, 0u, 64u};
inline constexpr Color Color::VeryDarkGreen{0u, 64u, 0u};
inline constexpr Color Color::VeryDarkCyan{0u, 64u, 64u};
inline constexpr Color Color::VeryDarkYellow{64u, 64u, 0u};
inline constexpr Color Color::VeryDarkWhite{64u, 64u, 64u};
inline constexpr Color Color::VeryDarkBrown{32u, 16u, 0u};
inline constexpr Color Color::VeryDarkOrange{64u, 32u, 0u};
inline constexpr Color Color::VeryDarkPink{64u, 32u, 48u};
inline constexpr Color Color::VeryDarkBabyPink{80u, 48u, 64u};
inline constexpr Color Color::VeryDarkHotPink{64u, 0u, 48u};
inline constexpr Color Color::VeryDarkSalmon{64u, 32u, 32u};
inline constexpr Color Color::VeryDarkViolet{32u, 0u, 64u};
inline constexpr Color Color::VeryDarkPurple{16u, 0u, 32u};
inline constexpr Color Color::VeryDarkPeach{64u, 32u, 24u};
inline constexpr Color Color::VeryDarkLime{32u, 64u, 0u};
inline constexpr Color Color::VeryDarkMint{16u, 64u, 48u};
inline constexpr Color Color::VeryDarkGray{32u, 32u, 32u};

} // namespace sf
