#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Config.hpp"

#include "SFML/Window/ContextSettings.hpp"
#include "SFML/Window/GLCheck.hpp"
#include "SFML/Window/GLUtils.hpp"
#include "SFML/Window/GlContext.hpp"
#include "SFML/Window/GlContextTypeImpl.hpp"
#include "SFML/Window/Glad.hpp"
#include "SFML/Window/JoystickManager.hpp"
#include "SFML/Window/SensorManager.hpp"
#include "SFML/Window/WindowContext.hpp"

#include "SFML/System/Err.hpp"

#include "SFML/Base/Abort.hpp"
#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/UniquePtr.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include <csignal>


namespace sf
{
namespace
{
////////////////////////////////////////////////////////////
/// \brief Load our extensions vector with the supported extensions
///
////////////////////////////////////////////////////////////
[[nodiscard]] std::vector<std::string> loadExtensions(DerivedGlContextType& glContext)
{
    std::vector<std::string> result; // Use a single local variable for NRVO

    auto glGetErrorFunc    = reinterpret_cast<glGetErrorFuncType>(glContext.getFunction("glGetError"));
    auto glGetIntegervFunc = reinterpret_cast<glGetIntegervFuncType>(glContext.getFunction("glGetIntegerv"));
    auto glGetStringFunc   = reinterpret_cast<glGetStringFuncType>(glContext.getFunction("glGetString"));

    if (!glGetErrorFunc || !glGetIntegervFunc || !glGetStringFunc)
        return result; // Empty vector

    // Check whether a >= 3.0 context is available
    int majorVersion = 0;
    glGetIntegervFunc(GL_MAJOR_VERSION, &majorVersion); // intentionally not checked, will be checked below

    auto glGetStringiFunc = reinterpret_cast<glGetStringiFuncType>(glContext.getFunction("glGetStringi"));

    // Try to load the >= 3.0 way
    int numExtensions = 0;
    glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_NUM_EXTENSIONS, &numExtensions));

    if (numExtensions == 0)
        return result; // Empty vector

    for (int i = 0; i < numExtensions; ++i)
        if (const auto* extensionString = reinterpret_cast<const char*>(
                glCheckIgnoreWithFunc(glGetErrorFunc, glGetStringiFunc(GL_EXTENSIONS, static_cast<unsigned int>(i)))))
            result.emplace_back(extensionString);

    return result;
}


////////////////////////////////////////////////////////////
[[maybe_unused]] void GLAPIENTRY debugGLMessageCallback(
    GLenum       source,
    GLenum       type,
    unsigned int id,
    GLenum       severity,
    GLsizei /* length */,
    const char* message,
    const void* /* userParam */)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    auto& multiLineErr = priv::err(true /* multiLine */);

    multiLineErr << "---------------" << '\n' << "Debug message (" << id << "): " << message << "\nSource: ";

    // clang-format off
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             multiLineErr << "API";             break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   multiLineErr << "Window System";   break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: multiLineErr << "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     multiLineErr << "Third Party";     break;
        case GL_DEBUG_SOURCE_APPLICATION:     multiLineErr << "Application";     break;
        case GL_DEBUG_SOURCE_OTHER:           multiLineErr << "Other";           break;
    }
    // clang-format on

    multiLineErr << "\nType: ";

    // clang-format off
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               multiLineErr << "Error";                break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: multiLineErr << "Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  multiLineErr << "Undefined Behaviour";  break;
        case GL_DEBUG_TYPE_PORTABILITY:         multiLineErr << "Portability";          break;
        case GL_DEBUG_TYPE_PERFORMANCE:         multiLineErr << "Performance";          break;
        case GL_DEBUG_TYPE_MARKER:              multiLineErr << "Marker";               break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          multiLineErr << "Push Group";           break;
        case GL_DEBUG_TYPE_POP_GROUP:           multiLineErr << "Pop Group";            break;
        case GL_DEBUG_TYPE_OTHER:               multiLineErr << "Other";                break;
    }
    // clang-format on

    multiLineErr << "\nSeverity: ";

    // clang-format off
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         multiLineErr << "High";         break;
        case GL_DEBUG_SEVERITY_MEDIUM:       multiLineErr << "Medium";       break;
        case GL_DEBUG_SEVERITY_LOW:          multiLineErr << "Low";          break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: multiLineErr << "Notification"; break;
    }
    // clang-format on

    priv::err() << '\n';
}


////////////////////////////////////////////////////////////
thread_local constinit struct
{
    unsigned int     id{0u};
    priv::GlContext* ptr{nullptr};
} activeGlContext;

} // namespace


////////////////////////////////////////////////////////////
struct WindowContextImpl
{
    ////////////////////////////////////////////////////////////
    std::atomic<unsigned int> nextThreadLocalGlContextId{2u}; // 1 is reserved for shared context

    ////////////////////////////////////////////////////////////
    DerivedGlContextType sharedGlContext; //!< The hidden, inactive context that will be shared with all other contexts
    std::recursive_mutex sharedGlContextMutex;

    ////////////////////////////////////////////////////////////
    std::vector<std::string> extensions; //!< Supported OpenGL extensions

    ////////////////////////////////////////////////////////////
    struct UnsharedFrameBuffer
    {
        unsigned int                    glContextId{};
        unsigned int                    frameBufferId;
        WindowContext::UnsharedDeleteFn deleteFn;
    };

    std::mutex                       unsharedFrameBuffersMutex;
    std::vector<UnsharedFrameBuffer> unsharedFrameBuffers;

    ////////////////////////////////////////////////////////////
    base::Optional<priv::JoystickManager> joystickManager;
    base::Optional<priv::SensorManager>   sensorManager;

    ////////////////////////////////////////////////////////////
    template <typename... SharedGlContextArgs>
    explicit WindowContextImpl(SharedGlContextArgs&&... args) : sharedGlContext(SFML_BASE_FORWARD(args)...)
    {
    }
};


namespace
{
////////////////////////////////////////////////////////////
constinit base::Optional<WindowContextImpl> installedWindowContext;
constinit std::atomic<unsigned int> windowContextRC{0u};


////////////////////////////////////////////////////////////
WindowContextImpl& ensureInstalled()
{
    if (!installedWindowContext.hasValue()) [[unlikely]]
    {
        priv::err() << "`sf::WindowContext` not installed -- did you forget to create one in `main`?";
        base::abort();
    }

    return *installedWindowContext;
}

} // namespace


////////////////////////////////////////////////////////////
base::Optional<WindowContext> WindowContext::create()
{
    const auto fail = [](const char* what)
    {
        priv::err() << "Error creating `sf::WindowContext`: " << what;
        return base::nullOpt;
    };

    //
    // Ensure window context is not already installed
    if (installedWindowContext.hasValue())
        return fail("a `sf::WindowContext` object already exists");

    //
    // Install window context
    auto& wc = installedWindowContext.emplace(/* id */ 1u, /* shared */ nullptr);

    //
    // Define fatal signal handlers for the user that will display a stack trace
    std::signal(SIGSEGV, [](int) { priv::err() << "FATAL SIGNAL: SIGSEGV"; });
    std::signal(SIGILL, [](int) { priv::err() << "FATAL SIGNAL: SIGILL"; });
    std::signal(SIGABRT, [](int) { priv::err() << "FATAL SIGNAL: SIGABRT"; });
    std::signal(SIGFPE, [](int) { priv::err() << "FATAL SIGNAL: SIGFPE"; });

    //
    // Enable shader GL context
    SFML_BASE_ASSERT(!hasActiveThreadLocalOrSharedGlContext());

    if (!setActiveThreadLocalGlContextToSharedContext(true))
        return fail("could not enable shared context");

    SFML_BASE_ASSERT(isActiveGlContextSharedContext());

    //
    // Try to initialize shared GL context
    if (!wc.sharedGlContext.initialize(wc.sharedGlContext, ContextSettings{}))
        return fail("could not initialize shared context");

    //
    // Load extensions and entrypoints
    SFML_BASE_ASSERT(isActiveGlContextSharedContext());
    wc.extensions = loadExtensions(wc.sharedGlContext);
    loadGLEntryPointsViaGLAD();

#ifndef SFML_SYSTEM_EMSCRIPTEN
    // TODO P0: maybe conditionally enable depending on graphicscontext's debug ctx param?
    // or for emscripten, try to enable without glcheck and then drain gl errors
    glCheck(glEnable(GL_DEBUG_OUTPUT));
    glCheck(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
    glCheck(glDebugMessageCallback(debugGLMessageCallback, nullptr));

    // Retrieve the context version number
    const auto majorVersion = priv::getGLInteger(GL_MAJOR_VERSION);
    const auto minorVersion = priv::getGLInteger(GL_MINOR_VERSION);

    if ((majorVersion < 1) || ((majorVersion == 1) && (minorVersion < 1)))
        return fail("support for OpenGL 1.1 or greater required, ensure hardware acceleration is enabled");
#else
    // Need to drain errors here or subsequent assertion will fail on Emscripten, unsure why
    while (glGetError() != GL_NO_ERROR)
        ;
#endif

    SFML_BASE_ASSERT(glGetError() == GL_NO_ERROR);

    return base::makeOptional<WindowContext>(base::PassKey<WindowContext>{});
}


////////////////////////////////////////////////////////////
WindowContext::WindowContext(base::PassKey<WindowContext>&&)
{
    windowContextRC.fetch_add(1u, std::memory_order_relaxed);
}


////////////////////////////////////////////////////////////
WindowContext::WindowContext(base::PassKey<GraphicsContext>&&) : WindowContext(base::PassKey<WindowContext>{})
{
}


////////////////////////////////////////////////////////////
WindowContext::WindowContext(WindowContext&&) noexcept : WindowContext(base::PassKey<WindowContext>{})
{
}


////////////////////////////////////////////////////////////
WindowContext::~WindowContext()
{
    if (windowContextRC.fetch_sub(1u, std::memory_order_relaxed) > 1u)
        return;

    SFML_BASE_ASSERT(ensureInstalled().unsharedFrameBuffers.empty());

    // SFML_BASE_ASSERT(hasActiveThreadLocalOrSharedGlContext());

    activeGlContext.id  = 0u;
    activeGlContext.ptr = nullptr;

    SFML_BASE_ASSERT(!hasActiveThreadLocalOrSharedGlContext());

    installedWindowContext.reset();
}


////////////////////////////////////////////////////////////
void WindowContext::registerUnsharedFrameBuffer(unsigned int glContextId, unsigned int frameBufferId, UnsharedDeleteFn deleteFn)
{
    auto& wc = ensureInstalled();

    SFML_BASE_ASSERT(getActiveThreadLocalGlContextId() == glContextId);

    const std::lock_guard lock(wc.unsharedFrameBuffersMutex);
    wc.unsharedFrameBuffers.emplace_back(glContextId, frameBufferId, deleteFn);
}


////////////////////////////////////////////////////////////
void WindowContext::unregisterUnsharedFrameBuffer(unsigned int glContextId, unsigned int frameBufferId)
{
    auto& wc = ensureInstalled();

    // If we're not on the right context, wait for the cleanup later on
    if (getActiveThreadLocalGlContextId() != glContextId)
        return;

    const std::lock_guard lock(wc.unsharedFrameBuffersMutex);

    // Find the object in unshared objects and remove it if its associated context is currently active
    // Assume that the object has already been deleted with the right OpenGL delete call
    const auto iter = base::findIf(wc.unsharedFrameBuffers.begin(),
                                   wc.unsharedFrameBuffers.end(),
                                   [&](const WindowContextImpl::UnsharedFrameBuffer& obj)
                                   { return obj.glContextId == glContextId && obj.frameBufferId == frameBufferId; });

    if (iter != wc.unsharedFrameBuffers.end())
    {
        iter->deleteFn(iter->frameBufferId);
        wc.unsharedFrameBuffers.erase(iter);
    }
}


////////////////////////////////////////////////////////////
void WindowContext::cleanupUnsharedFrameBuffers(priv::GlContext& glContext)
{
    auto& wc = ensureInstalled();

    // Save the current context so we can restore it later
    priv::GlContext* glContextToRestore = activeGlContext.ptr;

    // Make this context active so resources can be freed
    if (!setActiveThreadLocalGlContext(glContext, true))
        priv::err() << "Could not enable GL context in GlContext::cleanupUnsharedFrameBuffers()";

    // Scope for lock guard
    {
        const std::lock_guard lock(wc.unsharedFrameBuffersMutex);

        // Destroy the unshared objects contained in this context
        for (auto iter = wc.unsharedFrameBuffers.begin(); iter != wc.unsharedFrameBuffers.end();)
        {
            if (iter->glContextId == glContext.m_id)
            {
                iter->deleteFn(iter->frameBufferId);
                iter = wc.unsharedFrameBuffers.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    // Make the originally active context active again
    if (glContextToRestore != nullptr)
    {
        if (!setActiveThreadLocalGlContext(*glContextToRestore, true))
            priv::err() << "Could not restore context in GlContext::cleanupUnsharedFrameBuffers()";
    }
    else
    {
        activeGlContext.id  = 0u;
        activeGlContext.ptr = nullptr;
    }
}


////////////////////////////////////////////////////////////
const priv::GlContext* WindowContext::getActiveThreadLocalGlContextPtr()
{
    ensureInstalled();
    return activeGlContext.ptr;
}


////////////////////////////////////////////////////////////
priv::JoystickManager& WindowContext::getJoystickManager()
{
    auto& wc = ensureInstalled();

    if (!wc.joystickManager.hasValue())
        wc.joystickManager.emplace();

    return *wc.joystickManager;
}


////////////////////////////////////////////////////////////
priv::SensorManager& WindowContext::getSensorManager()
{
    auto& wc = ensureInstalled();

    if (!wc.sensorManager.hasValue())
        wc.sensorManager.emplace();

    return *wc.sensorManager;
}


////////////////////////////////////////////////////////////
unsigned int WindowContext::getActiveThreadLocalGlContextId()
{
    ensureInstalled();
    return activeGlContext.id;
}


////////////////////////////////////////////////////////////
bool WindowContext::hasActiveThreadLocalGlContext()
{
    ensureInstalled();
    return activeGlContext.id != 0;
}


////////////////////////////////////////////////////////////
bool WindowContext::setActiveThreadLocalGlContext(priv::GlContext& glContext, bool active)
{
    ensureInstalled();

    // If `glContext` is already the active one on this thread, don't do anything
    if (active && glContext.m_id == activeGlContext.id)
    {
        SFML_BASE_ASSERT(activeGlContext.ptr == &glContext);
        return true;
    }

    // If `glContext` is not the active one on this thread, don't do anything
    if (!active && glContext.m_id != activeGlContext.id)
    {
        SFML_BASE_ASSERT(activeGlContext.ptr != &glContext);
        return true;
    }

    // Activate/deactivate the context
    if (!glContext.makeCurrent(active))
    {
        priv::err() << "`glContext.makeCurrent` failure in `WindowContext::setActiveThreadLocalGlContext`";
        return false;
    }

    activeGlContext.id  = active ? glContext.m_id : 0u;
    activeGlContext.ptr = active ? &glContext : nullptr;

    return true;
}


////////////////////////////////////////////////////////////
bool WindowContext::setActiveThreadLocalGlContextToSharedContext(bool active)
{
    auto& wc = ensureInstalled();
    return setActiveThreadLocalGlContext(wc.sharedGlContext, active);
}


////////////////////////////////////////////////////////////
void WindowContext::onGlContextDestroyed(priv::GlContext& glContext)
{
    ensureInstalled();

    // If `glContext` is not the active one on this thread, don't do anything
    if (glContext.m_id != activeGlContext.id)
        return;

    if (!setActiveThreadLocalGlContextToSharedContext(true))
    {
        priv::err() << "Failed to enable shared GL context in `WindowContext::onGlContextDestroyed`";
        SFML_BASE_ASSERT(false);
    }
}


////////////////////////////////////////////////////////////
bool WindowContext::hasActiveThreadLocalOrSharedGlContext()
{
    ensureInstalled();
    return activeGlContext.id != 0u && activeGlContext.ptr != nullptr;
}


////////////////////////////////////////////////////////////
bool WindowContext::isActiveGlContextSharedContext()
{
    auto& wc = ensureInstalled();
    return activeGlContext.id == 1u && activeGlContext.ptr == &wc.sharedGlContext;
}


////////////////////////////////////////////////////////////
bool WindowContext::isInstalled()
{
    return installedWindowContext.hasValue();
}


////////////////////////////////////////////////////////////
void WindowContext::loadGLEntryPointsViaGLAD()
{
#ifdef SFML_OPENGL_ES
    gladLoadGLES2(&getFunction);
#else
    gladLoadGL(&getFunction);
#endif
}


////////////////////////////////////////////////////////////
template <typename... GLContextArgs>
base::UniquePtr<priv::GlContext> WindowContext::createGlContextImpl(const ContextSettings& contextSettings,
                                                                    GLContextArgs&&... args)
{
    auto& wc = ensureInstalled();

    const std::lock_guard lock(wc.sharedGlContextMutex);

    if (!setActiveThreadLocalGlContextToSharedContext(true))
        priv::err() << "Error enabling shared GL context in WindowContext::createGlContext()";

    auto glContext = base::makeUnique<DerivedGlContextType>(wc.nextThreadLocalGlContextId.fetch_add(1u),
                                                            &wc.sharedGlContext,
                                                            SFML_BASE_FORWARD(args)...);

    if (!setActiveThreadLocalGlContextToSharedContext(false))
        priv::err() << "Error disabling shared GL context in WindowContext::createGlContext()";

    if (!setActiveThreadLocalGlContext(*glContext, true))
    {
        priv::err() << "Error enabling newly created GL context in GlContext::initialize()";
        return nullptr;
    }

    if (!glContext->initialize(wc.sharedGlContext, contextSettings))
    {
        priv::err() << "Error initializing newly created GL context in WindowContext::createGlContext()";
        return nullptr;
    }

    glContext->checkSettings(contextSettings);
    return glContext;
}


////////////////////////////////////////////////////////////
base::UniquePtr<priv::GlContext> WindowContext::createGlContext()
{
    return createGlContextImpl(ContextSettings{});
}


////////////////////////////////////////////////////////////
base::UniquePtr<priv::GlContext> WindowContext::createGlContext(const ContextSettings&  contextSettings,
                                                                const priv::WindowImpl& owner,
                                                                unsigned int            bitsPerPixel)
{
    return createGlContextImpl(contextSettings, contextSettings, owner, bitsPerPixel);
}


////////////////////////////////////////////////////////////
bool WindowContext::isExtensionAvailable(const char* name)
{
    auto& wc = ensureInstalled();
    return base::find(wc.extensions.begin(), wc.extensions.end(), name) != wc.extensions.end();
}


////////////////////////////////////////////////////////////
GlFunctionPointer WindowContext::getFunction(const char* name)
{
    return ensureInstalled().sharedGlContext.getFunction(name);
}

} // namespace sf
