#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/ContextSettings.hpp"
#include "SFML/Window/GlContext.hpp"

#include "SFML/System/Vector2.hpp"

// Emscripten does not use GLAD
#ifdef SFML_SYSTEM_EMSCRIPTEN
#define KHRONOS_APIENTRY
#include <EGL/egl.h>
#else
#include <glad/egl.h>
#endif

#if defined(SFML_SYSTEM_LINUX) && !defined(SFML_USE_DRM)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif


namespace sf::priv
{
class WindowImpl;

class EglContext : public GlContext
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Create a new context, not associated to a window
    ///
    /// \param shared Context to share the new one with (can be a null pointer)
    ///
    ////////////////////////////////////////////////////////////
    explicit EglContext(WindowContext& windowContext, unsigned int id, EglContext* shared);

    ////////////////////////////////////////////////////////////
    /// \brief Create a new context attached to a window
    ///
    /// \param shared       Context to share the new one with
    /// \param settings     Creation parameters
    /// \param owner        Pointer to the owner window
    /// \param bitsPerPixel Pixel depth, in bits per pixel
    ///
    ////////////////////////////////////////////////////////////
    explicit EglContext(WindowContext&         windowContext,
                        unsigned int           id,
                        EglContext*            shared,
                        const ContextSettings& contextSettings,
                        const WindowImpl&      owner,
                        unsigned int           bitsPerPixel);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~EglContext() override;

    ////////////////////////////////////////////////////////////
    /// \brief Get the address of an OpenGL function
    ///
    /// \param name Name of the function to get the address of
    ///
    /// \return Address of the OpenGL function, 0 on failure
    ///
    ////////////////////////////////////////////////////////////
    GlFunctionPointer getFunction(const char* name) const;

    ////////////////////////////////////////////////////////////
    /// \brief Activate the context as the current target
    ///        for rendering
    ///
    /// \param current Whether to make the context current or no longer current
    ///
    /// \return True on success, false if any error happened
    ///
    ////////////////////////////////////////////////////////////
    bool makeCurrent(bool current) override;

    ////////////////////////////////////////////////////////////
    /// \brief Display what has been rendered to the context so far
    ///
    ////////////////////////////////////////////////////////////
    void display() override;

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable vertical synchronization
    ///
    /// Activating vertical synchronization will limit the number
    /// of frames displayed to the refresh rate of the monitor.
    /// This can avoid some visual artifacts, and limit the framerate
    /// to a good value (but not constant across different computers).
    ///
    /// \param enabled: True to enable v-sync, false to deactivate
    ///
    ////////////////////////////////////////////////////////////
    void setVerticalSyncEnabled(bool enabled) override;

    ////////////////////////////////////////////////////////////
    /// \brief Create the context
    ///
    /// \param shared       Context to share the new one with (can be a null pointer)
    /// \param bitsPerPixel Pixel depth, in bits per pixel
    /// \param settings     Creation parameters
    ///
    ////////////////////////////////////////////////////////////
    void createContext(EglContext* shared);

    ////////////////////////////////////////////////////////////
    /// \brief Create the EGL surface
    ///
    /// This function must be called when the activity (re)start, or
    /// when the orientation change.
    ///
    /// \param window: The native window type
    ///
    ////////////////////////////////////////////////////////////
    void createSurface(EGLNativeWindowType window);

    ////////////////////////////////////////////////////////////
    /// \brief Destroy the EGL surface
    ///
    /// This function must be called when the activity is stopped, or
    /// when the orientation change.
    ///
    ////////////////////////////////////////////////////////////
    void destroySurface();

    ////////////////////////////////////////////////////////////
    /// \brief Get the best EGL visual for a given set of video settings
    ///
    /// \param display      EGL display
    /// \param bitsPerPixel Pixel depth, in bits per pixel
    /// \param settings     Requested context settings
    ///
    /// \return The best EGL config
    ///
    ////////////////////////////////////////////////////////////
    static EGLConfig getBestConfig(EGLDisplay display, unsigned int bitsPerPixel, const ContextSettings& contextSettings);

#if defined(SFML_SYSTEM_LINUX) && !defined(SFML_USE_DRM)
    ////////////////////////////////////////////////////////////
    /// \brief Select the best EGL visual for a given set of settings
    ///
    /// \param display      X display
    /// \param bitsPerPixel Pixel depth, in bits per pixel
    /// \param settings     Requested context settings
    ///
    /// \return The best visual
    ///
    ////////////////////////////////////////////////////////////
    static XVisualInfo selectBestVisual(::Display* display, unsigned int bitsPerPixel, const ContextSettings& contextSettings);
#endif

private:
    ////////////////////////////////////////////////////////////
    /// \brief Helper to copy the picked EGL configuration
    ////////////////////////////////////////////////////////////
    void updateSettings();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    EGLDisplay m_display{EGL_NO_DISPLAY}; //!< The internal EGL display
    EGLContext m_context{EGL_NO_CONTEXT}; //!< The internal EGL context
    EGLSurface m_surface{EGL_NO_SURFACE}; //!< The internal EGL surface
    EGLConfig  m_config{};                //!< The internal EGL config
};

} // namespace sf::priv
