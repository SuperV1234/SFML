#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/PrimitiveType.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Shape.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "SFML/System/Vector2.hpp"

#include <vector>

#include <cstddef>


namespace
{
////////////////////////////////////////////////////////////
// Compute the normal of a segment
[[nodiscard]] sf::Vector2f computeNormal(sf::Vector2f p1, sf::Vector2f p2)
{
    sf::Vector2f normal = (p2 - p1).perpendicular();
    const float  length = normal.length();
    if (length != 0.f)
        normal /= length;
    return normal;
}

////////////////////////////////////////////////////////////
// Get bounds of a vertex range
[[nodiscard]] sf::FloatRect getVertexRangeBounds(const std::vector<sf::Vertex>& data)
{
    if (data.empty())
    {
        return {};
    }

    float left   = data[0].position.x;
    float top    = data[0].position.y;
    float right  = data[0].position.x;
    float bottom = data[0].position.y;

    for (std::size_t i = 1; i < data.size(); ++i)
    {
        const sf::Vector2f position = data[i].position;

        // Update left and right
        if (position.x < left)
            left = position.x;
        else if (position.x > right)
            right = position.x;

        // Update top and bottom
        if (position.y < top)
            top = position.y;
        else if (position.y > bottom)
            bottom = position.y;
    }

    return {{left, top}, {right - left, bottom - top}};
}

} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
struct Shape::Impl
{
    IntRect textureRect;             //!< Rectangle defining the area of the source texture to display for the fill
    IntRect outlineTextureRect;      //!< Rectangle defining the area of the source texture to display for the outline
    Color   fillColor{Color::White}; //!< Fill color
    Color   outlineColor{Color::White};  //!< Outline color
    float   outlineThickness{};          //!< Thickness of the shape's outline
    std::vector<Vertex> vertices;        //!< Vertex array containing the fill geometry
    std::vector<Vertex> outlineVertices; //!< Vertex array containing the outline geometry
    FloatRect           insideBounds;    //!< Bounding rectangle of the inside (fill)
    FloatRect           bounds;          //!< Bounding rectangle of the whole shape (outline + fill)
};


////////////////////////////////////////////////////////////
Shape::Shape() = default;


////////////////////////////////////////////////////////////
Shape::~Shape() = default;


////////////////////////////////////////////////////////////
Shape::Shape(const Shape& rhs) = default;


////////////////////////////////////////////////////////////
Shape& Shape::operator=(const Shape&) = default;


////////////////////////////////////////////////////////////
Shape::Shape(Shape&&) noexcept = default;


////////////////////////////////////////////////////////////
Shape& Shape::operator=(Shape&&) noexcept = default;


////////////////////////////////////////////////////////////
void Shape::setTextureRect(const IntRect& rect)
{
    m_impl->textureRect = rect;
    updateTexCoords();
}


////////////////////////////////////////////////////////////
void Shape::setOutlineTextureRect(const IntRect& rect)
{
    m_impl->outlineTextureRect = rect;
    updateOutlineTexCoords();
}


////////////////////////////////////////////////////////////
const IntRect& Shape::getTextureRect() const
{
    return m_impl->textureRect;
}


////////////////////////////////////////////////////////////
const IntRect& Shape::getOutlineTextureRect() const
{
    return m_impl->outlineTextureRect;
}

////////////////////////////////////////////////////////////
void Shape::setFillColor(Color color)
{
    m_impl->fillColor = color;
    updateFillColors();
}


////////////////////////////////////////////////////////////
Color Shape::getFillColor() const
{
    return m_impl->fillColor;
}


////////////////////////////////////////////////////////////
void Shape::setOutlineColor(Color color)
{
    m_impl->outlineColor = color;
    updateOutlineColors();
}


////////////////////////////////////////////////////////////
Color Shape::getOutlineColor() const
{
    return m_impl->outlineColor;
}


////////////////////////////////////////////////////////////
void Shape::setOutlineThickness(float thickness)
{
    m_impl->outlineThickness = thickness;

    const std::size_t pointCount = m_impl->vertices.size() - 2;

    std::vector<Vector2f> points;
    points.reserve(pointCount);

    for (std::size_t i = 0; i < pointCount; ++i)
        points.push_back(m_impl->vertices[i + 1].position);

    update(points.data(), pointCount); // recompute everything because the whole shape must be offset
}


////////////////////////////////////////////////////////////
float Shape::getOutlineThickness() const
{
    return m_impl->outlineThickness;
}


////////////////////////////////////////////////////////////
const FloatRect& Shape::getLocalBounds() const
{
    return m_impl->bounds;
}


////////////////////////////////////////////////////////////
FloatRect Shape::getGlobalBounds() const
{
    return getTransform().transformRect(getLocalBounds());
}


////////////////////////////////////////////////////////////
[[nodiscard]] base::Span<const Vertex> Shape::getFillVertices() const
{
    return {m_impl->vertices.data(), m_impl->vertices.size()};
}


////////////////////////////////////////////////////////////
[[nodiscard]] base::Span<const Vertex> Shape::getOutlineVertices() const
{
    return {m_impl->outlineVertices.data(), m_impl->outlineVertices.size()};
}


////////////////////////////////////////////////////////////
void Shape::update(const sf::Vector2f* points, const std::size_t pointCount)
{
    // Get the total number of points of the shape
    if (pointCount < 3)
    {
        m_impl->vertices.resize(0);
        m_impl->outlineVertices.resize(0);
        return;
    }

    m_impl->vertices.resize(pointCount + 2); // + 2 for center and repeated first point

    // Position
    for (std::size_t i = 0; i < pointCount; ++i)
        m_impl->vertices[i + 1].position = points[i];

    m_impl->vertices[pointCount + 1].position = m_impl->vertices[1].position;

    // Update the bounding rectangle
    m_impl->vertices[0]  = m_impl->vertices[1]; // so that the result of getBounds() is correct
    m_impl->insideBounds = getVertexRangeBounds(m_impl->vertices);

    // Compute the center and make it the first vertex
    m_impl->vertices[0].position = m_impl->insideBounds.getCenter();

    // Color
    updateFillColors();

    // Texture coordinates
    updateTexCoords();

    // Outline
    updateOutline();
}


////////////////////////////////////////////////////////////
void Shape::drawOnto(RenderTarget& renderTarget, const Texture* texture, RenderStates states) const
{
    states.transform *= getTransform();
    states.coordinateType = CoordinateType::Pixels;
    states.texture        = texture;

    // Render the inside
    renderTarget.draw(m_impl->vertices, PrimitiveType::TriangleFan, states);

    // Render the outline
    if (m_impl->outlineThickness != 0)
        renderTarget.draw(m_impl->outlineVertices, PrimitiveType::TriangleStrip, states);
}


////////////////////////////////////////////////////////////
void Shape::updateFillColors()
{
    for (Vertex& vertex : m_impl->vertices)
        vertex.color = m_impl->fillColor;
}


////////////////////////////////////////////////////////////
void Shape::updateTexCoords()
{
    const auto convertedTextureRect = m_impl->textureRect.to<FloatRect>();

    // Make sure not to divide by zero when the points are aligned on a vertical or horizontal line
    const Vector2f safeInsideSize(m_impl->insideBounds.size.x > 0 ? m_impl->insideBounds.size.x : 1.f,
                                  m_impl->insideBounds.size.y > 0 ? m_impl->insideBounds.size.y : 1.f);

    for (Vertex& vertex : m_impl->vertices)
    {
        const Vector2f ratio = (vertex.position - m_impl->insideBounds.position).cwiseDiv(safeInsideSize);
        vertex.texCoords     = convertedTextureRect.position + convertedTextureRect.size.cwiseMul(ratio);
    }
}


////////////////////////////////////////////////////////////
void Shape::updateOutlineTexCoords()
{
    const auto convertedTextureRect = m_impl->outlineTextureRect.to<FloatRect>();

    // Make sure not to divide by zero when the points are aligned on a vertical or horizontal line
    const Vector2f safeInsideSize(m_impl->bounds.size.x > 0 ? m_impl->bounds.size.x : 1.f,
                                  m_impl->bounds.size.y > 0 ? m_impl->bounds.size.y : 1.f);

    for (Vertex& vertex : m_impl->outlineVertices)
    {
        const Vector2f ratio = (vertex.position - m_impl->bounds.position).cwiseDiv(safeInsideSize);
        vertex.texCoords     = convertedTextureRect.position + convertedTextureRect.size.cwiseMul(ratio);
    }
}


////////////////////////////////////////////////////////////
void Shape::updateOutline()
{
    // Return if there is no outline
    if (m_impl->outlineThickness == 0.f)
    {
        m_impl->outlineVertices.clear();
        m_impl->bounds = m_impl->insideBounds;
        return;
    }

    const std::size_t count = m_impl->vertices.size() - 2;
    m_impl->outlineVertices.resize((count + 1) * 2);

    for (std::size_t i = 0; i < count; ++i)
    {
        const std::size_t index = i + 1;

        // Get the two segments shared by the current point
        const Vector2f p0 = (i == 0) ? m_impl->vertices[count].position : m_impl->vertices[index - 1].position;
        const Vector2f p1 = m_impl->vertices[index].position;
        const Vector2f p2 = m_impl->vertices[index + 1].position;

        // Compute their normal
        Vector2f n1 = computeNormal(p0, p1);
        Vector2f n2 = computeNormal(p1, p2);

        // Make sure that the normals point towards the outside of the shape
        // (this depends on the order in which the points were defined)
        if (n1.dot(m_impl->vertices[0].position - p1) > 0)
            n1 = -n1;
        if (n2.dot(m_impl->vertices[0].position - p1) > 0)
            n2 = -n2;

        // Combine them to get the extrusion direction
        const float    factor = 1.f + (n1.x * n2.x + n1.y * n2.y);
        const Vector2f normal = (n1 + n2) / factor;

        // Update the outline points
        m_impl->outlineVertices[i * 2 + 0].position = p1;
        m_impl->outlineVertices[i * 2 + 1].position = p1 + normal * m_impl->outlineThickness;
    }

    // Duplicate the first point at the end, to close the outline
    m_impl->outlineVertices[count * 2 + 0].position = m_impl->outlineVertices[0].position;
    m_impl->outlineVertices[count * 2 + 1].position = m_impl->outlineVertices[1].position;

    // Update outline colors
    updateOutlineColors();

    // Update the shape's bounds
    m_impl->bounds = getVertexRangeBounds(m_impl->outlineVertices);
}


////////////////////////////////////////////////////////////
void Shape::updateOutlineColors()
{
    for (Vertex& outlineVertex : m_impl->outlineVertices)
        outlineVertex.color = m_impl->outlineColor;
}

} // namespace sf
