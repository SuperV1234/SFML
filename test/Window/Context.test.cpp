
#include "SFML/Window/WindowBase.hpp"
#include "SFML/Window/WindowContext.hpp"

// Other 1st party headers
#include "SFML/Window/ContextSettings.hpp"

#include "SFML/System/Err.hpp"

#include "SFML/Base/Macros.hpp"
#include "SFML/Base/UniquePtr.hpp"

#include <Doctest.hpp>

#include <CommonTraits.hpp>
#include <WindowUtil.hpp>

#include <string>

#if defined(SFML_SYSTEM_WINDOWS)
#define GLAPI __stdcall
#else
#define GLAPI
#endif

namespace sf::priv
{
class GlContext
{
public:
    [[nodiscard]] const ContextSettings& getSettings() const;
    [[nodiscard]] unsigned int           getId() const;
};

}; // namespace sf::priv

struct TestContext
{
    TestContext() : glContext(sf::WindowContext::createGlContext())
    {
        if (!setActive(true))
            sf::priv::err() << "Failed to set context as active during construction";
    }

    ~TestContext()
    {
        if (glContext != nullptr && !setActive(false))
            sf::priv::err() << "Failed to set context as inactive during destruction";
    }

    [[nodiscard]] bool setActive(bool active) const
    {
        return sf::WindowContext::setActiveThreadLocalGlContext(*glContext, active);
    }

    TestContext(const TestContext&) = delete;

    TestContext& operator=(const TestContext&) = delete;

    TestContext(TestContext&& rhs) noexcept = default;

    TestContext& operator=(TestContext&& rhs) noexcept = default;

    [[nodiscard]] const sf::ContextSettings& getSettings() const
    {
        return glContext->getSettings();
    }

    sf::base::UniquePtr<sf::priv::GlContext> glContext;

    [[nodiscard]] static unsigned int getActiveThreadLocalGlContextId()
    {
        return sf::WindowContext::getActiveThreadLocalGlContextId();
    }

    [[nodiscard]] static bool hasActiveThreadLocalGlContext()
    {
        return sf::WindowContext::hasActiveThreadLocalGlContext();
    }

    [[nodiscard]] static bool hasActiveThreadLocalOrSharedGlContext()
    {
        return sf::WindowContext::hasActiveThreadLocalOrSharedGlContext();
    }

    [[nodiscard]] static bool isExtensionAvailable(const char* name)
    {
        return sf::WindowContext::isExtensionAvailable(name);
    }

    [[nodiscard]] static auto getFunction(const char* name)
    {
        return sf::WindowContext::getFunction(name);
    }
};

TEST_CASE("[Window] TestContext" * doctest::skip(skipDisplayTests))
{
    CHECK(!sf::WindowContext::isInstalled());
    auto windowContext = sf::WindowContext::create().value();
    CHECK(sf::WindowContext::isInstalled());

    SECTION("Type traits")
    {
        STATIC_CHECK(!SFML_BASE_IS_COPY_CONSTRUCTIBLE(TestContext));
        STATIC_CHECK(!SFML_BASE_IS_COPY_ASSIGNABLE(TestContext));
        STATIC_CHECK(SFML_BASE_IS_NOTHROW_MOVE_CONSTRUCTIBLE(TestContext));
        STATIC_CHECK(SFML_BASE_IS_NOTHROW_MOVE_ASSIGNABLE(TestContext));
    }

    SECTION("Construction")
    {
        const TestContext context;
        CHECK(context.getSettings().majorVersion > 0);
        CHECK(TestContext::getActiveThreadLocalGlContextId() == context.glContext->getId());
        CHECK(TestContext::hasActiveThreadLocalGlContext());
    }

    SECTION("Move semantics")
    {
        SECTION("Construction")
        {
            SECTION("From active context")
            {
                TestContext       movedContext;
                const TestContext context(SFML_BASE_MOVE(movedContext));
                CHECK(context.getSettings().majorVersion > 0);
                CHECK(TestContext::getActiveThreadLocalGlContextId() == context.glContext->getId());
                CHECK(TestContext::hasActiveThreadLocalGlContext());
            }

            SECTION("From inactive context")
            {
                TestContext movedContext;
                CHECK(movedContext.setActive(false));
                CHECK(TestContext::getActiveThreadLocalGlContextId() == 0u);
                CHECK(!TestContext::hasActiveThreadLocalGlContext());

                const TestContext context(SFML_BASE_MOVE(movedContext));
                CHECK(context.getSettings().majorVersion > 0);
                CHECK(TestContext::getActiveThreadLocalGlContextId() == 0u);
                CHECK(!TestContext::hasActiveThreadLocalGlContext());
            }
        }

        SECTION("Assignment")
        {
            SECTION("From active context")
            {
                TestContext movedContext;
                TestContext context;
                CHECK(movedContext.setActive(true));
                CHECK(TestContext::getActiveThreadLocalGlContextId() == movedContext.glContext->getId());
                CHECK(TestContext::hasActiveThreadLocalGlContext());

                context = SFML_BASE_MOVE(movedContext);
                CHECK(context.getSettings().majorVersion > 0);
                CHECK(TestContext::getActiveThreadLocalGlContextId() == context.glContext->getId());
                CHECK(TestContext::hasActiveThreadLocalGlContext());
            }

            SECTION("From inactive context")
            {
                TestContext movedContext;
                CHECK(movedContext.setActive(false));
                CHECK(TestContext::getActiveThreadLocalGlContextId() == 0u);
                CHECK(!TestContext::hasActiveThreadLocalGlContext());

                // TODO P1: fix
                /*
                TestContext context;
                context = SFML_BASE_MOVE(movedContext);
                CHECK(context.getSettings().majorVersion > 0);
                CHECK(TestContext::getActiveThreadLocalGlContextId() == 0u);
                CHECK(!TestContext::hasActiveThreadLocalGlContext());
                */
            }
        }
    }

    SECTION("setActive()")
    {
        {
            TestContext context;
            const auto  contextId = TestContext::getActiveThreadLocalGlContextId();

            // Set inactive
            CHECK(context.setActive(false));
            CHECK(TestContext::getActiveThreadLocalGlContextId() == 0u);
            CHECK(!TestContext::hasActiveThreadLocalGlContext());

            // Set active
            CHECK(context.setActive(true));
            CHECK(TestContext::getActiveThreadLocalGlContextId() == context.glContext->getId());
            CHECK(TestContext::getActiveThreadLocalGlContextId() == contextId);

            // Create new context which becomes active automatically
            const TestContext newContext;
            CHECK(TestContext::getActiveThreadLocalGlContextId() == newContext.glContext->getId());
            const auto newContextId = TestContext::getActiveThreadLocalGlContextId();
            CHECK(newContextId != 0);

            // Set old context as inactive but new context remains active
            CHECK(context.setActive(false));
            CHECK(TestContext::getActiveThreadLocalGlContextId() == newContext.glContext->getId());
            CHECK(TestContext::getActiveThreadLocalGlContextId() == newContextId);

            // Set old context as active again
            CHECK(context.setActive(true));
            CHECK(TestContext::getActiveThreadLocalGlContextId() == context.glContext->getId());
            CHECK(TestContext::getActiveThreadLocalGlContextId() == contextId);
        }

        CHECK(TestContext::getActiveThreadLocalGlContextId() == 0u);
        CHECK(!TestContext::hasActiveThreadLocalGlContext());
    }

    SECTION("getActiveThreadLocalGlContextId()/getActiveThreadLocalGlContextId()")
    {
        {
            const TestContext context;
            CHECK(context.getSettings().majorVersion > 0);
            CHECK(TestContext::getActiveThreadLocalGlContextId() == context.glContext->getId());
            CHECK(TestContext::hasActiveThreadLocalGlContext());
        }

        CHECK(TestContext::getActiveThreadLocalGlContextId() == 0u);
        CHECK(!TestContext::hasActiveThreadLocalGlContext());
    }

    SECTION("Version String")
    {
        TestContext context;
        CHECK(context.setActive(true));

        using glGetStringFuncType  = const char*(GLAPI*)(unsigned int);
        const auto glGetStringFunc = reinterpret_cast<glGetStringFuncType>(TestContext::getFunction("glGetString"));
        REQUIRE(glGetStringFunc);

        constexpr unsigned int glVendor   = 0x1F00;
        constexpr unsigned int glRenderer = 0x1F01;
        constexpr unsigned int glVersion  = 0x1F02;

        const char* vendor   = glGetStringFunc(glVendor);
        const char* renderer = glGetStringFunc(glRenderer);
        const char* version  = glGetStringFunc(glVersion);

        REQUIRE(vendor != nullptr);
        REQUIRE(renderer != nullptr);
        REQUIRE(version != nullptr);

        SUCCEED(std::string("OpenGL vendor: ") + vendor);
        SUCCEED(std::string("OpenGL renderer: ") + renderer);
        SUCCEED(std::string("OpenGL version: ") + version);
    }

    SECTION("isExtensionAvailable()")
    {
        CHECK(!TestContext::isExtensionAvailable("2024-04-01"));
        CHECK(!TestContext::isExtensionAvailable("let's assume this extension does not exist"));
    }

    SECTION("getFunction()")
    {
        const TestContext context; // Windows requires an active context to use getFunction
        CHECK(TestContext::getFunction("glEnable"));
        CHECK(TestContext::getFunction("glGetError"));
        CHECK(TestContext::getFunction("glGetIntegerv"));
        CHECK(TestContext::getFunction("glGetString"));
        CHECK(TestContext::getFunction("glGetStringi"));
        CHECK(TestContext::getFunction("glIsEnabled"));
    }
}
