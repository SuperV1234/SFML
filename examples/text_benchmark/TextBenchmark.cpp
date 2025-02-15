
#include "SFML/ImGui/ImGui.hpp"

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/DrawableBatch.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/PrimitiveType.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RenderTexture.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Shader.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/TextureAtlas.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "SFML/Window/ContextSettings.hpp"
#include "SFML/Window/Event.hpp"
#include "SFML/Window/EventUtils.hpp"

#include "SFML/System/AnchorPointMixin.hpp"
#include "SFML/System/Angle.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/System/Path.hpp"
#include "SFML/System/Rect.hpp"
#include "SFML/System/String.hpp"
#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Optional.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <array>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <cstddef>


////////////////////////////////////////////////////////////

#if 1

int main()
{
    auto graphicsContext = sf::GraphicsContext::create().value();
    sf::RenderWindow window({.size{800u, 600u}, .title = L"महसुस", .contextSettings = {.antiAliasingLevel = 4}});


    const float width     = 128.f;
    const float height    = 64.f;
    const float halfWidth = width / 2.f;

    const sf::Vector2u size{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};

    const auto     font0 = sf::Font::openFromFile("resources/tuffy.ttf").value();
    const sf::Text text0(font0, {.position = {0u, 0u}, .string = "Test", .characterSize = 16u});

    auto image   = sf::Image::create(size, sf::Color::White).value();
    auto texture = sf::Texture::loadFromImage(image).value();

    auto baseRenderTexture = sf::RenderTexture::create(size, {.antiAliasingLevel = 0, .sRgbCapable = true}).value();

    auto baseRenderTextureAA = sf::RenderTexture::create(size, {.antiAliasingLevel = 4, .sRgbCapable = true}).value();

    auto leftInnerRT = sf::RenderTexture::create(size, {.antiAliasingLevel = 4, .sRgbCapable = true}).value();

    const sf::Vertex leftVertexArray[6]{{{0.f, 0.f}, sf::Color::Red, {0.f, 0.f}},
                                        {{halfWidth, 0.f}, sf::Color::Red, {halfWidth, 0.f}},
                                        {{0.f, height}, sf::Color::Red, {0.f, height}},
                                        {{0.f, height}, sf::Color::Green, {0.f, height}},
                                        {{halfWidth, 0.f}, sf::Color::Green, {halfWidth, 0.f}},
                                        {{halfWidth, height}, sf::Color::Green, {halfWidth, height}}};

    leftInnerRT.clear();
    leftInnerRT.draw(texture);


    leftInnerRT.display();


    auto winRT = sf::Texture::create(window.getSize()).value();

    // auto finalImage = baseRenderTexture.getTexture().copyToImage();
    // auto finalTx    = sf::Texture::loadFromImage(finalImage).value();

#define CHECK(...)                                 \
    if (!(__VA_ARGS__))                            \
    {                                              \
        std::cout << "fail " #__VA_ARGS__ << '\n'; \
    }

    sf::Sprite rtSprite{.textureRect = baseRenderTexture.getTexture().getRect()};
    sf::Sprite rtAASprite{.textureRect = baseRenderTextureAA.getTexture().getRect()};

    while (true)
    {
        while (sf::base::Optional event = window.pollEvent())
        {
            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return EXIT_SUCCESS;
        }

        window.clear();

        const auto doit = [&](auto& rt, auto& rts, float xBias)
        {
            rt.clear();
            rt.draw(leftVertexArray, sf::PrimitiveType::Triangles, {.texture = &leftInnerRT.getTexture()});
            rt.display();

            rts.position = {xBias, 0};
            window.draw(rts, rt.getTexture());

            rt.clear();
            rt.draw(leftVertexArray, sf::PrimitiveType::Triangles, {.texture = &leftInnerRT.getTexture()});
            rt.display();

            rts.position = {xBias + 128, 0};
            window.draw(rts, rt.getTexture());

            rt.clear();
            rt.draw(leftVertexArray, sf::PrimitiveType::Triangles, {.texture = &leftInnerRT.getTexture()});
            rt.draw(text0);
            rt.display();

            rts.position = {xBias, 128};
            window.draw(rts, rt.getTexture());
        };

        doit(baseRenderTexture, rtSprite, 0);
        doit(baseRenderTextureAA, rtAASprite, 256);

        bool rc = winRT.update(window, {});
        if (!rc)
            throw 100;

        window.draw(winRT, {.position = {256, 256}, .scale = {0.2f, 0.2f}});
        window.display();
    }
}

#elif 0

int main()
{
    auto graphicsContext = sf::GraphicsContext::create().value();
    sf::RenderWindow window({.size{800u, 600u}, .title = L"महसुस", .contextSettings = {.antiAliasingLevel = 4}});


    sf::Vector2u size = window.getSize();

    auto texture = sf::Texture::loadFromFile("resources/biga.png").value();

    sf::Sprite sprite{.textureRect = texture.getRect()};

    sprite.scale = {(float)size.x / texture.getSize().x, (float)size.y / texture.getSize().y / 2.f};

    auto render = sf::RenderTexture::create({size.x, (unsigned int)(size.y / 2.f)}, {.antiAliasingLevel = 4}).value();

    sf::Sprite rndrSprite{.textureRect = render.getTexture().getRect()};
    rndrSprite.position = {0.f, (float)size.y / 2.f};

    const char* shaderSrc = R"glsl(
// This shader draws texture on the left using RB channels and
// texture2 on the right using G channel
layout(location = 2) uniform sampler2D sf_u_texture;
uniform sampler2D texture2;

in vec4 sf_v_color;
in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
    vec2 normalizedTexCoord = sf_v_texCoord / vec2(textureSize(sf_u_texture, 0));

	vec4 color1 = texture( sf_u_texture, normalizedTexCoord * vec2( 2.0, 1.0 ) + vec2( 0.0, 0.0 ));
	vec4 color2 = texture( texture2, normalizedTexCoord * vec2( 2.0, 1.0 ) + vec2( -1.0, 0.0 ));
	sf_fragColor = sf_v_color * ( vec4(color2.x, color1.y, color2.z, 1.0 ));
}
)glsl";

    auto shader     = sf::Shader::loadFromMemory(shaderSrc, sf::Shader::Type::Fragment).value();
    auto ulTexture2 = shader.getUniformLocation("texture2").value();
    (void)shader.setUniform(ulTexture2, texture);

    while (true)
    {
        while (sf::base::Optional event = window.pollEvent())
        {
            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return EXIT_SUCCESS;
        }

        render.clear();
        render.draw(sprite, texture);
        render.display();

        window.clear();
        window.draw(sprite, texture, {.shader = &shader});
        window.draw(rndrSprite, render.getTexture(), {.shader = &shader});
        window.display();
    }

    return 0;
}

#elif 1

int main()
{
    auto             graphicsContext = sf::GraphicsContext::create().value();
    sf::RenderWindow window({.size{800u, 600u}, .title = L"महसुस"});

    sf::RectangleShape rs0(
        {.position         = {250.f, 250.f},
         .origin           = {0.f, 0.f},
         .fillColor        = sf::Color::Red,
         .outlineColor     = sf::Color::Yellow,
         .outlineThickness = 3.f,
         .size             = {64.f, 64.f}});

    sf::RectangleShape cs0(
        {.position         = {450.f, 450.f},
         .origin           = {-25.f, 50.f},
         .fillColor        = sf::Color::Blue,
         .outlineColor     = sf::Color::Yellow,
         .outlineThickness = 2.f,
         .size             = {36.f, 36.f}});

    rs0.setTopRight(window.getTopRight());

    while (true)
    {
        while (sf::base::Optional event = window.pollEvent())
        {
            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return EXIT_SUCCESS;
        }

        window.clear();

        window.draw(rs0, /* texture */ nullptr);
        window.draw(cs0, /* texture */ nullptr);

        cs0.rotation += sf::radians(0.0005f);
        cs0.setCenter(rs0.getBottomLeft());

        window.display();
    }
}

#elif 0

int main()
{
    auto graphicsContext = sf::GraphicsContext::create().value();

    sf::RenderWindow window({.size{800u, 600u}, .title = L"महसुस"});

    auto textureAtlas = sf::TextureAtlas{sf::Texture::create({1024u, 1024u}).value()};

    const auto font0 = sf::Font::openFromFile("resources/tuffy.ttf", &textureAtlas).value();
    const auto font1 = sf::Font::openFromFile("resources/mouldycheese.ttf", &textureAtlas).value();

    const auto sfmlLogoImage     = sf::Image::loadFromFile("resources/sfml_logo.png").value();
    const auto sfmlLogoAtlasRect = textureAtlas.add(sfmlLogoImage).value();

    const auto whiteDotAtlasRect = textureAtlas.add(graphicsContext.getBuiltInWhiteDotTexture()).value();

    sf::Sprite sfmlLogo(sfmlLogoAtlasRect);

    const sf::Text text0(font0, {.position = {0u, 0u}, .string = "Test", .characterSize = 128u});
    const sf::Text text1(font0, {.position = {128u, 0u}, .string = "acbasdfbFOOBAR", .characterSize = 32u});
    const sf::Text text2(font0, {.position = {0u, 128u}, .string = "ssdfbsdbfudsy", .characterSize = 64u});
    const sf::Text text3(font1, {.position = {128u, 128u}, .string = "Test", .characterSize = 128u});
    const sf::Text text4(font1, {.position = {256u, 128u}, .string = "FOmfgj,ryfkmtdfOBAR", .characterSize = 32u});
    const sf::Text text5(font1, {.position = {128u, 256u}, .string = "abscas", .characterSize = 64u});

    const sf::CircleShape circle0{
        {.position           = {350.f, 350.f},
         .scale              = {2.f, 2.f},
         .textureRect        = {.position = whiteDotAtlasRect.position, .size{0.f, 0.f}},
         .outlineTextureRect = {.position = whiteDotAtlasRect.position, .size{0.f, 0.f}},
         .fillColor          = sf::Color::Red,
         .outlineColor       = sf::Color::Yellow,
         .outlineThickness   = 8.f,
         .radius             = 45.f}};

    // Create drawable batch to optimize rendering
    sf::CPUDrawableBatch drawableBatch;

    while (true)
    {
        while (sf::base::Optional event = window.pollEvent())
        {
            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return EXIT_SUCCESS;
        }

        window.clear();

        {
            drawableBatch.clear();

            drawableBatch.add(text0);

            sfmlLogo.position = {170.f, 50.f};
            sfmlLogo.scale    = {1.5f, 1.5f};
            drawableBatch.add(sfmlLogo);

            drawableBatch.add(text1);
            sfmlLogo.position = {100.f, 50.f};
            sfmlLogo.scale    = {1.0f, 1.0f};
            drawableBatch.add(sfmlLogo);

            drawableBatch.add(text2);
            sfmlLogo.position = {300.f, 150.f};
            sfmlLogo.scale    = {1.5f, 1.5f};
            drawableBatch.add(sfmlLogo);

            drawableBatch.add(text3);
            sfmlLogo.position = {250.f, 250.f};
            sfmlLogo.scale    = {1.0f, 1.0f};
            drawableBatch.add(sfmlLogo);

            drawableBatch.add(text4);
            drawableBatch.add(text5);

            drawableBatch.add(circle0);

            window.draw(drawableBatch, {.texture = &textureAtlas.getTexture()});
        }

        // window.draw(circle0, /* texture */ nullptr);

        window.display();
    }

    return -100;
}

#elif defined(FOOOO)

int main()
{
    const float screenWidth  = 800.0f;
    const float screenHeight = 600.0f;

    const sf::Vector2u screenSize{static_cast<unsigned int>(screenWidth), static_cast<unsigned int>(screenHeight)};

    auto graphicsContext = sf::GraphicsContext::create().value();

    std::cout << sf::Texture::getMaximumSize() << '\n';
    return 0;

    // TODO P0: aa level of 4 causes glcheck assert fail on opengl

    sf::RenderWindow window({.size{screenSize},
                             .title = "Window",
                             .vsync = true,
                             .contextSettings{.depthBits = 0, .stencilBits = 0, .antiAliasingLevel = 4}});

    auto image   = sf::Image::create(screenSize, sf::Color::White).value();
    auto texture = sf::Texture::loadFromImage(image).value();

    auto baseRenderTexture = sf::RenderTexture::create(screenSize, sf::ContextSettings{0, 0, 4 /* AA level*/}).value();

    sf::RenderTexture
        renderTextures[2]{sf::RenderTexture::create(screenSize, sf::ContextSettings{0, 0, 4 /* AA level*/}).value(),
                          sf::RenderTexture::create(screenSize, sf::ContextSettings{0, 0, 4 /* AA level*/}).value()};

    std::vector<sf::Vertex> vertexArrays[2];

    while (true)
    {
        while (sf::base::Optional event = window.pollEvent())
        {
            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return EXIT_SUCCESS;
        }

        window.clear();

        vertexArrays[0].clear();
        vertexArrays[1].clear();

        float xCenter = screenWidth / 2;

        vertexArrays[0].emplace_back(sf::Vector2f{0, 0}, sf::Color::White, sf::Vector2f{0, 0});
        vertexArrays[0].emplace_back(sf::Vector2f{xCenter, 0}, sf::Color::White, sf::Vector2f{xCenter, 0});
        vertexArrays[0].emplace_back(sf::Vector2f{0, screenHeight}, sf::Color::White, sf::Vector2f{0, screenHeight});

        vertexArrays[0].emplace_back(sf::Vector2f{0, screenHeight}, sf::Color::White, sf::Vector2f{0, screenHeight});
        vertexArrays[0].emplace_back(sf::Vector2f{xCenter, 0}, sf::Color::White, sf::Vector2f{xCenter, 0});
        vertexArrays[0].emplace_back(sf::Vector2f{xCenter, screenHeight}, sf::Color::White, sf::Vector2f{xCenter, screenHeight});

        // right half of screen
        vertexArrays[1].emplace_back(sf::Vector2f{xCenter, 0}, sf::Color::White, sf::Vector2f{xCenter, 0});
        vertexArrays[1].emplace_back(sf::Vector2f{screenWidth, 0}, sf::Color::White, sf::Vector2f{screenWidth, 0});
        vertexArrays[1].emplace_back(sf::Vector2f{xCenter, screenHeight}, sf::Color::White, sf::Vector2f{xCenter, screenHeight});

        vertexArrays[1].emplace_back(sf::Vector2f{xCenter, screenHeight}, sf::Color::White, sf::Vector2f{xCenter, screenHeight});
        vertexArrays[1].emplace_back(sf::Vector2f{screenWidth, 0}, sf::Color::White, sf::Vector2f{screenWidth, 0});
        vertexArrays[1].emplace_back(sf::Vector2f{screenWidth, screenHeight},
                                     sf::Color::White,
                                     sf::Vector2f{screenWidth, screenHeight});

        renderTextures[0].clear();
        renderTextures[1].clear();

        renderTextures[0].draw(texture);
        renderTextures[1].draw(texture, {.color = sf::Color::Green});

        baseRenderTexture.clear();


        renderTextures[0].display();
        baseRenderTexture.draw(vertexArrays[0],
                               sf::PrimitiveType::Triangles,
                               sf::RenderStates{&renderTextures[0].getTexture()});

        renderTextures[1].display();
        baseRenderTexture.draw(vertexArrays[1],
                               sf::PrimitiveType::Triangles,
                               sf::RenderStates{&renderTextures[1].getTexture()});

        baseRenderTexture.display();

        window.draw(baseRenderTexture.getTexture());
        window.display();
    }

    return 0;
}

#elif defined(BARABARAR)

#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/RenderTexture.hpp"
#include "SFML/Graphics/Text.hpp"

#include "SFML/System/Path.hpp"
#include "SFML/System/String.hpp"

#include <sstream>

int main()
{
    auto graphicsContext = sf::GraphicsContext::create().value();

    sf::RenderWindow window({.size{800u, 600u}, .title = "Test", .vsync = false, .resizable = false});

    const auto font = sf::Font::openFromFile("resources/tuffy.ttf").value();

    sf::Text text(font, "Test", 20);

    sf::RenderTexture renderTexture[10]{sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value(),
                                        sf::RenderTexture::create({800u, 600u}).value()};

    ;

    sf::Clock          clock;
    std::ostringstream sstr;

    while (true)
    {
        while (sf::base::Optional event = window.pollEvent())
        {
            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return EXIT_SUCCESS;
        }

        for (int i = 0; i < 20; ++i)
        {
            for (auto& j : renderTexture)
            {
                j.clear(sf::Color(0, 0, 0));

                for (auto& u : renderTexture)
                    u.draw(text);

                j.display();
            }

            window.clear(sf::Color(0, 0, 0));

            for (const auto& j : renderTexture)
            {
                for (int k = 0; k < 10; ++k)
                    window.draw(j.getTexture());
            }
        }

        window.display();

        sstr.str("");
        sstr << "Test -- Frame: " << clock.restart().asSeconds() << " sec";

        window.setTitle(sstr.str());
    }

    return EXIT_SUCCESS;
}

#else

#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/RenderTexture.hpp"
#include "SFML/Graphics/Text.hpp"

#include "SFML/System/Path.hpp"
#include "SFML/System/String.hpp"

#include <cstdlib>

int main()
{
    auto graphicsContext = sf::GraphicsContext::create().value();

    const auto       font         = sf::Font::openFromFile("resources/tuffy.ttf").value();
    const sf::String textContents = "abcdefghilmnopqrstuvz\nabcdefghilmnopqrstuvz\nabcdefghilmnopqrstuvz\n";

    auto text          = sf::Text(font, textContents);
    auto renderTexture = sf::RenderTexture::create({1680, 1050}).value();

    renderTexture.clear();

    for (std::size_t i = 0; i < 100'000; ++i)
    {
        text.setOutlineThickness(static_cast<float>(5 + (i % 2)));
        renderTexture.draw(text);
    }

    renderTexture.display();
}

#endif
