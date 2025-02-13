#pragma once

#include "SFML/Base/TokenPaste.hpp"


////////////////////////////////////////////////////////////
template <typename This, auto Fn>
struct MemberScopeGuard
{
    This* thisPtr;

    MemberScopeGuard(This* ptr) : thisPtr{ptr}
    {
    }

    MemberScopeGuard(const MemberScopeGuard&) = delete;
    MemberScopeGuard(MemberScopeGuard&&)      = delete;

    ~MemberScopeGuard()
    {
        Fn(*thisPtr);
    }
};

////////////////////////////////////////////////////////////
#define MEMBER_SCOPE_GUARD(type, ...) \
    MemberScopeGuard<type, [](auto& self) __VA_ARGS__> SFML_BASE_TOKEN_PASTE(bbMemberScopeGuard, __LINE__) { this }
