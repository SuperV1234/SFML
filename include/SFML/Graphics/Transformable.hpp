#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/Export.hpp"

#include "SFML/Graphics/Transform.hpp"

#include "SFML/System/Angle.hpp"


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Decomposed transform defined by a position, a rotation and a scale
///
////////////////////////////////////////////////////////////
class SFML_GRAPHICS_API Transformable
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    ////////////////////////////////////////////////////////////
    constexpr Transformable() = default;

    ////////////////////////////////////////////////////////////
    /// \brief set the position of the object
    ///
    /// This function completely overwrites the previous position.
    /// See the move function to apply an offset based on the previous position instead.
    /// The default position of a transformable object is (0, 0).
    ///
    /// \param position New position
    ///
    /// \see `move`, `getPosition`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void setPosition(Vector2f position);

    ////////////////////////////////////////////////////////////
    /// \brief set the orientation of the object
    ///
    /// This function completely overwrites the previous rotation.
    /// See the rotate function to add an angle based on the previous rotation instead.
    /// The default rotation of a transformable object is 0.
    ///
    /// \param angle New rotation
    ///
    /// \see `rotate`, `getRotation`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void setRotation(Angle angle);

    ////////////////////////////////////////////////////////////
    /// \brief set the scale factors of the object
    ///
    /// This function completely overwrites the previous scale.
    /// See the scale function to add a factor based on the previous scale instead.
    /// The default scale of a transformable object is (1, 1).
    ///
    /// \param factors New scale factors
    ///
    /// \see `scale`, `getScale`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void setScale(Vector2f factors);

    ////////////////////////////////////////////////////////////
    /// \brief set the local origin of the object
    ///
    /// The origin of an object defines the center point for
    /// all transformations (position, scale, rotation).
    /// The coordinates of this point must be relative to the
    /// top-left corner of the object, and ignore all
    /// transformations (position, scale, rotation).
    /// The default origin of a transformable object is (0, 0).
    ///
    /// \param origin New origin
    ///
    /// \see `getOrigin`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void setOrigin(Vector2f origin);

    ////////////////////////////////////////////////////////////
    /// \brief get the position of the object
    ///
    /// \return Current position
    ///
    /// \see `setPosition`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Vector2f getPosition() const;

    ////////////////////////////////////////////////////////////
    /// \brief get the orientation of the object
    ///
    /// The rotation is always in the range [0, 360].
    ///
    /// \return Current rotation
    ///
    /// \see `setRotation`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Angle getRotation() const;

    ////////////////////////////////////////////////////////////
    /// \brief get the current scale of the object
    ///
    /// \return Current scale factors
    ///
    /// \see `setScale`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Vector2f getScale() const;

    ////////////////////////////////////////////////////////////
    /// \brief get the local origin of the object
    ///
    /// \return Current origin
    ///
    /// \see `setOrigin`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Vector2f getOrigin() const;

    ////////////////////////////////////////////////////////////
    /// \brief Move the object by a given offset
    ///
    /// This function adds to the current position of the object,
    /// unlike `setPosition` which overwrites it.
    /// Thus, it is equivalent to the following code:
    /// \code
    /// object.setPosition(object.getPosition() + offset);
    /// \endcode
    ///
    /// \param offset Offset
    ///
    /// \see `setPosition`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void move(Vector2f offset);

    ////////////////////////////////////////////////////////////
    /// \brief Rotate the object
    ///
    /// This function adds to the current rotation of the object,
    /// unlike `setRotation` which overwrites it.
    /// Thus, it is equivalent to the following code:
    /// \code
    /// object.setRotation(object.getRotation() + angle);
    /// \endcode
    ///
    /// \param angle Angle of rotation
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void rotate(Angle angle);

    ////////////////////////////////////////////////////////////
    /// \brief Scale the object
    ///
    /// This function multiplies the current scale of the object,
    /// unlike `setScale` which overwrites it.
    /// Thus, it is equivalent to the following code:
    /// \code
    /// sf::Vector2f scale = object.getScale();
    /// object.setScale(scale.x * factor.x, scale.y * factor.y);
    /// \endcode
    ///
    /// \param factor Scale factors
    ///
    /// \see `setScale`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void scale(Vector2f factor);

    ////////////////////////////////////////////////////////////
    /// \brief get the combined transform of the object
    ///
    /// \return Transform combining the position/rotation/scale/origin of the object
    ///
    /// \see `getInverseTransform`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Transform getTransform() const;

    ////////////////////////////////////////////////////////////
    /// \brief get the inverse of the combined transform of the object
    ///
    /// \return Inverse of the combined transformations applied to the object
    ///
    /// \see `getTransform`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Transform getInverseTransform() const;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vector2f m_origin;          //!< Origin of translation/rotation/scaling of the object
    Vector2f m_position;        //!< Position of the object in the 2D world
    Angle    m_rotation;        //!< Orientation of the object
    Vector2f m_scale{1.f, 1.f}; //!< Scale of the object
};

} // namespace sf

#include "SFML/Graphics/Transformable.inl"


////////////////////////////////////////////////////////////
/// \class sf::Transformable
/// \ingroup graphics
///
/// This class is provided for convenience, on top of `sf::Transform`.
///
/// `sf::Transform`, as a low-level class, offers a great level of
/// flexibility but it is not always convenient to manage. Indeed,
/// one can easily combine any kind of operation, such as a translation
/// followed by a rotation followed by a scaling, but once the result
/// transform is built, there's no way to go backward and, let's say,
/// change only the rotation without modifying the translation and scaling.
/// The entire transform must be recomputed, which means that you
/// need to retrieve the initial translation and scale factors as
/// well, and combine them the same way you did before updating the
/// rotation. This is a tedious operation, and it requires to store
/// all the individual components of the final transform.
///
/// That's exactly what `sf::Transformable` was written for: it hides
/// these variables and the composed transform behind an easy to use
/// interface. You can set or get any of the individual components
/// without worrying about the others. It also provides the composed
/// transform (as a `sf::Transform`), and keeps it up-to-date.
///
/// In addition to the position, rotation and scale, `sf::Transformable`
/// provides an "origin" component, which represents the local origin
/// of the three other components. Let's take an example with a 10x10
/// pixels sprite. By default, the sprite is positioned/rotated/scaled
/// relatively to its top-left corner, because it is the local point
/// (0, 0). But if we change the origin to be (5, 5), the sprite will
/// be positioned/rotated/scaled around its center instead. And if
/// we set the origin to (10, 10), it will be transformed around its
/// bottom-right corner.
///
/// To keep the `sf::Transformable` class simple, there's only one
/// origin for all the components. You cannot position the sprite
/// relatively to its top-left corner while rotating it around its
/// center, for example. To do such things, use `sf::Transform` directly.
///
/// `sf::Transformable` can be used as a non-polymorphic base class.
/// \code
/// class MyEntity : public sf::Transformable
/// {
///     void draw(sf::RenderTarget& target, sf::RenderStates states) const
///     {
///         states.transform *= getTransform();
///         target.draw(..., states);
///     }
/// };
///
/// MyEntity entity;
/// entity.setPosition({10, 20});
/// entity.setRotation(sf::degrees(45));
/// window.draw(entity);
/// \endcode
///
/// It can also be used as a member, if you don't want to use
/// its API directly (because you don't need all its functions,
/// or you have different naming conventions for example).
/// \code
/// class MyEntity
/// {
/// public:
///     void SetPosition(const MyVector& v)
///     {
///         myTransform.setPosition(v.x(), v.y());
///     }
///
///     void draw(sf::RenderTarget& target) const
///     {
///         target.draw(..., myTransform.getTransform());
///     }
///
/// private:
///     sf::Transformable myTransform;
/// };
/// \endcode
///
/// A note on coordinates and undistorted rendering: \n
/// By default, SFML (or more exactly, OpenGL) may interpolate drawable objects
/// such as sprites or texts when rendering. While this allows transitions
/// like slow movements or rotations to appear smoothly, it can lead to
/// unwanted results in some cases, for example blurred or distorted objects.
/// In order to render a `sf::Drawable` object pixel-perfectly, make sure
/// the involved coordinates allow a 1:1 mapping of pixels in the window
/// to texels (pixels in the texture). More specifically, this means:
/// * The object's position, origin and scale have no fractional part
/// * The object's and the view's rotation are a multiple of 90 degrees
/// * The view's center and size have no fractional part
///
/// \see `sf::Transform`
///
////////////////////////////////////////////////////////////
