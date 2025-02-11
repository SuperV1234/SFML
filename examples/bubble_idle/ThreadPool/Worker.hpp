// Copyright (c) 2015-2016 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#pragma once

#include "ConsumerQueuePtr.hpp"
#include "MovableAtomic.hpp"

#include "SFML/Base/IntTypes.hpp"

#include <atomic>
#include <thread>


namespace hg::ThreadPool
{
////////////////////////////////////////////////////////////
/// \brief Wraps an `std::thread`, `ConsumerQueuePtr` and atomic control
/// variables.
class [[nodiscard]] Worker
{
private:
    enum class [[nodiscard]] State : sf::base::U8
    {
        // Initial state of the worker.
        Uninitialized = 0u,

        // The worker is dequeuing and accepting tasks in blocking mode.
        Running = 1u,

        // The worker is dequeuing and accepting tasks in non-blocking mode.
        // Will transition to `State::Finished` automatically when there are no
        // more tasks.
        Stopped = 2u,
    };

    /// \brief Worker thread.
    std::thread m_thread;

    /// \brief Pointer to queue + consumer token.
    ConsumerQueuePtr m_queue;

    /// \brief State of the worker, controlled both by the pool and internally.
    Utils::MovableAtomic<State> m_state;

    /// \brief Signals when the worker is done processing tasks in blocking
    /// mode. Controlled internally, checked by the pool to start posting dummy
    /// tasks.
    Utils::MovableAtomic<bool> m_doneBlockingProcessing;

public:
    Worker(TaskQueue& queue) noexcept;

    Worker(Worker&&) noexcept;
    Worker& operator=(Worker&&) noexcept;

    void start(std::atomic<unsigned int>& remainingInits);

    /// \brief Sets the running flag to false, preventing the worker to
    /// accept tasks.
    void stop() noexcept;

    /// \brief Assuming the worker is not running, tries to join the
    /// underlying thread.
    void join() noexcept;

    /// \brief Return `true` if the worker has finished processing tasks in a
    /// blocking manner.
    [[nodiscard]] bool isDoneBlockingProcessing() const noexcept;
};

} // namespace hg::ThreadPool
