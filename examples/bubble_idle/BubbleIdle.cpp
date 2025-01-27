#include "SFML/ImGui/ImGui.hpp"

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/DrawableBatch.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/RenderTexture.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/TextureAtlas.hpp"
#include "SFML/Graphics/View.hpp"

#include "SFML/Audio/AudioContext.hpp"
#include "SFML/Audio/Listener.hpp"
#include "SFML/Audio/Music.hpp"
#include "SFML/Audio/PlaybackDevice.hpp"
#include "SFML/Audio/Sound.hpp"
#include "SFML/Audio/SoundBuffer.hpp"

#include "SFML/Window/EventUtils.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"

#include "SFML/System/Angle.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/System/Path.hpp"
#include "SFML/System/Rect.hpp"
#include "SFML/System/RectUtils.hpp"
#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Math/Ceil.hpp"
#include "SFML/Base/Math/Exp.hpp"
#include "SFML/Base/Math/Sqrt.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/SizeT.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <cstdio>

#define BUBBLEBYTE_VERSION_STR "v0.0.3"


namespace
{
////////////////////////////////////////////////////////////
using sf::base::SizeT;
using sf::base::U64;

////////////////////////////////////////////////////////////
constexpr sf::Vector2f resolution{1366.f, 768.f};
constexpr auto         resolutionUInt = resolution.toVector2u();

////////////////////////////////////////////////////////////
constexpr sf::Vector2f boundaries{1366.f * 10.f, 768.f};

////////////////////////////////////////////////////////////
constexpr sf::Color colorBlueOutline{50, 84, 135};

////////////////////////////////////////////////////////////
[[nodiscard]] float getRndFloat(const float min, const float max)
{
    static std::default_random_engine randomEngine(std::random_device{}());
    return std::uniform_real_distribution<float>{min, max}(randomEngine);
}

////////////////////////////////////////////////////////////
[[nodiscard]] sf::Vector2f getRndVector2f(const sf::Vector2f mins, const sf::Vector2f maxs)
{
    return {getRndFloat(mins.x, maxs.x), getRndFloat(mins.y, maxs.y)};
}

////////////////////////////////////////////////////////////
[[nodiscard]] sf::Vector2f getRndVector2f(const sf::Vector2f maxs)
{
    return getRndVector2f({0.f, 0.f}, maxs);
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr float remap(const float x, const float oldMin, const float oldMax, const float newMin, const float newMax)
{
    SFML_BASE_ASSERT(oldMax != oldMin);
    return newMin + ((x - oldMin) / (oldMax - oldMin)) * (newMax - newMin);
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr float exponentialApproach(
    const float current,
    const float target,
    const float deltaTimeMs,
    const float speed)
{
    if (speed <= 0.0f)
        return target; // Instant snap if time constant is zero or negative

    const float factor = 1.0f - sf::base::exp(-deltaTimeMs / speed);
    return current + (target - current) * factor;
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] TextShakeEffect
{
    float grow  = 0.f;
    float angle = 0.f;

    void bump(const float strength)
    {
        grow  = strength;
        angle = getRndFloat(-grow * 0.2f, grow * 0.2f);
    }

    void update(const float deltaTimeMs)
    {
        if (grow > 0.f)
            grow -= deltaTimeMs * 0.0165f;

        if (angle != 0.f)
        {
            const float sign = angle > 0.f ? 1.f : -1.f;
            angle -= sign * deltaTimeMs * 0.00565f;

            if (sign * angle < 0.f)
                angle = 0.f;
        }

        grow  = sf::base::clamp(grow, 0.f, 5.f);
        angle = sf::base::clamp(angle, -0.5f, 0.5f);
    }

    ////////////////////////////////////////////////////////////
    void applyToText(sf::Text& text) const
    {
        text.scale    = {1.f + grow * 0.2f, 1.f + grow * 0.2f};
        text.rotation = sf::radians(angle);
    }
};

////////////////////////////////////////////////////////////
enum class [[nodiscard]] BubbleType : sf::base::U8
{
    Normal = 0u,
    Star   = 1u,
    Bomb   = 2u
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Bubble
{
    sf::Vector2f position;
    sf::Vector2f velocity;

    float scale;
    float radius;
    float rotation;

    BubbleType type;

    void update(const float deltaTime)
    {
        position += velocity * deltaTime;
    }

    void applyToSprite(sf::Sprite& sprite) const
    {
        sprite.position = position;
        sprite.scale    = {scale, scale};
        sprite.rotation = sf::radians(rotation);
    }
};

////////////////////////////////////////////////////////////
enum class [[nodiscard]] ParticleType : sf::base::U8
{
    Bubble = 0u,
    Star   = 1u,
    Fire   = 2u,
    Hex    = 3u,
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] ParticleData
{
    sf::Vector2f position;
    sf::Vector2f velocity;

    float scale;
    float accelerationY;

    float opacity;
    float opacityDecay;

    float rotation;
    float torque;

    void update(const float deltaTime)
    {
        velocity.y += accelerationY * deltaTime;
        position += velocity * deltaTime;

        rotation += torque * deltaTime;

        opacity = sf::base::clamp(opacity - opacityDecay * deltaTime, 0.f, 1.f);
    }

    void applyToTransformable(auto& transformable) const
    {
        transformable.position = position;
        transformable.scale    = {scale, scale};
        transformable.rotation = sf::radians(rotation);
    }

    [[nodiscard, gnu::pure]] sf::base::U8 opacityAsAlpha() const
    {
        return static_cast<sf::base::U8>(opacity * 255.0f);
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Particle
{
    ParticleData data;
    ParticleType type;

    void update(const float deltaTime)
    {
        data.update(deltaTime);
    }

    void applyToSprite(sf::Sprite& sprite) const
    {
        data.applyToTransformable(sprite);
        sprite.color.a = data.opacityAsAlpha();
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] Particle makeParticle(
    const float        x,
    const float        y,
    const ParticleType particleType,
    const float        scaleMult,
    const float        speedMult)
{
    return {.data = {.position      = {x, y},
                     .velocity      = getRndVector2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * speedMult,
                     .scale         = getRndFloat(0.08f, 0.27f) * scaleMult,
                     .accelerationY = 0.002f,
                     .opacity       = 1.f,
                     .opacityDecay  = getRndFloat(0.00025f, 0.0015f),
                     .rotation      = getRndFloat(0.f, sf::base::tau),
                     .torque        = getRndFloat(-0.002f, 0.002f)},
            .type = particleType};
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] TextParticle
{
    char         buffer[16];
    ParticleData data;

    void update(const float deltaTime)
    {
        data.update(deltaTime);
    }

    void applyToText(sf::Text& text) const
    {
        text.setString(buffer); // TODO P1: should find a way to assign directly to text buffer

        data.applyToTransformable(text);
        text.origin = text.getLocalBounds().size / 2.f;

        text.setFillColor(text.getFillColor().withAlpha(data.opacityAsAlpha()));
        text.setOutlineColor(text.getOutlineColor().withAlpha(data.opacityAsAlpha()));
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] TextParticle makeTextParticle(const float x, const float y, const int combo)
{
    return {.buffer = {},
            .data   = {.position      = {x, y - 10.f},
                       .velocity      = getRndVector2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.45f,
                       .scale         = sf::base::clamp(1.f + 0.1f * static_cast<float>(combo + 1) / 2.f, 1.f, 2.5f),
                       .accelerationY = 0.00425f,
                       .opacity       = 1.f,
                       .opacityDecay  = 0.0020f,
                       .rotation      = 0.f,
                       .torque        = getRndFloat(-0.002f, 0.002f)}};
}

////////////////////////////////////////////////////////////
enum class [[nodiscard]] CatType : sf::base::U8
{
    Normal    = 0u,
    Unicorn   = 1u,
    Devil     = 2u,
    Witch     = 3u,
    Astromeow = 4u,
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Cat
{
    CatType type;

    sf::Vector2f position;
    sf::Vector2f rangeOffset = {0.f, 0.f};
    float        wobbleTimer = 0.f;

    sf::Sprite sprite;
    sf::Sprite pawSprite;
    float      cooldown = 0.f;

    SizeT nameIdx;

    TextShakeEffect textStatusShakeEffect{};

    float beingDragged = 0.f;
    int   hits         = 0;

    [[nodiscard]] bool update(const float maxCooldown, const float deltaTime)
    {
        textStatusShakeEffect.update(deltaTime);

        wobbleTimer += deltaTime * 0.002f;
        sprite.position.x = position.x;
        sprite.position.y = position.y + std::sin(wobbleTimer * 2.f) * 7.5f;

        cooldown += deltaTime;

        if (cooldown >= maxCooldown)
        {
            cooldown = maxCooldown;
            return true;
        }

        return false;
    }
};

////////////////////////////////////////////////////////////
struct CollisionResolution
{
    sf::Vector2f iDisplacement;
    sf::Vector2f jDisplacement;
    sf::Vector2f iVelocityChange;
    sf::Vector2f jVelocityChange;
};

////////////////////////////////////////////////////////////
[[nodiscard]] sf::base::Optional<CollisionResolution> handleCollision(
    const float        deltaTimeMs,
    const sf::Vector2f iPosition,
    const sf::Vector2f jPosition,
    const sf::Vector2f iVelocity,
    const sf::Vector2f jVelocity,
    const float        iRadius,
    const float        jRadius,
    const float        iMassMult,
    const float        jMassMult)
{
    const sf::Vector2f diff        = jPosition - iPosition;
    const float        squaredDiff = diff.lengthSquared();

    const sf::Vector2f radii{iRadius, jRadius};
    const float        squaredRadiiSum = radii.lengthSquared();

    if (squaredDiff >= squaredRadiiSum)
        return sf::base::nullOpt;

    // Calculate the overlap between the bubbles
    const float distance = sf::base::sqrt(squaredDiff);    // Distance between centers
    const float overlap  = (iRadius + jRadius) - distance; // Amount of overlap

    // Define a "softness" factor to control how quickly the overlap is resolved
    const float softnessFactor = 0.005f * deltaTimeMs;

    // Calculate the displacement needed to resolve the overlap
    const sf::Vector2f normal       = diff == sf::Vector2f{} ? sf::Vector2f{1.f, 0.f} : diff.normalized();
    const sf::Vector2f displacement = normal * overlap * softnessFactor;

    // Move the bubbles apart based on their masses (heavier bubbles move less)
    const float m1        = iRadius * iRadius * iMassMult; // Mass of bubble i (quadratic scaling)
    const float m2        = jRadius * jRadius * jMassMult; // Mass of bubble j (quadratic scaling)
    const float totalMass = m1 + m2;

    // Velocity resolution calculations
    const float vRelDotNormal = (iVelocity - jVelocity).dot(normal);

    sf::Vector2f velocityChangeI;
    sf::Vector2f velocityChangeJ;

    // Only apply impulse if bubbles are moving towards each other
    if (vRelDotNormal > 0)
    {
        const float e = 0.65f; // Coefficient of restitution (1.0 = perfectly elastic)
        const float j = -(1.0f + e) * vRelDotNormal / ((1.0f / m1) + (1.0f / m2));

        const sf::Vector2f impulse = normal * j;

        velocityChangeI = impulse / m1;
        velocityChangeJ = -impulse / m2;
    }

    return sf::base::makeOptional( //
        CollisionResolution{.iDisplacement   = -displacement * (m2 / totalMass),
                            .jDisplacement   = displacement * (m1 / totalMass),
                            .iVelocityChange = velocityChangeI,
                            .jVelocityChange = velocityChangeJ});
}

////////////////////////////////////////////////////////////
bool handleBubbleCollision(const float deltaTimeMs, Bubble& iBubble, Bubble& jBubble)
{
    const auto result = handleCollision(deltaTimeMs,
                                        iBubble.position,
                                        jBubble.position,
                                        iBubble.velocity,
                                        jBubble.velocity,
                                        iBubble.radius,
                                        jBubble.radius,
                                        iBubble.type == BubbleType::Bomb ? 5.f : 1.f,
                                        jBubble.type == BubbleType::Bomb ? 5.f : 1.f);

    if (!result.hasValue())
        return false;

    iBubble.position += result->iDisplacement;
    jBubble.position += result->jDisplacement;
    iBubble.velocity += result->iVelocityChange;
    jBubble.velocity += result->jVelocityChange;

    return true;
}

////////////////////////////////////////////////////////////
bool handleCatCollision(const float deltaTimeMs, Cat& iCat, Cat& jCat)
{
    const auto iRadius = iCat.sprite.textureRect.size.x * iCat.sprite.scale.x * 0.75f;
    const auto jRadius = jCat.sprite.textureRect.size.x * jCat.sprite.scale.x * 0.75f;

    const auto result = handleCollision(deltaTimeMs, iCat.position, jCat.position, {}, {}, iRadius, jRadius, 1.f, 1.f);
    if (!result.hasValue())
        return false;

    iCat.position += result->iDisplacement;
    jCat.position += result->jDisplacement;

    return true;
}

////////////////////////////////////////////////////////////
[[nodiscard]] Bubble makeRandomBubble(const float mapLimit, const float maxY, const float radius)
{
    const float scaleFactor = getRndFloat(0.07f, 0.17f) * 0.65f;

    return Bubble{.position = getRndVector2f({mapLimit, maxY}),
                  .velocity = getRndVector2f({-0.1f, -0.1f}, {0.1f, 0.1f}),
                  .scale    = scaleFactor,
                  .radius   = radius * scaleFactor,
                  .rotation = 0.f,
                  .type     = BubbleType::Normal};
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] GrowthFactors
{
    float initial;
    float linear;
    float multiplicative;
    float exponential;
    float flat;
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::pure]] float computeGrowth(const GrowthFactors& factors, const float n)
{
    return (factors.initial + n * factors.multiplicative) * std::pow(factors.exponential, n) + factors.linear * n +
           factors.flat;
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] PurchasableScalingValue
{
    SizeT nPurchases;

    const SizeT nMaxPurchases;

    const GrowthFactors cost;
    const GrowthFactors value;

    [[nodiscard]] float nextCost() const
    {
        return computeGrowth(cost, static_cast<float>(nPurchases));
    }

    [[nodiscard]] float currentValue() const
    {
        return computeGrowth(value, static_cast<float>(nPurchases));
    }
};

////////////////////////////////////////////////////////////
void drawMinimap(const float                 minimapScale,
                 const float                 mapLimit,
                 const sf::View&             gameView,
                 const sf::View&             hudView,
                 sf::RenderTarget&           window,
                 const sf::Texture&          txBackground,
                 const sf::CPUDrawableBatch& cpuDrawableBatch,
                 const sf::TextureAtlas&     textureAtlas)
{
    // Screen position of minimap's top-left corner
    constexpr sf::Vector2f minimapPos = {15.f, 15.f};

    // Size of full map in minimap space
    const sf::Vector2f minimapSize = boundaries / minimapScale;

    //
    // White border around minimap
    const sf::RectangleShape minimapBorder{
        {.position         = minimapPos,
         .fillColor        = sf::Color::Transparent,
         .outlineColor     = sf::Color::White,
         .outlineThickness = 2.f,
         .size             = {mapLimit / minimapScale, minimapSize.y}}};

    //
    // Blue rectangle showing current visible area
    const sf::RectangleShape minimapIndicator{
        {.position         = minimapPos + sf::Vector2f{(gameView.center.x - resolution.x / 2.f) / minimapScale, 0.f},
         .fillColor        = sf::Color::Transparent,
         .outlineColor     = sf::Color::Blue,
         .outlineThickness = 2.f,
         .size             = resolution / minimapScale}};

    //
    // Convert minimap dimensions to normalized `[0, 1]` range for scissor rectangle
    const auto  minimapScaledPosition = minimapPos.componentWiseDiv(resolution);
    const auto  minimapScaledSized    = minimapSize.componentWiseDiv(resolution);
    const float progressRatio         = sf::base::clamp(mapLimit / boundaries.x, 0.f, 1.f);

    //
    // Special view that renders the world scaled down for minimap
    const sf::View minimapView                                       //
        {.center  = (resolution * 0.5f - minimapPos) * minimapScale, // Offset center to align minimap
         .size    = resolution * minimapScale,                       // Zoom out to show scaled-down world
         .scissor = {minimapScaledPosition,                          // Scissor rectangle position (normalized)
                     {
                         progressRatio * minimapScaledSized.x, // Only show accessible width
                         minimapScaledSized.y                  // Full height
                     }}};

    //
    // Draw minimap contents
    window.setView(minimapView);                                            // Use minimap projection
    window.draw(txBackground);                                              // Draw world background
    window.draw(cpuDrawableBatch, {.texture = &textureAtlas.getTexture()}); // Draw game objects

    //
    // Switch back to HUD view and draw overlay elements
    window.setView(hudView);
    window.draw(minimapBorder, /* texture */ nullptr);    // Draw border frame
    window.draw(minimapIndicator, /* texture */ nullptr); // Draw current view indicator
}

void drawSplashScreen(const float deltaTimeMs, sf::RenderWindow& window, const sf::Texture& txLogo)
{
    static float logoOpacity = 1000.f;
    logoOpacity -= deltaTimeMs * 0.5f;

    sf::Sprite logoSprite{.position    = resolution / 2.f,
                          .scale       = {0.70f, 0.70f},
                          .origin      = txLogo.getSize().toVector2f() / 2.f,
                          .textureRect = txLogo.getRect(),
                          .color = sf::Color{255, 255, 255, static_cast<sf::base::U8>(sf::base::clamp(logoOpacity, 0.f, 255.f))}};


    window.draw(logoSprite, txLogo);
}


} // namespace


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    // TODO
    PurchasableScalingValue psvComboStartTime //
        {.nPurchases    = 0u,
         .nMaxPurchases = 20u,

         .cost  = {.initial = 35.f, .linear = 125.f, .multiplicative = 1.f, .exponential = 1.7f, .flat = 0.f},
         .value = {.initial = 0.55f, .linear = 0.04f, .multiplicative = 1.f, .exponential = 1.02f, .flat = 0.f}};

    // TODO
    PurchasableScalingValue psvBubbleCount //
        {.nPurchases    = 0u,
         .nMaxPurchases = 30u,

         .cost  = {.initial = 35.f, .linear = 1500.f, .multiplicative = 1.f, .exponential = 2.5f, .flat = 0.f},
         .value = {.initial = 500.f, .linear = 325.f, .multiplicative = 1.f, .exponential = 1.01f, .flat = 0.f}};

    // TODO
    PurchasableScalingValue psvBubbleValue //
        {.nPurchases    = 0u,
         .nMaxPurchases = 19u,

         .cost  = {.initial = 1000.f, .linear = 2500.f, .multiplicative = 1.f, .exponential = 2.0f, .flat = 0.f},
         .value = {.initial = 0.f, .linear = 1.f, .multiplicative = 1.f, .exponential = 0.f, .flat = 0.f}};

    //
    //
    // Create an audio context and get the default playback device
    auto audioContext   = sf::AudioContext::create().value();
    auto playbackDevice = sf::PlaybackDevice::createDefault(audioContext).value();

    //
    //
    // Create the graphics context
    auto graphicsContext = sf::GraphicsContext::create().value();

    //
    //
    // Create the render window
    sf::RenderWindow window(
        {.size            = resolutionUInt,
         .title           = "BubbleByte " BUBBLEBYTE_VERSION_STR " | by Vittorio Romeo & Sonia Misericordia",
         .vsync           = true,
         .frametimeLimit  = 144,
         .contextSettings = {.antiAliasingLevel = sf::RenderTexture::getMaximumAntiAliasingLevel()}});

    //
    //
    // Create and initialize the ImGui context
    sf::ImGui::ImGuiContext imGuiContext;
    if (!imGuiContext.init(window))
        return -1;

    //
    //
    // Set up texture atlas
    sf::TextureAtlas textureAtlas{sf::Texture::create({4096u, 4096u}, {.smooth = true}).value()};

    //
    //
    // Load and initialize resources
    /* --- 1 */
    const auto fontDailyBubble = sf::Font::openFromFile("resources/dailybubble.ttf", &textureAtlas).value();

    /* --- ImGui fonts */
    ImFont* fontImGuiDailyBubble = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/dailybubble.ttf", 28.f);

    /* --- Music */
    auto musicBGM = sf::Music::openFromFile("resources/hibiscus.mp3").value();

    /* --- Sound buffers */
    const auto soundBufferPop       = sf::SoundBuffer::loadFromFile("resources/pop.ogg").value();
    const auto soundBufferShine     = sf::SoundBuffer::loadFromFile("resources/shine.ogg").value();
    const auto soundBufferClick     = sf::SoundBuffer::loadFromFile("resources/click2.ogg").value();
    const auto soundBufferByteMeow  = sf::SoundBuffer::loadFromFile("resources/bytemeow.ogg").value();
    const auto soundBufferGrab      = sf::SoundBuffer::loadFromFile("resources/grab.ogg").value();
    const auto soundBufferDrop      = sf::SoundBuffer::loadFromFile("resources/drop.ogg").value();
    const auto soundBufferScratch   = sf::SoundBuffer::loadFromFile("resources/scratch.ogg").value();
    const auto soundBufferBuy       = sf::SoundBuffer::loadFromFile("resources/buy.ogg").value();
    const auto soundBufferExplosion = sf::SoundBuffer::loadFromFile("resources/explosion.ogg").value();
    const auto soundBufferMakeBomb  = sf::SoundBuffer::loadFromFile("resources/makebomb.ogg").value();
    const auto soundBufferHex       = sf::SoundBuffer::loadFromFile("resources/hex.ogg").value();

    /* --- Sounds */
    const float worldAttenuation = 0.0015f;
    const float uiAttenuation    = 0.f;

    sf::Sound soundPop(soundBufferPop);
    soundPop.setAttenuation(worldAttenuation);

    sf::Sound soundShine(soundBufferShine);
    soundShine.setAttenuation(worldAttenuation);

    sf::Sound soundClick(soundBufferClick);
    soundClick.setAttenuation(uiAttenuation);

    sf::Sound soundByteMeow(soundBufferByteMeow);
    soundByteMeow.setAttenuation(uiAttenuation);

    sf::Sound soundGrab(soundBufferGrab); // TODO: use
    soundGrab.setAttenuation(uiAttenuation);

    sf::Sound soundDrop(soundBufferDrop); // TODO: use
    soundDrop.setAttenuation(uiAttenuation);

    sf::Sound soundScratch(soundBufferScratch);
    soundScratch.setAttenuation(uiAttenuation);
    soundScratch.setVolume(35.f);

    sf::Sound soundBuy(soundBufferBuy);
    soundBuy.setAttenuation(uiAttenuation);

    sf::Sound soundExplosion(soundBufferExplosion);
    soundExplosion.setAttenuation(worldAttenuation);
    soundExplosion.setVolume(75.f);

    sf::Sound soundMakeBomb(soundBufferMakeBomb);
    soundMakeBomb.setAttenuation(worldAttenuation);

    sf::Sound soundHex(soundBufferHex);
    soundHex.setAttenuation(worldAttenuation);

    /* --- Listener */
    sf::Listener listener;

    /* --- Images */
    const auto imgBubble128    = sf::Image::loadFromFile("resources/bubble2.png").value();
    const auto imgBubbleStar   = sf::Image::loadFromFile("resources/bubble3.png").value();
    const auto imgCat          = sf::Image::loadFromFile("resources/cat.png").value();
    const auto imgUniCat       = sf::Image::loadFromFile("resources/unicat.png").value();
    const auto imgDevilCat     = sf::Image::loadFromFile("resources/devilcat.png").value();
    const auto imgCatPaw       = sf::Image::loadFromFile("resources/catpaw.png").value();
    const auto imgUniCatPaw    = sf::Image::loadFromFile("resources/unicatpaw.png").value();
    const auto imgDevilCatPaw  = sf::Image::loadFromFile("resources/devilcatpaw.png").value();
    const auto imgParticle     = sf::Image::loadFromFile("resources/particle.png").value();
    const auto imgStarParticle = sf::Image::loadFromFile("resources/starparticle.png").value();
    const auto imgFireParticle = sf::Image::loadFromFile("resources/fireparticle.png").value();
    const auto imgHexParticle  = sf::Image::loadFromFile("resources/hexparticle.png").value();
    const auto imgWitchCat     = sf::Image::loadFromFile("resources/witchcat.png").value();
    const auto imgWitchCatPaw  = sf::Image::loadFromFile("resources/witchcatpaw.png").value();
    const auto imgAstromeowCat = sf::Image::loadFromFile("resources/astromeow.png").value();
    const auto imgBomb         = sf::Image::loadFromFile("resources/bomb.png").value();

    /* --- Textures */
    const auto txLogo       = sf::Texture::loadFromFile("resources/logo.png", {.smooth = true}).value();
    const auto txBackground = sf::Texture::loadFromFile("resources/background.jpg", {.smooth = true}).value();
    const auto txByteTip    = sf::Texture::loadFromFile("resources/bytetip.png", {.smooth = true}).value();

    /* --- Texture atlas rects */
    const auto txrWhiteDot     = textureAtlas.add(graphicsContext.getBuiltInWhiteDotTexture()).value();
    const auto txrBubble128    = textureAtlas.add(imgBubble128).value();
    const auto txrBubbleStar   = textureAtlas.add(imgBubbleStar).value();
    const auto txrCat          = textureAtlas.add(imgCat).value();
    const auto txrUniCat       = textureAtlas.add(imgUniCat).value();
    const auto txrDevilCat     = textureAtlas.add(imgDevilCat).value();
    const auto txrCatPaw       = textureAtlas.add(imgCatPaw).value();
    const auto txrUniCatPaw    = textureAtlas.add(imgUniCatPaw).value();
    const auto txrDevilCatPaw  = textureAtlas.add(imgDevilCatPaw).value();
    const auto txrParticle     = textureAtlas.add(imgParticle).value();
    const auto txrStarParticle = textureAtlas.add(imgStarParticle).value();
    const auto txrFireParticle = textureAtlas.add(imgFireParticle).value();
    const auto txrHexParticle  = textureAtlas.add(imgHexParticle).value();
    const auto txrWitchCat     = textureAtlas.add(imgWitchCat).value();
    const auto txrWitchCatPaw  = textureAtlas.add(imgWitchCatPaw).value();
    const auto txrAstromeowCat = textureAtlas.add(imgAstromeowCat).value();
    const auto txrBomb         = textureAtlas.add(imgBomb).value();

    //
    //
    // Constants
    const float baseBubbleRadius = txrBubble128.size.x / 2.f;

    //
    //
    // UI Text
    sf::Text moneyText{fontDailyBubble,
                       {.position         = {15.f, 70.f},
                        .string           = "$0",
                        .characterSize    = 32u,
                        .fillColor        = sf::Color::White,
                        .outlineColor     = colorBlueOutline,
                        .outlineThickness = 2.f}};

    sf::Text comboText{fontDailyBubble,
                       {.position         = {15.f, 105.f},
                        .string           = "x1",
                        .characterSize    = 24u,
                        .fillColor        = sf::Color::White,
                        .outlineColor     = colorBlueOutline,
                        .outlineThickness = 1.5f}};

    TextShakeEffect moneyTextShakeEffect;
    TextShakeEffect comboTextShakeEffect;

    //
    // Spatial partitioning
    const float gridSize = 64.f;
    const auto  nCellsX  = static_cast<SizeT>(sf::base::ceil(boundaries.x / gridSize)) + 1;
    const auto  nCellsY  = static_cast<SizeT>(sf::base::ceil(boundaries.y / gridSize)) + 1;

    const auto convert2DTo1D = [](const SizeT x, const SizeT y, const SizeT width) { return y * width + x; };

    std::vector<SizeT> bubbleIndices;          // Flat list of all bubble indices in all cells
    std::vector<SizeT> cellStartIndices;       // Tracks where each cell's data starts in `bubbleIndices`
    std::vector<SizeT> cellInsertionPositions; // Temporary copy of `cellStartIndices` to track insertion points

    //
    //
    // Particles
    std::vector<Particle>     particles;
    std::vector<TextParticle> textParticles;

    //
    //
    // Purchasables (persistent)
    const auto costFunction = [](const float baseCost, const float nOwned, const float growthFactor)
    { return baseCost * std::pow(growthFactor, nOwned); };

    bool  comboPurchased = false;
    bool  mapPurchased   = false;
    float mapLimit       = resolution.x;

    //
    //
    // Scaling values (persistent)
    const U64 rewardPerType[3]{
        1u,  // Normal
        25u, // Star
        1u,  // Bomb
    };

    const auto getScaledReward = [&](const BubbleType type)
    { return rewardPerType[static_cast<SizeT>(type)] * static_cast<U64>(psvBubbleValue.currentValue() + 1); };

    // TODO: make const and apply scaler on use
    float catCooldownPerType[5]{
        1000.f, // Normal
        3000.f, // Unicorn
        7000.f, // Devil
        2000.f, // Witch
        1000.f  // Astromeow
    };

    // TODO: make const and apply scaler on use
    float catRangePerType[5]{
        96.f,  // Normal
        64.f,  // Unicorn
        48.f,  // Devil
        256.f, // Witch
        96.f   // Astromeow
    };

    //
    //
    // Cat names
    std::array catNames{"Gorgonzola", "Provolino",  "Pistacchietto", "Ricottina",  "Mozzarellina",  "Tiramisu",
                        "Cannolino",  "Biscottino", "Cannolina",     "Biscottina", "Pistacchietta", "Provolina",
                        "Arancino",   "Limoncello", "Ciabatta",      "Focaccina",  "Amaretto",      "Pallino",
                        "Birillo",    "Trottola",   "Baffo",         "Poldo",      "Fuffi",         "Birba",
                        "Ciccio",     "Pippo",      "Tappo",         "Briciola",   "Braciola",      "Pulce",
                        "Dante",      "Bolla",      "Fragolina",     "Luppolo",    "Sirena",        "Polvere",
                        "Stellina",   "Lunetta",    "Briciolo",      "Fiammetta",  "Nuvoletta",     "Scintilla",
                        "Piuma",      "Fulmine",    "Arcobaleno",    "Stelluccia", "Lucciola",      "Pepita",
                        "Fiocco",     "Girandola",  "Bombetta",      "Fusillo",    "Cicciobello",   "Palloncino",
                        "Joe Biden",  "Trump",      "Obama",         "De Luca",    "Salvini",       "Renzi",
                        "Nutella",    "Vespa",      "Mandolino",     "Ferrari",    "Pavarotti",     "Espresso",
                        "Sir",        "Nocciolina", "Fluffy",        "Costanzo",   "Mozart",        "DB",
                        "Soniuccia",  "Pupi",       "Pupetta",       "Genitore 1", "Genitore 2",    "Stonks",
                        "Carotina",   "Waffle",     "Pancake",       "Muffin",     "Cupcake",       "Donut",
                        "Jinx",       "Miao",       "Arnold",        "Granita",    "Leone",         "Pangocciolo"};

    // TODO: use seed for persistance
    std::shuffle(catNames.begin(), catNames.end(), std::mt19937{std::random_device{}()});
    auto getNextCatNameIdx = [&, nextCatName = 0u]() mutable { return nextCatName++ % catNames.size(); };

    //
    //
    // Persistent game state
    std::vector<Bubble> bubbles;
    std::vector<Cat>    cats;

    U64 money = 0;

    //
    //
    // Transient game state
    int   combo      = 0;
    float comboTimer = 0.f;

    //
    //
    // Clocks
    sf::Clock playedClock;
    sf::Clock fpsClock;
    sf::Clock deltaClock;

    //
    //
    // Drawable batch
    sf::CPUDrawableBatch cpuDrawableBatch;

    //
    //
    // UI State
    sf::base::Optional<sf::Vector2f> dragPosition;
    float                            scroll       = 0.f;
    float                            actualScroll = 0.f;

    sf::base::Optional<sf::Vector2f> catDragPosition;

    //
    //
    // Touch state
    std::vector<sf::base::Optional<sf::Vector2f>> fingerPositions;
    fingerPositions.resize(10);

    //
    //
    // Startup sound
    soundByteMeow.play(playbackDevice);

    //
    //
    // Tips
    bool        tipsEnabled = true;
    float       tipTimer    = 0.f;
    std::string tipString;

    // Other settings
    float minimapScale          = 20.f;
    bool  playAudioInBackground = true;

    //
    //
    // Background music
    musicBGM.setLooping(true);
    musicBGM.setAttenuation(0.f);
    musicBGM.play(playbackDevice);
    musicBGM.setVolume(75.f);

    //
    //
    // Money helper functions
    const auto addReward = [&](const U64 reward)
    {
        money += reward;
        moneyTextShakeEffect.bump(1.f + static_cast<float>(combo) * 0.1f);
    };

    //
    //
    // Game loop
    playedClock.start();

    while (true)
    {
        fpsClock.restart();

        sf::base::Optional<sf::Vector2f> clickPosition;

        while (const sf::base::Optional event = window.pollEvent())
        {
            imGuiContext.processEvent(window, *event);

            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return 0;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
            if (const auto* e = event->getIf<sf::Event::TouchBegan>())
            {
                fingerPositions[e->finger].emplace(e->position.toVector2f());
            }
            else if (const auto* e = event->getIf<sf::Event::TouchEnded>())
            {
                // TODO: is this guaranteed to be called even if the finger is lifted?
                fingerPositions[e->finger].reset();
            }
            else if (const auto* e = event->getIf<sf::Event::TouchMoved>())
            {
                fingerPositions[e->finger].emplace(e->position.toVector2f());
            }
            else if (const auto* e = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (e->button == sf::Mouse::Button::Left)
                    clickPosition.emplace(e->position.toVector2f());

                if (e->button == sf::Mouse::Button::Left && !catDragPosition.hasValue())
                {
                    catDragPosition.emplace(e->position.toVector2f());
                }
                else if (e->button == sf::Mouse::Button::Left)
                {
                    catDragPosition.reset();

                    for (auto& cat : cats)
                        cat.beingDragged = 0.f;
                }

                if (e->button == sf::Mouse::Button::Right && !dragPosition.hasValue())
                {
                    clickPosition.reset();

                    dragPosition.emplace(e->position.toVector2f());
                    dragPosition->x += scroll;
                }
            }
            else if (const auto* e = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (e->button == sf::Mouse::Button::Right)
                    dragPosition.reset();
            }
            else if (const auto* e = event->getIf<sf::Event::MouseMoved>())
            {
                if (mapPurchased && dragPosition.hasValue())
                {
                    scroll = dragPosition->x - static_cast<float>(e->position.x);
                }
            }
#pragma clang diagnostic pop
        }

        const auto deltaTime   = deltaClock.restart();
        const auto deltaTimeMs = static_cast<float>(deltaTime.asMilliseconds());

        constexpr float scrollSpeed = 2.f;

        if (mapPurchased && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        {
            dragPosition.reset();
            scroll -= scrollSpeed * deltaTimeMs;
        }
        else if (mapPurchased && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        {
            dragPosition.reset();
            scroll += scrollSpeed * deltaTimeMs;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F4))
        {
            comboPurchased = true;
            // longerComboPurchased       = true;
            // bubbleTargetCountPurchased = true;

            money = 1'000'000'000u;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5))
        {
            money = 1'000'000'000u;
        }

        const auto countFingersDown = std::count_if(fingerPositions.begin(),
                                                    fingerPositions.end(),
                                                    [](const auto& fingerPos) { return fingerPos.hasValue(); });

        if (mapPurchased && countFingersDown == 2)
        {
            // TODO: check fingers distance

            const auto [fingerPos0, fingerPos1] = [&]
            {
                std::pair<sf::base::Optional<sf::Vector2f>, sf::base::Optional<sf::Vector2f>> result;

                for (const auto& fingerPosition : fingerPositions)
                {
                    if (fingerPosition.hasValue())
                    {
                        if (!result.first.hasValue())
                            result.first.emplace(*fingerPosition);
                        else if (!result.second.hasValue())
                            result.second.emplace(*fingerPosition);
                    }
                }

                return result;
            }();

            const auto avg = (*fingerPos0 + *fingerPos1) / 2.f;

            if (dragPosition.hasValue())
            {
                scroll = dragPosition->x - avg.x;
            }
            else
            {
                dragPosition.emplace(avg);
                dragPosition->x += scroll;
            }
        }

        if (dragPosition.hasValue() && countFingersDown != 2 && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            dragPosition.reset();
        }

        scroll       = sf::base::clamp(scroll,
                                 0.f,
                                 sf::base::min(mapLimit / 2.f - resolution.x / 2.f, (boundaries.x - resolution.x) / 2.f));
        actualScroll = exponentialApproach(actualScroll, scroll, deltaTimeMs, 75.f);

        const sf::View gameView //
            {.center = {sf::base::clamp(resolution.x / 2.f + actualScroll * 2.f,
                                        resolution.x / 2.f,
                                        boundaries.x - resolution.x / 2.f),
                        resolution.y / 2.f},
             .size   = resolution};


        const auto mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), gameView);

        listener.position = {mousePos.x, mousePos.y, 0.f};
        // TODO: master volume controls here
        (void)playbackDevice.updateListener(listener);

        const auto targetBubbleCountPerScreen = static_cast<SizeT>(
            psvBubbleCount.currentValue() / (boundaries.x / resolution.x));
        const auto nScreens          = static_cast<SizeT>(mapLimit / resolution.x) + 1;
        const auto targetBubbleCount = targetBubbleCountPerScreen * nScreens;

        for (SizeT i = bubbles.size(); i < targetBubbleCount; ++i)
            bubbles.emplace_back(makeRandomBubble(mapLimit, boundaries.y, baseBubbleRadius));

        //
        //
        // Update spatial partitioning
        cellStartIndices.clear();
        cellStartIndices.resize(nCellsX * nCellsY + 1, 0); // +1 for prefix sum

        const auto computeGridRange = [&](const auto& bubble)
        {
            const auto minX = bubble.position.x - bubble.radius;
            const auto minY = bubble.position.y - bubble.radius;
            const auto maxX = bubble.position.x + bubble.radius;
            const auto maxY = bubble.position.y + bubble.radius;

            struct Result
            {
                SizeT xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx;
            };

            return Result{sf::base::max(SizeT{0u}, static_cast<SizeT>(minX / gridSize)),
                          sf::base::max(SizeT{0u}, static_cast<SizeT>(minY / gridSize)),
                          sf::base::min(SizeT{nCellsX - 1}, static_cast<SizeT>(maxX / gridSize)),
                          sf::base::min(SizeT{nCellsY - 1}, static_cast<SizeT>(maxY / gridSize))};
        };

        //
        // First Pass (Counting):
        // - Calculate how many bubbles will be placed in each grid cell.
        for (auto& bubble : bubbles)
        {
            const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(bubble);

            // For each cell the bubble covers, increment the count
            for (SizeT x = xCellStartIdx; x <= xCellEndIdx; ++x)
                for (SizeT y = yCellStartIdx; y <= yCellEndIdx; ++y)
                {
                    const SizeT cellIdx = convert2DTo1D(x, y, nCellsX);
                    ++cellStartIndices[cellIdx + 1]; // +1 offsets for prefix sum
                }
        }

        //
        // Second Pass (Prefix Sum):
        // - Calculate the starting index for each cell’s data in `bubbleIndices`.

        // Prefix sum to compute start indices
        for (SizeT i = 1; i < cellStartIndices.size(); ++i)
            cellStartIndices[i] += cellStartIndices[i - 1];

        bubbleIndices.resize(cellStartIndices.back()); // Total bubbles across all cells

        // Used to track where to insert the next bubble index into the `bubbleIndices` buffer for each cell
        cellInsertionPositions.assign(cellStartIndices.begin(), cellStartIndices.end());

        //
        // Third Pass (Populating):
        // - Place the bubble indices into the correct positions in the `bubbleIndices` buffer.
        for (SizeT i = 0; i < bubbles.size(); ++i)
        {
            const auto& bubble                                                  = bubbles[i];
            const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(bubble);

            // Insert the bubble index into all overlapping cells
            for (SizeT x = xCellStartIdx; x <= xCellEndIdx; ++x)
            {
                for (SizeT y = yCellStartIdx; y <= yCellEndIdx; ++y)
                {
                    const SizeT cellIdx      = convert2DTo1D(x, y, nCellsX);
                    const SizeT insertPos    = cellInsertionPositions[cellIdx]++;
                    bubbleIndices[insertPos] = i;
                }
            }
        }

        auto popBubble = [&](auto self, BubbleType bubbleType, SizeT reward, int combo, float x, float y) -> void
        {
            auto& tp = textParticles.emplace_back(makeTextParticle(x, y, combo));
            std::snprintf(tp.buffer, sizeof(tp.buffer), "+$%zu", reward);

            soundPop.setPosition({x, y});
            soundPop.setPitch(remap(static_cast<float>(combo), 1, 10, 1.f, 2.f));
            soundPop.play(playbackDevice);

            for (int i = 0; i < 32; ++i)
                particles.emplace_back(makeParticle(x, y, ParticleType::Bubble, 0.5f, 0.5f));

            for (int i = 0; i < 8; ++i)
                particles.emplace_back(makeParticle(x, y, ParticleType::Bubble, 1.2f, 0.25f));

            if (bubbleType == BubbleType::Star)
                for (int i = 0; i < 16; ++i)
                    particles.emplace_back(makeParticle(x, y, ParticleType::Star, 0.25f, 0.35f));

            if (bubbleType == BubbleType::Bomb)
            {
                soundExplosion.setPosition({x, y});
                soundExplosion.play(playbackDevice);

                for (int i = 0; i < 32; ++i)
                    particles.emplace_back(makeParticle(x, y, ParticleType::Fire, 3.f, 1.f));

                const int cellX = static_cast<int>(x / gridSize);
                const int cellY = static_cast<int>(y / gridSize);

                for (int ox = -4; ox <= 4; ++ox)
                    for (int oy = -4; oy <= 4; ++oy)
                    {
                        const auto cellIdx = convert2DTo1D( //
                            static_cast<SizeT>(sf::base::clamp(cellX + ox, 0, static_cast<int>(nCellsX) - 1)),
                            static_cast<SizeT>(sf::base::clamp(cellY + oy, 0, static_cast<int>(nCellsY) - 1)),
                            nCellsX);

                        const SizeT start = cellStartIndices[cellIdx];
                        const SizeT end   = cellStartIndices[cellIdx + 1];

                        // Iterate over all bubbles in this cell
                        for (SizeT i = start; i < end; ++i)
                        {
                            const SizeT bubbleIdx = bubbleIndices[i];
                            auto&       bubble    = bubbles[bubbleIdx];

                            if (bubble.type == BubbleType::Bomb)
                                continue;

                            // TODO: add scaling explosion radius, right after unlocking devilcat
                            float explosionRadius = 128.f;
                            if ((bubble.position - sf::Vector2f{x, y}).lengthSquared() > explosionRadius * explosionRadius)
                                continue;

                            // TODO: repetition due to recursion
                            const SizeT newReward = getScaledReward(bubble.type) * 10u;
                            self(self, bubble.type, newReward, 1, bubble.position.x, bubble.position.y);
                            addReward(newReward);
                            bubble = makeRandomBubble(mapLimit, 0.f, baseBubbleRadius);
                            bubble.position.y -= bubble.radius;
                        }
                    }
            }
        };

        const auto popWithRewardAndReplaceBubble = [&](Bubble& bubble, int combo)
        {
            const auto reward = getScaledReward(bubble.type) * static_cast<U64>(combo);
            popBubble(popBubble, bubble.type, reward, combo, bubble.position.x, bubble.position.y);
            addReward(reward);
            bubble = makeRandomBubble(mapLimit, 0.f, baseBubbleRadius);
            bubble.position.y -= bubble.radius;
        };

        for (auto& bubble : bubbles)
        {
            if (bubble.type == BubbleType::Bomb)
                bubble.rotation += deltaTimeMs * 0.01f;

            auto& pos = bubble.position;

            pos += bubble.velocity * deltaTimeMs;

            if (pos.x - bubble.radius > mapLimit)
                pos.x = -bubble.radius;
            else if (pos.x + bubble.radius < 0.f)
                pos.x = mapLimit + bubble.radius;

            if (pos.y - bubble.radius > boundaries.y)
            {
                pos.y             = -bubble.radius;
                bubble.velocity.y = 0.f;
                bubble.type       = BubbleType::Normal;
            }
            else if (pos.y + bubble.radius < 0.f)
                pos.y = boundaries.y + bubble.radius;

            bubble.velocity.y += 0.00005f * deltaTimeMs;

            const auto handleClick = [&](const sf::Vector2f clickPos)
            {
                const auto [x, y] = window.mapPixelToCoords(clickPos.toVector2i(), gameView);

                if ((x - pos.x) * (x - pos.x) + (y - pos.y) * (y - pos.y) >= bubble.radius * bubble.radius)
                    return false;

                if (comboPurchased)
                {
                    if (combo == 0)
                    {
                        combo      = 1;
                        comboTimer = psvComboStartTime.currentValue() * 1000.f;
                    }
                    else
                    {
                        combo += 1;
                        comboTimer += 150.f - sf::base::clamp(static_cast<float>(combo) * 10.f, 0.f, 100.f);

                        comboTextShakeEffect.bump(1.f + static_cast<float>(combo) * 0.2f);
                    }
                }
                else
                {
                    combo = 1;
                }

                popWithRewardAndReplaceBubble(bubble, combo);
                return true;
            };

            if (clickPosition.hasValue())
            {
                if (handleClick(*clickPosition))
                {
                    clickPosition.reset();
                    continue;
                }
            }

            if (countFingersDown == 1)
                for (const auto& fingerPos : fingerPositions)
                    if (fingerPos.hasValue() && handleClick(*fingerPos))
                        break;
        }

        for (SizeT ix = 0; ix < nCellsX; ++ix)
        {
            for (SizeT iy = 0; iy < nCellsY; ++iy)
            {
                const SizeT cellIdx = convert2DTo1D(ix, iy, nCellsX);
                const SizeT start   = cellStartIndices[cellIdx];
                const SizeT end     = cellStartIndices[cellIdx + 1];

                // Iterate over all bubbles in this cell
                for (SizeT i = start; i < end; ++i)
                {
                    const SizeT bubbleA = bubbleIndices[i];
                    for (SizeT j = i + 1; j < end; ++j)
                    {
                        const SizeT bubbleB = bubbleIndices[j];
                        handleBubbleCollision(deltaTimeMs, bubbles[bubbleA], bubbles[bubbleB]);
                    }
                }
            }
        }

        for (SizeT i = 0u; i < cats.size(); ++i)
            for (SizeT j = i + 1; j < cats.size(); ++j)
                if (cats[i].beingDragged == 0.f && cats[j].beingDragged == 0.f)
                    handleCatCollision(deltaTimeMs, cats[i], cats[j]);

        for (auto& cat : cats)
        {
            if (catDragPosition.hasValue())
            {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && cat.sprite.getGlobalBounds().contains(mousePos))
                {
                    if (cat.beingDragged < 250.f)
                        cat.beingDragged += deltaTimeMs;

                    break;
                }
            }

            if (fingerPositions[0].hasValue())
            {
                const auto fingerPos = window.mapPixelToCoords(fingerPositions[0]->toVector2i(), gameView);

                if (cat.sprite.getGlobalBounds().contains(fingerPos))
                {
                    if (cat.beingDragged < 250.f)
                        cat.beingDragged += deltaTimeMs;

                    break;
                }
            }

            if (cat.beingDragged > 0.f)
                cat.beingDragged -= deltaTimeMs;
        }

        for (auto& cat : cats)
        {
            if (cat.beingDragged > 250.f)
            {
                cat.cooldown = -1000.f;

                if (fingerPositions[0].hasValue())
                {
                    const auto fingerPos = window.mapPixelToCoords(fingerPositions[0]->toVector2i(), gameView);
                    cat.position         = fingerPos;
                }
                else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) || catDragPosition.hasValue())
                {
                    cat.position = mousePos;
                }

                cat.position.x = sf::base::clamp(cat.position.x, 0.f, boundaries.x);
                cat.position.y = sf::base::clamp(cat.position.y, 0.f, boundaries.y);
            }
        }

        for (auto& cat : cats)
        {
            const auto maxCooldown = catCooldownPerType[static_cast<int>(cat.type)];
            const auto range       = catRangePerType[static_cast<int>(cat.type)];

            auto diff = cat.pawSprite.position - cat.sprite.position - sf::Vector2f{-25.f, 25.f};
            cat.pawSprite.position -= diff * 0.01f * deltaTimeMs;
            cat.pawSprite.rotation = cat.pawSprite.rotation.rotatedTowards(sf::degrees(-45.f), deltaTimeMs * 0.005f);

            if (cat.cooldown < 0.f)
            {
                cat.pawSprite.color.a = 128;
                cat.sprite.color.a    = 128;
            }
            else
            {
                cat.sprite.color.a = 255;
            }

            if (cat.cooldown == maxCooldown && cat.pawSprite.color.a > 10)
                cat.pawSprite.color.a -= static_cast<sf::base::U8>(0.5f * deltaTimeMs);

            if (cat.update(maxCooldown, deltaTimeMs))
            {
                const auto [cx, cy] = cat.position + cat.rangeOffset;

                if (cat.type == CatType::Witch)
                {
                    int  witchHits = 0;
                    bool pawSet    = false;

                    for (auto& otherCat : cats)
                    {
                        if (otherCat.type == CatType::Witch)
                            continue;

                        if ((otherCat.position - cat.position).length() > range)
                            continue;

                        otherCat.cooldown = catCooldownPerType[static_cast<int>(otherCat.type)];
                        ++witchHits;

                        if (!pawSet && getRndFloat(0.f, 100.f) > 50.f)
                        {
                            pawSet = true;

                            cat.pawSprite.position = {otherCat.position.x, otherCat.position.y};
                            cat.pawSprite.color.a  = 255;
                            cat.pawSprite.rotation = (otherCat.position - cat.position).angle() + sf::degrees(45);
                        }

                        for (int i = 0; i < 8; ++i)
                            particles.emplace_back(
                                makeParticle(otherCat.position.x, otherCat.position.y, ParticleType::Hex, 0.5f, 0.35f));
                    }

                    if (witchHits > 0)
                    {
                        soundHex.setPosition({cx, cy});
                        soundHex.play(playbackDevice);

                        cat.textStatusShakeEffect.bump(1.5f);
                        cat.hits += witchHits;
                    }

                    cat.cooldown = 0.f;
                    continue;
                }

                // TODO: use spatial grid
                for (auto& bubble : bubbles)
                {
                    if (cat.type == CatType::Unicorn && bubble.type != BubbleType::Normal)
                        continue;

                    const auto [x, y] = bubble.position;

                    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) < range * range)
                    {
                        cat.pawSprite.position = {x, y};
                        cat.pawSprite.color.a  = 255;
                        cat.pawSprite.rotation = (bubble.position - cat.position).angle() + sf::degrees(45);

                        if (cat.type == CatType::Unicorn)
                        {
                            bubble.type       = BubbleType::Star;
                            bubble.velocity.y = getRndFloat(-0.1f, -0.05f);
                            soundShine.setPosition({x, y});
                            soundShine.play(playbackDevice);

                            for (int i = 0; i < 4; ++i)
                                particles.emplace_back(makeParticle(x, y, ParticleType::Star, 0.25f, 0.35f));

                            cat.textStatusShakeEffect.bump(1.5f);
                            ++cat.hits;
                        }
                        else if (cat.type == CatType::Normal)
                        {
                            popWithRewardAndReplaceBubble(bubble, /* combo */ 1);

                            cat.textStatusShakeEffect.bump(1.5f);
                            ++cat.hits;
                        }
                        else if (cat.type == CatType::Devil)
                        {
                            bubble.type = BubbleType::Bomb;
                            bubble.velocity.y += getRndFloat(0.1f, 0.2f);
                            soundMakeBomb.setPosition({x, y});
                            soundMakeBomb.play(playbackDevice);

                            for (int i = 0; i < 8; ++i)
                                particles.emplace_back(makeParticle(x, y, ParticleType::Fire, 1.25f, 0.35f));

                            cat.textStatusShakeEffect.bump(1.5f);
                            ++cat.hits;
                        }

                        cat.cooldown = 0.f;
                        break;
                    }
                }
            }
        }

        const auto updateParticleLike = [&](auto& particleLikeVec)
        {
            for (auto& particleLike : particleLikeVec)
                particleLike.update(deltaTimeMs);

            std::erase_if(particleLikeVec, [](const auto& particleLike) { return particleLike.data.opacity <= 0.f; });
        };

        updateParticleLike(particles);
        updateParticleLike(textParticles);

        if (comboTimer > 0.f)
        {
            comboTimer -= deltaTimeMs;

            if (comboTimer <= 0.f)
            {
                if (combo > 2)
                    soundScratch.play(playbackDevice);

                combo      = 0;
                comboTimer = 0.f;
            }
        }

        imGuiContext.update(window, deltaTime);

        ImGui::SetNextWindowPos({resolution.x - 15.f, 15.f}, 0, {1.f, 0.f});
        ImGui::SetNextWindowSizeConstraints(ImVec2(400.f, 0.f), ImVec2(1000.f, 600.f));
        ImGui::PushFont(fontImGuiDailyBubble);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f); // Set corner radius

        ImGuiStyle& style               = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.65f); // 65% transparent black

        ImGui::Begin("##menu",
                     nullptr,
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoTitleBar);

        ImGui::PopStyleVar();

        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_DrawSelectedOverline))
        {
            if (ImGui::BeginTabItem(" X "))
            {
                ImGui::EndTabItem();
            }

            static auto selectOnce = ImGuiTabItemFlags_SetSelected;
            if (ImGui::BeginTabItem(" Shop ", nullptr, selectOnce))
            {
                selectOnce = {};

                char buffer[256];
                char labelBuffer[512];

                const auto makeDoneButton = [&](const char* label)
                {
                    ImGui::BeginDisabled(true);

                    ImGui::Text("%s", label);
                    ImGui::SameLine();

                    ImGui::SetWindowFontScale(0.5f);
                    ImGui::Text("%s", labelBuffer);
                    ImGui::SetWindowFontScale(1.f);
                    ImGui::SameLine();

                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 160.f);
                    ImGui::Button("DONE", ImVec2(135.f, 0.0f));

                    ImGui::EndDisabled();
                };

                const auto countCatsByType = [&](CatType type)
                {
                    return static_cast<int>(
                        std::count_if(cats.begin(), cats.end(), [type](const auto& cat) { return cat.type == type; }));
                };

                const auto nCatNormal  = countCatsByType(CatType::Normal);
                const auto nCatUnicorn = countCatsByType(CatType::Unicorn);
                const auto nCatDevil   = countCatsByType(CatType::Devil);
                const auto nCatWitch   = countCatsByType(CatType::Witch);

                const auto globalCostMultiplier = [&]
                {
                    // [ 0.25, 0.25 + 0.125, 0.25 + 0.125 + 0.0625, ... ]
                    const auto geomSum = [](auto n)
                    {
                        return static_cast<float>(n) <= 0.f ? 0.f
                                                            : 0.5f * (1.f - std::pow(0.5f, static_cast<float>(n) + 1.f));
                    };

                    return 1.f +                                            //
                           (geomSum(psvComboStartTime.nPurchases) * 0.1f) + //
                           (geomSum(psvBubbleCount.nPurchases) * 0.5f) +    //
                           (geomSum(psvBubbleValue.nPurchases) * 1.f) +     //
                           (geomSum(nCatNormal) * 0.35f) +                  //
                           (geomSum(nCatUnicorn) * 0.5f) +                  //
                           (geomSum(nCatDevil) * 0.75f) +                   //
                           (geomSum(nCatWitch) * 1.f);
                };

                const auto makePurchasableButton = [&](const char* label, float baseCost, float growthFactor, float count)
                {
                    bool result = false;

                    const auto cost = static_cast<U64>(globalCostMultiplier() * costFunction(baseCost, count, growthFactor));
                    std::sprintf(buffer, "$%zu##%s", cost, label);

                    ImGui::BeginDisabled(money < cost);

                    ImGui::Text("%s", label);
                    ImGui::SameLine();

                    ImGui::SetWindowFontScale(0.5f);
                    ImGui::Text("%s", labelBuffer);
                    ImGui::SetWindowFontScale(1.f);
                    ImGui::SameLine();

                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 160.f);
                    ImGui::Button(buffer, ImVec2(135.f, 0.0f));

                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        if (!imGuiContext.wasLastInputTouch())
                            catDragPosition.emplace(mousePos);

                        soundClick.play(playbackDevice);
                        soundBuy.play(playbackDevice);

                        result = true;
                        money -= cost;
                    }

                    ImGui::EndDisabled();
                    return result;
                };

                const auto makePurchasableButtonPSV = [&](const char* label, PurchasableScalingValue& psv)
                {
                    const bool maxedOut = psv.nPurchases == psv.nMaxPurchases;

                    bool result = false;

                    const auto cost = static_cast<U64>(globalCostMultiplier() * psv.nextCost());

                    if (maxedOut)
                        std::sprintf(buffer, "MAX");
                    else
                        std::sprintf(buffer, "$%zu##%s", cost, label);

                    ImGui::BeginDisabled(maxedOut || money < cost);

                    ImGui::Text("%s", label);
                    ImGui::SameLine();

                    ImGui::SetWindowFontScale(0.5f);
                    ImGui::Text("%s", labelBuffer);
                    ImGui::SetWindowFontScale(1.f);
                    ImGui::SameLine();

                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 160.f);
                    ImGui::Button(buffer, ImVec2(135.f, 0.0f));

                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        soundClick.play(playbackDevice);
                        soundBuy.play(playbackDevice);

                        result = true;
                        money -= cost;

                        ++psv.nPurchases;
                    }

                    ImGui::EndDisabled();
                    return result;
                };

                const auto makePurchasableButtonOneTime = [&](const char* label, const U64 xcost, bool& done)
                {
                    bool result = false;

                    const auto cost = static_cast<U64>(globalCostMultiplier() * static_cast<float>(xcost));

                    if (done)
                        std::sprintf(buffer, "DONE");
                    else
                        std::sprintf(buffer, "$%zu##%s", cost, label);

                    ImGui::BeginDisabled(done || money < cost);

                    ImGui::Text("%s", label);

                    ImGui::SameLine();


                    ImGui::SetWindowFontScale(0.5f);
                    ImGui::Text("%s", labelBuffer);
                    ImGui::SetWindowFontScale(1.f);
                    ImGui::SameLine();

                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 160.f);
                    ImGui::Button(buffer, ImVec2(135.f, 0.0f));

                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        soundClick.play(playbackDevice);
                        soundBuy.play(playbackDevice);

                        result = true;
                        money -= cost;

                        done = true;
                    }


                    ImGui::EndDisabled();

                    return result;
                };

                const auto doTip = [&](const char* str)
                {
                    if (!tipsEnabled)
                        return;

                    soundByteMeow.play(playbackDevice);
                    tipString = str;
                    tipTimer  = 4500.f;
                };

                std::sprintf(labelBuffer, "");
                if (makePurchasableButtonOneTime("Combo", 15, comboPurchased))
                {
                    combo = 0;
                    doTip("Pop bubbles in quick successions to\nkeep your combo up and make money!");
                }

                if (comboPurchased)
                {
                    std::sprintf(labelBuffer, "%.2fs", static_cast<double>(psvComboStartTime.currentValue()));
                    makePurchasableButtonPSV("Longer combo", psvComboStartTime);
                }

                if (nCatNormal > 0 && psvComboStartTime.nPurchases > 0)
                {
                    std::sprintf(labelBuffer, "");
                    if (makePurchasableButtonOneTime("Map scrolling", 50, mapPurchased))
                    {
                        mapLimit += resolution.x;
                        scroll = 0.f;

                        doTip("You can scroll the map with right click\nor by dragging with two fingers!");
                    }

                    if (mapPurchased)
                    {
                        if (mapLimit < boundaries.x - resolution.x)
                        {
                            std::sprintf(labelBuffer,
                                         "%.2f%%",
                                         static_cast<double>(remap(mapLimit, 0.f, boundaries.x, 0.f, 100.f) + 10.f));
                            if (makePurchasableButton("Extend map", 75.f, 4.75f, mapLimit / resolution.x))
                            {
                                mapLimit += resolution.x;
                                mapPurchased = true;
                            }
                        }
                        else
                        {
                            std::sprintf(labelBuffer, "100%%");
                            makeDoneButton("Extend map");
                        }
                    }

                    std::sprintf(labelBuffer, "%zu bubbles", static_cast<SizeT>(psvBubbleCount.currentValue()));
                    makePurchasableButtonPSV("More bubbles", psvBubbleCount);
                }

                const bool bubbleValueUnlocked = psvBubbleCount.nPurchases > 0 && nCatUnicorn >= 3;
                if (bubbleValueUnlocked)
                {
                    std::sprintf(labelBuffer, "x%zu", getScaledReward(BubbleType::Normal));
                    makePurchasableButtonPSV("Bubble value", psvBubbleValue);
                }

                const auto makeCat = [&](const CatType        catType,
                                         const sf::Vector2f   rangeOffset,
                                         const sf::FloatRect& txr,
                                         const sf::FloatRect& txrPaw)
                {
                    const auto pos = window.mapPixelToCoords((resolution / 2.f).toVector2i(), gameView);

                    for (int i = 0; i < 32; ++i)
                        particles.emplace_back(makeParticle(pos.x, pos.y, ParticleType::Star, 0.25f, 0.75f));

                    return Cat{.type        = catType,
                               .position    = pos,
                               .rangeOffset = rangeOffset,
                               .sprite = {.position = pos, .scale{0.2f, 0.2f}, .origin = txr.size / 2.f, .textureRect = txr},
                               .pawSprite = {.position = pos, .scale{0.1f, 0.1f}, .origin = txrPaw.size / 2.f, .textureRect = txrPaw},
                               .nameIdx      = getNextCatNameIdx(),
                               .beingDragged = 0.f};
                };

                const auto makeCatModifiers =
                    [&](const char* labelCooldown, const char* labelRange, const CatType catType, const float initialCooldown)
                {
                    auto& maxCooldown = catCooldownPerType[static_cast<int>(catType)];
                    auto& range       = catRangePerType[static_cast<int>(catType)];

                    std::sprintf(labelBuffer, "%.2fs", static_cast<double>(maxCooldown / 1000.f));
                    if (makePurchasableButton(labelCooldown, 20.f, 1.25f, 13.f + (initialCooldown - maxCooldown) / 35.f))
                        maxCooldown *= 0.95f;

                    std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(range));
                    if (makePurchasableButton(labelRange, 18.f, 1.75f, range / 15.f))
                        range *= 1.05f;
                };

                const auto resetCatDrag = [&]
                {
                    for (auto& cat : cats)
                        cat.beingDragged = 0.f;

                    catDragPosition.reset();
                };

                if (countFingersDown == 0 && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
                    resetCatDrag();

                if (comboPurchased && psvComboStartTime.nPurchases > 0)
                {
                    std::sprintf(labelBuffer, "%d cats", nCatNormal);
                    if (makePurchasableButton("Cat", 35, 1.7f, static_cast<float>(nCatNormal)))
                    {
                        resetCatDrag();
                        cats.emplace_back(makeCat(CatType::Normal, {0.f, 0.f}, txrCat, txrCatPaw));

                        if (nCatNormal == 0)
                        {
                            doTip("Cat periodically pop bubbles for you!\nYou can drag them around to position them.");
                        }
                    }
                }

                const bool catUpgradesUnlocked = psvBubbleCount.nPurchases > 0 && nCatNormal >= 2 && nCatUnicorn >= 1;
                if (catUpgradesUnlocked)
                    makeCatModifiers("Cat cooldown", "Cat range", CatType::Normal, 1000.f);

                // UNICORN CAT
                const bool unicatsUnlocked        = psvBubbleCount.nPurchases > 0 && nCatNormal >= 3;
                const bool unicatUpgradesUnlocked = unicatsUnlocked && nCatUnicorn >= 2 && nCatDevil >= 1;
                if (unicatsUnlocked)
                {
                    std::sprintf(labelBuffer, "%d unicats", nCatUnicorn);
                    if (makePurchasableButton("Unicat", 250, 1.75f, static_cast<float>(nCatUnicorn)))
                    {
                        resetCatDrag();
                        cats.emplace_back(makeCat(CatType::Unicorn, {0.f, -100.f}, txrUniCat, txrUniCatPaw));

                        if (nCatUnicorn == 0)
                        {
                            doTip(
                                "Unicats transform bubbles in star bubbles,\nwhich are worth much more!\nPop them at "
                                "the end of a combo for huge gains!");
                        }
                    }

                    if (unicatUpgradesUnlocked)
                        makeCatModifiers("Unicat cooldown", "Unicat range", CatType::Unicorn, 3000.f);
                }

                // DEVIL CAT
                const bool devilcatsUnlocked = psvBubbleValue.nPurchases > 0 && nCatNormal >= 6 && nCatUnicorn >= 4;
                const bool devilcatsUpgradesUnlocked = devilcatsUnlocked && nCatDevil >= 2 && nCatWitch >= 1;
                if (devilcatsUnlocked)
                {
                    std::sprintf(labelBuffer, "%d devilcats", nCatDevil);
                    if (makePurchasableButton("Devilcat", 15000.f, 1.6f, static_cast<float>(nCatDevil)))
                    {
                        resetCatDrag();
                        cats.emplace_back(makeCat(CatType::Devil, {0.f, 100.f}, txrDevilCat, txrDevilCatPaw));

                        if (nCatDevil == 0)
                        {
                            doTip(
                                "Devilcats transform bubbles in bombs!\nPop a bomb to explode all nearby bubbles\nwith "
                                "a huge x10 money multiplier!");
                        }
                    }

                    if (devilcatsUpgradesUnlocked)
                        makeCatModifiers("Devilcat cooldown", "Devilcat range", CatType::Devil, 7000.f);
                }

                // WITCH CAT
                const bool witchCatUnlocked         = nCatNormal >= 10 && nCatUnicorn >= 5 && nCatDevil >= 2;
                const bool witchCatUpgradesUnlocked = witchCatUnlocked && nCatDevil >= 10 && nCatWitch >= 5;
                if (witchCatUnlocked)
                {
                    std::sprintf(labelBuffer, "%d witch cats", nCatWitch);
                    if (makePurchasableButton("Witch cat", 200000.f, 1.5f, static_cast<float>(nCatWitch)))
                    {
                        resetCatDrag();
                        cats.emplace_back(makeCat(CatType::Witch, {0.f, 0.f}, txrWitchCat, txrWitchCatPaw));
                    }

                    if (witchCatUpgradesUnlocked)
                        makeCatModifiers("Witch cat cooldown", "Witch cat range", CatType::Witch, 2000.f);
                }

                const auto milestoneText = [&]() -> std::string
                {
                    if (!comboPurchased)
                        return "buy combo to earn money faster";

                    if (psvComboStartTime.nPurchases == 0)
                        return "buy longer combo to unlock cats";

                    if (nCatNormal == 0)
                        return "buy a cat";

                    std::string result;
                    const auto  startList = [&](const char* s)
                    {
                        result += result.empty() ? "" : "\n\n";
                        result += s;
                    };

                    const auto needNCats = [&](auto& count, auto needed)
                    {
                        const char* name = "";

                        // clang-format off
                        if      (&count == &nCatNormal)  name = "cat";
                        else if (&count == &nCatUnicorn) name = "unicat";
                        else if (&count == &nCatDevil)   name = "devilcat";
                        else if (&count == &nCatWitch)   name = "witch cat";
                        // clang-format on

                        if (count < needed)
                            result += "\n- buy " + std::to_string(needed - count) + " more " + name + "(s)";
                    };

                    if (!mapPurchased)
                    {
                        startList("to increase playing area:");
                        result += "\n- buy map scrolling";
                    }

                    if (!unicatsUnlocked)
                    {
                        startList("to unlock unicats:");

                        if (psvBubbleCount.nPurchases == 0)
                            result += "\n- buy more bubbles";

                        needNCats(nCatNormal, 3);
                    }

                    if (!catUpgradesUnlocked && unicatsUnlocked)
                    {
                        startList("to unlock cat upgrades:");

                        if (psvBubbleCount.nPurchases == 0)
                            result += "\n- buy more bubbles";

                        needNCats(nCatNormal, 2);
                        needNCats(nCatUnicorn, 1);
                    }

                    if (unicatsUnlocked && !bubbleValueUnlocked)
                    {
                        startList("to unlock bubble value:");

                        if (psvBubbleCount.nPurchases == 0)
                            result += "\n- buy more bubbles";

                        needNCats(nCatUnicorn, 3);
                    }

                    if (unicatsUnlocked && bubbleValueUnlocked && !devilcatsUnlocked)
                    {
                        startList("to unlock devilcats:");

                        if (psvBubbleValue.nPurchases == 0)
                            result += "\n- buy bubble value";

                        needNCats(nCatNormal, 6);
                        needNCats(nCatUnicorn, 4);
                    }

                    if (unicatsUnlocked && devilcatsUnlocked && !unicatUpgradesUnlocked)
                    {
                        startList("to unlock unicat upgrades:");
                        needNCats(nCatUnicorn, 2);
                        needNCats(nCatDevil, 1);
                    }

                    if (unicatsUnlocked && devilcatsUnlocked && !witchCatUnlocked)
                    {
                        startList("to unlock witch cats:");
                        needNCats(nCatNormal, 10);
                        needNCats(nCatUnicorn, 5);
                        needNCats(nCatDevil, 2);
                    }

                    if (unicatsUnlocked && devilcatsUnlocked && witchCatUnlocked && !devilcatsUpgradesUnlocked)
                    {
                        startList("to unlock devilcat upgrades:");
                        needNCats(nCatDevil, 2);
                        needNCats(nCatWitch, 1);
                    }

                    if (unicatsUnlocked && devilcatsUnlocked && witchCatUnlocked && !witchCatUpgradesUnlocked)
                    {
                        startList("to unlock witch cat upgrades:");
                        needNCats(nCatDevil, 10);
                        needNCats(nCatWitch, 5);
                    }

                    return result;
                }();

                if (milestoneText != "")
                {
                    ImGui::Separator();

                    ImGui::SetWindowFontScale(0.65f);
                    ImGui::Text("%s", milestoneText.c_str());

                    ImGui::SetWindowFontScale(1.f);
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(" Settings "))
            {
                static float masterVolume = 100.f;
                static float musicVolume  = 100.f;

                ImGui::SetNextItemWidth(250.f);
                ImGui::SliderFloat("Master volume", &masterVolume, 0.f, 100.f, "%.0f%%");

                ImGui::SetNextItemWidth(250.f);
                ImGui::SliderFloat("Music volume", &musicVolume, 0.f, 100.f, "%.0f%%");

                ImGui::Checkbox("Play audio in background", &playAudioInBackground);
                const float volumeMult = playAudioInBackground ? 1.f : window.hasFocus() ? 1.f : 0.f;

                listener.volume = masterVolume * volumeMult;
                musicBGM.setVolume(musicVolume * volumeMult);

                ImGui::Checkbox("Enable tips", &tipsEnabled);

                ImGui::SetNextItemWidth(250.f);
                ImGui::SliderFloat("Minimap Scale", &minimapScale, 5.f, 30.f, "%.2f");

                const float fps = 1.f / fpsClock.getElapsedTime().asSeconds();
                ImGui::Text("FPS: %.2f", static_cast<double>(fps));

                const float timePlayed    = playedClock.getElapsedTime().asSeconds();
                const U64   secondsPlayed = static_cast<U64>(std::fmod(timePlayed, 60.f));
                const U64   minutesPlayed = static_cast<U64>(timePlayed / 60);
                ImGui::Text("Time played: %llu min %llu sec", minutesPlayed, secondsPlayed);

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
        ImGui::PopFont();

        window.clear(sf::Color{157, 171, 191});

        window.setView(gameView);
        window.draw(txBackground);

        cpuDrawableBatch.clear();

        // ---
        sf::CircleShape catRadiusCircle{{
            .outlineTextureRect = txrWhiteDot,
            .fillColor          = sf::Color::Transparent,
            .outlineThickness   = 1.f,
            .pointCount         = 32,
        }};


        //
        //
        // TODO: move up to avoid reallocation
        sf::Text catTextName{fontDailyBubble,
                             {.characterSize    = 24u,
                              .fillColor        = sf::Color::White,
                              .outlineColor     = colorBlueOutline,
                              .outlineThickness = 1.5f}};

        sf::Text catTextStatus{fontDailyBubble,
                               {.characterSize    = 16u,
                                .fillColor        = sf::Color::White,
                                .outlineColor     = colorBlueOutline,
                                .outlineThickness = 1.f}};

        for (auto& cat : cats)
        {
            cpuDrawableBatch.add(cat.sprite);
            cpuDrawableBatch.add(cat.pawSprite);

            const auto maxCooldown = catCooldownPerType[static_cast<int>(cat.type)];
            const auto range       = catRangePerType[static_cast<int>(cat.type)];

            constexpr sf::Color colorsByType[5]{
                sf::Color::Blue,   // Cat
                sf::Color::Purple, // Unicorn
                sf::Color::Red,    // Devil
                sf::Color::Green,  // Witch
                sf::Color::White,  // Astromeow
            };

            catRadiusCircle.position = cat.position + cat.rangeOffset;
            catRadiusCircle.origin   = {range, range};
            catRadiusCircle.setRadius(range);
            catRadiusCircle.setOutlineColor(colorsByType[static_cast<int>(cat.type)].withAlpha(
                cat.cooldown < 0.f ? static_cast<sf::base::U8>(0u)
                                   : static_cast<sf::base::U8>(cat.cooldown / maxCooldown * 128.f)));
            cpuDrawableBatch.add(catRadiusCircle);

            catTextName.setString(catNames[cat.nameIdx]);
            catTextName.position = cat.position + sf::Vector2f{0.f, 48.f};
            catTextName.origin   = catTextName.getLocalBounds().size / 2.f;
            cpuDrawableBatch.add(catTextName);

            constexpr const char* catActions[5]{"Pops", "Shines", "IEDs", "Hexes", "Flights"};
            catTextStatus.setString(std::to_string(cat.hits) + " " + catActions[static_cast<int>(cat.type)]);
            catTextStatus.position = cat.position + sf::Vector2f{0.f, 68.f};
            catTextStatus.origin   = catTextStatus.getLocalBounds().size / 2.f;
            cat.textStatusShakeEffect.applyToText(catTextStatus);
            cpuDrawableBatch.add(catTextStatus);
        };
        // ---

        sf::Sprite tempSprite;

        // ---
        const sf::FloatRect bubbleRects[3]{txrBubble128, txrBubbleStar, txrBomb};

        for (const auto& bubble : bubbles)
        {
            bubble.applyToSprite(tempSprite);
            tempSprite.textureRect = bubbleRects[static_cast<int>(bubble.type)];
            tempSprite.origin      = tempSprite.textureRect.size / 2.f;

            cpuDrawableBatch.add(tempSprite);
        }
        // ---

        // ---
        const sf::FloatRect particleRects[4]{txrParticle, txrStarParticle, txrFireParticle, txrHexParticle};

        for (const auto& particle : particles)
        {
            particle.applyToSprite(tempSprite);
            tempSprite.textureRect = particleRects[static_cast<int>(particle.type)];
            tempSprite.origin      = tempSprite.textureRect.size / 2.f;

            cpuDrawableBatch.add(tempSprite);
        }
        // ---

        // ---
        sf::Text tempText{fontDailyBubble,
                          {.characterSize    = 16u,
                           .fillColor        = sf::Color::White,
                           .outlineColor     = colorBlueOutline,
                           .outlineThickness = 1.0f}};

        for (const auto& textParticle : textParticles)
        {
            textParticle.applyToText(tempText);
            cpuDrawableBatch.add(tempText);
        }
        // ---

        window.draw(cpuDrawableBatch, {.texture = &textureAtlas.getTexture()});

        const sf::View hudView{.center = {resolution.x / 2.f, resolution.y / 2.f}, .size = resolution};
        window.setView(hudView);

        moneyText.setString("$" + std::to_string(money));
        moneyText.scale  = {1.f, 1.f};
        moneyText.origin = moneyText.getLocalBounds().size / 2.f;

        moneyText.setTopLeft({15.f, 70.f});
        moneyTextShakeEffect.update(deltaTimeMs);
        moneyTextShakeEffect.applyToText(moneyText);

        const float yBelowMinimap = mapPurchased ? (boundaries.y / minimapScale) + 15.f : 0.f;

        moneyText.position.y = yBelowMinimap + 30.f;
        window.draw(moneyText);

        if (comboPurchased)
        {
            comboText.setString("x" + std::to_string(combo + 1));

            comboTextShakeEffect.update(deltaTimeMs);
            comboTextShakeEffect.applyToText(comboText);

            comboText.position.y = yBelowMinimap + 50.f;
            window.draw(comboText);
        }

        //
        // Combo bar
        window.draw(sf::RectangleShape{{.position  = {comboText.getCenterRight().x + 3.f, mapPurchased ? 110.f : 55.f},
                                        .fillColor = sf::Color{255, 255, 255, 75},
                                        .size      = {100.f * comboTimer / 700.f, 20.f}}},
                    /* texture */ nullptr);

        //
        // Minimap
        if (mapPurchased)
            drawMinimap(minimapScale, mapLimit, gameView, hudView, window, txBackground, cpuDrawableBatch, textureAtlas);

        //
        // UI
        imGuiContext.render(window);

        //
        // Splash screen
        drawSplashScreen(deltaTimeMs, window, txLogo);

        //
        // Tips
        if (tipTimer > 0.f)
        {
            tipTimer -= deltaTimeMs;

            if (tipsEnabled)
            {
                float fade = 255.f;

                if (tipTimer > 4000.f)
                    fade = remap(tipTimer, 4000.f, 4500.f, 255.f, 0.f);
                else if (tipTimer < 500.f)
                    fade = remap(tipTimer, 0.f, 500.f, 0.f, 255.f);

                const auto fadeAlpha = static_cast<sf::base::U8>(sf::base::clamp(fade, 0.f, 255.f));

                sf::Text tipText{fontDailyBubble,
                                 {.position      = {195.f, 265.f},
                                  .string        = tipString,
                                  .characterSize = 32u,
                                  .fillColor     = sf::Color::Black.withAlpha(fadeAlpha)}};

                sf::Sprite tipSprite{.position    = resolution / 2.f,
                                     .scale       = {0.70f, 0.70f},
                                     .origin      = txByteTip.getSize().toVector2f() / 2.f,
                                     .textureRect = txByteTip.getRect(),
                                     .color       = sf::Color::White.withAlpha(fadeAlpha)};

                window.draw(tipSprite, txByteTip);
                window.draw(tipText);
            }
        }

        //
        // Display
        window.display();
    }
}

// TODO IDEAS:
// - bubbles that need to be weakened
// - individual cat levels and upgrades
// - unlockable areas, different bubble types
// - timer
// - prestige
// - unicat cooldown scaling should be less powerful and capped
// - cat cooldown scaling should be a bit more powerful at the beginning and capped around 0.4
// - unicat range scaling should be much more exponential and be capped
// x - bomb explosion should have circular range (slight nerf)
// x - bomb should have higher mass for bubble collisions ??
// - cats can go out of bounds if pushed by other cats
// - cats should not be allowed next to the bounds, should be gently pushed away
