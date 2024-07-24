////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>

#include <SFML/Audio/AudioContext.hpp>
#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/EventUtils.hpp>
#include <SFML/Window/GraphicsContext.hpp>
#include <SFML/Window/Touch.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>

#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Path.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

// TODO P0:
#ifdef SFML_SYSTEM_EMSCRIPTEN
#include <emscripten.h>
#endif

#include <random>
#include <string>

#include <cmath>
#include <cstdlib>

#ifdef SFML_SYSTEM_IOS
#include <SFML/Main.hpp>
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
    const float        gameWidth  = 800;
    const float        gameHeight = 600;
    const sf::Vector2f paddleSize(25, 100);
    const float        ballRadius = 10.f;

    // Create the graphics context
    sf::GraphicsContext graphicsContext;

    // Create the window of the application
    sf::RenderWindow window(graphicsContext,
                            sf::VideoMode({static_cast<unsigned int>(gameWidth), static_cast<unsigned int>(gameHeight)}, 32),
                            "SFML Tennis",
                            sf::Style::Titlebar | sf::Style::Close);

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
    sfmlLogo.setPosition({170.f, 50.f});

    // Create the left paddle
    sf::RectangleShape leftPaddle;
    leftPaddle.setSize(paddleSize - sf::Vector2f{3, 3});
    leftPaddle.setOutlineThickness(3);
    leftPaddle.setOutlineColor(sf::Color::Black);
    leftPaddle.setFillColor(sf::Color(100, 100, 200));
    leftPaddle.setOrigin(paddleSize / 2.f);

    // Create the right paddle
    sf::RectangleShape rightPaddle;
    rightPaddle.setSize(paddleSize - sf::Vector2f{3, 3});
    rightPaddle.setOutlineThickness(3);
    rightPaddle.setOutlineColor(sf::Color::Black);
    rightPaddle.setFillColor(sf::Color(200, 100, 100));
    rightPaddle.setOrigin(paddleSize / 2.f);

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
    pauseMessage.setCharacterSize(40);
    pauseMessage.setPosition({170.f, 200.f});
    pauseMessage.setFillColor(sf::Color::White);

#ifdef SFML_SYSTEM_IOS
    pauseMessage.setString("Welcome to SFML Tennis!\nTouch the screen to start the game.");
#else
    pauseMessage.setString("Welcome to SFML Tennis!\n\nPress space to start the game.");
#endif

    // Define the paddles properties
    sf::Clock      aiTimer;
    const sf::Time aiTime           = sf::seconds(0.1f);
    const float    paddleSpeed      = 400.f;
    float          rightPaddleSpeed = 0.f;
    const float    ballSpeed        = 400.f;
    sf::Angle      ballAngle        = sf::degrees(0); // to be changed later

    sf::Clock clock;
    bool      isPlaying = false;
    auto      mainLoop  = [&]
    {
        // Handle events
        while (const sf::base::Optional event = window.pollEvent())
        {
            // TODO P0:
            // if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
            // return EXIT_SUCCESS;

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
                    leftPaddle.setPosition({10.f + paddleSize.x / 2.f, gameHeight / 2.f});
                    rightPaddle.setPosition({gameWidth - 10.f - paddleSize.x / 2.f, gameHeight / 2.f});
                    ball.setPosition({gameWidth / 2.f, gameHeight / 2.f});

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
            {
                sf::View view;
                view.setSize({gameWidth, gameHeight});
                view.setCenter({gameWidth / 2.f, gameHeight / 2.f});
                window.setView(view);
            }
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
                (leftPaddle.getPosition().y + paddleSize.y / 2 < gameHeight - 5.f))
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
                ((rightPaddleSpeed > 0.f) && (rightPaddle.getPosition().y + paddleSize.y / 2 < gameHeight - 5.f)))
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
            if (ball.getPosition().x + ballRadius > gameWidth)
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
            if (ball.getPosition().y + ballRadius > gameHeight)
            {
                ballSound.play(playbackDevice);
                ballAngle = -ballAngle;
                ball.setPosition({ball.getPosition().x, gameHeight - ballRadius - 0.1f});
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
        window.clear(sf::Color(50, 50, 50));

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
    };

// TODO P0:
#ifdef SFML_SYSTEM_EMSCRIPTEN
    static auto f          = mainLoop;
    auto        mainLoopFn = [] { f(); };

    emscripten_set_main_loop(mainLoopFn, 0, 1);
#endif

    return EXIT_SUCCESS;
}
