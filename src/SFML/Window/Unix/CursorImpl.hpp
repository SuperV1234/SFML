#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/Cursor.hpp"

#include "SFML/System/Vector2.hpp"

#include <X11/Xlib.h>

#include <memory>


namespace sf::priv
{
////////////////////////////////////////////////////////////
/// \brief Unix implementation of Cursor
///
////////////////////////////////////////////////////////////
class [[nodiscard]] CursorImpl
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Refer to sf::Cursor::Cursor().
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit CursorImpl();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Refer to sf::Cursor::~Cursor().
    ///
    ////////////////////////////////////////////////////////////
    ~CursorImpl();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    CursorImpl(const CursorImpl&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    CursorImpl& operator=(const CursorImpl&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    CursorImpl(CursorImpl&&) noexcept = default;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    CursorImpl& operator=(CursorImpl&&) noexcept = default;

    ////////////////////////////////////////////////////////////
    /// \brief Create a cursor with the provided image
    ///
    /// Refer to sf::Cursor::loadFromPixels().
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromPixels(const std::uint8_t* pixels, Vector2u size, Vector2u hotspot);

    ////////////////////////////////////////////////////////////
    /// \brief Create a native system cursor
    ///
    /// Refer to sf::Cursor::loadFromSystem().
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromSystem(Cursor::Type type);

private:
    friend class WindowImplX11;

    ////////////////////////////////////////////////////////////
    /// \brief Checks if colored cursors are supported for this display.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isColorCursorSupported();

    ////////////////////////////////////////////////////////////
    /// \brief Create a cursor with the provided image (ARGB support)
    ///
    /// Refer to sf::Cursor::loadFromPixels().
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromPixelsARGB(const std::uint8_t* pixels, Vector2u size, Vector2u hotspot);

    ////////////////////////////////////////////////////////////
    /// \brief Create a cursor with the provided image (monochrome)
    ///
    /// Refer to sf::Cursor::loadFromPixels().
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromPixelsMonochrome(const std::uint8_t* pixels, Vector2u size, Vector2u hotspot);

    ////////////////////////////////////////////////////////////
    /// \brief Release the cursor, if we have loaded one.
    ///
    ////////////////////////////////////////////////////////////
    void release();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::shared_ptr<::Display> m_display;
    ::Cursor                   m_cursor{None};
};

} // namespace sf::priv
