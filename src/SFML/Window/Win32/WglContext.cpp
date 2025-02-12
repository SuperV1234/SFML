#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/ContextSettings.hpp"
#include "SFML/Window/GlContext.hpp"
#include "SFML/Window/VideoMode.hpp"
#include "SFML/Window/VideoModeUtils.hpp"
#include "SFML/Window/Win32/Utils.hpp"
#include "SFML/Window/Win32/WglContext.hpp"
#include "SFML/Window/WindowContext.hpp"
#include "SFML/Window/WindowImpl.hpp"

#include "SFML/System/Err.hpp"

#include "SFML/Base/Abort.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/PtrDiffT.hpp"

#include <mutex>
#include <string> // Needed by `getErrorString`
#include <vector>
#include <windef.h>


// We check for this definition in order to avoid multiple definitions of GLAD
// entities during unity builds of SFML.
#ifndef GLAD_WGL_IMPLEMENTATION_INCLUDED
#define GLAD_WGL_IMPLEMENTATION_INCLUDED
#define GLAD_WGL_IMPLEMENTATION
#include <glad/wgl.h>
#endif

namespace
{
namespace WglContextImpl
{
// We use a different loader for wgl functions since we load them directly from OpenGL32.dll
[[nodiscard]] sf::GlFunctionPointer getOpenGl32Function(const char* name)
{
    static const HMODULE hModule = GetModuleHandleA("OpenGL32.dll");

    return hModule == nullptr
               ? nullptr
               : reinterpret_cast<sf::GlFunctionPointer>(GetProcAddress(hModule, reinterpret_cast<LPCSTR>(name)));
}


////////////////////////////////////////////////////////////
void ensureInit()
{
    static bool initialized = false;
    if (initialized)
        return;

    initialized = true;

    gladLoadWGL(nullptr, getOpenGl32Function);
}

////////////////////////////////////////////////////////////
struct ExtensionInitState
{
    bool                  initialized{false};
    sf::priv::WglContext* firstContext{nullptr}; // Should always be the shared context
};


////////////////////////////////////////////////////////////
[[nodiscard]] ExtensionInitState& getExtensionInitState()
{
    static ExtensionInitState result;
    return result;
}


////////////////////////////////////////////////////////////
void ensureWGLExtensionsInit(sf::priv::WglContext& wglContext, HDC deviceContext)
{
    ExtensionInitState& extensionInitState = getExtensionInitState();
    if (extensionInitState.initialized)
        return;

    extensionInitState.initialized  = true;
    extensionInitState.firstContext = &wglContext; // Should always be the shared context

    // We don't check the return value since the extension
    // flags are cleared even if loading fails
    gladLoadWGL(deviceContext, [](const char* name) { return getExtensionInitState().firstContext->getFunction(name); });
}

} // namespace WglContextImpl
} // namespace

// TODO P1: this is movable but destructor fails if moved-from

namespace sf::priv
{
////////////////////////////////////////////////////////////
WglContext::WglContext(unsigned int id, WglContext* shared, const ContextSettings& contextSettings, const SurfaceData& surfaceData) :
GlContext(id, contextSettings),
m_surfaceData(surfaceData),
m_context(createContext(m_settings, m_surfaceData, shared))
{
    WglContextImpl::ensureInit();

    if (shared == nullptr && m_context)
    {
        (void)makeCurrent(true);
        WglContextImpl::ensureWGLExtensionsInit(*this, m_surfaceData.deviceContext);
        (void)makeCurrent(false);
    }
}


////////////////////////////////////////////////////////////
WglContext::WglContext(unsigned int      id,
                       WglContext*       shared,
                       ContextSettings   contextSettings,
                       const WindowImpl& owner,
                       unsigned int      bitsPerPixel) :
WglContext(id, shared, contextSettings, createSurface(contextSettings, owner.getNativeHandle(), bitsPerPixel))
{
}


////////////////////////////////////////////////////////////
WglContext::WglContext(unsigned int id, WglContext* shared) :
WglContext(id,
           shared,
           ContextSettings{},
           createSurface(ContextSettings{}, shared, {1u, 1u}, VideoModeUtils::getDesktopMode().bitsPerPixel))
{
}


////////////////////////////////////////////////////////////
WglContext::~WglContext()
{
    // Notify unshared OpenGL resources of context destruction
    cleanupUnsharedFrameBuffers();

    // Destroy the OpenGL context
    if (m_context)
    {
        [[maybe_unused]] const bool rc = wglMakeCurrent(m_surfaceData.deviceContext, nullptr);
        SFML_BASE_ASSERT(rc == TRUE);

        wglDeleteContext(m_context);
    }

    // Destroy the device context
    if (m_surfaceData.deviceContext)
    {
        if (m_surfaceData.pbuffer)
        {
            wglReleasePbufferDCARB(m_surfaceData.pbuffer, m_surfaceData.deviceContext);
            wglDestroyPbufferARB(m_surfaceData.pbuffer);
        }
        else
        {
            ReleaseDC(m_surfaceData.window, m_surfaceData.deviceContext);
        }
    }

    // Destroy the window if we own it
    if (m_surfaceData.window && m_surfaceData.ownsWindow)
        DestroyWindow(m_surfaceData.window);
}


////////////////////////////////////////////////////////////
GlFunctionPointer WglContext::getFunction(const char* name) const
{
    auto address = reinterpret_cast<GlFunctionPointer>(wglGetProcAddress(reinterpret_cast<LPCSTR>(name)));

    if (address)
    {
        // Test whether the returned value is a valid error code
        auto errorCode = reinterpret_cast<base::PtrDiffT>(address);

        if ((errorCode != -1) && (errorCode != 1) && (errorCode != 2) && (errorCode != 3))
            return address;
    }

    return WglContextImpl::getOpenGl32Function(name);
}


////////////////////////////////////////////////////////////
bool WglContext::makeCurrent(bool current)
{
    const char* const currentStr = (current ? "activate" : "deactivate");

    if (!m_surfaceData.deviceContext)
    {
        err() << "Failed to " << currentStr << " WGL context: null surface data";
        return false;
    }

    if (!m_context)
    {
        err() << "Failed to " << currentStr << " WGL context: null context";
        return false;
    }

    if (wglMakeCurrent(m_surfaceData.deviceContext, current ? m_context : nullptr) == FALSE)
    {
        err() << "Failed to " << currentStr << " WGL context: " << getErrorString(GetLastError());
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////
void WglContext::display()
{
    if (m_surfaceData.deviceContext && m_context)
        SwapBuffers(m_surfaceData.deviceContext);
}


////////////////////////////////////////////////////////////
void WglContext::setVerticalSyncEnabled(bool enabled)
{
    // Make sure that extensions are initialized
    SFML_BASE_ASSERT(WglContextImpl::getExtensionInitState().initialized);

    if (GLAD_WGL_EXT_swap_control)
    {
        if (wglSwapIntervalEXT(enabled) == FALSE)
            err() << "Setting vertical sync failed: " << getErrorString(GetLastError());

        return;
    }

    static bool warned = false;

    if (!warned)
    {
        // wglSwapIntervalEXT not supported
        err() << "Setting vertical sync not supported";

        warned = true;
    }
}


////////////////////////////////////////////////////////////
int WglContext::selectBestPixelFormat(HDC deviceContext, unsigned int bitsPerPixel, const ContextSettings& contextSettings, bool pbuffer)
{
    // Selecting a pixel format can be an expensive process on some implementations
    // Since the same pixel format should always be selected for a specific combination of inputs
    // we can cache the result of the lookup instead of having to perform it multiple times for the same inputs
    struct PixelFormatCacheEntry
    {
        unsigned int bitsPerPixel{};
        unsigned int depthBits{};
        unsigned int stencilBits{};
        unsigned int antiAliasingLevel{};
        bool         pbuffer{};
        int          bestFormat{};
    };

    static std::mutex                         cacheMutex;
    static std::vector<PixelFormatCacheEntry> pixelFormatCache;

    // Check if we have already previously found a pixel format for
    // the current inputs and return it if one has been previously found
    {
        const std::lock_guard lock(cacheMutex);

        for (const auto& entry : pixelFormatCache)
        {
            if (bitsPerPixel == entry.bitsPerPixel && contextSettings.depthBits == entry.depthBits &&
                contextSettings.stencilBits == entry.stencilBits &&
                contextSettings.antiAliasingLevel == entry.antiAliasingLevel && pbuffer == entry.pbuffer)
                return entry.bestFormat;
        }
    }

    WglContextImpl::ensureInit();

    // Find a suitable pixel format -- first try with wglChoosePixelFormatARB
    int bestFormat = 0;
    if (GLAD_WGL_ARB_pixel_format)
    {
        // Define the basic attributes we want for our window
        constexpr int intAttributes[]{WGL_DRAW_TO_WINDOW_ARB,
                                      GL_TRUE,
                                      WGL_SUPPORT_OPENGL_ARB,
                                      GL_TRUE,
                                      WGL_DOUBLE_BUFFER_ARB,
                                      GL_TRUE,
                                      WGL_PIXEL_TYPE_ARB,
                                      WGL_TYPE_RGBA_ARB,
                                      0,
                                      0};

        // Check how many formats are supporting our requirements
        int formats[512];
        UINT nbFormats = 0; // We must initialize to 0 otherwise broken drivers might fill with garbage in the following call
        const bool isValid = wglChoosePixelFormatARB(deviceContext, intAttributes, nullptr, 512, formats, &nbFormats) != FALSE;

        if (!isValid)
            err() << "Failed to enumerate pixel formats: " << getErrorString(GetLastError());

        // Get the best format among the returned ones
        if (isValid && (nbFormats > 0))
        {
            int bestScore = 0x7FFFFFFF;
            for (UINT i = 0; i < nbFormats; ++i)
            {
                // Extract the components of the current format
                int           values[7];
                constexpr int attributes[]{WGL_RED_BITS_ARB,
                                           WGL_GREEN_BITS_ARB,
                                           WGL_BLUE_BITS_ARB,
                                           WGL_ALPHA_BITS_ARB,
                                           WGL_DEPTH_BITS_ARB,
                                           WGL_STENCIL_BITS_ARB,
                                           WGL_ACCELERATION_ARB};

                if (wglGetPixelFormatAttribivARB(deviceContext, formats[i], PFD_MAIN_PLANE, 7, attributes, values) == FALSE)
                {
                    err() << "Failed to retrieve pixel format information: " << getErrorString(GetLastError());
                    break;
                }

                int sampleValues[2] = {0, 0};
                if (GLAD_WGL_ARB_multisample)
                {
                    constexpr int sampleAttributes[]{WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB};

                    if (wglGetPixelFormatAttribivARB(deviceContext, formats[i], PFD_MAIN_PLANE, 2, sampleAttributes, sampleValues) ==
                        FALSE)
                    {
                        err() << "Failed to retrieve pixel format multisampling information: "
                              << getErrorString(GetLastError());
                        break;
                    }
                }

                int sRgbCapableValue = 0;
                if (GLAD_WGL_ARB_framebuffer_sRGB || GLAD_WGL_EXT_framebuffer_sRGB)
                {
                    constexpr int sRgbCapableAttribute = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;

                    if (wglGetPixelFormatAttribivARB(deviceContext, formats[i], PFD_MAIN_PLANE, 1, &sRgbCapableAttribute, &sRgbCapableValue) ==
                        FALSE)
                    {
                        err() << "Failed to retrieve pixel format sRGB capability information: "
                              << getErrorString(GetLastError());
                        break;
                    }
                }

                if (pbuffer)
                {
                    constexpr int pbufferAttributes[]{WGL_DRAW_TO_PBUFFER_ARB};

                    int pbufferValue = 0;

                    if (wglGetPixelFormatAttribivARB(deviceContext, formats[i], PFD_MAIN_PLANE, 1, pbufferAttributes, &pbufferValue) ==
                        FALSE)
                    {
                        err() << "Failed to retrieve pixel format pbuffer information: " << getErrorString(GetLastError());
                        break;
                    }

                    if (pbufferValue != GL_TRUE)
                        continue;
                }

                // Evaluate the current configuration
                const int color = values[0] + values[1] + values[2] + values[3];
                const int score = GlContext::evaluateFormat(bitsPerPixel,
                                                            contextSettings,
                                                            color,
                                                            values[4],
                                                            values[5],
                                                            sampleValues[0] ? sampleValues[1] : 0,
                                                            values[6] == WGL_FULL_ACCELERATION_ARB,
                                                            sRgbCapableValue == TRUE);

                // Keep it if it's better than the current best
                if (score < bestScore)
                {
                    bestScore  = score;
                    bestFormat = formats[i];
                }
            }
        }
    }

    // Find a pixel format with ChoosePixelFormat, if wglChoosePixelFormatARB is not supported
    // ChoosePixelFormat doesn't support pbuffers
    if ((bestFormat == 0) && !pbuffer)
    {
        // Setup a pixel format descriptor from the rendering settings
        PIXELFORMATDESCRIPTOR descriptor;
        ZeroMemory(&descriptor, sizeof(descriptor));
        descriptor.nSize        = sizeof(descriptor);
        descriptor.nVersion     = 1;
        descriptor.iLayerType   = PFD_MAIN_PLANE;
        descriptor.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        descriptor.iPixelType   = PFD_TYPE_RGBA;
        descriptor.cColorBits   = static_cast<BYTE>(bitsPerPixel);
        descriptor.cDepthBits   = static_cast<BYTE>(contextSettings.depthBits);
        descriptor.cStencilBits = static_cast<BYTE>(contextSettings.stencilBits);
        descriptor.cAlphaBits   = bitsPerPixel == 32 ? 8 : 0;

        // Get the pixel format that best matches our requirements
        bestFormat = ChoosePixelFormat(deviceContext, &descriptor);
    }

    // If we get this far, the format wasn't found in the cache so add it here
    {
        const std::lock_guard lock(cacheMutex);

        pixelFormatCache.emplace_back(bitsPerPixel,
                                      contextSettings.depthBits,
                                      contextSettings.stencilBits,
                                      contextSettings.antiAliasingLevel,
                                      pbuffer,
                                      bestFormat);
    }

    return bestFormat;
}


////////////////////////////////////////////////////////////
void WglContext::setDevicePixelFormat(const ContextSettings& contextSettings, HDC deviceContext, unsigned int bitsPerPixel)
{
    const int bestFormat = selectBestPixelFormat(deviceContext, bitsPerPixel, contextSettings);

    if (bestFormat == 0)
    {
        err() << "Failed to find a suitable pixel format for device context: " << getErrorString(GetLastError()) << '\n'
              << "Cannot create OpenGL context";

        return;
    }

    // Extract the depth and stencil bits from the chosen format
    PIXELFORMATDESCRIPTOR actualFormat;
    actualFormat.nSize    = sizeof(actualFormat);
    actualFormat.nVersion = 1;
    DescribePixelFormat(deviceContext, bestFormat, sizeof(actualFormat), &actualFormat);

    // Set the chosen pixel format
    if (SetPixelFormat(deviceContext, bestFormat, &actualFormat) == FALSE)
    {
        err() << "Failed to set pixel format for device context: " << getErrorString(GetLastError()) << '\n'
              << "Cannot create OpenGL context";

        return;
    }
}


////////////////////////////////////////////////////////////
void WglContext::updateSettingsFromPixelFormat(ContextSettings& contextSettings, HDC deviceContext)
{
    const int format = GetPixelFormat(deviceContext);

    if (format == 0)
    {
        err() << "Failed to get selected pixel format: " << getErrorString(GetLastError());
        return;
    }

    PIXELFORMATDESCRIPTOR actualFormat;
    actualFormat.nSize    = sizeof(actualFormat);
    actualFormat.nVersion = 1;

    if (DescribePixelFormat(deviceContext, format, sizeof(actualFormat), &actualFormat) == 0)
    {
        err() << "Failed to retrieve pixel format information: " << getErrorString(GetLastError());
        return;
    }

    // Detect if we are running using the generic GDI implementation and warn
    if (actualFormat.dwFlags & PFD_GENERIC_FORMAT)
    {
        err() << "Warning: Detected \"Microsoft Corporation GDI Generic\" OpenGL implementation";

        // Detect if the generic GDI implementation is not accelerated
        if (!(actualFormat.dwFlags & PFD_GENERIC_ACCELERATED))
            err() << "Warning: The \"Microsoft Corporation GDI Generic\" OpenGL implementation is not "
                     "hardware-accelerated";

        base::abort();
    }

    if (!GLAD_WGL_ARB_pixel_format)
    {
        contextSettings.depthBits         = actualFormat.cDepthBits;
        contextSettings.stencilBits       = actualFormat.cStencilBits;
        contextSettings.antiAliasingLevel = 0;

        return;
    }

    constexpr int attributes[]{WGL_DEPTH_BITS_ARB, WGL_STENCIL_BITS_ARB};
    int           values[2];

    if (wglGetPixelFormatAttribivARB(deviceContext, format, PFD_MAIN_PLANE, 2, attributes, values) == TRUE)
    {
        contextSettings.depthBits   = static_cast<unsigned int>(values[0]);
        contextSettings.stencilBits = static_cast<unsigned int>(values[1]);
    }
    else
    {
        err() << "Failed to retrieve pixel format information: " << getErrorString(GetLastError());

        contextSettings.depthBits   = actualFormat.cDepthBits;
        contextSettings.stencilBits = actualFormat.cStencilBits;
    }

    if (GLAD_WGL_ARB_multisample)
    {
        constexpr int sampleAttributes[]{WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB};
        int           sampleValues[2];

        if (wglGetPixelFormatAttribivARB(deviceContext, format, PFD_MAIN_PLANE, 2, sampleAttributes, sampleValues) == TRUE)
        {
            contextSettings.antiAliasingLevel = static_cast<unsigned int>(sampleValues[0] ? sampleValues[1] : 0);
        }
        else
        {
            err() << "Failed to retrieve pixel format multisampling information: " << getErrorString(GetLastError());

            contextSettings.antiAliasingLevel = 0;
        }
    }
    else
    {
        contextSettings.antiAliasingLevel = 0;
    }

    if (GLAD_WGL_ARB_framebuffer_sRGB || GLAD_WGL_EXT_framebuffer_sRGB)
    {
        constexpr int sRgbCapableAttribute = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
        int           sRgbCapableValue     = 0;

        if (wglGetPixelFormatAttribivARB(deviceContext, format, PFD_MAIN_PLANE, 1, &sRgbCapableAttribute, &sRgbCapableValue) ==
            TRUE)
        {
            contextSettings.sRgbCapable = (sRgbCapableValue == TRUE);
        }
        else
        {
            err() << "Failed to retrieve pixel format sRGB capability information: " << getErrorString(GetLastError());

            contextSettings.sRgbCapable = false;
        }
    }
    else
    {
        contextSettings.sRgbCapable = false;
    }
}


////////////////////////////////////////////////////////////
WglContext::SurfaceData WglContext::createSurface(ContextSettings contextSettings,
                                                  WglContext*     shared,
                                                  Vector2u        size,
                                                  unsigned int    bitsPerPixel)
{
    // If pbuffers are not available we use a hidden window as the off-screen surface to draw to
    const auto createHiddenWindow = [](ContextSettings& xSettings, const Vector2u& xSize, unsigned int xBitsPerPixel) -> SurfaceData
    {
        // We can't create a memory DC, the resulting context wouldn't be compatible
        // with other contexts and thus wglShareLists would always fail

        // Create the hidden window
        const HWND windowHandle = CreateWindowA("STATIC",
                                                "",
                                                WS_POPUP | WS_DISABLED,
                                                0,
                                                0,
                                                static_cast<int>(xSize.x),
                                                static_cast<int>(xSize.y),
                                                nullptr,
                                                nullptr,
                                                GetModuleHandle(nullptr),
                                                nullptr);
        // Set window properties
        ShowWindow(windowHandle, SW_HIDE);

        // Get device context
        const HDC deviceContextHandle = GetDC(windowHandle);

        // Set the pixel format of the device context
        setDevicePixelFormat(xSettings, deviceContextHandle, xBitsPerPixel);

        // Update context settings from the selected pixel format
        updateSettingsFromPixelFormat(xSettings, deviceContextHandle);

        return SurfaceData{.window = windowHandle, .pbuffer = {}, .deviceContext = deviceContextHandle, .ownsWindow = true};
    };

    // Check if the shared context already exists and pbuffers are supported
    if (!shared || !shared->m_surfaceData.deviceContext || !GLAD_WGL_ARB_pbuffer)
        return createHiddenWindow(contextSettings, size, bitsPerPixel);

    const int bestFormat = selectBestPixelFormat(shared->m_surfaceData.deviceContext, bitsPerPixel, contextSettings, true);

    if (bestFormat <= 0)
        return createHiddenWindow(contextSettings, size, bitsPerPixel);

    const int attributes[]{0, 0};

    const HPBUFFERARB pbuffer = wglCreatePbufferARB(shared->m_surfaceData.deviceContext,
                                                    bestFormat,
                                                    static_cast<int>(size.x),
                                                    static_cast<int>(size.y),
                                                    attributes);

    if (!pbuffer)
    {
        err() << "Failed to create pixel buffer: " << getErrorString(GetLastError());
        return createHiddenWindow(contextSettings, size, bitsPerPixel);
    }

    const HDC deviceContextHandle = wglGetPbufferDCARB(pbuffer);

    if (!deviceContextHandle)
    {
        err() << "Failed to retrieve pixel buffer device context: " << getErrorString(GetLastError());

        wglDestroyPbufferARB(pbuffer);
        return createHiddenWindow(contextSettings, size, bitsPerPixel);
    }

    // Update context settings from the selected pixel format
    updateSettingsFromPixelFormat(contextSettings, deviceContextHandle);

    return SurfaceData{.window        = shared->m_surfaceData.window,
                       .pbuffer       = pbuffer,
                       .deviceContext = deviceContextHandle,
                       .ownsWindow    = false};
}


////////////////////////////////////////////////////////////
WglContext::SurfaceData WglContext::createSurface(ContextSettings& contextSettings, HWND window, unsigned int bitsPerPixel)
{
    const HDC deviceContextHandle = GetDC(window);

    // Set the pixel format of the device context
    setDevicePixelFormat(contextSettings, deviceContextHandle, bitsPerPixel);

    // Update context settings from the selected pixel format
    updateSettingsFromPixelFormat(contextSettings, deviceContextHandle);

    return SurfaceData{.window = window, .pbuffer = {}, .deviceContext = deviceContextHandle, .ownsWindow = false};
}


////////////////////////////////////////////////////////////
HGLRC WglContext::createContext(ContextSettings& contextSettings, const SurfaceData& surfaceData, WglContext* shared)
{
    const auto createContextViaAttributes =
        [](ContextSettings&           xSettings,
           const SurfaceData&         xSurfaceData,
           WglContext*                xShared,
           HGLRC                      sharedContext,
           ContextSettings::Attribute originalAttributeFlags) -> HGLRC
    {
        HGLRC result{};

        // Create the OpenGL context -- first try using wglCreateContextAttribsARB
        while (!result && xSettings.majorVersion)
        {
            std::vector<int> attributes;

            // Check if the user requested a specific context version (anything > 1.1)
            if ((xSettings.majorVersion > 1) || ((xSettings.majorVersion == 1) && (xSettings.minorVersion > 1)))
            {
                attributes.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
                attributes.push_back(static_cast<int>(xSettings.majorVersion));
                attributes.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
                attributes.push_back(static_cast<int>(xSettings.minorVersion));
            }

            // Check if setting the profile is supported
            if (GLAD_WGL_ARB_create_context_profile)
            {
                const int profile = !!(xSettings.attributeFlags & ContextSettings::Attribute::Core)
                                        ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB
                                        : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;

                const int debug = !!(xSettings.attributeFlags & ContextSettings::Attribute::Debug)
                                      ? WGL_CONTEXT_DEBUG_BIT_ARB
                                      : 0;

                attributes.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
                attributes.push_back(profile);
                attributes.push_back(WGL_CONTEXT_FLAGS_ARB);
                attributes.push_back(debug);
            }
            else
            {
                if (!!(xSettings.attributeFlags & ContextSettings::Attribute::Core) ||
                    !!(xSettings.attributeFlags & ContextSettings::Attribute::Debug))
                    err() << "Selecting a profile during context creation is not supported,"
                          << "disabling compatibility and debug";

                xSettings.attributeFlags = ContextSettings::Attribute::Default;
            }

            // Append the terminating 0
            attributes.push_back(0);
            attributes.push_back(0);

            if (sharedContext)
            {
                static std::recursive_mutex mutex;
                const std::lock_guard       lock(mutex);

                if (wglMakeCurrent(xShared->m_surfaceData.deviceContext, nullptr) == FALSE)
                {
                    err() << "Failed to deactivate shared context before sharing: " << getErrorString(GetLastError());
                    return {};
                }
            }

            // Create the context
            result = wglCreateContextAttribsARB(xSurfaceData.deviceContext, sharedContext, attributes.data());

            if (result)
                return result;

            err() << "Could not create context with version " << xSettings.majorVersion << "." << xSettings.minorVersion;

            // If we couldn't create the context, first try disabling flags,
            // then lower the version number and try again -- stop at 0.0
            // Invalid version numbers will be generated by this algorithm (like 3.9), but we really don't care
            if (xSettings.attributeFlags != ContextSettings::Attribute::Default)
            {
                xSettings.attributeFlags = ContextSettings::Attribute::Default;
            }
            else if (xSettings.minorVersion > 0)
            {
                // If the minor version is not 0, we decrease it and try again
                --xSettings.minorVersion;

                xSettings.attributeFlags = originalAttributeFlags;
            }
            else
            {
                // If the minor version is 0, we decrease the major version
                --xSettings.majorVersion;
                xSettings.minorVersion = 9;

                xSettings.attributeFlags = originalAttributeFlags;
            }
        }

        err() << "WglContext::createContext failure: unable to create context via attributes";
        return {};
    };

    // We can't create an OpenGL context if we don't have a DC
    if (!surfaceData.deviceContext)
    {
        err() << "WglContext::createContext failure: no device context";
        return {};
    }

    // Get the context to share display lists with
    HGLRC sharedContext = shared ? shared->m_context : nullptr;

    // Get a working copy of the context settings attribute flags
    const ContextSettings::Attribute originalAttributeFlags = contextSettings.attributeFlags;

    if (GLAD_WGL_ARB_create_context)
    {
        HGLRC resultViaAttributes = createContextViaAttributes(contextSettings, surfaceData, shared, sharedContext, originalAttributeFlags);

        if (resultViaAttributes)
            return resultViaAttributes;
    }

    // set the context version to 1.1 (arbitrary) and disable flags
    contextSettings.majorVersion   = 1;
    contextSettings.minorVersion   = 1;
    contextSettings.attributeFlags = ContextSettings::Attribute::Default;

    // If wglCreateContextAttribsARB failed or unavailable, use wglCreateContext
    HGLRC result = wglCreateContext(surfaceData.deviceContext);
    if (!result)
    {
        err() << "Failed to create an OpenGL context for this window: " << getErrorString(GetLastError());
        return {};
    }

    // Share this context with others
    if (sharedContext)
    {
        // wglShareLists doesn't seem to be thread-safe
        static std::recursive_mutex mutex;
        const std::lock_guard       lock(mutex);

        if (wglMakeCurrent(shared->m_surfaceData.deviceContext, nullptr) == FALSE)
        {
            err() << "Failed to deactivate shared context before sharing: " << getErrorString(GetLastError());
            return {};
        }

        if (wglShareLists(sharedContext, result) == FALSE)
        {
            err() << "Failed to share the OpenGL context: " << getErrorString(GetLastError());
            return {};
        }
    }

    SFML_BASE_ASSERT(result != HGLRC{});
    return result;
}

} // namespace sf::priv
