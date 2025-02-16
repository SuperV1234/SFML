#pragma once

#include "AssertAssume.hpp"

#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Math/Fabs.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] TimerStatusLoop : bool
{
    Running,
    Looping
};

////////////////////////////////////////////////////////////
enum class [[nodiscard]] TimerStatusStop : sf::base::U8
{
    Running,
    JustFinished,
    AlreadyFinished
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Timer // TODO P2: turn to free funcs?
{
    float value = 0.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr TimerStatusStop updateForwardAndStop(const float delta) noexcept
    {
        ASSERT_AND_ASSUME(delta >= 0.f);

        if (value == 1.f)
            return TimerStatusStop::AlreadyFinished;

        ASSERT_AND_ASSUME(value < 1.f);
        value += delta;

        if (value >= 1.f)
        {
            value = 1.f;
            return TimerStatusStop::JustFinished;
        }

        return TimerStatusStop::Running;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr TimerStatusStop updateBackwardsAndStop(const float delta) noexcept
    {
        ASSERT_AND_ASSUME(delta >= 0.f);

        if (value == 0.f)
            return TimerStatusStop::AlreadyFinished;

        ASSERT_AND_ASSUME(value > 0.f);
        value -= delta;

        if (value <= 0.f)
        {
            value = 0.f;
            return TimerStatusStop::JustFinished;
        }

        return TimerStatusStop::Running;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr TimerStatusLoop updateForwardAndLoop(const float delta) noexcept
    {
        if (updateForwardAndStop(delta) == TimerStatusStop::Running)
            return TimerStatusLoop::Running;

        value = 0.f;
        return TimerStatusLoop::Looping;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr TimerStatusLoop updateBackwardsAndLoop(const float delta) noexcept
    {
        if (updateBackwardsAndStop(delta) == TimerStatusStop::Running)
            return TimerStatusLoop::Running;

        value = 1.f;
        return TimerStatusLoop::Looping;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr bool updateForwardAndIsRunning(const float delta) noexcept
    {
        return updateForwardAndStop(delta) == TimerStatusStop::Running;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr bool updateBackwardsAndIsRunning(const float delta) noexcept
    {
        return updateBackwardsAndStop(delta) == TimerStatusStop::Running;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr float getProgressForward() const noexcept
    {
        return value;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr float getProgressBackwards() const noexcept
    {
        return 1.f - value;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr float getProgressBounced() const noexcept
    {
        return 1.f - sf::base::fabs(value - 0.5f) * 2.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr inline bool isDoneForward() const noexcept
    {
        return value == 1.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr inline bool isDoneBackwards() const noexcept
    {
        return value == 0.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr inline float remap(const float min, const float max) const noexcept
    {
        return min + value * (max - min);
    }
};

////////////////////////////////////////////////////////////
enum class TimerDirection : bool
{
    Forward,
    Backwards
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] BidirectionalTimer : Timer
{
    TimerDirection direction;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr TimerStatusStop updateAndStop(const float delta) noexcept
    {
        return (direction == TimerDirection::Forward) ? updateForwardAndStop(delta) : updateBackwardsAndStop(delta);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr TimerStatusLoop updateAndLoop(const float delta) noexcept
    {
        return (direction == TimerDirection::Forward) ? updateForwardAndLoop(delta) : updateBackwardsAndLoop(delta);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr bool updateAndIsRunning(const float delta) noexcept
    {
        return updateAndStop(delta) == TimerStatusStop::Running;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr inline bool isDone() const noexcept
    {
        return (direction == TimerDirection::Forward) ? isDoneForward() : isDoneBackwards();
    }
};
