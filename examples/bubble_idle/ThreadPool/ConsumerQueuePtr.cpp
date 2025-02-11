// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#include "ConsumerQueuePtr.hpp"

#include "Types.hpp"

#include "SFML/Base/Assert.hpp"

namespace hg::ThreadPool
{

consumer_queue_ptr::consumer_queue_ptr(task_queue& queue) noexcept : _queue{&queue}, _ctok{queue}
{
}

consumer_queue_ptr::consumer_queue_ptr(consumer_queue_ptr&& rhs) noexcept : _queue{rhs._queue}, _ctok{*_queue}
{
}

consumer_queue_ptr& consumer_queue_ptr::operator=(consumer_queue_ptr&& rhs) noexcept
{
    _queue = rhs._queue;
    _ctok  = task_queue_consumer_token(*_queue);

    return *this;
}

[[nodiscard]] task_queue_consumer_token& consumer_queue_ptr::ctok() noexcept
{
    return _ctok;
}

[[nodiscard]] task_queue* consumer_queue_ptr::operator->() noexcept
{
    SFML_BASE_ASSERT(_queue != nullptr);
    return _queue;
}

[[nodiscard]] task_queue* consumer_queue_ptr::operator->() const noexcept
{
    SFML_BASE_ASSERT(_queue != nullptr);
    return _queue;
}

} // namespace hg::ThreadPool
