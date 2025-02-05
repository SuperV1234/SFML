#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/BlendMode.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/CoordinateType.hpp"
#include "SFML/Graphics/DrawableBatch.hpp"
#include "SFML/Graphics/DrawableBatchUtils.hpp"
#include "SFML/Graphics/GLBufferObject.hpp"
#include "SFML/Graphics/GLPersistentBuffer.hpp"
#include "SFML/Graphics/GLSyncGuard.hpp"
#include "SFML/Graphics/GLVAOGroup.hpp"
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
#include "SFML/Window/Glad.hpp"

#include "SFML/System/Err.hpp"
#include "SFML/System/Rect.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/OffsetOf.hpp"
#include "SFML/Base/FastSinCos.hpp"
#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Math/Lround.hpp"
#include "SFML/Base/SizeT.hpp"

#include <atomic>


namespace
{
// A nested named namespace is used here to allow unity builds of SFML.
namespace RenderTargetImpl
{
////////////////////////////////////////////////////////////
// Type alias for a render target or context id
using IdType = unsigned int;


////////////////////////////////////////////////////////////
// Unique identifier, used for identifying RenderTargets when
// tracking the currently active RenderTarget within a given context
constinit std::atomic<IdType> nextUniqueId{1ul};


////////////////////////////////////////////////////////////
// Invalid/null render target or context id value
constexpr IdType invalidId{0ul};


////////////////////////////////////////////////////////////
// Maximum supported number of render targets or contexts
constexpr sf::base::SizeT maxIdCount{256ul};


////////////////////////////////////////////////////////////
// Map to help us detect whether a different RenderTarget has been activated within a single context
constinit std::atomic<IdType> contextRenderTargetMap[maxIdCount]{};


////////////////////////////////////////////////////////////
// Check if a render target with the given ID is active in the current context
[[nodiscard]] bool isActive(const IdType contextId, const IdType id)
{
    SFML_BASE_ASSERT(contextId < maxIdCount);

    const auto renderTargetId = contextRenderTargetMap[contextId].load();
    return (renderTargetId != invalidId) && (renderTargetId == id);
}


////////////////////////////////////////////////////////////
#define SFML_PRIV_DEFINE_ENUM_TO_GLENUM_CONVERSION_FN(fnName, sfEnumType, ...)                                  \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] constexpr GLenum fnName(sfEnumType sfEnumValue) \
    {                                                                                                           \
        constexpr GLenum glValues[] __VA_ARGS__;                                                                \
                                                                                                                \
        SFML_BASE_ASSERT(static_cast<unsigned int>(sfEnumValue) < ::sf::base::getArraySize(glValues));          \
        return glValues[static_cast<unsigned int>(sfEnumValue)];                                                \
    }


////////////////////////////////////////////////////////////
// Convert an sf::BlendMode::Factor constant to the corresponding OpenGL constant.
SFML_PRIV_DEFINE_ENUM_TO_GLENUM_CONVERSION_FN(
    factorToGlConstant,
    sf::BlendMode::Factor,
    {GL_ZERO,
     GL_ONE,
     GL_SRC_COLOR,
     GL_ONE_MINUS_SRC_COLOR,
     GL_DST_COLOR,
     GL_ONE_MINUS_DST_COLOR,
     GL_SRC_ALPHA,
     GL_ONE_MINUS_SRC_ALPHA,
     GL_DST_ALPHA,
     GL_ONE_MINUS_DST_ALPHA});


////////////////////////////////////////////////////////////
// Convert an sf::BlendMode::Equation constant to the corresponding OpenGL constant.
SFML_PRIV_DEFINE_ENUM_TO_GLENUM_CONVERSION_FN(equationToGlConstant,
                                              sf::BlendMode::Equation,
                                              {GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX});


////////////////////////////////////////////////////////////
// Convert an UpdateOperation constant to the corresponding OpenGL constant.
SFML_PRIV_DEFINE_ENUM_TO_GLENUM_CONVERSION_FN(stencilOperationToGlConstant,
                                              sf::StencilUpdateOperation,
                                              {GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT});


////////////////////////////////////////////////////////////
// Convert a Comparison constant to the corresponding OpenGL constant.
SFML_PRIV_DEFINE_ENUM_TO_GLENUM_CONVERSION_FN(
    stencilFunctionToGlConstant,
    sf::StencilComparison,
    {GL_NEVER, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL, GL_EQUAL, GL_NOTEQUAL, GL_ALWAYS});


////////////////////////////////////////////////////////////
SFML_PRIV_DEFINE_ENUM_TO_GLENUM_CONVERSION_FN(
    primitiveTypeToOpenGLMode,
    sf::PrimitiveType,
    {GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN});


////////////////////////////////////////////////////////////
#undef SFML_PRIV_DEFINE_ENUM_TO_GLENUM_CONVERSION_FN


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline sf::IntRect getMultipliedBySizeAndRoundedRect(
    sf::Vector2u         renderTargetSize,
    const sf::FloatRect& inputRect)
{
    const auto [width, height] = renderTargetSize.toVector2f();

    return sf::Rect<long>({sf::base::lround(width * inputRect.position.x), sf::base::lround(height * inputRect.position.y)},
                          {sf::base::lround(width * inputRect.size.x), sf::base::lround(height * inputRect.size.y)})
        .to<sf::IntRect>();
}


////////////////////////////////////////////////////////////
enum : bool
{
#ifdef SFML_OPENGL_ES
    isOpenGLES = true
#else
    isOpenGLES = false
#endif
};


////////////////////////////////////////////////////////////
[[gnu::always_inline, gnu::flatten]] inline void streamToGPU(unsigned int bufferId, const void* data, sf::base::SizeT dataByteCount)
{
    if constexpr (isOpenGLES)
    {
        // On OpenGL ES, the "naive" method seems faster, also named buffers are not supported
        glCheck(glBufferData(bufferId, static_cast<GLsizeiptr>(dataByteCount), data, GL_STREAM_DRAW));
    }
    else
    {
        // For small batches, the "naive" method also seems faster
        if (dataByteCount < sizeof(sf::Vertex) * 64)
        {
            glCheck(glNamedBufferData(bufferId, static_cast<GLsizeiptr>(dataByteCount), nullptr, GL_STREAM_DRAW)); // Must orphan first
            glCheck(glNamedBufferData(bufferId, static_cast<GLsizeiptr>(dataByteCount), data, GL_STREAM_DRAW));
            return;
        }

        // For larger batches, memcpying into a transient mapped buffer seems faster
        glCheck(glNamedBufferData(bufferId, static_cast<GLsizeiptr>(dataByteCount), nullptr, GL_STREAM_DRAW));

        void* const ptr = glCheck(
            glMapNamedBufferRange(bufferId,
                                  0u,
                                  static_cast<GLsizeiptr>(dataByteCount),
                                  GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT));

        SFML_BASE_MEMCPY(ptr, data, dataByteCount);

        [[maybe_unused]] const auto rc = glCheck(glUnmapNamedBuffer(bufferId));
        SFML_BASE_ASSERT(rc == GL_TRUE);
    }
}


////////////////////////////////////////////////////////////
[[gnu::always_inline, gnu::flatten]] inline void streamVerticesToGPU([[maybe_unused]] unsigned int bufferId,
                                                                     const sf::Vertex*             vertexData,
                                                                     sf::base::SizeT               vertexCount)
{
    streamToGPU(isOpenGLES ? GL_ARRAY_BUFFER : bufferId, vertexData, sizeof(sf::Vertex) * vertexCount);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline, gnu::flatten]] inline void streamIndicesToGPU([[maybe_unused]] unsigned int bufferId,
                                                                    const sf::IndexType*          indexData,
                                                                    sf::base::SizeT               indexCount)
{
    streamToGPU(isOpenGLES ? GL_ELEMENT_ARRAY_BUFFER : bufferId, indexData, sizeof(sf::IndexType) * indexCount);
}

////////////////////////////////////////////////////////////
void setupVertexAttribPointers()
{
#define SFML_PRIV_OFFSETOF(...) reinterpret_cast<const void*>(SFML_BASE_OFFSETOF(__VA_ARGS__))

    // Hardcoded layout location `0u` for `sf_a_position`
    glCheck(glEnableVertexAttribArray(0u));
    glCheck(glVertexAttribPointer(/*      index */ 0u,
                                  /*       size */ 2,
                                  /*       type */ GL_FLOAT,
                                  /* normalized */ GL_FALSE,
                                  /*     stride */ sizeof(sf::Vertex),
                                  /*     offset */ SFML_PRIV_OFFSETOF(sf::Vertex, position)));

    // Hardcoded layout location `1u` for `sf_a_color`
    glCheck(glEnableVertexAttribArray(1u));
    glCheck(glVertexAttribPointer(/*      index */ 1u,
                                  /*       size */ 4,
                                  /*       type */ GL_UNSIGNED_BYTE,
                                  /* normalized */ GL_TRUE,
                                  /*     stride */ sizeof(sf::Vertex),
                                  /*     offset */ SFML_PRIV_OFFSETOF(sf::Vertex, color)));

    // Hardcoded layout location `2u` for `sf_a_texCoord`
    glCheck(glEnableVertexAttribArray(2u));
    glCheck(glVertexAttribPointer(/*      index */ 2u,
                                  /*       size */ 2,
                                  /*       type */ GL_FLOAT,
                                  /* normalized */ GL_FALSE,
                                  /*     stride */ sizeof(sf::Vertex),
                                  /*     offset */ SFML_PRIV_OFFSETOF(sf::Vertex, texCoords)));

#undef SFML_PRIV_OFFSETOF
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
    bool enable{false};      //!< Is the cache enabled?
    bool glStatesSet{false}; //!< Are our internal GL states set yet?

    bool      viewChanged{false}; //!< Has the current view changed since last draw?
    Transform lastViewTransform;  //!< Cached transform of latest view

    Transform lastDrawTransform; //!< Cached last draw transform

    bool scissorEnabled{false}; //!< Is scissor testing enabled?
    bool stencilEnabled{false}; //!< Is stencil testing enabled?

    unsigned int lastVaoGroup{0u}; //!< Last bound vertex array object id

    BlendMode      lastBlendMode{BlendAlpha};                      //!< Cached blending mode
    StencilMode    lastStencilMode;                                //!< Cached stencil
    base::U64      lastTextureId{0u};                              //!< Cached texture
    CoordinateType lastCoordinateType{CoordinateType::Normalized}; //!< Cached texture coordinate type

    GLuint lastProgramId{0u}; //!< GL id of the last used shader program
};


////////////////////////////////////////////////////////////
struct RenderTarget::Impl
{
    explicit Impl(const View& theView) :
    view(theView),
    id(RenderTargetImpl::nextUniqueId.fetch_add(1u, std::memory_order_relaxed)),
    vaoGroup(),
    persistentVaoGroup(),
    vboPersistentBuffer(persistentVaoGroup.vbo),
    eboPersistentBuffer(persistentVaoGroup.ebo)
    {
        vaoGroup.bind();
        persistentVaoGroup.bind();
    }

    View view; //!< Current view

    StatesCache cache{}; //!< Render states cache

    RenderTargetImpl::IdType id{}; //!< Unique number that identifies the render target

    GLVAOGroup vaoGroup;           //!< VAO, VBO, and EBO associated with the render target (non-persistent storage)
    GLVAOGroup persistentVaoGroup; //!< VAO, VBO, and EBO associated with the render target (persistent storage)

    GLPersistentBuffer<GLVertexBufferObject>  vboPersistentBuffer; //!< Persistent VBO buffer (used for batching)
    GLPersistentBuffer<GLElementBufferObject> eboPersistentBuffer; //!< Persistent EBO buffer (used for batching)

    void bindGLObjects(GLVAOGroup& theVAOGroup)
    {
        theVAOGroup.bind();
        cache.lastVaoGroup = theVAOGroup.getId();

        RenderTargetImpl::setupVertexAttribPointers();
    }
};


////////////////////////////////////////////////////////////
RenderTarget::RenderTarget(const View& currentView) : m_impl(currentView)
{
}


////////////////////////////////////////////////////////////
RenderTarget::~RenderTarget() = default;


////////////////////////////////////////////////////////////
RenderTarget::RenderTarget(RenderTarget&&) noexcept = default;


////////////////////////////////////////////////////////////
RenderTarget& RenderTarget::operator=(RenderTarget&&) noexcept = default;


////////////////////////////////////////////////////////////
[[nodiscard]] GLPersistentBuffer<GLVertexBufferObject>& RenderTarget::getVBOPersistentBuffer()
{
    return m_impl->vboPersistentBuffer;
}


////////////////////////////////////////////////////////////
[[nodiscard]] GLPersistentBuffer<GLElementBufferObject>& RenderTarget::getEBOPersistentBuffer()
{
    return m_impl->eboPersistentBuffer;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool RenderTarget::clearImpl()
{
    if (!setActive(true))
    {
        priv::err() << "Failed to activate render target in `clearImpl`";
        return false;
    }

    // Unbind texture to fix RenderTexture preventing clear
    unapplyTexture(); // See https://en.sfml-dev.org/forums/index.php?topic=9350

    // Apply the view (scissor testing can affect clearing)
    if (!m_impl->cache.enable || m_impl->cache.viewChanged)
        applyView(m_impl->view);

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
    if (view == m_impl->view)
        return;

    m_impl->view              = view;
    m_impl->cache.viewChanged = true;
}


////////////////////////////////////////////////////////////
const View& RenderTarget::getView() const
{
    return m_impl->view;
}


////////////////////////////////////////////////////////////
IntRect RenderTarget::getViewport(const View& view) const
{
    return RenderTargetImpl::getMultipliedBySizeAndRoundedRect(getSize(), view.viewport);
}


////////////////////////////////////////////////////////////
IntRect RenderTarget::getScissor(const View& view) const
{
    return RenderTargetImpl::getMultipliedBySizeAndRoundedRect(getSize(), view.scissor);
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
    const Vector2f normalized = Vector2f(-1.f, 1.f) + Vector2f(2.f, -2.f)
                                                          .componentWiseMul(point.toVector2f() - viewport.position)
                                                          .componentWiseDiv(viewport.size);

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
    return ((normalized.componentWiseMul({1.f, -1.f}) + sf::Vector2f{1.f, 1.f})
                .componentWiseMul({0.5f, 0.5f})
                .componentWiseMul(viewport.size) +
            viewport.position)
        .toVector2i();
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Texture& texture, RenderStates states)
{
    states.texture        = &texture;
    states.coordinateType = CoordinateType::Pixels;

    Vertex buffer[4];

    appendPreTransformedSpriteVertices(Transform::from(/* position */ {0.f, 0.f},
                                                       /* scale */ {1.f, 1.f},
                                                       /* origin */ {0.f, 0.f}),
                                       texture.getRect(),
                                       Color::White,
                                       buffer);

    draw(buffer, PrimitiveType::TriangleStrip, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Texture& texture, const TextureDrawParams& params, RenderStates states)
{
    states.texture        = &texture;
    states.coordinateType = CoordinateType::Pixels;

    const auto [sine, cosine] = base::fastSinCos(params.rotation.wrapUnsigned().asRadians());

    Vertex buffer[4];

    appendPreTransformedSpriteVertices(Transform::from(params.position, params.scale, params.origin, sine, cosine),
                                       (params.textureRect == FloatRect{}) ? texture.getRect() : params.textureRect,
                                       params.color,
                                       buffer);

    draw(buffer, PrimitiveType::TriangleStrip, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Sprite& sprite, const Texture& texture, RenderStates states)
{
    states.texture        = &texture;
    states.coordinateType = CoordinateType::Pixels;

    Vertex buffer[4];
    appendPreTransformedSpriteVertices(sprite.getTransform(), sprite.textureRect, sprite.color, buffer);
    draw(buffer, PrimitiveType::TriangleStrip, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Shape& shape, const Texture* texture, const RenderStates& states)
{
    shape.drawOnto(*this, texture, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawVertices(const Vertex* vertexData, base::SizeT vertexCount, PrimitiveType type, const RenderStates& states)
{
    // Nothing to draw or inactive target
    if (vertexData == nullptr || vertexCount == 0u || !setActive(true))
        return;

    setupDraw(/* persistent */ false, states);

    RenderTargetImpl::streamVerticesToGPU(m_impl->vaoGroup.vbo.getId(), vertexData, vertexCount);

    drawPrimitives(type, 0u, vertexCount);
    cleanupDraw(states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawIndexedVertices(
    const Vertex*       vertexData,
    base::SizeT         vertexCount,
    const IndexType*    indexData,
    base::SizeT         indexCount,
    PrimitiveType       type,
    const RenderStates& states)
{
    // Nothing to draw or inactive target
    if (vertexData == nullptr || vertexCount == 0u || indexData == nullptr || indexCount == 0u || !setActive(true))
        return;

    setupDraw(/* persistent */ false, states);

    RenderTargetImpl::streamVerticesToGPU(m_impl->vaoGroup.vbo.getId(), vertexData, vertexCount);
    RenderTargetImpl::streamIndicesToGPU(m_impl->vaoGroup.ebo.getId(), indexData, indexCount);

    drawIndexedPrimitives(type, indexCount);
    cleanupDraw(states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawPersistentMappedVertices(base::SizeT vertexCount, PrimitiveType type, const RenderStates& states)
{
    // Nothing to draw or inactive target
    if (vertexCount == 0u || !setActive(true))
        return;

    setupDraw(/* persistent */ true, states);

    {
        GLSyncGuard syncGuard;
        drawPrimitives(type, 0u, vertexCount);
    }

    cleanupDraw(states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawPersistentMappedIndexedVertices(base::SizeT indexCount, PrimitiveType type, const RenderStates& states)
{
    // Nothing to draw or inactive target
    if (indexCount == 0u || !setActive(true))
        return;

    setupDraw(/* persistent */ true, states);

    {
        GLSyncGuard syncGuard;
        drawIndexedPrimitives(type, indexCount);
    }

    cleanupDraw(states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const CPUDrawableBatch& drawableBatch, RenderStates states)
{
    states.transform *= drawableBatch.getTransform();

    drawIndexedVertices(drawableBatch.m_storage.vertices.data(),
                        drawableBatch.m_storage.vertices.size(),
                        drawableBatch.m_storage.indices.data(),
                        drawableBatch.m_storage.indices.size(),
                        PrimitiveType::Triangles,
                        states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const PersistentGPUDrawableBatch& drawableBatch, RenderStates states)
{
    states.transform *= drawableBatch.getTransform();
    drawPersistentMappedIndexedVertices(drawableBatch.m_storage.nIndices, PrimitiveType::Triangles, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const VertexBuffer& vertexBuffer, const RenderStates& states)
{
    draw(vertexBuffer, 0, vertexBuffer.getVertexCount(), states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const VertexBuffer& vertexBuffer, base::SizeT firstVertex, base::SizeT vertexCount, const RenderStates& states)
{
    // Sanity check
    if (firstVertex > vertexBuffer.getVertexCount())
        return;

    // Clamp vertexCount to something that makes sense
    vertexCount = base::min(vertexCount, vertexBuffer.getVertexCount() - firstVertex);

    // Nothing to draw or inactive target

    if (!vertexCount || !vertexBuffer.getNativeHandle() || !setActive(true))
        return;

    setupDraw(/* persistent */ false, states);

    // Bind vertex buffer
    vertexBuffer.bind();

    // Always enable texture coordinates (needed because different buffer is bound)
    RenderTargetImpl::setupVertexAttribPointers();

    drawPrimitives(vertexBuffer.getPrimitiveType(), firstVertex, vertexCount);

    // Unbind vertex buffer
    VertexBuffer::unbind();

    // Needed to restore attrib pointers on regular VBO
    m_impl->bindGLObjects(m_impl->vaoGroup);

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
    // Mark this RenderTarget as active or no longer active in the tracking map
    const RenderTargetImpl::IdType contextId = GraphicsContext::getActiveThreadLocalGlContextId();

    // If this RenderTarget is already active on the current GL context, do nothing
    if (const bool isAlreadyActive = RenderTargetImpl::isActive(contextId, m_impl->id);
        (active && isAlreadyActive) || (!active && !isAlreadyActive))
        return true;

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
// Workaround for states not being properly reset on
// macOS unless a context switch really takes place
#if defined(SFML_SYSTEM_MACOS)
    if (!setActive(false))
        priv::err() << "Failed to set render target inactive";
#endif

    if (!setActive(true))
        return;

#ifdef SFML_DEBUG
    // Make sure that the user didn't leave an unchecked OpenGL error
    if (const GLenum error = glGetError(); error != GL_NO_ERROR)
        priv::err() << "OpenGL error (" << error << ") detected in user code, you should check for errors with glGetError()";
#endif

    // Make sure that the texture unit which is active is the number 0
    glCheck(glActiveTexture(GL_TEXTURE0));

    // Define the default OpenGL states
    glCheck(glDisable(GL_CULL_FACE));
    glCheck(glDisable(GL_STENCIL_TEST));
    glCheck(glDisable(GL_DEPTH_TEST));
    glCheck(glDisable(GL_SCISSOR_TEST));
    glCheck(glEnable(GL_BLEND));
    glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    m_impl->cache.scissorEnabled = false;
    m_impl->cache.stencilEnabled = false;
    m_impl->cache.lastVaoGroup   = 0u;

    m_impl->cache.glStatesSet = true;

    // Apply the default SFML states
    applyBlendMode(BlendAlpha);
    applyStencilMode(StencilMode{});
    unapplyTexture();

    {
        Shader::unbind();
        m_impl->cache.lastProgramId = 0u;
    }

    VertexBuffer::unbind();

    // Set the default view
    setView(getView());

    m_impl->cache.enable = true;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyView(const View& view)
{
    // Set the viewport
    const IntRect viewport    = getViewport(view);
    const int     viewportTop = static_cast<int>(getSize().y) - (viewport.position.y + viewport.size.y);
    glCheck(glViewport(viewport.position.x, viewportTop, viewport.size.x, viewport.size.y));

    // Set the scissor rectangle and enable/disable scissor testing
    if (view.scissor == FloatRect{{0.f, 0.f}, {1.f, 1.f}})
    {
        if (!m_impl->cache.enable || m_impl->cache.scissorEnabled)
        {
            glCheck(glDisable(GL_SCISSOR_TEST));
            m_impl->cache.scissorEnabled = false;
        }
    }
    else
    {
        const IntRect pixelScissor = getScissor(view);
        const int     scissorTop   = static_cast<int>(getSize().y) - (pixelScissor.position.y + pixelScissor.size.y);
        glCheck(glScissor(pixelScissor.position.x, scissorTop, pixelScissor.size.x, pixelScissor.size.y));

        if (!m_impl->cache.enable || !m_impl->cache.scissorEnabled)
        {
            glCheck(glEnable(GL_SCISSOR_TEST));
            m_impl->cache.scissorEnabled = true;
        }
    }

    m_impl->cache.lastViewTransform = view.getTransform();
    m_impl->cache.viewChanged       = false;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyBlendMode(const BlendMode& mode)
{
    using RenderTargetImpl::equationToGlConstant;
    using RenderTargetImpl::factorToGlConstant;

    glCheck(glBlendFuncSeparate(factorToGlConstant(mode.colorSrcFactor),
                                factorToGlConstant(mode.colorDstFactor),
                                factorToGlConstant(mode.alphaSrcFactor),
                                factorToGlConstant(mode.alphaDstFactor)));

    glCheck(glBlendEquationSeparate(equationToGlConstant(mode.colorEquation), equationToGlConstant(mode.alphaEquation)));

    m_impl->cache.lastBlendMode = mode;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyStencilMode(const StencilMode& mode)
{
    using RenderTargetImpl::stencilFunctionToGlConstant;
    using RenderTargetImpl::stencilOperationToGlConstant;

    m_impl->cache.lastStencilMode = mode;

    // Fast path if we have a default (disabled) stencil mode
    if (mode == StencilMode{})
    {
        if (!m_impl->cache.enable || m_impl->cache.stencilEnabled)
        {
            glCheck(glDisable(GL_STENCIL_TEST));
            glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

            m_impl->cache.stencilEnabled = false;
        }

        return;
    }

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


////////////////////////////////////////////////////////////
void RenderTarget::unapplyTexture()
{
    Texture::unbind();

    m_impl->cache.lastTextureId      = 0ul;
    m_impl->cache.lastCoordinateType = CoordinateType::Pixels;
}


////////////////////////////////////////////////////////////
void RenderTarget::setupDraw(bool persistent, const RenderStates& states)
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
#else
    SFML_BASE_ASSERT(!persistent && "Persistent OpenGL buffers are not available in OpenGL ES");
#endif

    // First set the persistent OpenGL states if it's the very first call
    if (!m_impl->cache.glStatesSet)
        resetGLStates();

    // Bind GL objects
    if (GLVAOGroup& vaoGroupToBind = persistent ? m_impl->persistentVaoGroup : m_impl->vaoGroup;
        !m_impl->cache.enable || m_impl->cache.lastVaoGroup != vaoGroupToBind.getId())
    {
        m_impl->cache.lastVaoGroup = vaoGroupToBind.getId();
        m_impl->bindGLObjects(vaoGroupToBind);
    }

    // Select shader to be used
    const Shader& usedShader = states.shader != nullptr ? *states.shader : GraphicsContext::getInstalledBuiltInShader();

    // Update shader
    const auto usedNativeHandle = usedShader.getNativeHandle();
    const bool shaderChanged    = m_impl->cache.lastProgramId != usedNativeHandle;

    if (shaderChanged)
    {
        usedShader.bind();
        m_impl->cache.lastProgramId = usedNativeHandle;
    }

    // Apply the view
    if (!m_impl->cache.enable || m_impl->cache.viewChanged)
        applyView(m_impl->view);

    // Set the model-view-projection matrix
    setupDrawMVP(states, m_impl->cache.lastViewTransform, shaderChanged);

    // Apply the blend mode
    if (!m_impl->cache.enable || (states.blendMode != m_impl->cache.lastBlendMode))
        applyBlendMode(states.blendMode);

    // Apply the stencil mode
    if (!m_impl->cache.enable || (states.stencilMode != m_impl->cache.lastStencilMode))
        applyStencilMode(states.stencilMode);

    // Mask the color buffer off if necessary
    if (states.stencilMode.stencilOnly)
        glCheck(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));

    // Deal with texture
    setupDrawTexture(states);
}


////////////////////////////////////////////////////////////
void RenderTarget::setupDrawMVP(const RenderStates& states, const Transform& viewTransform, bool shaderChanged)
{
    // Compute the final draw transform
    const Transform trsfm = viewTransform * /* model-view matrix */ states.transform;

    // If there's no difference from the cached one, exit early
    if (!shaderChanged && (m_impl->cache.enable && trsfm == m_impl->cache.lastDrawTransform))
        return;

    // Update the cached transform
    m_impl->cache.lastDrawTransform = trsfm;

    // clang-format off
    const float transformMatrixBuffer[]{trsfm.a00, trsfm.a10, 0.f, 0.f,
                                        trsfm.a01, trsfm.a11, 0.f, 0.f,
                                        0.f,       0.f,       1.f, 0.f,
                                        trsfm.a02, trsfm.a12, 0.f, 1.f};
    // clang-format on

    // Upload uniform data to GPU (hardcoded layout location `0u` for `sf_u_mvpMatrix`)
    glCheck(glUniformMatrix4fv(/* location */ 0u, /* count */ 1, /* transpose */ GL_FALSE, transformMatrixBuffer));
}


////////////////////////////////////////////////////////////
void RenderTarget::setupDrawTexture(const RenderStates& states)
{
    // Select texture to be used
    const Texture& usedTexture = states.texture != nullptr ? *states.texture
                                                           : GraphicsContext::getInstalledBuiltInWhiteDotTexture();

    // If the texture is an FBO attachment, always rebind it in order to inform the OpenGL driver that we
    // want changes made to it in other contexts to be visible here as well. This saves us from having to
    // call `glFlush()` in `RenderTextureImplFBO` which can be quite costly
    //
    // See: https://www.khronos.org/opengl/wiki/Memory_Model

    // Should the texture be bound?
    const bool mustApplyTexture = !m_impl->cache.enable || usedTexture.m_fboAttachment ||
                                  usedTexture.m_cacheId != m_impl->cache.lastTextureId ||
                                  states.coordinateType != m_impl->cache.lastCoordinateType;

    // If not, exit early
    if (!mustApplyTexture)
        return;

    // Bind the texture
    usedTexture.bind();

    // Update basic cache texture stuff
    m_impl->cache.lastTextureId      = usedTexture.m_cacheId;
    m_impl->cache.lastCoordinateType = states.coordinateType;
}


////////////////////////////////////////////////////////////
void RenderTarget::drawPrimitives(PrimitiveType type, base::SizeT firstVertex, base::SizeT vertexCount)
{
    glCheck(glDrawArrays(/*     primitive type */ RenderTargetImpl::primitiveTypeToOpenGLMode(type),
                         /* first vertex index */ static_cast<GLint>(firstVertex),
                         /*       vertex count */ static_cast<GLsizei>(vertexCount)));
}


////////////////////////////////////////////////////////////
void RenderTarget::drawIndexedPrimitives(PrimitiveType type, base::SizeT indexCount)
{
    static_assert(SFML_BASE_IS_SAME(IndexType, unsigned int));

    glCheck(glDrawElements(/* primitive type */ RenderTargetImpl::primitiveTypeToOpenGLMode(type),
                           /*    index count */ static_cast<GLsizei>(indexCount),
                           /*     index type */ GL_UNSIGNED_INT,
                           /*   index offset */ nullptr));
}


////////////////////////////////////////////////////////////
void RenderTarget::cleanupDraw(const RenderStates& states)
{
    // Do not unbind the shader here, as it could be reused for the next draw call.

    // If the texture we used to draw belonged to a RenderTexture, then forcibly unbind that texture.
    // This prevents a bug where some drivers do not clear RenderTextures properly.
    if (states.texture != nullptr && states.texture->m_fboAttachment)
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
