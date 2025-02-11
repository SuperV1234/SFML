#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/GlContext.hpp"
#include "SFML/Window/VideoMode.hpp"
#include "SFML/Window/VideoModeUtils.hpp"
#include "SFML/Window/Window.hpp"
#include "SFML/Window/WindowContext.hpp"
#include "SFML/Window/WindowImpl.hpp"
#include "SFML/Window/WindowSettings.hpp"

#include "SFML/System/Clock.hpp"
#include "SFML/System/Err.hpp"
#include "SFML/System/Sleep.hpp"
#include "SFML/System/Time.hpp"

#include "SFML/Base/Macros.hpp"

#ifdef SFML_SYSTEM_EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif


namespace sf
{
////////////////////////////////////////////////////////////
struct Window::Window::Impl
{
    base::UniquePtr<priv::GlContext> glContext;      //!< Platform-specific implementation of the OpenGL context
    Clock                            clock;          //!< Clock for measuring the elapsed time between frames
    Time                             frameTimeLimit; //!< Current framerate limit

    explicit Impl(base::UniquePtr<priv::GlContext>&& theContext) : glContext(SFML_BASE_MOVE(theContext))
    {
    }
};


////////////////////////////////////////////////////////////
template <typename... TWindowBaseArgs>
Window::Window(const Settings& windowSettings, unsigned int bitsPerPixel, TWindowBaseArgs&&... windowBaseArgs) :
WindowBase(SFML_BASE_FORWARD(windowBaseArgs)...),
m_impl(WindowContext::createGlContext(windowSettings.contextSettings, getWindowImpl(), bitsPerPixel))
{
    // Perform common initializations
    SFML_BASE_ASSERT(m_impl->glContext != nullptr);

    // Setup default behaviors (to get a consistent behavior across different implementations)
    setVerticalSyncEnabled(windowSettings.vsync);
    setFramerateLimit(windowSettings.frametimeLimit);

    // Activate the window
    if (!setActive())
        priv::err() << "Failed to set window as active during initialization";
}


////////////////////////////////////////////////////////////
Window::Window(const Settings& windowSettings) :
Window(windowSettings, windowSettings.bitsPerPixel, priv::WindowImpl::create(windowSettings))
{
}


////////////////////////////////////////////////////////////
Window::Window(WindowHandle handle, const ContextSettings& contextSettings) :
Window(WindowSettings{.size{}, .contextSettings = contextSettings}, VideoModeUtils::getDesktopMode().bitsPerPixel, handle)
{
}


////////////////////////////////////////////////////////////
bool Window::isMovedFrom() const
{
    return m_impl->glContext == nullptr;
}


////////////////////////////////////////////////////////////
Window::~Window()
{
    if (isMovedFrom())
        return;

    // Need to activate window context during destruction to avoid GL errors
    [[maybe_unused]] const bool rc = setActive(true);
    SFML_BASE_ASSERT(rc);
}


////////////////////////////////////////////////////////////
Window::Window(Window&&) noexcept = default;


////////////////////////////////////////////////////////////
Window& Window::operator=(Window&&) noexcept = default;


////////////////////////////////////////////////////////////
const ContextSettings& Window::getSettings() const
{
    SFML_BASE_ASSERT(m_impl->glContext != nullptr);
    return m_impl->glContext->getSettings();
}


////////////////////////////////////////////////////////////
void Window::setVerticalSyncEnabled(bool enabled)
{
    if (setActive())
        m_impl->glContext->setVerticalSyncEnabled(enabled);
}


////////////////////////////////////////////////////////////
void Window::setFramerateLimit(unsigned int limit)
{
    m_impl->frameTimeLimit = limit > 0 ? seconds(1.f / static_cast<float>(limit)) : Time::Zero;
}


////////////////////////////////////////////////////////////
bool Window::setActive(bool active) const
{
    SFML_BASE_ASSERT(m_impl->glContext != nullptr);

    if (WindowContext::setActiveThreadLocalGlContext(*m_impl->glContext, active))
        return true;

    priv::err() << "Failed to activate the window's context";
    return false;
}


////////////////////////////////////////////////////////////
void Window::display()
{
    // Display the backbuffer on screen
    if (setActive())
        m_impl->glContext->display();

    // Limit the framerate if needed
    if (m_impl->frameTimeLimit != Time::Zero)
    {
        sleep(m_impl->frameTimeLimit - m_impl->clock.getElapsedTime());
        m_impl->clock.restart();
    }

#ifdef SFML_SYSTEM_EMSCRIPTEN
    emscripten_sleep(0u);
#endif
}

} // namespace sf
