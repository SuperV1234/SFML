#pragma once

#include <SFML/Base/InitializerList.hpp>

////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::const]] consteval auto count(const std::initializer_list<T>& l) noexcept
{
    return l.size();
}

////////////////////////////////////////////////////////////
#define EXACT_ARRAY(type, name, requiredCount, ...) \
    type name[requiredCount] __VA_ARGS__;           \
    static_assert(::count<type>(__VA_ARGS__) == requiredCount)
