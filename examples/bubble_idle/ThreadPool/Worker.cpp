// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#include "Worker.hpp"

#include "ConsumerQueuePtr.hpp"

#include "SFML/Base/Assert.hpp"

#include <atomic>
#include <thread>


namespace hg::ThreadPool
{
////////////////////////////////////////////////////////////
Worker::Worker(TaskQueue& queue) noexcept :
m_queue{queue},
m_state{State::Uninitialized},
m_doneBlockingProcessing{false}
{
}


////////////////////////////////////////////////////////////
Worker::Worker(Worker&&) noexcept            = default;
Worker& Worker::operator=(Worker&&) noexcept = default;


////////////////////////////////////////////////////////////
void Worker::start(std::atomic<unsigned int>& remainingInits)
{
    SFML_BASE_ASSERT(m_state.load() == State::Uninitialized);

    m_thread = std::thread([this, &remainingInits]
    {
        // Set the running flag and signal to the pool that we are initialized.
        m_state.store(State::Running);
        remainingInits.fetch_sub(1u);

        Task taskBuffer;

        while (m_state.load() == State::Running)
        {
            m_queue->wait_dequeue(m_queue.ctok(), taskBuffer); // Blocking
            taskBuffer();
        }

        // Signal the thread pool to send dummy final tasks.
        SFML_BASE_ASSERT(m_state.load() == State::Stopped);
        m_doneBlockingProcessing.store(true);

        while (m_state.load() == State::Stopped)
        {
            if (!m_queue->try_dequeue(m_queue.ctok(), taskBuffer)) // Non-blocking
                break;                                             // No more tasks available

            taskBuffer();
        }
    });
}


////////////////////////////////////////////////////////////
void Worker::stop() noexcept
{
    SFML_BASE_ASSERT(m_state.load() == State::Running);

    m_state.store(State::Stopped);
}


////////////////////////////////////////////////////////////
void Worker::join() noexcept
{
    SFML_BASE_ASSERT(m_thread.joinable());
    SFML_BASE_ASSERT(m_state.load() == State::Stopped);

    m_thread.join();
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool Worker::isDoneBlockingProcessing() const noexcept
{
    return m_doneBlockingProcessing.load();
}

} // namespace hg::ThreadPool
