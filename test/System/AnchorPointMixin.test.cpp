#include "SFML/System/AnchorPointMixin.hpp"

#include "SFML/System/Rect.hpp"

#include <Doctest.hpp>


namespace
{
////////////////////////////////////////////////////////////
constexpr sf::FloatRect testRect{{53.f, 88.f}, {512.f, 5839.f}};


////////////////////////////////////////////////////////////
struct TestLayoutObject : sf::AnchorPointMixin<TestLayoutObject>
{
    constexpr TestLayoutObject() = default;

    [[nodiscard]] constexpr sf::FloatRect getLocalBounds() const
    {
        return {{0.f, 0.f}, {512.f, 5839.f}};
    }

    [[nodiscard]] constexpr sf::FloatRect getGlobalBounds() const
    {
        const auto localBounds = getLocalBounds();
        return {position + localBounds.position, localBounds.size};
    }

    sf::Vector2f position{42.f, 55.f};
};


////////////////////////////////////////////////////////////
consteval bool doSetAnchorPointTest(sf::Vector2f factors)
{
    constexpr sf::Vector2f newPos{24.f, 24.f};

    TestLayoutObject testObject;
    testObject.setAnchorPoint(factors, newPos);
    return testObject.position == newPos - sf::Vector2f{testRect.size.x * factors.x, testRect.size.y * factors.y};
};


////////////////////////////////////////////////////////////
TEST_CASE("[System] sf::AnchorPointMixin")
{
    SECTION("getAnchorPoint")
    {
        constexpr TestLayoutObject testObject;

        STATIC_CHECK(testObject.getAnchorPoint({0.f, 0.f}) == testObject.getTopLeft());
        STATIC_CHECK(testObject.getAnchorPoint({0.5f, 0.f}) == testObject.getTopCenter());
        STATIC_CHECK(testObject.getAnchorPoint({1.f, 0.f}) == testObject.getTopRight());
        STATIC_CHECK(testObject.getAnchorPoint({0.f, 0.5f}) == testObject.getCenterLeft());
        STATIC_CHECK(testObject.getAnchorPoint({0.5f, 0.5f}) == testObject.getCenter());
        STATIC_CHECK(testObject.getAnchorPoint({1.f, 0.5f}) == testObject.getCenterRight());
        STATIC_CHECK(testObject.getAnchorPoint({0.f, 1.f}) == testObject.getBottomLeft());
        STATIC_CHECK(testObject.getAnchorPoint({0.5f, 1.f}) == testObject.getBottomCenter());
        STATIC_CHECK(testObject.getAnchorPoint({1.f, 1.f}) == testObject.getBottomRight());
    }

    SECTION("setAnchorPoint")
    {
        STATIC_CHECK(doSetAnchorPointTest({0.f, 0.f}));
        STATIC_CHECK(doSetAnchorPointTest({0.5f, 0.f}));
        STATIC_CHECK(doSetAnchorPointTest({1.f, 0.f}));
        STATIC_CHECK(doSetAnchorPointTest({0.f, 0.5f}));
        STATIC_CHECK(doSetAnchorPointTest({0.5f, 0.5f}));
        STATIC_CHECK(doSetAnchorPointTest({1.f, 0.5f}));
        STATIC_CHECK(doSetAnchorPointTest({0.f, 1.f}));
        STATIC_CHECK(doSetAnchorPointTest({0.5f, 1.f}));
        STATIC_CHECK(doSetAnchorPointTest({1.f, 1.f}));
    }
}

} // namespace
