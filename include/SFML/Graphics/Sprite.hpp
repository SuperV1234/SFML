#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/Export.hpp"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "SFML/System/Rect.hpp"


namespace sf
{
class DrawableBatch;
class RenderTarget;

////////////////////////////////////////////////////////////
/// \brief Geometry that can render a texture, with its
///        own transformations, color, etc.
///
////////////////////////////////////////////////////////////
class SFML_GRAPHICS_API Sprite : public Transformable
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct the sprite from a sub-rectangle of an eventual source texture
    ///
    /// \param rectangle Sub-rectangle of the eventual texture (specified during drawing)
    ///
    /// \see `setTextureRect`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Sprite(const IntRect& rectangle);

    ////////////////////////////////////////////////////////////
    /// \brief Set the sub-rectagle of the texture that the sprite will display
    ///
    /// \param rectangle Rectangle defining the region of the texture to display (specified during drawing)
    ///
    /// \see `getTextureRect`
    ///
    ////////////////////////////////////////////////////////////
    void setTextureRect(const IntRect& rectangle);

    ////////////////////////////////////////////////////////////
    /// \brief Set the global color of the sprite
    ///
    /// This color is modulated (multiplied) with the sprite's
    /// texture. It can be used to colorize the sprite, or change
    /// its global opacity.
    /// By default, the sprite's color is opaque white.
    ///
    /// \param color New color of the sprite
    ///
    /// \see `getColor`
    ///
    ////////////////////////////////////////////////////////////
    void setColor(Color color);

    ////////////////////////////////////////////////////////////
    /// \brief Get the sub-rectangle of the texture displayed by the sprite
    ///
    /// \return Texture rectangle of the sprite
    ///
    /// \see `setTextureRect`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const IntRect& getTextureRect() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the global color of the sprite
    ///
    /// \return Global color of the sprite
    ///
    /// \see `setColor`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Color getColor() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the local bounding rectangle of the entity
    ///
    /// The returned rectangle is in local coordinates, which means
    /// that it ignores the transformations (translation, rotation,
    /// scale, ...) that are applied to the entity.
    /// In other words, this function returns the bounds of the
    /// entity in the entity's coordinate system.
    ///
    /// \return Local bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] FloatRect getLocalBounds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the global bounding rectangle of the entity
    ///
    /// The returned rectangle is in global coordinates, which means
    /// that it takes into account the transformations (translation,
    /// rotation, scale, ...) that are applied to the entity.
    /// In other words, this function returns the bounds of the
    /// sprite in the global 2D world's coordinate system.
    ///
    /// \return Global bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] FloatRect getGlobalBounds() const;

private:
    friend DrawableBatch;
    friend RenderTarget;

    ////////////////////////////////////////////////////////////
    /// \brief Update the vertices' positions and texture coordinates
    ///
    ////////////////////////////////////////////////////////////
    void updateVertices(Vertex* target) const;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    IntRect m_textureRect;         //!< Rectangle defining the area of the source texture to display
    Color   m_color{Color::White}; //!< TODO P0:
};

} // namespace sf


////////////////////////////////////////////////////////////
/// \class sf::Sprite
/// \ingroup graphics
///
/// `sf::Sprite` is a drawable class that allows to easily display
/// a texture (or a part of it) on a render target.
///
/// It inherits all the functions from `sf::Transformable`:
/// position, rotation, scale, origin. It also adds sprite-specific
/// properties such as the texture to use, the part of it to display,
/// and some convenience functions to change the overall color of the
/// sprite, or to get its bounding rectangle.
///
/// `sf::Sprite` works in combination with the `sf::Texture` class, which
/// loads and provides the pixel data of a given texture.
///
/// The separation of `sf::Sprite` and `sf::Texture` allows more flexibility
/// and better performances: indeed a `sf::Texture` is a heavy resource,
/// and any operation on it is slow (often too slow for real-time
/// applications). On the other side, a `sf::Sprite` is a lightweight
/// object which can use the pixel data of a `sf::Texture` and draw
/// it with its own transformation/color/blending attributes.
///
/// It is important to note that the `sf::Sprite` instance doesn't
/// even keep a reference to the `sf::Texture` it uses, the texture
/// must be provided prior to drawing the sprite via `sf::RenderStates`.
///
/// See also the note on coordinates and undistorted rendering in `sf::Transformable`.
///
/// Usage example:
/// \code
/// // Load a texture
/// const auto texture = sf::Texture::loadFromFile("texture.png").value();
///
/// // Create a sprite
/// sprite.setTextureRect({{10, 10}, {50, 30}});
/// sprite.setColor({255, 255, 255, 200});
/// sprite.setPosition({100.f, 25.f});
///
/// // Draw it
/// window.draw(sprite, texture);
/// \endcode
///
/// \see `sf::Texture`, `sf::Transformable`
///
////////////////////////////////////////////////////////////
