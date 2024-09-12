#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <cstdint>


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Represents a time value
///
////////////////////////////////////////////////////////////
class Time
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Sets the time value to zero.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr Time() = default;

    ////////////////////////////////////////////////////////////
    /// \brief Construct from microseconds
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] constexpr explicit Time(std::int64_t microseconds);

    ////////////////////////////////////////////////////////////
    /// \brief Return the time value as a number of seconds
    ///
    /// \return Time in seconds
    ///
    /// \see `asMilliseconds`, `asMicroseconds`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr float asSeconds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return the time value as a number of milliseconds
    ///
    /// \return Time in milliseconds
    ///
    /// \see `asSeconds`, `asMicroseconds`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr std::int32_t asMilliseconds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return the time value as a number of microseconds
    ///
    /// \return Time in microseconds
    ///
    /// \see `asSeconds`, `asMilliseconds`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr std::int64_t asMicroseconds() const;

    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(readability-identifier-naming)
    static const Time Zero; //!< Predefined "zero" time value

private:
    friend constexpr Time seconds(float);
    friend constexpr Time milliseconds(std::int32_t);
    friend constexpr Time microseconds(std::int64_t);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::int64_t m_microseconds{}; //!< Time value stored as microseconds
};

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Construct a time value from a number of seconds
///
/// \param amount Number of seconds
///
/// \return Time value constructed from the amount of seconds
///
/// \see `milliseconds`, `microseconds`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time seconds(float amount);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Construct a time value from a number of milliseconds
///
/// \param amount Number of milliseconds
///
/// \return Time value constructed from the amount of milliseconds
///
/// \see `seconds`, `microseconds`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time milliseconds(std::int32_t amount);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Construct a time value from a number of microseconds
///
/// \param amount Number of microseconds
///
/// \return Time value constructed from the amount of microseconds
///
/// \see `seconds`, `milliseconds`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time microseconds(std::int64_t amount);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of `operator==` to compare two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return `true` if both time values are equal
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator==(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of `operator!=` to compare two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return `true` if both time values are different
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator!=(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of `operator<` to compare two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return `true` if \a `left` is lesser than \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator<(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of `operator>` to compare two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return `true` if \a `left` is greater than \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator>(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of `operator<=` to compare two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return `true` if \a `left` is lesser or equal than \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator<=(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of `operator>=` to compare two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return `true` if \a `left` is greater or equal than \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator>=(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of unary `operator-` to negate a time value
///
/// \param right Right operand (a time)
///
/// \return Opposite of the time value
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator-(Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator+` to add two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return Sum of the two times values
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator+(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator+=` to add/assign two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return Sum of the two times values
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Time& operator+=(Time& left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator-` to subtract two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return Difference of the two times values
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator-(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator-=` to subtract/assign two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return Difference of the two times values
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Time& operator-=(Time& left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator*` to scale a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` multiplied by \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator*(Time left, float right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator*` to scale a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` multiplied by \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator*(Time left, std::int64_t right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator*` to scale a time value
///
/// \param left  Left operand (a number)
/// \param right Right operand (a time)
///
/// \return \a `left` multiplied by \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator*(float left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator*` to scale a time value
///
/// \param left  Left operand (a number)
/// \param right Right operand (a time)
///
/// \return \a `left` multiplied by \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator*(std::int64_t left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator*=` to scale/assign a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` multiplied by \a `right`
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Time& operator*=(Time& left, float right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator*=` to scale/assign a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` multiplied by \a `right`
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Time& operator*=(Time& left, std::int64_t right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator/` to scale a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` divided by \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator/(Time left, float right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator/` to scale a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` divided by \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator/(Time left, std::int64_t right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator/=` to scale/assign a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` divided by \a `right`
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Time& operator/=(Time& left, float right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator/=` to scale/assign a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a number)
///
/// \return \a `left` divided by \a `right`
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Time& operator/=(Time& left, std::int64_t right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator/` to compute the ratio of two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return \a `left` divided by \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr float operator/(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator%` to compute remainder of a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return \a `left` modulo \a `right`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Time operator%(Time left, Time right);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary `operator%=` to compute/assign remainder of a time value
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return \a `left` modulo \a `right`
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr Time& operator%=(Time& left, Time right);

} // namespace sf

#include "SFML/System/Time.inl"


////////////////////////////////////////////////////////////
/// \class sf::Time
/// \ingroup system
///
/// `sf::Time` encapsulates a time value in a flexible way.
/// It allows to define a time value either as a number of
/// seconds, milliseconds or microseconds. It also works the
/// other way round: you can read a time value as either
/// a number of seconds, milliseconds or microseconds. It
/// even interoperates with the `<chrono>` header. You can
/// construct an `sf::Time` from a `chrono::duration` and read
/// any `sf::Time` as a chrono::duration.
///
/// By using such a flexible interface, the API doesn't
/// impose any fixed type or resolution for time values,
/// and let the user choose its own favorite representation.
///
/// Time values support the usual mathematical operations:
/// you can add or subtract two times, multiply or divide
/// a time by a number, compare two times, etc.
///
/// Since they represent a time span and not an absolute time
/// value, times can also be negative.
///
/// Usage example:
/// \code
/// sf::Time t1 = sf::seconds(0.1f);
/// std::int32_t milli = t1.asMilliseconds(); // 100
///
/// sf::Time t2 = sf::milliseconds(30);
/// std::int64_t micro = t2.asMicroseconds(); // 30000
///
/// sf::Time t3 = sf::microseconds(-800000);
/// float sec = t3.asSeconds(); // -0.8
///
/// sf::Time t4 = std::chrono::milliseconds(250);
/// std::chrono::microseconds micro2 = t4.toDuration(); // 250000us
/// \endcode
///
/// \code
/// void update(sf::Time elapsed)
/// {
///    position += speed * elapsed.asSeconds();
/// }
///
/// update(sf::milliseconds(100));
/// \endcode
///
/// \see `sf::Clock`
///
////////////////////////////////////////////////////////////
