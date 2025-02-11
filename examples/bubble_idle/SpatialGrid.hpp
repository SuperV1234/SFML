#pragma once

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/SizeT.hpp"
#include "SFML/Base/TrivialVector.hpp"

#include <algorithm>
#include <latch>

#include <cassert>

class SpatialGrid
{
private:
    struct AABB
    {
        float           minX, maxX, minY, maxY;
        sf::base::SizeT objIdx;
    };

    sf::base::TrivialVector<AABB> m_aabbs;

public:
    ////////////////////////////////////////////////////////////
    void forEachUniqueIndexPair(const unsigned int nWorkers, std::latch& latch, auto& pool, auto&& func)
    {
        const sf::base::SizeT numObjects = m_aabbs.size();
        if (numObjects < 2)
        {
            latch.count_down(nWorkers);
            return;
        }

        // Compute a chunk size for dividing the outer loop among tasks.
        // (Note: Some chunks will have more work than others, since early exits in the inner loop
        // mean that iterations with low 'i' do more work. For more even load balancing,
        // consider dynamic task scheduling or a work-stealing scheduler.)
        const sf::base::SizeT chunkSize = (numObjects + nWorkers - 1) / nWorkers;

        for (unsigned int iWorker = 0u; iWorker < nWorkers; ++iWorker)
        {
            // Calculate the start and end indices for this chunk.
            const sf::base::SizeT start = iWorker * chunkSize;
            const sf::base::SizeT end   = sf::base::min((iWorker + 1) * chunkSize, numObjects);

            if (start >= numObjects)
            {
                latch.count_down(nWorkers - iWorker);
                break;
            }

            // Launch an asynchronous task to process the chunk.
            pool.post([start, end, numObjects, &func, &latch, this]
            {
                // Process each object in the chunk.
                for (sf::base::SizeT i = start; i < end; ++i)
                {
                    const AABB& aabb1 = m_aabbs[i];

                    // Check against all objects with a greater index.
                    for (sf::base::SizeT j = i + 1; j < numObjects; ++j)
                    {
                        const AABB& aabb2 = m_aabbs[j];

                        // Early exit: since m_aabbs is sorted by minX,
                        // if aabb2.minX is greater than aabb1.maxX, no further objects
                        // will overlap on the x-axis.
                        if (aabb2.minX > aabb1.maxX)
                            break;

                        // Since the x intervals overlap, check the y intervals.
                        if (aabb1.minY <= aabb2.maxY && aabb1.maxY >= aabb2.minY)
                        {
                            // Call the user-supplied function with the indices in sorted order.
                            func(sf::base::min(aabb1.objIdx, aabb2.objIdx), sf::base::max(aabb1.objIdx, aabb2.objIdx));
                        }
                    }
                }

                latch.count_down();
            });
        }
    }

    ////////////////////////////////////////////////////////////
    void clear()
    {
        m_aabbs.clear();
    }

    ////////////////////////////////////////////////////////////
    void populate(const auto& bubbles)
    {
        m_aabbs.reserve(bubbles.size());
        m_aabbs.clear();

        for (sf::base::SizeT i = 0u; i < bubbles.size(); ++i)
        {
            const auto& b = bubbles[i];
            m_aabbs.unsafeEmplaceBack(b.position.x - b.radius,
                                      b.position.x + b.radius,
                                      b.position.y - b.radius,
                                      b.position.y + b.radius,
                                      i);
        }

        std::sort(m_aabbs.begin(), m_aabbs.end(), [](const AABB& a, const AABB& b) { return a.minX < b.minX; });
    }
};
