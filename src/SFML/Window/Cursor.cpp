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
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/CursorImpl.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/UniquePtr.hpp>
#include <SFML/System/Vector2.hpp>

#include <ostream>


namespace sf
{

////////////////////////////////////////////////////////////
Cursor::Cursor(priv::PassKey<Cursor>&&) : m_impl(priv::makeUnique<priv::CursorImpl>())
{
}


////////////////////////////////////////////////////////////
Cursor::~Cursor() = default;


////////////////////////////////////////////////////////////
Cursor::Cursor(Cursor&&) noexcept = default;


////////////////////////////////////////////////////////////
Cursor& Cursor::operator=(Cursor&&) noexcept = default;


////////////////////////////////////////////////////////////
std::optional<Cursor> Cursor::loadFromPixels(const std::uint8_t* pixels, Vector2u size, Vector2u hotspot)
{
    std::optional<Cursor> cursor; // Use a single local variable for NRVO

    if ((pixels == nullptr) || (size.x == 0) || (size.y == 0))
        return cursor;

    cursor = std::make_optional<Cursor>(priv::PassKey<Cursor>{});
    if (!cursor->m_impl->loadFromPixels(pixels, size, hotspot))
    {
        err() << "Failed to load cursor from pixels (invalid arguments)" << std::endl;
        return cursor; // Empty optional
    }

    cursor.emplace(priv::PassKey<Cursor>{});
    if (!cursor->m_impl->loadFromPixels(pixels, size, hotspot))
    {
        // Error message generated in called function.
        cursor.reset();
    }

    // Error message generated in called function.
    return cursor;
}


////////////////////////////////////////////////////////////
std::optional<Cursor> Cursor::loadFromSystem(Type type)
{
    auto cursor = std::make_optional<Cursor>(priv::PassKey<Cursor>{}); // Use a single local variable for NRVO

    if (!cursor->m_impl->loadFromSystem(type))
    {
        // Error message generated in called function.
        cursor.reset();
    }

    // Error message generated in called function.
    return cursor;
}


////////////////////////////////////////////////////////////
const priv::CursorImpl& Cursor::getImpl() const
{
    return *m_impl;
}

} // namespace sf
