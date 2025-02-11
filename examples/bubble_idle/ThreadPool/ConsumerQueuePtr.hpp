// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#pragma once

#include "Types.hpp"


namespace hg::ThreadPool
{
////////////////////////////////////////////////////////////
/// \brief Wraps a `TaskQueue` pointer and a consumer token.
class [[nodiscard]] ConsumerQueuePtr
{
private:
    TaskQueue*             m_queue;
    TaskQueueConsumerToken m_ctok;

public:
    ////////////////////////////////////////////////////////////
    explicit ConsumerQueuePtr(TaskQueue& queue) noexcept;

    ////////////////////////////////////////////////////////////
    ConsumerQueuePtr(const ConsumerQueuePtr&)            = delete;
    ConsumerQueuePtr& operator=(const ConsumerQueuePtr&) = delete;

    ////////////////////////////////////////////////////////////
    ConsumerQueuePtr(ConsumerQueuePtr&& rhs) noexcept;
    ConsumerQueuePtr& operator=(ConsumerQueuePtr&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] TaskQueueConsumerToken& ctok() noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] TaskQueue* operator->() noexcept;
    [[nodiscard]] TaskQueue* operator->() const noexcept;
};

} // namespace hg::ThreadPool
