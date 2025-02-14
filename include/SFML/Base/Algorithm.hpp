#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Base/Macros.hpp"
#include "SFML/Base/SizeT.hpp"


namespace sf::base
{
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T& min(const T& a, const T& b) noexcept
{
    return b < a ? b : a;
}


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T& max(const T& a, const T& b) noexcept
{
    return a < b ? b : a;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename TargetIter>
[[gnu::always_inline]] constexpr TargetIter copy(Iter rangeBegin, Iter rangeEnd, TargetIter targetIter)
{
    while (rangeBegin != rangeEnd)
        *targetIter++ = *rangeBegin++;

    return targetIter;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Iter find(Iter rangeBegin, Iter rangeEnd, const T& target) noexcept
{
    for (; rangeBegin != rangeEnd; ++rangeBegin)
        if (*rangeBegin == target)
            return rangeBegin;

    return rangeEnd;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename Predicate>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr Iter findIf(Iter rangeBegin, Iter rangeEnd, Predicate&& predicate) noexcept
{
    for (; rangeBegin != rangeEnd; ++rangeBegin)
        if (predicate(*rangeBegin))
            return rangeBegin;

    return rangeEnd;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename Predicate>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool anyOf(Iter rangeBegin, Iter rangeEnd, Predicate&& predicate) noexcept
{
    for (; rangeBegin != rangeEnd; ++rangeBegin)
        if (predicate(*rangeBegin))
            return true;

    return false;
}


////////////////////////////////////////////////////////////
template <typename Iter>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr SizeT count(Iter rangeBegin, Iter rangeEnd)
{
    SizeT result = 0u;

    for (; rangeBegin != rangeEnd; ++rangeBegin)
        if (static_cast<bool>(*rangeBegin))
            ++result;

    return result;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename Predicate>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr SizeT countIf(Iter rangeBegin, Iter rangeEnd, Predicate&& predicate)
{
    SizeT result = 0u;

    for (; rangeBegin != rangeEnd; ++rangeBegin)
        if (predicate(*rangeBegin))
            ++result;

    return result;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename Predicate>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool allOf(Iter rangeBegin, Iter rangeEnd, Predicate&& predicate) noexcept
{
    for (; rangeBegin != rangeEnd; ++rangeBegin)
        if (!predicate(*rangeBegin))
            return false;

    return true;
}


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T& clamp(const T& value, const T& minValue, const T& maxValue) noexcept
{
    return (value < minValue) ? minValue : ((value > maxValue) ? maxValue : value);
}


////////////////////////////////////////////////////////////
template <typename T, auto N>
[[nodiscard, gnu::always_inline, gnu::const]] consteval SizeT getArraySize(const T (&)[N]) noexcept
{
    return N;
}


////////////////////////////////////////////////////////////
template <typename S, typename T, auto N>
[[nodiscard, gnu::always_inline, gnu::const]] consteval auto getArraySize(const T (S::*)[N]) noexcept
{
    return N;
}


////////////////////////////////////////////////////////////
template <typename T>
class BackInserter
{
private:
    T* m_container;

public:
    using container_type = T;
    using value_type     = T::value_type;

    [[nodiscard, gnu::always_inline]] explicit BackInserter(T& container) noexcept : m_container(&container)
    {
    }

    [[gnu::always_inline]] BackInserter& operator=(const value_type& value)
    {
        m_container->push_back(value);
        return *this;
    }

    [[gnu::always_inline]] BackInserter& operator=(value_type&& value)
    {
        m_container->push_back(static_cast<value_type&&>(value));
        return *this;
    }

    [[nodiscard, gnu::always_inline, gnu::pure]] BackInserter& operator*() noexcept
    {
        return *this;
    }

    [[gnu::always_inline, gnu::pure]] BackInserter& operator++() noexcept
    {
        return *this;
    }

    [[nodiscard, gnu::always_inline, gnu::pure]] BackInserter operator++(int) noexcept
    {
        return *this;
    }
};


////////////////////////////////////////////////////////////
template <typename T, typename U = T>
[[nodiscard, gnu::always_inline]] inline constexpr T exchange(T& obj, U&& newVal)
{
    T oldVal = SFML_BASE_MOVE(obj);
    obj      = SFML_BASE_FORWARD(newVal);
    return oldVal;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename Predicate>
[[nodiscard, gnu::always_inline]] inline constexpr Iter removeIf(Iter first, Iter last, Predicate&& predicate)
{
    first = findIf(first, last, predicate);

    if (first != last)
        for (Iter i = first; ++i != last;)
            if (!predicate(*i))
                *first++ = SFML_BASE_MOVE(*i);

    return first;
}


////////////////////////////////////////////////////////////
template <typename Vector, typename Predicate>
[[gnu::always_inline]] inline constexpr SizeT vectorEraseIf(Vector& vector, Predicate&& predicate)
{
    const auto it       = removeIf(vector.begin(), vector.end(), predicate);
    const auto nRemoved = static_cast<SizeT>(vector.end() - it);

    vector.erase(it, vector.end());
    return nRemoved;
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] constexpr void swap(T& a, T& b) noexcept
{
    T tempA = SFML_BASE_MOVE(a);
    a       = SFML_BASE_MOVE(b);
    b       = SFML_BASE_MOVE(tempA);
}


////////////////////////////////////////////////////////////
template <typename Iter1, typename Iter2>
[[gnu::always_inline]] constexpr void iterSwap(Iter1 a, Iter2 b)
{
    using base::swap;
    swap(*a, *b);
}


////////////////////////////////////////////////////////////
template <typename Iter1, typename Iter2>
[[gnu::always_inline]] constexpr Iter2 swapRanges(Iter1 first1, Iter1 last1, Iter2 first2)
{
    for (; first1 != last1; ++first1, ++first2)
        iterSwap(first1, first2);

    return first2;
}


////////////////////////////////////////////////////////////
template <typename Iter, typename Comparer>
[[nodiscard]] constexpr bool isSorted(Iter first, Iter last, Comparer comp)
{
    if (first != last)
    {
        Iter next = first;
        while (++next != last)
        {
            if (comp(*next, *first))
                return false;

            first = next;
        }
    }

    return true;
}

} // namespace sf::base
