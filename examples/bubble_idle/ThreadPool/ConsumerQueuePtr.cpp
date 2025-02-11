// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#include "ConsumerQueuePtr.hpp"

#include "Types.hpp"

#include "SFML/Base/Assert.hpp"


namespace hg::ThreadPool
{
////////////////////////////////////////////////////////////
ConsumerQueuePtr::ConsumerQueuePtr(TaskQueue& queue) noexcept : m_queue{&queue}, m_ctok{queue}
{
}


////////////////////////////////////////////////////////////
ConsumerQueuePtr::ConsumerQueuePtr(ConsumerQueuePtr&& rhs) noexcept : m_queue{rhs.m_queue}, m_ctok{*m_queue}
{
}


////////////////////////////////////////////////////////////
ConsumerQueuePtr& ConsumerQueuePtr::operator=(ConsumerQueuePtr&& rhs) noexcept
{
    m_queue = rhs.m_queue;
    m_ctok  = TaskQueueConsumerToken(*m_queue);

    return *this;
}


////////////////////////////////////////////////////////////
[[nodiscard]] TaskQueueConsumerToken& ConsumerQueuePtr::ctok() noexcept
{
    return m_ctok;
}


////////////////////////////////////////////////////////////
[[nodiscard]] TaskQueue* ConsumerQueuePtr::operator->() noexcept
{
    SFML_BASE_ASSERT(m_queue != nullptr);
    return m_queue;
}


////////////////////////////////////////////////////////////
[[nodiscard]] TaskQueue* ConsumerQueuePtr::operator->() const noexcept
{
    SFML_BASE_ASSERT(m_queue != nullptr);
    return m_queue;
}

} // namespace hg::ThreadPool
