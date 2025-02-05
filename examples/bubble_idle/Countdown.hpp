#pragma once


////////////////////////////////////////////////////////////
struct [[nodiscard]] Countdown // TODO: turn to free funcs
{
    float value = 0.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndLoop(const float deltaTimeMs, const float startingValue)
    {
        if (!updateAndStop(deltaTimeMs))
            return false;

        value = startingValue;
        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndStop(const float deltaTimeMs)
    {
        if (value > 0.f)
        {
            value -= deltaTimeMs;
            return false;
        }

        value = 0.f;
        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndIsActive(const float deltaTimeMs)
    {
        return !updateAndStop(deltaTimeMs);
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] TargetedCountdown : Countdown
{
    float startingValue;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void reset()
    {
        Countdown::value = startingValue;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndLoop(const float deltaTimeMs)
    {
        return Countdown::updateAndLoop(deltaTimeMs, startingValue);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool updateAndStop(const float deltaTimeMs)
    {
        return Countdown::updateAndStop(deltaTimeMs);
    }
};
