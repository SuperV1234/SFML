#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION


#if !defined(_LIBCPP_INITIALIZER_LIST) && !defined(_INITIALIZER_LIST) && !defined(_INITIALIZER_LIST_)

#define _LIBCPP_INITIALIZER_LIST // libcpp
#define _INITIALIZER_LIST        // libstdc++
#define _INITIALIZER_LIST_       // msstl

namespace std
{
////////////////////////////////////////////////////////////
/// \brief TODO P1: docs
///
////////////////////////////////////////////////////////////
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
class initializer_list
{
public:
    using value_type      = T;
    using reference       = const T&;
    using const_reference = const T&;
    using size_type       = decltype(sizeof(0));

    using iterator       = const T*;
    using const_iterator = const T*;

    constexpr initializer_list() noexcept = default;

    [[nodiscard, gnu::always_inline]] constexpr initializer_list(const T* b, const T* e) noexcept
    {
        m_begin = b;
        m_end   = e;
    }

    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* begin() const noexcept
    {
        return m_begin;
    }

    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* end() const noexcept
    {
        return m_end;
    }

    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr size_type size() const noexcept
    {
        return static_cast<size_type>(m_end - m_begin);
    }

private:
    const T* m_begin = nullptr;
    const T* m_end   = nullptr;
};

template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* begin(initializer_list<T> il) noexcept
{
    return il.begin();
}

template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* end(initializer_list<T> il) noexcept
{
    return il.end();
}

} // namespace std

#endif
