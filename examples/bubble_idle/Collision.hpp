#pragma once

#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Math/Sqrt.hpp"
#include "SFML/Base/Optional.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] CollisionResolution
{
    sf::Vector2f iDisplacement;
    sf::Vector2f jDisplacement;
    sf::Vector2f iVelocityChange;
    sf::Vector2f jVelocityChange;
};

////////////////////////////////////////////////////////////
[[nodiscard]] inline bool detectCollision(const sf::Vector2f iPosition,
                                          const sf::Vector2f jPosition,
                                          const float        iRadius,
                                          const float        jRadius)
{
    const sf::Vector2f diff            = jPosition - iPosition;
    const float        squaredDistance = diff.lengthSquared();
    const float        sumRadii        = iRadius + jRadius;

    return squaredDistance < sumRadii * sumRadii;
}

////////////////////////////////////////////////////////////
[[nodiscard]] inline sf::base::Optional<CollisionResolution> handleCollision(
    const float        deltaTimeMs,
    const sf::Vector2f iPosition,
    const sf::Vector2f jPosition,
    const sf::Vector2f iVelocity,
    const sf::Vector2f jVelocity,
    const float        iRadius,
    const float        jRadius,
    const float        iMassMult,
    const float        jMassMult)
{
    const sf::Vector2f diff            = jPosition - iPosition;
    const float        squaredDistance = diff.lengthSquared();
    const float        sumRadii        = iRadius + jRadius;

    if (squaredDistance >= sumRadii * sumRadii)
        return sf::base::nullOpt;

    // Calculate the overlap between the bubbles
    const float distance = sf::base::sqrt(squaredDistance); // Distance between centers
    const float overlap  = sumRadii - distance;             // Amount of overlap

    // Calculate the normal between the bubbles
    const sf::Vector2f normal = (distance > 0.f) ? (diff / distance) : sf::Vector2f{1.f, 0.f};

    // Move the bubbles apart based on their masses (heavier bubbles move less)
    const float m1           = iRadius * iRadius * iMassMult; // Mass of bubble i (quadratic scaling)
    const float m2           = jRadius * jRadius * jMassMult; // Mass of bubble j (quadratic scaling)
    const float totalMassInv = 1.f / (m1 + m2);
    const float invM1        = 1.f / m1;
    const float invM2        = 1.f / m2;

    // Velocity resolution calculations
    const float vRelDotNormal = (iVelocity - jVelocity).dot(normal);

    sf::Vector2f velocityChangeI;
    sf::Vector2f velocityChangeJ;

    // Only apply impulse if bubbles are moving towards each other
    if (vRelDotNormal > 0.f)
    {
        constexpr float e = 0.65f; // Coefficient of restitution (1.0 = perfectly elastic)
        const float     j = -(1.f + e) * vRelDotNormal / (invM1 + invM2);

        const sf::Vector2f impulse = normal * j;

        velocityChangeI = impulse * invM1;
        velocityChangeJ = -impulse * invM2;
    }

    // Define a "softness" factor to control how quickly the overlap is resolved
    const float softnessFactor = 0.00075f * deltaTimeMs;

    // Calculate the displacement needed to resolve the overlap
    const sf::Vector2f displacement = normal * overlap * softnessFactor;

    return sf::base::makeOptional( //
        CollisionResolution{.iDisplacement   = -displacement * (m2 * totalMassInv),
                            .jDisplacement   = displacement * (m1 * totalMassInv),
                            .iVelocityChange = velocityChangeI,
                            .jVelocityChange = velocityChangeJ});
}
