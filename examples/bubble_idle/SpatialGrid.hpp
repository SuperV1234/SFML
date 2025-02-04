#pragma once

#include "Constants.hpp"
#include "ControlFlow.hpp"

#include "SFML/Base/SizeT.hpp"

#include <vector>


////////////////////////////////////////////////////////////
class SpatialGrid
{
private:
    using SizeT = sf::base::SizeT;

    static inline constexpr float gridSize    = 64.f;
    static inline constexpr float invGridSize = 1.f / gridSize;

    static inline constexpr auto nCellsX = static_cast<SizeT>(boundaries.x * invGridSize) + 1;
    static inline constexpr auto nCellsY = static_cast<SizeT>(boundaries.y * invGridSize) + 1;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline static constexpr SizeT convert2DTo1D(const SizeT x, const SizeT y, const SizeT width)
    {
        return y * width + x;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr auto computeGridRange(const sf::Vector2f center, const float radius) const
    {
        const float minX = center.x - radius;
        const float minY = center.y - radius;
        const float maxX = center.x + radius;
        const float maxY = center.y + radius;

        struct Result
        {
            SizeT xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx;
        };

        return Result{static_cast<SizeT>(sf::base::max(0.f, minX * invGridSize)),
                      static_cast<SizeT>(sf::base::max(0.f, minY * invGridSize)),
                      static_cast<SizeT>(sf::base::clamp(maxX * invGridSize, 0.f, static_cast<float>(nCellsX) - 1.f)),
                      static_cast<SizeT>(sf::base::clamp(maxY * invGridSize, 0.f, static_cast<float>(nCellsY) - 1.f))};
    }


    std::vector<SizeT> m_objectIndices;          // Flat list of all bubble indices in all cells
    std::vector<SizeT> m_cellStartIndices;       // Tracks where each cell's data starts in `bubbleIndices`
    std::vector<SizeT> m_cellInsertionPositions; // Temporary copy of `cellStartIndices` to track insertion points

public:
    ////////////////////////////////////////////////////////////
    void forEachIndexInRadius(const sf::Vector2f center, const float radius, auto&& func)
    {
        const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(center, radius);

        // Check all candidate cells
        for (SizeT cellX = xCellStartIdx; cellX <= xCellEndIdx; ++cellX)
            for (SizeT cellY = yCellStartIdx; cellY <= yCellEndIdx; ++cellY)
            {
                const SizeT cellIdx = convert2DTo1D(cellX, cellY, nCellsX);

                // Get range of bubbles in this cell
                const SizeT start = m_cellStartIndices[cellIdx];
                const SizeT end   = m_cellStartIndices[cellIdx + 1];

                // Check each bubble in cell
                for (SizeT i = start; i < end; ++i)
                    if (func(m_objectIndices[i]) == ControlFlow::Break)
                        return;
            }
    }

    ////////////////////////////////////////////////////////////
    void forEachUniqueIndexPair(auto&& func)
    {
        for (SizeT ix = 0; ix < nCellsX; ++ix)
            for (SizeT iy = 0; iy < nCellsY; ++iy)
            {
                const SizeT cellIdx = convert2DTo1D(ix, iy, nCellsX);
                const SizeT start   = m_cellStartIndices[cellIdx];
                const SizeT end     = m_cellStartIndices[cellIdx + 1];

                for (SizeT i = start; i < end; ++i)
                    for (SizeT j = i + 1; j < end; ++j)
                        func(m_objectIndices[i], m_objectIndices[j]);
            }
    }

    ////////////////////////////////////////////////////////////
    void clear()
    {
        m_cellStartIndices.clear();
        m_cellStartIndices.resize(nCellsX * nCellsY + 1, 0); // +1 for prefix sum
    }

    ////////////////////////////////////////////////////////////
    void populate(const auto& bubbles)
    {
        //
        // First Pass (Counting):
        // - Calculate how many bubbles will be placed in each grid cell.
        for (auto& bubble : bubbles)
        {
            const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(bubble.position,
                                                                                                   bubble.getRadius());

            // For each cell the bubble covers, increment the count
            for (SizeT x = xCellStartIdx; x <= xCellEndIdx; ++x)
                for (SizeT y = yCellStartIdx; y <= yCellEndIdx; ++y)
                {
                    const SizeT cellIdx = convert2DTo1D(x, y, nCellsX) + 1; // +1 offsets for prefix sum
                    ++m_cellStartIndices[sf::base::min(cellIdx, m_cellStartIndices.size() - 1)];
                }
        }

        //
        // Second Pass (Prefix Sum):
        // - Calculate the starting index for each cell’s data in `m_objectIndices`.

        // Prefix sum to compute start indices
        for (SizeT i = 1; i < m_cellStartIndices.size(); ++i)
            m_cellStartIndices[i] += m_cellStartIndices[i - 1];

        m_objectIndices.resize(m_cellStartIndices.back()); // Total bubbles across all cells

        // Used to track where to insert the next bubble index into the `m_objectIndices` buffer for each cell
        m_cellInsertionPositions.assign(m_cellStartIndices.begin(), m_cellStartIndices.end());

        //
        // Third Pass (Populating):
        // - Place the bubble indices into the correct positions in the `m_objectIndices` buffer.
        for (SizeT i = 0; i < bubbles.size(); ++i)
        {
            const auto& bubble                                                  = bubbles[i];
            const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(bubble.position,
                                                                                                   bubble.getRadius());

            // Insert the bubble index into all overlapping cells
            for (SizeT x = xCellStartIdx; x <= xCellEndIdx; ++x)
                for (SizeT y = yCellStartIdx; y <= yCellEndIdx; ++y)
                {
                    const SizeT cellIdx        = convert2DTo1D(x, y, nCellsX);
                    const SizeT insertPos      = m_cellInsertionPositions[cellIdx]++;
                    m_objectIndices[insertPos] = i;
                }
        }
    }
};
