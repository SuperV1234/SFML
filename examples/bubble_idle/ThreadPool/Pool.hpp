// Copyright (c) 2015-2016 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#pragma once

#include "Types.hpp"

#include <atomic>
#include <vector>


namespace hg::ThreadPool
{

class Worker;

class Pool
{
private:
    TaskQueue                 m_queue;
    std::vector<Worker>       m_workers;
    std::atomic<unsigned int> m_remainingInits;

    /// \brief  Returns `true` if all workers have finished processing packets
    /// in a blocking manner.
    [[nodiscard]] bool areAllWorkersDoneBlockingProcessing() const noexcept;

    /// \brief Posts a dummy empty task, used to unblock workers waiting to
    /// be destroyed.
    void postDummyTask();

public:
    explicit Pool(unsigned int workerCount);
    ~Pool();

    void post(Task&& f);

    [[nodiscard]] unsigned int getWorkerCount() const noexcept;
};

} // namespace hg::ThreadPool
