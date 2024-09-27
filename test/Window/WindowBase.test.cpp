#include "SFML/Window/WindowBase.hpp"

// Other 1st party headers
#include "SFML/Window/Event.hpp"
#include "SFML/Window/WindowContext.hpp"

#include "SFML/System/Clock.hpp"

#include <Doctest.hpp>

#include <CommonTraits.hpp>
#include <StringifyOptionalUtil.hpp>
#include <SystemUtil.hpp>
#include <WindowUtil.hpp>

TEST_CASE("[Window] sf::WindowBase" * doctest::skip(skipDisplayTests))
{
    sf::WindowContext windowContext;

    SECTION("Type traits")
    {
        STATIC_CHECK(!SFML_BASE_HAS_VIRTUAL_DESTRUCTOR(sf::WindowBase));
        STATIC_CHECK(!SFML_BASE_IS_COPY_CONSTRUCTIBLE(sf::WindowBase));
        STATIC_CHECK(!SFML_BASE_IS_COPY_ASSIGNABLE(sf::WindowBase));
        STATIC_CHECK(SFML_BASE_IS_NOTHROW_MOVE_CONSTRUCTIBLE(sf::WindowBase));
        STATIC_CHECK(SFML_BASE_IS_NOTHROW_MOVE_ASSIGNABLE(sf::WindowBase));
    }

    SECTION("Construction")
    {
        SECTION("Mode and title constructor")
        {
            const sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            CHECK(windowBase.getSize() == sf::Vector2u{360, 240});
            CHECK(windowBase.getNativeHandle() != sf::WindowHandle());
        }

        SECTION("Mode, title, and style constructor")
        {
            const sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            CHECK(windowBase.getSize() == sf::Vector2u{360, 240});
            CHECK(windowBase.getNativeHandle() != sf::WindowHandle());
        }

        SECTION("Mode, title, style, and state constructor")
        {
            const sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            CHECK(windowBase.getSize() == sf::Vector2u{360, 240});
            CHECK(windowBase.getNativeHandle() != sf::WindowHandle());
        }

        SECTION("Mode, title, and state constructor")
        {
            const sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            CHECK(windowBase.getSize() == sf::Vector2u{360, 240});
            CHECK(windowBase.getNativeHandle() != sf::WindowHandle());
        }
    }

    SECTION("waitEvent()")
    {
        SECTION("Initialized window")
        {
            sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            constexpr auto timeout = sf::milliseconds(50);

            sf::Clock clock;

            const auto startTime = clock.getElapsedTime();
            const auto event     = windowBase.waitEvent(timeout);
            const auto elapsed   = clock.getElapsedTime() - startTime;

            REQUIRE(elapsed < timeout);

            if (elapsed <= timeout)
                CHECK(event.hasValue());
            else
                CHECK(!event.hasValue());
        }
    }

    SECTION("Set/get position")
    {
        sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

        windowBase.setPosition({12, 34});
        CHECK(windowBase.getPosition() == sf::Vector2i{});
    }

    SECTION("Set/get size")
    {
        SECTION("Uninitialized window")
        {
            sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            windowBase.setSize({128, 256});
            CHECK(windowBase.getSize() == sf::Vector2u{});
        }

        SECTION("Initialized window")
        {
            sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            windowBase.setSize({128, 256});
            CHECK(windowBase.getSize() == sf::Vector2u{128, 256});
        }

        SECTION("Minimum size")
        {
            sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            windowBase.setMinimumSize({128u, 256u});
            windowBase.setSize({100, 100});
            CHECK(windowBase.getSize() == sf::Vector2u{128, 256});
        }

        SECTION("Maximum size")
        {
            sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

            windowBase.setMaximumSize({128u, 256u});
            windowBase.setSize({400, 400});
            CHECK(windowBase.getSize() == sf::Vector2u{128, 256});
        }
    }

    SECTION("setMinimumSize()")
    {
        sf::WindowBase windowBase(windowContext, {.size{100u, 100u}, .title = "WindowBase Tests"});

        windowBase.setMinimumSize({200u, 300u});
        CHECK(windowBase.getSize() == sf::Vector2u{200, 300});
        windowBase.setMaximumSize({200u, 300u});
    }

    SECTION("setMinimumSize()")
    {
        sf::WindowBase windowBase(windowContext, {.size{400u, 400u}, .title = "WindowBase Tests"});

        windowBase.setMaximumSize({200u, 300u});
        CHECK(windowBase.getSize() == sf::Vector2u{200, 300});
        windowBase.setMinimumSize({200u, 300u});
    }

    // Test for compilation but do not run. This code sometimes hangs indefinitely
    // when running on the BuildBot CI pipeline. Because it contains no
    // assertions we have nothing to gain by running it anyways
    (void)[&windowContext]
    {
        sf::WindowBase windowBase(windowContext, {.size{360u, 240u}, .title = "WindowBase Tests"});

        // Should compile if user provides only a specific handler
        windowBase.pollAndHandleEvents([](sf::Event::Closed) {});

        // Should compile if user provides only a catch-all
        windowBase.pollAndHandleEvents([](const auto&) {});

        // Should compile if user provides both a specific handler and a catch-all
        windowBase.pollAndHandleEvents([](sf::Event::Closed) {}, [](const auto&) {});
    };
}
