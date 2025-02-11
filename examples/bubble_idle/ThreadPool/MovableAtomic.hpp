// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#pragma once

#include <atomic>


namespace hg::Utils
{

/// \brief Wrapper around `std::atomic<T>` that allows move operations.
/// \details Moves are handled by using `store(rhs.load())`.
template <typename T>
class [[nodiscard]] MovableAtomic : public std::atomic<T>
{
private:
    using base_type = std::atomic<T>;

public:
    using base_type::base_type;

    [[nodiscard]] MovableAtomic() = default;

    MovableAtomic(const MovableAtomic&)            = delete;
    MovableAtomic& operator=(const MovableAtomic&) = delete;

    MovableAtomic(MovableAtomic&& rhs) noexcept : base_type{rhs.load()}
    {
    }

    MovableAtomic& operator=(MovableAtomic&& rhs) noexcept
    {
        this->store(rhs.load());
        return *this;
    }
};

} // namespace hg::Utils
