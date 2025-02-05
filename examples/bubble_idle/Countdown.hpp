#pragma once


#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Optional.hpp"


////////////////////////////////////////////////////////////
enum class [[nodiscard]] CountdownStatusLoop : sf::base::U8
{
    Running,
    Looping
};

////////////////////////////////////////////////////////////
enum class [[nodiscard]] CountdownStatusStop : sf::base::U8
{
    Running,
    JustFinished,
    AlreadyFinished
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Countdown // TODO: turn to free funcs
{
    float value = 0.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] CountdownStatusLoop updateAndLoop(const float deltaTimeMs, const float startingValue)
    {
        if (updateAndStop(deltaTimeMs) == CountdownStatusStop::Running)
            return CountdownStatusLoop::Running;

        value = startingValue;
        return CountdownStatusLoop::Looping;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] CountdownStatusStop updateAndStop(const float deltaTimeMs)
    {
        if (value > 0.f)
        {
            value -= deltaTimeMs;

            if (value <= 0.f)
                return CountdownStatusStop::JustFinished;

            return CountdownStatusStop::Running;
        }

        value = 0.f;
        return CountdownStatusStop::AlreadyFinished;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAndIsActive(const float deltaTimeMs)
    {
        return updateAndStop(deltaTimeMs) == CountdownStatusStop::Running;
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] TargetedCountdown : Countdown
{
    float startingValue;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void restart()
    {
        Countdown::value = startingValue;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] CountdownStatusLoop updateAndLoop(const float deltaTimeMs)
    {
        return Countdown::updateAndLoop(deltaTimeMs, startingValue);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] CountdownStatusStop updateAndStop(const float deltaTimeMs)
    {
        return Countdown::updateAndStop(deltaTimeMs);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] float getProgress() const
    {
        return 1.f - (value / startingValue);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool isDone() const
    {
        return value == 0.f;
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] OptionalTargetedCountdown : private sf::base::Optional<TargetedCountdown>
{
    using BaseType = sf::base::Optional<TargetedCountdown>;
    using BaseType::BaseType;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] float getProgress() const
    {
        return hasValue() ? (*this)->getProgress() : 0.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool isDone() const
    {
        return hasValue() && (*this)->isDone();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] BaseType& asBase()
    {
        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] const BaseType& asBase() const
    {
        return *this;
    }

    ////////////////////////////////////////////////////////////
    using BaseType::emplace;
    using BaseType::hasValue;
    using BaseType::value;
    using BaseType::operator->;
};
