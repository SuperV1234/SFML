#include <iostream>
#include <type_traits>
#include <cstddef>

using VoidPtrFn = void (*)(void*);

struct MemberScopeGuard
{
    VoidPtrFn fn;
    ~MemberScopeGuard() { fn(this); }
};

#define TOKEN_PASTE_IMPL(x, y) x##y
#define TOKEN_PASTE(x, y)      TOKEN_PASTE_IMPL(x, y)

#define MEMBER_SCOPE_GUARD(...)                                                                           \
    MemberScopeGuard TOKEN_PASTE(memberScopeGuard, __LINE__)                                              \
    {                                                                                                     \
        ([]<auto Fn>(auto* xThis) -> VoidPtrFn                                                            \
        {                                                                                                 \
            return [](void* x) -> void                                                                    \
            {                                                                                             \
                using T = std::remove_pointer_t<decltype(xThis)>;                                         \
                Fn(*(reinterpret_cast<T*>(                                                                \
                    reinterpret_cast<char*>(x) - offsetof(T, TOKEN_PASTE(memberScopeGuard, __LINE__))))); \
            };                                                                                            \
        }).template operator()<[](auto& self) __VA_ARGS__>(this)                                          \
    }

struct parent
{
    int x = 10;

    MEMBER_SCOPE_GUARD({
        std::cout << "Parent pointer is " << &self << '\n';
        std::cout << "Parent x is " << self.x << '\n';
    });

    int y = 25;

    MEMBER_SCOPE_GUARD({
        std::cout << "Parent pointer is " << &self << '\n';
        std::cout << "Parent y is " << self.y << '\n';
    });
};

int main()
{
    parent x;
    std::cout << "Address of parent is " << &x << '\n';
}
