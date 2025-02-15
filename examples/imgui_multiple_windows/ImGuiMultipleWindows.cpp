#include "SFML/ImGui/ImGui.hpp"

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/RenderWindow.hpp"

#include "SFML/Window/Event.hpp"
#include "SFML/Window/EventUtils.hpp"

#include "SFML/System/Clock.hpp"

#include "SFML/Base/Optional.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

int main()
{
    auto                    graphicsContext = sf::GraphicsContext::create().value();
    sf::ImGui::ImGuiContext imGuiContext;

    sf::RenderWindow window({.size{1280u, 720u}, .title = "ImGui + SFML = <3", .vsync = true});
    if (!imGuiContext.init(window))
        return -1;

    sf::base::Optional<sf::RenderWindow>
        childWindow(sf::base::inPlace,
                    sf::RenderWindow::Settings{.size{640u, 480u}, .title = "ImGui-SFML Child window", .vsync = true});

    if (!imGuiContext.init(*childWindow))
        return -1;

    sf::Clock deltaClock;
    while (true)
    {
        // Main window event processing
        while (const sf::base::Optional event = window.pollEvent())
        {
            imGuiContext.processEvent(window, *event);

            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return 0;
        }

        // Update
        const sf::Time dt = deltaClock.restart();
        imGuiContext.update(window, dt);

        // Add ImGui widgets in the first window
        imGuiContext.setCurrentWindow(window);
        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        ImGui::ShowDemoWindow();

        // Main window drawing
        const sf::CircleShape shape{{.fillColor = sf::Color::Green, .radius = 100.f}};

        window.clear();
        window.draw(shape, /* texture */ nullptr);
        imGuiContext.render(window);
        window.display();

        const auto processChildWindow = [&](sf::RenderWindow& childWindowRef)
        {
            while (const sf::base::Optional event = childWindowRef.pollEvent())
            {
                imGuiContext.processEvent(childWindowRef, *event);

                if (event->is<sf::Event::Closed>())
                {
                    imGuiContext.shutdown(childWindowRef);
                    childWindow.reset();
                    return;
                }
            }

            imGuiContext.update(childWindowRef, dt);

            imGuiContext.setCurrentWindow(childWindowRef);
            ImGui::Begin("Works in a second window!");
            ImGui::Button("Example button");
            ImGui::End();

            const sf::CircleShape shape2{{.fillColor = sf::Color::Red, .radius = 50.f}};

            childWindowRef.clear();
            childWindowRef.draw(shape2, /* texture */ nullptr);
            imGuiContext.render(childWindowRef);
            childWindowRef.display();
        };

        // Child window event processing
        if (childWindow.hasValue())
            processChildWindow(*childWindow);
    }
}
