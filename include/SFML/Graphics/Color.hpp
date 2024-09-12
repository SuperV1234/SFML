#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/Export.hpp"

#include <cstdint>


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Utility class for manipulating RGBA colors
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] SFML_GRAPHICS_API Color
{
    ////////////////////////////////////////////////////////////
    /// \brief Construct the color from 32-bit unsigned integer
    ///
    /// \param color Number containing the RGBA components (in that order)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] constexpr static Color fromRGBA(std::uint32_t color);

    ////////////////////////////////////////////////////////////
    /// \brief Retrieve the color as a 32-bit unsigned integer
    ///
    /// \return Color represented as a 32-bit unsigned integer
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr std::uint32_t toInteger() const;

    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    // NOLINTBEGIN(readability-identifier-naming)
    static const Color Black;       //!< Black predefined color
    static const Color White;       //!< White predefined color
    static const Color Red;         //!< Red predefined color
    static const Color Green;       //!< Green predefined color
    static const Color Blue;        //!< Blue predefined color
    static const Color Yellow;      //!< Yellow predefined color
    static const Color Magenta;     //!< Magenta predefined color
    static const Color Cyan;        //!< Cyan predefined color
    static const Color Transparent; //!< Transparent (black) predefined color
    // NOLINTEND(readability-identifier-naming)

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::uint8_t r{};     //!< Red component
    std::uint8_t g{};     //!< Green component
    std::uint8_t b{};     //!< Blue component
    std::uint8_t a{255u}; //!< Alpha (opacity) component
};

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the `operator==`
///
/// This operator compares two colors and check if they are equal.
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return `true` if colors are equal, `false` if they are different
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] constexpr bool operator==(Color left, Color right);

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the `operator!=`
///
/// This operator compares two colors and check if they are different.
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return `true` if colors are different, `false` if they are equal
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] constexpr bool operator!=(Color left, Color right);

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the binary `operator+`
///
/// This operator returns the component-wise sum of two colors.
/// Components that exceed 255 are clamped to 255.
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return Result of \a left + \a right
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] constexpr Color operator+(Color left, Color right);

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the binary `operator-`
///
/// This operator returns the component-wise subtraction of two colors.
/// Components below 0 are clamped to 0.
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return Result of \a left - \a right
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] constexpr Color operator-(Color left, Color right);

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the binary `operator*`
///
/// This operator returns the component-wise multiplication
/// (also called "modulation") of two colors.
/// Components are then divided by 255 so that the result is
/// still in the range [0, 255].
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return Result of \a left * \a right
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] constexpr Color operator*(Color left, Color right);

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the binary `operator+=`
///
/// This operator computes the component-wise sum of two colors,
/// and assigns the result to the left operand.
/// Components that exceed 255 are clamped to 255.
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return Reference to \a left
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Color& operator+=(Color& left, Color right);

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the binary `operator-=`
///
/// This operator computes the component-wise subtraction of two colors,
/// and assigns the result to the left operand.
/// Components below 0 are clamped to 0.
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return Reference to \a left
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Color& operator-=(Color& left, Color right);

////////////////////////////////////////////////////////////
/// \relates Color
/// \brief Overload of the binary `operator*=`
///
/// This operator returns the component-wise multiplication
/// (also called "modulation") of two colors, and assigns
/// the result to the left operand.
/// Components are then divided by 255 so that the result is
/// still in the range [0, 255].
///
/// \param left  Left operand
/// \param right Right operand
///
/// \return Reference to \a left
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Color& operator*=(Color& left, Color right);

} // namespace sf

#include "SFML/Graphics/Color.inl"


////////////////////////////////////////////////////////////
/// \class sf::Color
/// \ingroup graphics
///
/// `sf::Color` is a simple color class composed of 4 components:
/// \li Red
/// \li Green
/// \li Blue
/// \li Alpha (opacity)
///
/// Each component is a public member, an unsigned integer in
/// the range [0, 255]. Thus, colors can be constructed and
/// manipulated very easily:
///
/// \code
/// sf::Color color(255, 0, 0); // red
/// color.r = 0;                // make it black
/// color.b = 128;              // make it dark blue
/// \endcode
///
/// The fourth component of colors, named "alpha", represents
/// the opacity of the color. A color with an alpha value of
/// 255 will be fully opaque, while an alpha value of 0 will
/// make a color fully transparent, whatever the value of the
/// other components is.
///
/// The most common colors are already defined as static variables:
/// \code
/// sf::Color black       = sf::Color::Black;
/// sf::Color white       = sf::Color::White;
/// sf::Color red         = sf::Color::Red;
/// sf::Color green       = sf::Color::Green;
/// sf::Color blue        = sf::Color::Blue;
/// sf::Color yellow      = sf::Color::Yellow;
/// sf::Color magenta     = sf::Color::Magenta;
/// sf::Color cyan        = sf::Color::Cyan;
/// sf::Color transparent = sf::Color::Transparent;
/// \endcode
///
/// Colors can also be added and modulated (multiplied) using the
/// overloaded operators + and *.
///
////////////////////////////////////////////////////////////
