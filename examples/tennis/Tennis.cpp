////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/View.hpp"

#include "SFML/Audio/AudioContext.hpp"
#include "SFML/Audio/PlaybackDevice.hpp"
#include "SFML/Audio/Sound.hpp"
#include "SFML/Audio/SoundBuffer.hpp"

#include "SFML/Window/Event.hpp"
#include "SFML/Window/EventUtils.hpp"
#include "SFML/Window/Touch.hpp"
#include "SFML/Window/WindowEnums.hpp"
#include "SFML/Window/WindowSettings.hpp"

#include "SFML/System/Angle.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/System/Path.hpp"
#include "SFML/System/String.hpp"
#include "SFML/System/Time.hpp"
#include "SFML/System/Vector2.hpp"

#include <iostream>
#include <random>
#include <string>

#include <cmath>
#include <cstdlib>

#ifdef SFML_SYSTEM_IOS
#include "SFML/Main.hpp"
#endif

sf::Path resourcesDir()
{
#ifdef SFML_SYSTEM_IOS
    return "";
#else
    return "resources";
#endif
}

////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    std::random_device rd;
    std::mt19937       rng(rd());

    // Define some constants
    constexpr sf::Vector2f gameSize{800.f, 600.f};
    constexpr sf::Vector2f paddleSize{25.f, 100.f};
    constexpr float        ballRadius = 10.f;

    // Create the graphics context
    sf::GraphicsContext graphicsContext;

    // Create the window of the application
    sf::RenderWindow window(graphicsContext,
                            {.size         = gameSize.to<sf::Vector2u>(),
                             .bitsPerPixel = 32u,
                             .title        = "SFML Tennis",
                             .style        = sf::Style::Titlebar | sf::Style::Close,
                             .contextSettings{.depthBits = 24u}});

    window.setVerticalSyncEnabled(true);

    // Create an audio context and get the default playback device
    auto audioContext   = sf::AudioContext::create().value();
    auto playbackDevice = sf::PlaybackDevice::createDefault(audioContext).value();

    // Load the sounds used in the game
    const auto ballSoundBuffer = sf::SoundBuffer::loadFromFile(resourcesDir() / "ball.wav").value();
    sf::Sound  ballSound(ballSoundBuffer);

    // Create the SFML logo texture:
    const auto sfmlLogoTexture = sf::Texture::loadFromFile(graphicsContext, resourcesDir() / "sfml_logo.png").value();
    sf::Sprite sfmlLogo(sfmlLogoTexture.getRect());
    sfmlLogo.setPosition({170.f, 160.f});
    sfmlLogo.setZ(0.1f);

    // Create the left paddle
    sf::RectangleShape leftPaddle;
    leftPaddle.setSize(paddleSize - sf::Vector2f{3.f, 3.f});
    leftPaddle.setOutlineThickness(3.f);
    leftPaddle.setOutlineColor(sf::Color::Black);
    leftPaddle.setFillColor({100u, 100u, 200u});
    leftPaddle.setOrigin(paddleSize / 2.f);

    // Create the right paddle
    sf::RectangleShape rightPaddle = leftPaddle;
    rightPaddle.setFillColor({200u, 100u, 100u});

    // Create the ball
    sf::CircleShape ball;
    ball.setRadius(ballRadius - 3);
    ball.setOutlineThickness(2);
    ball.setOutlineColor(sf::Color::Black);
    ball.setFillColor(sf::Color::White);
    ball.setOrigin({ballRadius / 2.f, ballRadius / 2.f});

    // Open the text font
    const auto font = sf::Font::openFromFile(graphicsContext, resourcesDir() / "tuffy.ttf").value();

    // Initialize the pause message
    sf::Text pauseMessage(font);
    pauseMessage.setCharacterSize(40.f);
    pauseMessage.setPosition({170.f, 200.f});
    pauseMessage.setFillColor(sf::Color::White);

#ifdef SFML_SYSTEM_IOS
    pauseMessage.setString("Welcome to SFML Tennis!\nTouch the screen to start the game.");
#else
    pauseMessage.setString("Welcome to SFML Tennis!\n\nPress space to start the game.");
#endif

    // Define game constants
    constexpr float paddleSpeed = 400.f;
    constexpr float ballSpeed   = 400.f;

    // Define the paddles properties
    sf::Clock      aiTimer;
    const sf::Time aiTime           = sf::seconds(0.1f);
    float          rightPaddleSpeed = 0.f;
    sf::Angle      ballAngle        = sf::degrees(0); // to be changed later

    sf::Clock clock;
    bool      isPlaying = false;

    while (true)
    {
        // Handle events
        while (const sf::base::Optional event = window.pollEvent())
        {
            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return EXIT_SUCCESS;

            // Space key pressed: play
            if ((event->is<sf::Event::KeyPressed>() &&
                 event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Space) ||
                event->is<sf::Event::TouchBegan>())
            {
                if (!isPlaying)
                {
                    // (re)start the game
                    isPlaying = true;
                    clock.restart();

                    // Reset the position of the paddles and ball
                    leftPaddle.setPosition({10.f + paddleSize.x / 2.f, gameSize.y / 2.f});
                    rightPaddle.setPosition({gameSize.x - 10.f - paddleSize.x / 2.f, gameSize.y / 2.f});
                    ball.setPosition(gameSize / 2.f);

                    // Reset the ball angle
                    do
                    {
                        // Make sure the ball initial angle is not too much vertical
                        ballAngle = sf::degrees(std::uniform_real_distribution<float>(0, 360)(rng));
                    } while (std::abs(std::cos(ballAngle.asRadians())) < 0.7f);
                }
            }

            // Window size changed, adjust view appropriately
            if (event->is<sf::Event::Resized>())
                window.setView({/* center */ gameSize / 2.f, /* size */ gameSize});
        }

        if (isPlaying)
        {
            const float deltaTime = clock.restart().asSeconds();

            // Move the player's paddle
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) && (leftPaddle.getPosition().y - paddleSize.y / 2 > 5.f))
            {
                leftPaddle.move({0.f, -paddleSpeed * deltaTime});
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) &&
                (leftPaddle.getPosition().y + paddleSize.y / 2 < gameSize.y - 5.f))
            {
                leftPaddle.move({0.f, paddleSpeed * deltaTime});
            }

            if (sf::Touch::isDown(0))
            {
                const sf::Vector2i pos       = sf::Touch::getPosition(0);
                const sf::Vector2f mappedPos = window.mapPixelToCoords(pos);
                leftPaddle.setPosition({leftPaddle.getPosition().x, mappedPos.y});
            }

            // Move the computer's paddle
            if (((rightPaddleSpeed < 0.f) && (rightPaddle.getPosition().y - paddleSize.y / 2 > 5.f)) ||
                ((rightPaddleSpeed > 0.f) && (rightPaddle.getPosition().y + paddleSize.y / 2 < gameSize.y - 5.f)))
            {
                rightPaddle.move({0.f, rightPaddleSpeed * deltaTime});
            }

            // Update the computer's paddle direction according to the ball position
            if (aiTimer.getElapsedTime() > aiTime)
            {
                aiTimer.restart();
                if (ball.getPosition().y + ballRadius > rightPaddle.getPosition().y + paddleSize.y / 2)
                    rightPaddleSpeed = paddleSpeed;
                else if (ball.getPosition().y - ballRadius < rightPaddle.getPosition().y - paddleSize.y / 2)
                    rightPaddleSpeed = -paddleSpeed;
                else
                    rightPaddleSpeed = 0.f;
            }

            // Move the ball
            ball.move(sf::Vector2f::fromAngle(ballSpeed * deltaTime, ballAngle));

#ifdef SFML_SYSTEM_IOS
            const std::string inputString = "Touch the screen to restart.";
#else
            const std::string inputString = "Press space to restart or\nescape to exit.";
#endif

            // Check collisions between the ball and the screen
            if (ball.getPosition().x - ballRadius < 0.f)
            {
                isPlaying = false;
                pauseMessage.setString("You Lost!\n\n" + inputString);
            }
            else if (ball.getPosition().x + ballRadius > gameSize.x)
            {
                isPlaying = false;
                pauseMessage.setString("You Won!\n\n" + inputString);
            }

            if (ball.getPosition().y - ballRadius < 0.f)
            {
                ballSound.play(playbackDevice);
                ballAngle = -ballAngle;
                ball.setPosition({ball.getPosition().x, ballRadius + 0.1f});
            }
            else if (ball.getPosition().y + ballRadius > gameSize.y)
            {
                ballSound.play(playbackDevice);
                ballAngle = -ballAngle;
                ball.setPosition({ball.getPosition().x, gameSize.y - ballRadius - 0.1f});
            }

            std::uniform_real_distribution<float> dist(0, 20);

            // Check the collisions between the ball and the paddles
            // Left Paddle
            if (ball.getPosition().x - ballRadius < leftPaddle.getPosition().x + paddleSize.x / 2 &&
                ball.getPosition().x - ballRadius > leftPaddle.getPosition().x &&
                ball.getPosition().y + ballRadius >= leftPaddle.getPosition().y - paddleSize.y / 2 &&
                ball.getPosition().y - ballRadius <= leftPaddle.getPosition().y + paddleSize.y / 2)
            {
                if (ball.getPosition().y > leftPaddle.getPosition().y)
                    ballAngle = sf::degrees(180) - ballAngle + sf::degrees(dist(rng));
                else
                    ballAngle = sf::degrees(180) - ballAngle - sf::degrees(dist(rng));

                ballSound.play(playbackDevice);
                ball.setPosition({leftPaddle.getPosition().x + ballRadius + paddleSize.x / 2 + 0.1f, ball.getPosition().y});
            }

            // Right Paddle
            if (ball.getPosition().x + ballRadius > rightPaddle.getPosition().x - paddleSize.x / 2 &&
                ball.getPosition().x + ballRadius < rightPaddle.getPosition().x &&
                ball.getPosition().y + ballRadius >= rightPaddle.getPosition().y - paddleSize.y / 2 &&
                ball.getPosition().y - ballRadius <= rightPaddle.getPosition().y + paddleSize.y / 2)
            {
                if (ball.getPosition().y > rightPaddle.getPosition().y)
                    ballAngle = sf::degrees(180) - ballAngle + sf::degrees(dist(rng));
                else
                    ballAngle = sf::degrees(180) - ballAngle - sf::degrees(dist(rng));

                ballSound.play(playbackDevice);
                ball.setPosition({rightPaddle.getPosition().x - ballRadius - paddleSize.x / 2 - 0.1f, ball.getPosition().y});
            }
        }

        // Clear the window
        window.clear({50u, 50u, 50u});

        if (isPlaying)
        {
            // Draw the paddles and the ball
            window.draw(leftPaddle, /* texture */ nullptr);
            window.draw(rightPaddle, /* texture */ nullptr);
            window.draw(ball, /* texture */ nullptr);
        }
        else
        {
            // Draw the pause message
            window.draw(pauseMessage);
            window.draw(sfmlLogo, sfmlLogoTexture);
        }

        // Display things on screen
        window.display();
    }

    std::cout << "CLEANING UP" << std::endl;

    return EXIT_SUCCESS;
}
