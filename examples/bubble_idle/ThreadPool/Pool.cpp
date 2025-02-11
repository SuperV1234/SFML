// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#include "Pool.hpp"

#include "Types.hpp"
#include "Worker.hpp"

#include <chrono>
#include <thread>
#include <utility>

namespace hg::ThreadPool
{
////////////////////////////////////////////////////////////
[[nodiscard]] bool Pool::areAllWorkersDoneBlockingProcessing() const noexcept
{
    for (const Worker& w : m_workers)
        if (!w.isDoneBlockingProcessing())
            return false;

    return true;
}


////////////////////////////////////////////////////////////
void Pool::postDummyTask()
{
    post([] {});
}


////////////////////////////////////////////////////////////
Pool::Pool(unsigned int workerCount)
{
    for (unsigned int i = 0u; i < workerCount; ++i)
        m_workers.emplace_back(m_queue);

    m_remainingInits.store(workerCount);
    for (Worker& w : m_workers)
        w.start(m_remainingInits);
}


////////////////////////////////////////////////////////////
Pool::~Pool()
{
    // Busy wait until all workers are initialized.
    while (m_remainingInits.load() > 0u)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Signal all workers to exit their processing loops.
    for (Worker& w : m_workers)
        w.stop();

    // Post dummy tasks until all workers have exited their loops.
    while (!areAllWorkersDoneBlockingProcessing())
        postDummyTask();

    // Join the workers' threads.
    for (Worker& w : m_workers)
        w.join();
}


////////////////////////////////////////////////////////////
void Pool::post(Task&& f)
{
    [[maybe_unused]] const bool enqueued = m_queue.enqueue(std::move(f));
    SFML_BASE_ASSERT(enqueued);
}


////////////////////////////////////////////////////////////
unsigned int Pool::getWorkerCount() const noexcept
{
    return static_cast<unsigned int>(m_workers.size());
}

} // namespace hg::ThreadPool
