////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/GLExtensions.hpp>
#include <SFML/Window/GlContext.hpp>
#include <SFML/Window/GraphicsContext.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Window/WindowImpl.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Time.hpp>

#include <SFML/Base/Macros.hpp>


namespace sf
{
////////////////////////////////////////////////////////////
struct Window::Window::Impl
{
    GraphicsContext*                 graphicsContext;
    base::UniquePtr<priv::GlContext> glContext;      //!< Platform-specific implementation of the OpenGL context
    Clock                            clock;          //!< Clock for measuring the elapsed time between frames
    Time                             frameTimeLimit; //!< Current framerate limit

    explicit Impl(GraphicsContext& theGraphicsContext, base::UniquePtr<priv::GlContext>&& theContext) :
    graphicsContext(&theGraphicsContext),
    glContext(SFML_BASE_MOVE(theContext))
    {
    }
};


////////////////////////////////////////////////////////////
template <typename TWindowBaseArg>
Window::Window(GraphicsContext&       graphicsContext,
               const ContextSettings& settings,
               TWindowBaseArg&&       windowBaseArg,
               unsigned int           bitsPerPixel) :
WindowBase(SFML_BASE_FORWARD(windowBaseArg)),
m_impl(graphicsContext, graphicsContext.createGlContext(settings, *WindowBase::m_impl, bitsPerPixel))
{
    // Perform common initializations
    SFML_BASE_ASSERT(m_impl->glContext != nullptr);

    // Setup default behaviors (to get a consistent behavior across different implementations)
    setVerticalSyncEnabled(false);
    setFramerateLimit(0);

    // Activate the window
    if (!setActive())
        priv::err() << "Failed to set window as active during initialization";
}


////////////////////////////////////////////////////////////
Window::Window(GraphicsContext&       graphicsContext,
               VideoMode              mode,
               const String&          title,
               Style                  style,
               State                  state,
               const ContextSettings& settings) :
Window(graphicsContext, settings, priv::WindowImpl::create(mode, title, style, state, settings), mode.bitsPerPixel)
{
}


////////////////////////////////////////////////////////////
Window::Window(GraphicsContext& graphicsContext, VideoMode mode, const String& title, State state, const ContextSettings& settings) :
Window(graphicsContext, mode, title, sf::Style::Default, state, settings)
{
}


////////////////////////////////////////////////////////////
Window::Window(GraphicsContext& graphicsContext, WindowHandle handle, const ContextSettings& settings) :
Window(graphicsContext, settings, handle, VideoMode::getDesktopMode().bitsPerPixel)
{
}


////////////////////////////////////////////////////////////
Window::Window(GraphicsContext&       graphicsContext,
               VideoMode              mode,
               const char*            title,
               Style                  style,
               State                  state,
               const ContextSettings& settings) :
Window(graphicsContext, mode, String(title), style, state, settings)
{
}


////////////////////////////////////////////////////////////
Window::Window(GraphicsContext& graphicsContext, VideoMode mode, const char* title, State state, const ContextSettings& settings) :
Window(graphicsContext, mode, String(title), state, settings)
{
}


////////////////////////////////////////////////////////////
Window::~Window() = default;


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

    if (m_impl->graphicsContext->setActiveThreadLocalGlContext(*m_impl->glContext, active))
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
}

} // namespace sf
