#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/View.hpp"

#include "SFML/Base/Assert.hpp"


namespace sf
{
////////////////////////////////////////////////////////////
View::View(const FloatRect& rectangle) : m_center(rectangle.getCenter()), m_size(rectangle.size)
{
}


////////////////////////////////////////////////////////////
View::View(Vector2f center, Vector2f size) : m_center(center), m_size(size)
{
}


////////////////////////////////////////////////////////////
void View::setCenter(Vector2f center)
{
    m_center = center;
}


////////////////////////////////////////////////////////////
void View::setSize(Vector2f size)
{
    m_size = size;
}


////////////////////////////////////////////////////////////
void View::setRotation(Angle angle)
{
    m_rotation = angle.wrapUnsigned();
}


////////////////////////////////////////////////////////////
void View::setViewport(const FloatRect& viewport)
{
    m_viewport = viewport;
}


////////////////////////////////////////////////////////////
void View::setScissor(const FloatRect& scissor)
{
    SFML_BASE_ASSERT(scissor.position.x >= 0.0f && scissor.position.x <= 1.0f &&
                     "scissor.position.x must lie within [0, 1]");
    SFML_BASE_ASSERT(scissor.position.y >= 0.0f && scissor.position.y <= 1.0f &&
                     "scissor.position.y must lie within [0, 1]");
    SFML_BASE_ASSERT(scissor.size.x >= 0.0f && "scissor.size.x must lie within [0, 1]");
    SFML_BASE_ASSERT(scissor.size.y >= 0.0f && "scissor.size.y must lie within [0, 1]");
    SFML_BASE_ASSERT(scissor.position.x + scissor.size.x <= 1.0f &&
                     "scissor.position.x + scissor.size.x must lie within [0, 1]");
    SFML_BASE_ASSERT(scissor.position.y + scissor.size.y <= 1.0f &&
                     "scissor.position.y + scissor.size.y must lie within [0, 1]");

    m_scissor = scissor;
}


////////////////////////////////////////////////////////////
Vector2f View::getCenter() const
{
    return m_center;
}


////////////////////////////////////////////////////////////
Vector2f View::getSize() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
Angle View::getRotation() const
{
    return m_rotation;
}


////////////////////////////////////////////////////////////
const FloatRect& View::getViewport() const
{
    return m_viewport;
}


////////////////////////////////////////////////////////////
const FloatRect& View::getScissor() const
{
    return m_scissor;
}


////////////////////////////////////////////////////////////
void View::move(Vector2f offset)
{
    setCenter(m_center + offset);
}


////////////////////////////////////////////////////////////
void View::rotate(Angle angle)
{
    setRotation(m_rotation + angle);
}


////////////////////////////////////////////////////////////
void View::zoom(float factor)
{
    setSize(m_size * factor);
}


////////////////////////////////////////////////////////////
Transform View::getTransform() const
{
    // Rotation components
    const float angle  = m_rotation.asRadians();
    const float cosine = base::cos(angle);
    const float sine   = base::sin(angle);
    const float tx     = -m_center.x * cosine - m_center.y * sine + m_center.x;
    const float ty     = m_center.x * sine - m_center.y * cosine + m_center.y;

    // Projection components
    const float a = 2.f / m_size.x;
    const float b = -2.f / m_size.y;
    const float c = -a * m_center.x;
    const float d = -b * m_center.y;

    // Rebuild the projection matrix
    return {a * cosine, a * sine, a * tx + c, -b * sine, b * cosine, b * ty + d};
}


////////////////////////////////////////////////////////////
Transform View::getInverseTransform() const
{
    return getTransform().getInverse();
}

} // namespace sf
