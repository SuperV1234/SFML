#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/Export.hpp"

#include "SFML/System/RectPacker.hpp"
#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Optional.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace sf
{
class Texture;
} // namespace sf


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Image living on the graphics card that can be used for drawing
///
////////////////////////////////////////////////////////////
class [[nodiscard]] SFML_GRAPHICS_API TextureAtlas
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit TextureAtlas(Texture& atlasTexture);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] base::Optional<Vector2f> add(const Texture& texture);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Texture*   m_atlasTexturePtr;
    RectPacker m_rectPacker;
};


} // namespace sf


////////////////////////////////////////////////////////////
/// \class sf::TextureAtlas
/// \ingroup graphics
///
/// TODO P1: docs
///
/// \see sf::Texture, sf::Image, sf::RenderTexture
///
////////////////////////////////////////////////////////////
