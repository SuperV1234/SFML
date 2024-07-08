////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <SFML/Window/GraphicsContext.hpp>

#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Path.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cmath>

HWND button;


////////////////////////////////////////////////////////////
/// Function called whenever one of our windows receives a message
///
////////////////////////////////////////////////////////////
LRESULT CALLBACK onEvent(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        // Quit when we close the main window
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }

        // Quit when we click the "quit" button
        case WM_COMMAND:
        {
            if (reinterpret_cast<HWND>(lParam) == button)
            {
                PostQuitMessage(0);
                return 0;
            }
        }
    }

    return DefWindowProc(handle, message, wParam, lParam);
}


////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \param Instance: Instance of the application
///
/// \return Error code
///
////////////////////////////////////////////////////////////
int main()
{
    HINSTANCE instance = GetModuleHandle(nullptr);

    // Define a class for our main window
    WNDCLASS windowClass;
    windowClass.style         = 0;
    windowClass.lpfnWndProc   = &onEvent;
    windowClass.cbClsExtra    = 0;
    windowClass.cbWndExtra    = 0;
    windowClass.hInstance     = instance;
    windowClass.hIcon         = nullptr;
    windowClass.hCursor       = nullptr;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND);
    windowClass.lpszMenuName  = nullptr;
    windowClass.lpszClassName = TEXT("SFML App");
    RegisterClass(&windowClass);

    // Let's create the main window
    HWND window = CreateWindow(TEXT("SFML App"),
                               TEXT("SFML Win32"),
                               WS_SYSMENU | WS_VISIBLE,
                               200,
                               200,
                               660,
                               520,
                               nullptr,
                               nullptr,
                               instance,
                               nullptr);

    // Add a button for exiting
    button = CreateWindow(TEXT("BUTTON"), TEXT("Quit"), WS_CHILD | WS_VISIBLE, 560, 440, 80, 40, window, nullptr, instance, nullptr);

    // Let's create two SFML views
    HWND view1 = CreateWindow(TEXT("STATIC"),
                              nullptr,
                              WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                              20,
                              20,
                              300,
                              400,
                              window,
                              nullptr,
                              instance,
                              nullptr);
    HWND view2 = CreateWindow(TEXT("STATIC"),
                              nullptr,
                              WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                              340,
                              20,
                              300,
                              400,
                              window,
                              nullptr,
                              instance,
                              nullptr);

    sf::GraphicsContext graphicsContext;

    {
        sf::RenderWindow sfmlView1(graphicsContext, view1);
        sf::RenderWindow sfmlView2(graphicsContext, view2);

        // Load some textures to display
        const auto texture1 = sf::Texture::loadFromFile(graphicsContext, "resources/image1.jpg").value();
        const auto texture2 = sf::Texture::loadFromFile(graphicsContext, "resources/image2.jpg").value();
        sf::Sprite sprite1(texture1.getRect());
        sf::Sprite sprite2(texture2.getRect());
        sprite1.setOrigin(texture1.getSize().to<sf::Vector2f>() / 2.f);
        sprite1.setPosition(sprite1.getOrigin());

        // Create a clock for measuring elapsed time
        const sf::Clock clock;

        // Loop until a WM_QUIT message is received
        MSG message;
        message.message = static_cast<UINT>(~WM_QUIT);
        while (message.message != WM_QUIT)
        {
            if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
            {
                // If a message was waiting in the message queue, process it
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
            else
            {
                const float time = clock.getElapsedTime().asSeconds();

                // Clear views
                sfmlView1.clear();
                sfmlView2.clear();

                // Draw sprite 1 on view 1
                sprite1.setRotation(sf::degrees(time * 100));
                sfmlView1.draw(sprite1, texture1);

                // Draw sprite 2 on view 2
                sprite2.setPosition({std::cos(time) * 100.f, 0.f});
                sfmlView2.draw(sprite2, texture2);

                // Display each view on screen
                sfmlView1.display();
                sfmlView2.display();
            }
        }
    }

    // Destroy the main window (all its child controls will be destroyed)
    DestroyWindow(window);

    // Don't forget to unregister the window class
    UnregisterClass(TEXT("SFML App"), instance);
}
