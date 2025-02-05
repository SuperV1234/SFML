#pragma once


////////////////////////////////////////////////////////////
struct [[nodiscard]] LoopingTimer
{
    float value = 0.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndLoop(const float deltaTimeMs, const float target)
    {
        if (!updateAndStop(deltaTimeMs, target))
            return false;

        value = 0.f;
        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndStop(const float deltaTimeMs, const float target)
    {
        if (value < target)
        {
            value += deltaTimeMs;
            return false;
        }

        value = target;
        return true;
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] TargetedLoopingTimer : LoopingTimer
{
    float target;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndLoop(const float deltaTimeMs)
    {
        return LoopingTimer::updateAndLoop(deltaTimeMs, target);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndStop(const float deltaTimeMs)
    {
        return LoopingTimer::updateAndStop(deltaTimeMs, target);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] float getProgress() const
    {
        return value / target;
    }
};
