#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/BlendMode.hpp"
#include "SFML/Graphics/CoordinateType.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/PrimitiveType.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Shader.hpp"
#include "SFML/Graphics/Shape.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/StencilMode.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/Transform.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexBuffer.hpp"
#include "SFML/Graphics/View.hpp"

#include "SFML/Window/GLCheck.hpp"
#include "SFML/Window/GLExtensions.hpp"

#include "SFML/System/Err.hpp"
#include "SFML/System/Rect.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Math/Lround.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/TrivialVector.hpp"

#include <atomic>

#include <cstddef>
#include <cstdint>


namespace
{
// A nested named namespace is used here to allow unity builds of SFML.
namespace RenderTargetImpl
{
////////////////////////////////////////////////////////////
// Type alias for a render target or context id
using IdType = std::uint64_t;


////////////////////////////////////////////////////////////
// Unique identifier, used for identifying RenderTargets when
// tracking the currently active RenderTarget within a given context
constinit std::atomic<IdType> nextUniqueId{1ul};


////////////////////////////////////////////////////////////
// Invalid/null render target or context id value
constexpr IdType invalidId{0ul};


////////////////////////////////////////////////////////////
// Maximum supported number of render targets or contexts
constexpr std::size_t maxIdCount{256ul};


////////////////////////////////////////////////////////////
// Map to help us detect whether a different RenderTarget has been activated within a single context
constinit std::atomic<IdType> contextRenderTargetMap[maxIdCount]{};


////////////////////////////////////////////////////////////
// Check if a render target with the given ID is active in the current context
[[nodiscard]] bool isActive(sf::GraphicsContext& graphicsContext, IdType id)
{
    const IdType contextId = graphicsContext.getActiveThreadLocalGlContextId();
    SFML_BASE_ASSERT(contextId < maxIdCount);

    const auto renderTargetId = contextRenderTargetMap[contextId].load();
    return (renderTargetId != invalidId) && (renderTargetId == id);
}


////////////////////////////////////////////////////////////
// Convert an sf::BlendMode::Factor constant to the corresponding OpenGL constant.
[[nodiscard]] std::uint32_t factorToGlConstant(sf::BlendMode::Factor blendFactor)
{
    // clang-format off
    switch (blendFactor)
    {
        case sf::BlendMode::Factor::Zero:             return GL_ZERO;
        case sf::BlendMode::Factor::One:              return GL_ONE;
        case sf::BlendMode::Factor::SrcColor:         return GL_SRC_COLOR;
        case sf::BlendMode::Factor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
        case sf::BlendMode::Factor::DstColor:         return GL_DST_COLOR;
        case sf::BlendMode::Factor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
        case sf::BlendMode::Factor::SrcAlpha:         return GL_SRC_ALPHA;
        case sf::BlendMode::Factor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case sf::BlendMode::Factor::DstAlpha:         return GL_DST_ALPHA;
        case sf::BlendMode::Factor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
    }
    // clang-format on

    sf::priv::err() << "Invalid value for sf::BlendMode::Factor! Fallback to sf::BlendMode::Factor::Zero.";
    SFML_BASE_ASSERT(false);
    return GL_ZERO;
}


////////////////////////////////////////////////////////////
// Convert an sf::BlendMode::Equation constant to the corresponding OpenGL constant.
[[nodiscard]] std::uint32_t equationToGlConstant(sf::BlendMode::Equation blendEquation)
{
    switch (blendEquation)
    {
        case sf::BlendMode::Equation::Add:
            return GLEXT_GL_FUNC_ADD;
        case sf::BlendMode::Equation::Subtract:
            if (GLEXT_blend_subtract)
                return GLEXT_GL_FUNC_SUBTRACT;
            break;
        case sf::BlendMode::Equation::ReverseSubtract:
            if (GLEXT_blend_subtract)
                return GLEXT_GL_FUNC_REVERSE_SUBTRACT;
            break;
        case sf::BlendMode::Equation::Min:
            if (GLEXT_blend_minmax)
                return GLEXT_GL_MIN;
            break;
        case sf::BlendMode::Equation::Max:
            if (GLEXT_blend_minmax)
                return GLEXT_GL_MAX;
            break;
    }

    static bool warned = false;
    if (!warned)
    {
        sf::priv::err() << "OpenGL extension EXT_blend_minmax or EXT_blend_subtract unavailable" << '\n'
                        << "Some blending equations will fallback to sf::BlendMode::Equation::Add" << '\n'
                        << "Ensure that hardware acceleration is enabled if available";

        warned = true;
    }

    return GLEXT_GL_FUNC_ADD;
}


////////////////////////////////////////////////////////////
// Convert an UpdateOperation constant to the corresponding OpenGL constant.
[[nodiscard]] std::uint32_t stencilOperationToGlConstant(sf::StencilUpdateOperation operation)
{
    // clang-format off
    switch (operation)
    {
        case sf::StencilUpdateOperation::Keep:      return GL_KEEP;
        case sf::StencilUpdateOperation::Zero:      return GL_ZERO;
        case sf::StencilUpdateOperation::Replace:   return GL_REPLACE;
        case sf::StencilUpdateOperation::Increment: return GL_INCR;
        case sf::StencilUpdateOperation::Decrement: return GL_DECR;
        case sf::StencilUpdateOperation::Invert:    return GL_INVERT;
    }
    // clang-format on

    sf::priv::err() << "Invalid value for sf::StencilUpdateOperation! Fallback to sf::StencilMode::Keep.";
    SFML_BASE_ASSERT(false);
    return GL_KEEP;
}


////////////////////////////////////////////////////////////
// Convert a Comparison constant to the corresponding OpenGL constant.
[[nodiscard]] std::uint32_t stencilFunctionToGlConstant(sf::StencilComparison comparison)
{
    // clang-format off
    switch (comparison)
    {
        case sf::StencilComparison::Never:        return GL_NEVER;
        case sf::StencilComparison::Less:         return GL_LESS;
        case sf::StencilComparison::LessEqual:    return GL_LEQUAL;
        case sf::StencilComparison::Greater:      return GL_GREATER;
        case sf::StencilComparison::GreaterEqual: return GL_GEQUAL;
        case sf::StencilComparison::Equal:        return GL_EQUAL;
        case sf::StencilComparison::NotEqual:     return GL_NOTEQUAL;
        case sf::StencilComparison::Always:       return GL_ALWAYS;
    }
    // clang-format on

    sf::priv::err() << "Invalid value for sf::StencilComparison! Fallback to sf::StencilMode::Always.";
    SFML_BASE_ASSERT(false);
    return GL_ALWAYS;
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] constexpr GLenum primitiveTypeToOpenGLMode(sf::PrimitiveType type)
{
    constexpr GLenum modes[]{GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN};
    return modes[static_cast<std::size_t>(type)];
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] sf::IntRect getMultipliedBySizeAndRoundedRect(sf::Vector2u         renderTargetSize,
                                                                                const sf::FloatRect& inputRect)
{
    const auto [width, height] = renderTargetSize.to<sf::Vector2f>();

    return sf::Rect<long>({sf::base::lround(width * inputRect.position.x), sf::base::lround(height * inputRect.position.y)},
                          {sf::base::lround(width * inputRect.size.x), sf::base::lround(height * inputRect.size.y)})
        .to<sf::IntRect>();
}

} // namespace RenderTargetImpl
} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Render states cache
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] StatesCache
{
    bool enable{};      //!< Is the cache enabled?
    bool glStatesSet{}; //!< Are our internal GL states set yet?

    bool viewChanged{}; //!< Has the current view changed since last draw?

    bool scissorEnabled{}; //!< Is scissor testing enabled?
    bool stencilEnabled{}; //!< Is stencil testing enabled?

    BlendMode      lastBlendMode;        //!< Cached blending mode
    StencilMode    lastStencilMode;      //!< Cached stencil
    std::uint64_t  lastTextureId{};      //!< Cached texture
    CoordinateType lastCoordinateType{}; //!< Texture coordinate type

    GLuint lastUsedProgramId{}; //!< GL id of the last used shader program

    GLint sfAttribPositionIdx{}; //!< Index of the "sf_a_position" attribute
    GLint sfAttribColorIdx{};    //!< Index of the "sf_a_color" attribute
    GLint sfAttribTexCoordIdx{}; //!< Index of the "sf_a_texCoord" attribute

    base::Optional<Shader::UniformLocation> ulTextureMatrix;             //!< Built-in texture matrix uniform location
    base::Optional<Shader::UniformLocation> ulModelViewProjectionMatrix; //!< Built-in model-view-projection matrix uniform location
};


////////////////////////////////////////////////////////////
template <auto FnGen, auto FnBind, auto FnGet, auto FnDelete>
class OpenGLRAII
{
public:
    [[nodiscard, gnu::always_inline]] explicit OpenGLRAII(GraphicsContext&)
    {
        SFML_BASE_ASSERT(m_id == 0u);
        FnGen(m_id);
        SFML_BASE_ASSERT(m_id != 0u);
    }

    [[nodiscard, gnu::always_inline]] bool isBound() const
    {
        int out{};
        FnGet(out);
        return out != 0u;
    }

    [[gnu::always_inline]] void bind() const
    {
        SFML_BASE_ASSERT(m_id != 0u);
        FnBind(m_id);

        SFML_BASE_ASSERT(isBound());
    }

    [[gnu::always_inline]] ~OpenGLRAII()
    {
        if (m_id != 0u)
            FnDelete(m_id);
    }

    OpenGLRAII(const OpenGLRAII&)            = delete;
    OpenGLRAII& operator=(const OpenGLRAII&) = delete;

    [[gnu::always_inline]] OpenGLRAII(OpenGLRAII&& rhs) noexcept : m_id(base::exchange(rhs.m_id, 0u))
    {
    }

    [[gnu::always_inline]] OpenGLRAII& operator=(OpenGLRAII&& rhs) noexcept
    {
        if (&rhs == this)
            return *this;

        m_id = base::exchange(rhs.m_id, 0u);
        return *this;
    }

private:
    unsigned int m_id{};
};


////////////////////////////////////////////////////////////
using VAO = OpenGLRAII<[](auto& id) { glCheck(glGenVertexArrays(1, &id)); },
                       [](auto id) { glCheck(glBindVertexArray(id)); },
                       [](auto& id) { glCheck(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &id)); },
                       [](auto& id) { glCheck(glDeleteVertexArrays(1, &id)); }>;


////////////////////////////////////////////////////////////
using VBO = OpenGLRAII<[](auto& id) { glCheck(glGenBuffers(1, &id)); },
                       [](auto id) { glCheck(glBindBuffer(GL_ARRAY_BUFFER, id)); },
                       [](auto& id) { glCheck(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &id)); },
                       [](auto& id) { glCheck(glDeleteBuffers(1, &id)); }>;


////////////////////////////////////////////////////////////
using EBO = OpenGLRAII<[](auto& id) { glCheck(glGenBuffers(1, &id)); },
                       [](auto id) { glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id)); },
                       [](auto& id) { glCheck(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &id)); },
                       [](auto& id) { glCheck(glDeleteBuffers(1, &id)); }>;


////////////////////////////////////////////////////////////
void setupVertexAttribPointers(const GLint sfAttribPositionIdx, const GLint sfAttribColorIdx, const GLint sfAttribTexCoordIdx)
{
#define SFML_PRIV_OFFSETOF(...) reinterpret_cast<const void*>(offsetof(__VA_ARGS__))

    SFML_BASE_ASSERT(sfAttribPositionIdx >= 0);

    glCheck(glEnableVertexAttribArray(static_cast<GLuint>(sfAttribPositionIdx)));
    glCheck(glVertexAttribPointer(/*      index */ static_cast<GLuint>(sfAttribPositionIdx),
                                  /*       size */ 2,
                                  /*       type */ GL_FLOAT,
                                  /* normalized */ GL_FALSE,
                                  /*     stride */ sizeof(Vertex),
                                  /*     offset */ SFML_PRIV_OFFSETOF(Vertex, position)));

    if (sfAttribColorIdx >= 0)
    {
        glCheck(glEnableVertexAttribArray(static_cast<GLuint>(sfAttribColorIdx)));
        glCheck(glVertexAttribPointer(/*      index */ static_cast<GLuint>(sfAttribColorIdx),
                                      /*       size */ 4,
                                      /*       type */ GL_UNSIGNED_BYTE,
                                      /* normalized */ GL_TRUE,
                                      /*     stride */ sizeof(Vertex),
                                      /*     offset */ SFML_PRIV_OFFSETOF(Vertex, color)));
    }

    if (sfAttribTexCoordIdx >= 0)
    {
        glCheck(glEnableVertexAttribArray(static_cast<GLuint>(sfAttribTexCoordIdx)));
        glCheck(glVertexAttribPointer(/*      index */ static_cast<GLuint>(sfAttribTexCoordIdx),
                                      /*       size */ 2,
                                      /*       type */ GL_FLOAT,
                                      /* normalized */ GL_FALSE,
                                      /*     stride */ sizeof(Vertex),
                                      /*     offset */ SFML_PRIV_OFFSETOF(Vertex, texCoords)));
    }

#undef SFML_PRIV_OFFSETOF
}


////////////////////////////////////////////////////////////
struct RenderTarget::Impl
{
    explicit Impl(GraphicsContext& theGraphicsContext) :
    graphicsContext(&theGraphicsContext),
    vao(theGraphicsContext),
    vbo(theGraphicsContext),
    ebo(theGraphicsContext)
    {
    }

    GraphicsContext* graphicsContext; //!< The window context

    View defaultView; //!< Default view
    View view;        //!< Current view

    StatesCache cache{}; //!< Render states cache

    RenderTargetImpl::IdType id{}; //!< Unique number that identifies the render target

    VAO vao; //!< Vertex array object associated with the render target
    VBO vbo; //!< Vertex buffer object associated with the render target
    EBO ebo; //!< Element index buffer object associated with the render target

    base::TrivialVector<Vertex>         batchVertexCache; //!< TODO P0:
    base::TrivialVector<unsigned short> batchIndexCache;  //!< TODO P0:
};


////////////////////////////////////////////////////////////
RenderTarget::RenderTarget(GraphicsContext& graphicsContext) : m_impl(graphicsContext)
{
}


////////////////////////////////////////////////////////////
RenderTarget::~RenderTarget() = default;


////////////////////////////////////////////////////////////
RenderTarget::RenderTarget(RenderTarget&&) noexcept = default;


////////////////////////////////////////////////////////////
RenderTarget& RenderTarget::operator=(RenderTarget&&) noexcept = default;


////////////////////////////////////////////////////////////
[[nodiscard]] bool RenderTarget::clearImpl()
{
    if (!RenderTargetImpl::isActive(*m_impl->graphicsContext, m_impl->id) && !setActive(true))
    {
        priv::err() << "Failed to activate render target in `clearImpl`";
        return false;
    }

    // Unbind texture to fix RenderTexture preventing clear
    unapplyTexture();

    // Apply the view (scissor testing can affect clearing)
    if (!m_impl->cache.enable || m_impl->cache.viewChanged)
        applyCurrentView();

    return true;
}


////////////////////////////////////////////////////////////
void RenderTarget::clear(Color color)
{
    if (!clearImpl())
        return;

    glCheck(glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));
}


////////////////////////////////////////////////////////////
void RenderTarget::clearStencil(StencilValue stencilValue)
{
    if (!clearImpl())
        return;

    glCheck(glClearStencil(static_cast<int>(stencilValue.value)));
    glCheck(glClear(GL_STENCIL_BUFFER_BIT));
}


////////////////////////////////////////////////////////////
void RenderTarget::clear(Color color, StencilValue stencilValue)
{
    if (!clearImpl())
        return;

    glCheck(glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f));
    glCheck(glClearStencil(static_cast<int>(stencilValue.value)));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}


////////////////////////////////////////////////////////////
void RenderTarget::setView(const View& view)
{
    m_impl->view              = view;
    m_impl->cache.viewChanged = true;
}


////////////////////////////////////////////////////////////
const View& RenderTarget::getView() const
{
    return m_impl->view;
}


////////////////////////////////////////////////////////////
const View& RenderTarget::getDefaultView() const
{
    return m_impl->defaultView;
}


////////////////////////////////////////////////////////////
IntRect RenderTarget::getViewport(const View& view) const
{
    return RenderTargetImpl::getMultipliedBySizeAndRoundedRect(getSize(), view.getViewport());
}


////////////////////////////////////////////////////////////
IntRect RenderTarget::getScissor(const View& view) const
{
    return RenderTargetImpl::getMultipliedBySizeAndRoundedRect(getSize(), view.getScissor());
}


////////////////////////////////////////////////////////////
Vector2f RenderTarget::mapPixelToCoords(Vector2i point) const
{
    return mapPixelToCoords(point, getView());
}


////////////////////////////////////////////////////////////
Vector2f RenderTarget::mapPixelToCoords(Vector2i point, const View& view) const
{
    // First, convert from viewport coordinates to homogeneous coordinates
    const auto     viewport   = getViewport(view).to<FloatRect>();
    const Vector2f normalized = Vector2f(-1.f, 1.f) +
                                Vector2f(2.f, -2.f).cwiseMul(point.to<Vector2f>() - viewport.position).cwiseDiv(viewport.size);

    // Then transform by the inverse of the view matrix
    return view.getInverseTransform().transformPoint(normalized);
}


////////////////////////////////////////////////////////////
Vector2i RenderTarget::mapCoordsToPixel(Vector2f point) const
{
    return mapCoordsToPixel(point, getView());
}


////////////////////////////////////////////////////////////
Vector2i RenderTarget::mapCoordsToPixel(Vector2f point, const View& view) const
{
    // First, transform the point by the view matrix
    const Vector2f normalized = view.getTransform().transformPoint(point);

    // Then convert to viewport coordinates
    const auto viewport = getViewport(view).to<FloatRect>();
    return ((normalized.cwiseMul({1.f, -1.f}) + sf::Vector2f{1.f, 1.f}).cwiseDiv({2.f, 2.f}).cwiseMul(viewport.size) +
            viewport.position)
        .to<Vector2i>();
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Sprite& sprite, const Texture& texture, RenderStates states)
{
    states.texture = &texture;
    states.transform *= sprite.getTransform();
    states.coordinateType = CoordinateType::Pixels;

    // TODO P1: consider making vertices here on demand? Also better for batching I guess. But need to pretransform?
    draw(sprite.m_vertices, PrimitiveType::TriangleStrip, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Shape& shape, const Texture* texture, const RenderStates& states)
{
    shape.drawOnto(*this, texture, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Vertex* vertices, std::size_t vertexCount, PrimitiveType type, const RenderStates& states)
{
    // Nothing to draw or inactive target
    if (vertices == nullptr || vertexCount == 0u ||
        (!RenderTargetImpl::isActive(*m_impl->graphicsContext, m_impl->id) && !setActive(true)))
        return;

    setupDraw(states);

    glCheck(glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Vertex) * vertexCount), vertices, GL_STATIC_DRAW));

    setupVertexAttribPointers(m_impl->cache.sfAttribPositionIdx,
                              m_impl->cache.sfAttribColorIdx,
                              m_impl->cache.sfAttribTexCoordIdx);

    drawPrimitives(type, 0u, vertexCount);
    cleanupDraw(states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawIndexedVertices(
    const Vertex*         vertices,
    std::size_t           vertexCount,
    const unsigned short* indices,
    std::size_t           indexCount,
    PrimitiveType         type,
    const RenderStates&   states)
{
    // Nothing to draw or inactive target
    if (vertices == nullptr || vertexCount == 0u || indices == nullptr || indexCount == 0u ||
        (!RenderTargetImpl::isActive(*m_impl->graphicsContext, m_impl->id) && !setActive(true)))
        return;

    setupDraw(states);

    glCheck(glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Vertex) * vertexCount), vertices, GL_STATIC_DRAW));

    glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(sizeof(unsigned short) * indexCount),
                         indices,
                         GL_STATIC_DRAW));

    setupVertexAttribPointers(m_impl->cache.sfAttribPositionIdx,
                              m_impl->cache.sfAttribColorIdx,
                              m_impl->cache.sfAttribTexCoordIdx);

    drawIndexedPrimitives(type, indexCount);
    cleanupDraw(states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const VertexBuffer& vertexBuffer, const RenderStates& states)
{
    draw(vertexBuffer, 0, vertexBuffer.getVertexCount(), states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const VertexBuffer& vertexBuffer, std::size_t firstVertex, std::size_t vertexCount, const RenderStates& states)
{
    // VertexBuffer not supported?
    if (!VertexBuffer::isAvailable(*m_impl->graphicsContext))
    {
        priv::err() << "sf::VertexBuffer is not available, drawing skipped";
        return;
    }

    // Sanity check
    if (firstVertex > vertexBuffer.getVertexCount())
        return;

    // Clamp vertexCount to something that makes sense
    vertexCount = base::min(vertexCount, vertexBuffer.getVertexCount() - firstVertex);

    // Nothing to draw or inactive target

    if (!vertexCount || !vertexBuffer.getNativeHandle() ||
        (!RenderTargetImpl::isActive(*m_impl->graphicsContext, m_impl->id) && !setActive(true)))
        return;

    setupDraw(states);

    // Bind vertex buffer
    VertexBuffer::bind(*m_impl->graphicsContext, &vertexBuffer);

    // Always enable texture coordinates
    setupVertexAttribPointers(m_impl->cache.sfAttribPositionIdx,
                              m_impl->cache.sfAttribColorIdx,
                              m_impl->cache.sfAttribTexCoordIdx);

    drawPrimitives(vertexBuffer.getPrimitiveType(), firstVertex, vertexCount);

    // Unbind vertex buffer
    VertexBuffer::bind(*vertexBuffer.m_graphicsContext, nullptr);

    cleanupDraw(states);
}


////////////////////////////////////////////////////////////
bool RenderTarget::isSrgb() const
{
    // By default sRGB encoding is not enabled for an arbitrary RenderTarget
    return false;
}


////////////////////////////////////////////////////////////
bool RenderTarget::setActive(bool active)
{
    // If this RenderTarget is already active on the current GL context, do nothing
    if (const bool isAlreadyActive = RenderTargetImpl::isActive(*m_impl->graphicsContext, m_impl->id);
        (active && isAlreadyActive) || (!active && !isAlreadyActive))
        return true;

    // Mark this RenderTarget as active or no longer active in the tracking map
    const RenderTargetImpl::IdType contextId = m_impl->graphicsContext->getActiveThreadLocalGlContextId();

    SFML_BASE_ASSERT(contextId < RenderTargetImpl::maxIdCount);
    std::atomic<RenderTargetImpl::IdType>& renderTargetId = RenderTargetImpl::contextRenderTargetMap[contextId];

    // Deactivation
    if (!active)
    {
        SFML_BASE_ASSERT(renderTargetId.load() != RenderTargetImpl::invalidId);
        renderTargetId.store(RenderTargetImpl::invalidId);

        m_impl->cache.enable = false;
        return true;
    }

    // First ever activation
    if (renderTargetId.load() == RenderTargetImpl::invalidId)
    {
        renderTargetId.store(m_impl->id);

        m_impl->cache.glStatesSet = false;
        m_impl->cache.enable      = false;
        return true;
    }

    // Activation on a different context
    SFML_BASE_ASSERT(renderTargetId.load() != RenderTargetImpl::invalidId);
    SFML_BASE_ASSERT(renderTargetId.load() != m_impl->id);
    renderTargetId.store(m_impl->id);

    m_impl->cache.enable = false;
    return true;
}


////////////////////////////////////////////////////////////
void RenderTarget::resetGLStates()
{
    // Check here to make sure a context change does not happen after activate(true)
    const bool vertexBufferAvailable = VertexBuffer::isAvailable(*m_impl->graphicsContext);

// Workaround for states not being properly reset on
// macOS unless a context switch really takes place
#if defined(SFML_SYSTEM_MACOS)
    if (!setActive(false))
    {
        priv::err() << "Failed to set render target inactive";
    }
#endif

    if (RenderTargetImpl::isActive(*m_impl->graphicsContext, m_impl->id) || setActive(true))
    {
#ifdef SFML_DEBUG
        // Make sure that the user didn't leave an unchecked OpenGL error
        if (const GLenum error = glGetError(); error != GL_NO_ERROR)
        {
            priv::err() << "OpenGL error (" << error
                        << ") detected in user code, you should check for errors with glGetError()";
        }
#endif

        // Make sure that the texture unit which is active is the number 0
        if (GLEXT_multitexture)
        {
            // glCheck(GLEXT_glClientActiveTexture(GLEXT_GL_TEXTURE0));
            glCheck(GLEXT_glActiveTexture(GLEXT_GL_TEXTURE0));
        }

        // Define the default OpenGL states
        glCheck(glDisable(GL_CULL_FACE));
        glCheck(glDisable(GL_STENCIL_TEST));
        glCheck(glDisable(GL_DEPTH_TEST));
        glCheck(glDisable(GL_SCISSOR_TEST));
        glCheck(glEnable(GL_BLEND));
        glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

        const auto disableCacheAttrib = [&](const GLint cacheAttrib)
        {
            if (cacheAttrib >= 0)
                glCheck(glDisableVertexAttribArray(static_cast<GLuint>(cacheAttrib)));
        };

        disableCacheAttrib(m_impl->cache.sfAttribPositionIdx);
        disableCacheAttrib(m_impl->cache.sfAttribColorIdx);
        disableCacheAttrib(m_impl->cache.sfAttribTexCoordIdx);

        m_impl->cache.scissorEnabled = false;
        m_impl->cache.stencilEnabled = false;
        m_impl->cache.glStatesSet    = true;

        // Apply the default SFML states
        applyBlendMode(BlendAlpha);
        applyStencilMode(StencilMode());
        unapplyTexture();
        unapplyShader();

        if (vertexBufferAvailable)
            glCheck(VertexBuffer::bind(*m_impl->graphicsContext, nullptr));

        // Set the default view
        setView(getView());

        m_impl->cache.enable = true;
    }
}


////////////////////////////////////////////////////////////
RenderTarget::BatchDraw::BatchDraw(const RenderStates& renderStates, RenderTarget& renderTarget) :
m_renderStates(renderStates),
m_renderTarget(renderTarget)
{
    m_renderTarget.m_impl->batchVertexCache.clear();
    m_renderTarget.m_impl->batchIndexCache.clear();
}


////////////////////////////////////////////////////////////
RenderTarget::BatchDraw::~BatchDraw()
{
    const auto& vertices = m_renderTarget.m_impl->batchVertexCache;
    const auto& indices  = m_renderTarget.m_impl->batchIndexCache;

    m_renderTarget.drawIndexedVertices(vertices.data(),
                                       vertices.size(),
                                       indices.data(),
                                       indices.size(),
                                       PrimitiveType::Triangles,
                                       m_renderStates);
}


////////////////////////////////////////////////////////////
void RenderTarget::BatchDraw::add(const Sprite& sprite)
{
    const auto [data, size] = sprite.getVertices();

    const auto& vertices  = m_renderTarget.m_impl->batchVertexCache;
    auto&       indices   = m_renderTarget.m_impl->batchIndexCache;
    const auto  nextIndex = static_cast<unsigned short>(vertices.size());

    indices.reserveMore(6u);

    indices.unsafePushBackMultiple(
        // Triangle 0
        nextIndex + 0u,
        nextIndex + 1u,
        nextIndex + 2u,

        // Triangle 1
        nextIndex + 1u,
        nextIndex + 2u,
        nextIndex + 3u);

    appendPreTransformedVertices(data, size, sprite.getTransform());
}


////////////////////////////////////////////////////////////
void RenderTarget::BatchDraw::add(const Shape& shape)
{
    auto& vertices = m_renderTarget.m_impl->batchVertexCache;
    auto& indices  = m_renderTarget.m_impl->batchIndexCache;

    if (const auto [fillData, fillSize] = shape.getFillVertices(); fillSize > 2u)
    {
        const auto nextFillIndex = static_cast<unsigned short>(vertices.size());

        indices.reserveMore(fillSize * 3u);

        for (unsigned short i = 1u; i < fillSize - 1; ++i)
            indices.unsafePushBackMultiple(nextFillIndex, nextFillIndex + i, nextFillIndex + i + 1u);

        appendPreTransformedVertices(fillData, fillSize, shape.getTransform());
    }

    if (const auto [outlineData, outlineSize] = shape.getOutlineVertices(); outlineSize > 2u)
    {
        const auto nextOutlineIndex = static_cast<unsigned short>(vertices.size());

        indices.reserveMore(outlineSize * 3u);

        for (unsigned short i = 0u; i < outlineSize - 2; ++i)
            indices.unsafePushBackMultiple(nextOutlineIndex + i, nextOutlineIndex + i + 1u, nextOutlineIndex + i + 2u);

        appendPreTransformedVertices(outlineData, outlineSize, shape.getTransform());
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::BatchDraw::addSubsequentIndices(base::SizeT count)
{
    auto& indices = m_renderTarget.m_impl->batchIndexCache;

    const auto nextIndex = static_cast<unsigned short>(m_renderTarget.m_impl->batchVertexCache.size());

    indices.reserveMore(count);

    for (unsigned short i = 0; i < static_cast<unsigned short>(count); ++i)
        indices.unsafeEmplaceBack(nextIndex + i);
}


////////////////////////////////////////////////////////////
void RenderTarget::BatchDraw::appendPreTransformedVertices(const Vertex* data, base::SizeT size, const Transform& transform)
{
    auto& vertices = m_renderTarget.m_impl->batchVertexCache;

    vertices.reserveMore(size);
    vertices.unsafeEmplaceRange(data, size);

    for (auto i = vertices.size() - size; i < vertices.size(); ++i)
        vertices[i].position = transform * vertices[i].position;
}


////////////////////////////////////////////////////////////
RenderTarget::BatchDraw RenderTarget::startBatchDraw(const RenderStates& renderStates)
{
    return BatchDraw(renderStates, *this);
}


////////////////////////////////////////////////////////////
const RenderStates& RenderTarget::getDefaultRenderStates()
{
    return RenderStates::Default;
}


////////////////////////////////////////////////////////////
void RenderTarget::initialize()
{
    // Setup the default and current views
    m_impl->defaultView = View(FloatRect({0, 0}, getSize().to<Vector2f>()));
    m_impl->view        = m_impl->defaultView;

    // Set GL states only on first draw, so that we don't pollute user's states
    m_impl->cache.glStatesSet         = false;
    m_impl->cache.lastUsedProgramId   = 0u;
    m_impl->cache.sfAttribPositionIdx = -1;
    m_impl->cache.sfAttribColorIdx    = -1;
    m_impl->cache.sfAttribTexCoordIdx = -1;

    // Generate a unique ID for this RenderTarget to track
    // whether it is active within a specific context
    m_impl->id = RenderTargetImpl::nextUniqueId.fetch_add(1u, std::memory_order_relaxed);
}


////////////////////////////////////////////////////////////
GraphicsContext& RenderTarget::getGraphicsContext()
{
    return *m_impl->graphicsContext;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyCurrentView()
{
    // Set the viewport
    const IntRect viewport    = getViewport(m_impl->view);
    const int     viewportTop = static_cast<int>(getSize().y) - (viewport.position.y + viewport.size.y);
    glCheck(glViewport(viewport.position.x, viewportTop, viewport.size.x, viewport.size.y));

    // Set the scissor rectangle and enable/disable scissor testing
    if (m_impl->view.getScissor() == FloatRect({0, 0}, {1, 1}))
    {
        if (!m_impl->cache.enable || m_impl->cache.scissorEnabled)
        {
            glCheck(glDisable(GL_SCISSOR_TEST));
            m_impl->cache.scissorEnabled = false;
        }
    }
    else
    {
        const IntRect pixelScissor = getScissor(m_impl->view);
        const int     scissorTop   = static_cast<int>(getSize().y) - (pixelScissor.position.y + pixelScissor.size.y);
        glCheck(glScissor(pixelScissor.position.x, scissorTop, pixelScissor.size.x, pixelScissor.size.y));

        if (!m_impl->cache.enable || !m_impl->cache.scissorEnabled)
        {
            glCheck(glEnable(GL_SCISSOR_TEST));
            m_impl->cache.scissorEnabled = true;
        }
    }

    m_impl->cache.viewChanged = false;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyBlendMode(const BlendMode& mode)
{
    using RenderTargetImpl::equationToGlConstant;
    using RenderTargetImpl::factorToGlConstant;

    // Apply the blend mode, falling back to the non-separate versions if necessary
    if (GLEXT_blend_func_separate)
    {
        glCheck(GLEXT_glBlendFuncSeparate(factorToGlConstant(mode.colorSrcFactor),
                                          factorToGlConstant(mode.colorDstFactor),
                                          factorToGlConstant(mode.alphaSrcFactor),
                                          factorToGlConstant(mode.alphaDstFactor)));
    }
    else
    {
        glCheck(glBlendFunc(factorToGlConstant(mode.colorSrcFactor), factorToGlConstant(mode.colorDstFactor)));
    }

    if (GLEXT_blend_minmax || GLEXT_blend_subtract)
    {
        if (GLEXT_blend_equation_separate)
        {
            glCheck(GLEXT_glBlendEquationSeparate(equationToGlConstant(mode.colorEquation),
                                                  equationToGlConstant(mode.alphaEquation)));
        }
        else
        {
            glCheck(GLEXT_glBlendEquation(equationToGlConstant(mode.colorEquation)));
        }
    }
    else if ((mode.colorEquation != BlendMode::Equation::Add) || (mode.alphaEquation != BlendMode::Equation::Add))
    {
        static bool warned = false;

        if (!warned)
        {
#ifdef SFML_OPENGL_ES
            priv::err() << "OpenGL ES extension OES_blend_subtract unavailable";
#else
            priv::err() << "OpenGL extension EXT_blend_minmax and EXT_blend_subtract unavailable";
#endif
            priv::err() << "Selecting a blend equation not possible" << '\n'
                        << "Ensure that hardware acceleration is enabled if available";

            warned = true;
        }
    }

    m_impl->cache.lastBlendMode = mode;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyStencilMode(const StencilMode& mode)
{
    using RenderTargetImpl::stencilFunctionToGlConstant;
    using RenderTargetImpl::stencilOperationToGlConstant;

    // Fast path if we have a default (disabled) stencil mode
    if (mode == StencilMode())
    {
        if (!m_impl->cache.enable || m_impl->cache.stencilEnabled)
        {
            glCheck(glDisable(GL_STENCIL_TEST));
            glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

            m_impl->cache.stencilEnabled = false;
        }
    }
    else
    {
        // Apply the stencil mode
        if (!m_impl->cache.enable || !m_impl->cache.stencilEnabled)
            glCheck(glEnable(GL_STENCIL_TEST));

        glCheck(glStencilOp(GL_KEEP,
                            stencilOperationToGlConstant(mode.stencilUpdateOperation),
                            stencilOperationToGlConstant(mode.stencilUpdateOperation)));

        glCheck(glStencilFunc(stencilFunctionToGlConstant(mode.stencilComparison),
                              static_cast<int>(mode.stencilReference.value),
                              mode.stencilMask.value));

        m_impl->cache.stencilEnabled = true;
    }

    m_impl->cache.lastStencilMode = mode;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyTexture(const Texture& texture, CoordinateType coordinateType)
{
    texture.bind(*m_impl->graphicsContext);

    m_impl->cache.lastTextureId      = texture.m_cacheId;
    m_impl->cache.lastCoordinateType = coordinateType;
}


////////////////////////////////////////////////////////////
void RenderTarget::unapplyTexture()
{
    Texture::unbind(*m_impl->graphicsContext);

    m_impl->cache.lastTextureId      = 0ul;
    m_impl->cache.lastCoordinateType = CoordinateType::Pixels;
}


////////////////////////////////////////////////////////////
void RenderTarget::unapplyShader()
{
    Shader::unbind(*m_impl->graphicsContext);

    m_impl->cache.lastUsedProgramId = 0u;
}


////////////////////////////////////////////////////////////
void RenderTarget::setupDraw(const RenderStates& states)
{
    // GL_FRAMEBUFFER_SRGB is not available on OpenGL ES
    // If a framebuffer supports sRGB, it will always be enabled on OpenGL ES
#ifndef SFML_OPENGL_ES
    // Enable or disable sRGB encoding
    // This is needed for drivers that do not check the format of the surface drawn to before applying sRGB conversion
    if (!m_impl->cache.enable)
    {
        if (isSrgb())
            glCheck(glEnable(GL_FRAMEBUFFER_SRGB));
        else
            glCheck(glDisable(GL_FRAMEBUFFER_SRGB));
    }
#endif

    // First set the persistent OpenGL states if it's the very first call
    if (!m_impl->cache.glStatesSet)
        resetGLStates();

    const Shader& usedShader = states.shader != nullptr ? *states.shader : m_impl->graphicsContext->getBuiltInShader();

    // Apply the shader
    usedShader.bind();

    // Bind GL objects
    m_impl->vao.bind();
    m_impl->vbo.bind();
    m_impl->ebo.bind();

    // Update cache
    const auto usedNativeHandle  = usedShader.getNativeHandle();
    const bool usedShaderChanged = m_impl->cache.lastUsedProgramId != usedNativeHandle;

    if (usedShaderChanged)
    {
        m_impl->cache.lastUsedProgramId = usedNativeHandle;

        const auto updateCacheAttrib = [&](GLint& cacheAttrib, const char* attribName)
        {
            cacheAttrib = glCheckExpr(glGetAttribLocation(usedNativeHandle, attribName));

            if (cacheAttrib >= 0)
                glCheck(glEnableVertexAttribArray(static_cast<GLuint>(cacheAttrib)));
        };

        updateCacheAttrib(m_impl->cache.sfAttribPositionIdx, "sf_a_position");
        updateCacheAttrib(m_impl->cache.sfAttribColorIdx, "sf_a_color");
        updateCacheAttrib(m_impl->cache.sfAttribTexCoordIdx, "sf_a_texCoord");

        m_impl->cache.ulTextureMatrix             = usedShader.getUniformLocation("sf_u_textureMatrix");
        m_impl->cache.ulModelViewProjectionMatrix = usedShader.getUniformLocation("sf_u_modelViewProjectionMatrix");
    }

    // Apply the view
    if (!m_impl->cache.enable || m_impl->cache.viewChanged)
        applyCurrentView();

    // Set the model-view-projection matrix
    const Transform& modelViewMatrix(states.transform);
    usedShader.setMat4Uniform(*m_impl->cache.ulModelViewProjectionMatrix,
                              (m_impl->view.getTransform() * modelViewMatrix).getMatrix());

    // Apply the blend mode
    if (!m_impl->cache.enable || (states.blendMode != m_impl->cache.lastBlendMode))
        applyBlendMode(states.blendMode);

    // Apply the stencil mode
    if (!m_impl->cache.enable || (states.stencilMode != m_impl->cache.lastStencilMode))
        applyStencilMode(states.stencilMode);

    // Mask the color buffer off if necessary
    if (states.stencilMode.stencilOnly)
        glCheck(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));

    // Apply the texture
    const Texture& usedTexture = states.texture != nullptr ? *states.texture
                                                           : getGraphicsContext().getBuiltInWhiteDotTexture();

    const std::uint64_t usedTextureId = usedTexture.m_cacheId;

    // If the texture is an FBO attachment, always rebind it in order to inform the OpenGL driver that we
    // want changes made to it in other contexts to be visible here as well This saves us from having to
    // call `glFlush()` in `RenderTextureImplFBO` which can be quite costly
    //
    // See: https://www.khronos.org/opengl/wiki/Memory_Model

    const bool mustApplyTexture = !m_impl->cache.enable || usedTexture.m_fboAttachment ||
                                  usedTextureId != m_impl->cache.lastTextureId ||
                                  states.coordinateType != m_impl->cache.lastCoordinateType;

    if (mustApplyTexture)
    {
        applyTexture(usedTexture, states.coordinateType);

        if (m_impl->cache.ulTextureMatrix.hasValue())
        {
            // clang-format off
            float textureMatrixBuffer[]{1.f, 0.f, 0.f, 0.f,
                                        0.f, 1.f, 0.f, 0.f,
                                        0.f, 0.f, 1.f, 0.f,
                                        0.f, 0.f, 0.f, 1.f};
            // clang-format on

            usedTexture.getMatrix(textureMatrixBuffer, m_impl->cache.lastCoordinateType);
            usedShader.setMat4Uniform(*m_impl->cache.ulTextureMatrix, textureMatrixBuffer);
        }
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::drawPrimitives(PrimitiveType type, std::size_t firstVertex, std::size_t vertexCount)
{
    m_impl->vao.bind();

    glCheck(glDrawArrays(RenderTargetImpl::primitiveTypeToOpenGLMode(type),
                         static_cast<GLint>(firstVertex),
                         static_cast<GLsizei>(vertexCount)));
}


////////////////////////////////////////////////////////////
void RenderTarget::drawIndexedPrimitives(PrimitiveType type, std::size_t indexCount)
{
    m_impl->vao.bind();

    glCheck(glDrawElements(RenderTargetImpl::primitiveTypeToOpenGLMode(type),
                           indexCount,
                           GL_UNSIGNED_SHORT,
                           /* index offset */ nullptr));
}


////////////////////////////////////////////////////////////
void RenderTarget::cleanupDraw(const RenderStates& states)
{
    // Unbind the shader, if any
    unapplyShader();

    // If the texture we used to draw belonged to a RenderTexture, then forcibly unbind that texture.
    // This prevents a bug where some drivers do not clear RenderTextures properly.
    if (states.texture && states.texture->m_fboAttachment)
        unapplyTexture();

    // Mask the color buffer back on if necessary
    if (states.stencilMode.stencilOnly)
        glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    // Re-enable the cache at the end of the draw if it was disabled
    m_impl->cache.enable = true;
}

} // namespace sf


////////////////////////////////////////////////////////////
// Render states caching strategies
//
// * View
//   If SetView was called since last draw, the projection
//   matrix is updated. We don't need more, the view doesn't
//   change frequently.
//
// * Blending mode
//   Since it overloads the == operator, we can easily check
//   whether any of the 6 blending components changed and,
//   thus, whether we need to update the blend mode.
//
// * Texture
//   Storing the pointer or OpenGL ID of the last used texture
//   is not enough; if the sf::Texture instance is destroyed,
//   both the pointer and the OpenGL ID might be recycled in
//   a new texture instance. We need to use our own unique
//   identifier system to ensure consistent caching.
//
// * Shader
//   Shaders are very hard to optimize, because they have
//   parameters that can be hard (if not impossible) to track,
//   like matrices or textures. The only optimization that we
//   do is that we avoid setting a null shader if there was
//   already none for the previous draw.
//
////////////////////////////////////////////////////////////
