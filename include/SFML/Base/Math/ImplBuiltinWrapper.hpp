// Intentionally include multiple times.
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Base/Traits/IsSame.hpp"


////////////////////////////////////////////////////////////
#define SFML_BASE_PRIV_DEFINE_BUILTIN_MATH_WRAPPER_1ARG(name)                                              \
    namespace sf::base                                                                                     \
    {                                                                                                      \
                                                                                                           \
    template <typename T>                                                                                  \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr auto name(T arg) noexcept \
    {                                                                                                      \
        if constexpr (SFML_BASE_IS_SAME(T, float))                                                         \
        {                                                                                                  \
            return __builtin_##name##f(arg);                                                               \
        }                                                                                                  \
        else if constexpr (SFML_BASE_IS_SAME(T, double))                                                   \
        {                                                                                                  \
            return __builtin_##name(arg);                                                                  \
        }                                                                                                  \
        else if constexpr (SFML_BASE_IS_SAME(T, long double))                                              \
        {                                                                                                  \
            return __builtin_##name##l(arg);                                                               \
        }                                                                                                  \
    }                                                                                                      \
                                                                                                           \
    } // namespace sf::base

////////////////////////////////////////////////////////////
#define SFML_BASE_PRIV_DEFINE_BUILTIN_MATH_WRAPPER_2ARG(name)                                                       \
    namespace sf::base                                                                                              \
    {                                                                                                               \
                                                                                                                    \
    template <typename T>                                                                                           \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr auto name(T arg0, T arg1) noexcept \
    {                                                                                                               \
        if constexpr (SFML_BASE_IS_SAME(T, float))                                                                  \
        {                                                                                                           \
            return __builtin_##name##f(arg0, arg1);                                                                 \
        }                                                                                                           \
        else if constexpr (SFML_BASE_IS_SAME(T, double))                                                            \
        {                                                                                                           \
            return __builtin_##name(arg0, arg1);                                                                    \
        }                                                                                                           \
        else if constexpr (SFML_BASE_IS_SAME(T, long double))                                                       \
        {                                                                                                           \
            return __builtin_##name##l(arg0, arg1);                                                                 \
        }                                                                                                           \
    }                                                                                                               \
                                                                                                                    \
    } // namespace sf::base
