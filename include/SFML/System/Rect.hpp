#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/System/Vector2.hpp"


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Utility class for manipulating 2D axis aligned rectangles
///
////////////////////////////////////////////////////////////
template <typename T>
class [[nodiscard]] Rect
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Check if a point is inside the rectangle's area
    ///
    /// This check is non-inclusive. If the point lies on the
    /// edge of the rectangle, this function will return `false`.
    ///
    /// \param point Point to test
    ///
    /// \return `true` if the point is inside, `false` otherwise
    ///
    /// \see `findIntersection`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr bool contains(Vector2<T> point) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the position of the center of the rectangle
    ///
    /// \return Center of rectangle
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline constexpr Vector2<T> getCenter() const;

    ////////////////////////////////////////////////////////////
    /// \brief Convert to another `Rect` of type `OtherRect`
    ///
    /// `OtherRect` must be a `Rect<...>` type.
    ///
    ////////////////////////////////////////////////////////////
    template <typename U>
    [[nodiscard, gnu::always_inline, gnu::pure]] inline constexpr U to() const;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vector2<T> position{}; //!< Position of the top-left corner of the rectangle
    Vector2<T> size{};     //!< Size of the rectangle
};

////////////////////////////////////////////////////////////
/// \relates Rect
/// \brief Overload of binary `operator==`
///
/// This operator compares strict equality between two rectangles.
///
/// \param lhs Left operand (a rectangle)
/// \param rhs Right operand (a rectangle)
///
/// \return `true` if \a lhs is equal to \a rhs
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator==(const Rect<T>& lhs, const Rect<T>& rhs);

////////////////////////////////////////////////////////////
/// \relates Rect
/// \brief Overload of binary `operator!=`
///
/// This operator compares strict difference between two rectangles.
///
/// \param lhs Left operand (a rectangle)
/// \param rhs Right operand (a rectangle)
///
/// \return `true` if \a lhs is not equal to \a rhs
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator!=(const Rect<T>& lhs, const Rect<T>& rhs);

// Create type aliases for the most common types
using IntRect   = Rect<int>;
using FloatRect = Rect<float>;

} // namespace sf

#include "SFML/System/Rect.inl"


////////////////////////////////////////////////////////////
// Explicit instantiation declarations
////////////////////////////////////////////////////////////

extern template class sf::Rect<float>;
extern template class sf::Rect<double>;
extern template class sf::Rect<long double>;
extern template class sf::Rect<int>;
extern template class sf::Rect<unsigned int>;


////////////////////////////////////////////////////////////
/// \class sf::Rect
/// \ingroup graphics
///
/// A rectangle is defined by its top-left corner and its size.
/// It is a very simple class defined for convenience, so
/// its member variables (position and size) are public
/// and can be accessed directly, just like the vector classes
/// (`Vector2` and `Vector3`).
///
/// To keep things simple, `sf::Rect` doesn't define
/// functions to emulate the properties that are not directly
/// members (such as right, bottom, etc.), it rather
/// only provides intersection functions.
///
/// `sf::Rect` uses the usual rules for its boundaries:
/// \li The left and top edges are included in the rectangle's area
/// \li The right and bottom edges are excluded from the rectangle's area
///
/// This means that `sf::IntRect({0, 0}, {1, 1})` and `sf::IntRect({1, 1}, {1, 1})`
/// don't intersect.
///
/// `sf::Rect` is a template and may be used with any numeric type, but
/// for simplicity type aliases for the instantiations used by SFML are given:
/// \li `sf::Rect<int>` is `sf::IntRect`
/// \li `sf::Rect<float>` is `sf::FloatRect`
///
/// So that you don't have to care about the template syntax.
///
/// Usage example:
/// \code
/// // Define a rectangle, located at (0, 0) with a size of 20x5
/// sf::IntRect r1({0, 0}, {20, 5});
///
/// // Define another rectangle, located at (4, 2) with a size of 18x10
/// sf::Vector2i position(4, 2);
/// sf::Vector2i size(18, 10);
/// sf::IntRect r2(position, size);
///
/// // Test intersections with the point (3, 1)
/// bool b1 = r1.contains({3, 1}); // true
/// bool b2 = r2.contains({3, 1}); // false
///
/// // Test the intersection between r1 and r2
/// sf::base::Optional<sf::IntRect> result = sf::findIntersection(r1, r2);
/// // result.hasValue() == true
/// // result.value() == sf::IntRect({4, 2}, {16, 3})
/// \endcode
///
////////////////////////////////////////////////////////////
