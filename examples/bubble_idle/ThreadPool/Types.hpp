// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#pragma once

#include "blockingconcurrentqueue.h"
#include "concurrentqueue.h"

#include "SFML/Base/FixedFunction.hpp"

/*
namespace moodycamel
{
struct ConcurrentQueueDefaultTraits;

template <typename T, typename Traits = ConcurrentQueueDefaultTraits>
class BlockingConcurrentQueue;

struct ConsumerToken;
struct ProducerToken;

} // namespace moodycamel
*/

namespace hg::ThreadPool
{

using Task                      = sf::base::FixedFunction<void(), 128>;
using TaskQueue                 = moodycamel::BlockingConcurrentQueue<Task>;
using TaskQueueConsumerToken    = moodycamel::ConsumerToken;
using task_queue_producer_token = moodycamel::ProducerToken;

} // namespace hg::ThreadPool
