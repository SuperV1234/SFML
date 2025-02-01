#include "json.hpp"

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
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Assume.hpp"
#include "SFML/Base/Macros.hpp"
#include "SFML/Base/Math/Ceil.hpp"
#include "SFML/Base/Math/Exp.hpp"
#include "SFML/Base/Math/Sqrt.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/SizeT.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <cstdio>


////////////////////////////////////////////////////////////
#define BUBBLEBYTE_VERSION_STR "v0.0.3"

namespace sf
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vector2f, x, y);
}

namespace
{
////////////////////////////////////////////////////////////
using json = nlohmann::json;

////////////////////////////////////////////////////////////
using sf::base::SizeT;
using sf::base::U64;
using sf::base::U8;

////////////////////////////////////////////////////////////
using MoneyType = U64;

////////////////////////////////////////////////////////////
constexpr sf::Vector2f resolution{1366.f, 768.f};
constexpr auto         resolutionUInt = resolution.toVector2u();

////////////////////////////////////////////////////////////
constexpr sf::Vector2f boundaries{1366.f * 10.f, 768.f};

////////////////////////////////////////////////////////////
constexpr sf::Color colorBlueOutline{50, 84, 135};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline std::minstd_rand0& getRandomEngine()
{
    static std::minstd_rand0 randomEngine(std::random_device{}());
    return randomEngine;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline float getRndFloat(const float min, const float max)
{
    SFML_BASE_ASSERT(min <= max);
    SFML_BASE_ASSUME(min <= max);

    return std::uniform_real_distribution<float>{min, max}(getRandomEngine());
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getRndVector2f(const sf::Vector2f mins,
                                                                                   const sf::Vector2f maxs)
{
    return {getRndFloat(mins.x, maxs.x), getRndFloat(mins.y, maxs.y)};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline sf::Vector2f getRndVector2f(const sf::Vector2f maxs)
{
    return getRndVector2f({0.f, 0.f}, maxs);
}

////////////////////////////////////////////////////////////
[[nodiscard]] auto getShuffledCatNames(auto&& randomEngine)
{
    std::array names{"Gorgonzola", "Provolino",  "Pistacchietto", "Ricottina",  "Mozzarellina",  "Tiramisu",
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

    std::shuffle(names.begin(), names.end(), randomEngine);
    return names;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const, gnu::always_inline]] inline constexpr float remap(
    const float x,
    const float oldMin,
    const float oldMax,
    const float newMin,
    const float newMax)
{
    SFML_BASE_ASSERT(oldMax != oldMin);
    SFML_BASE_ASSUME(oldMax != oldMin);

    return newMin + ((x - oldMin) / (oldMax - oldMin)) * (newMax - newMin);
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const, gnu::always_inline]] inline constexpr float exponentialApproach(
    const float current,
    const float target,
    const float deltaTimeMs,
    const float speed)
{
    SFML_BASE_ASSERT(speed >= 0.f);
    SFML_BASE_ASSUME(speed >= 0.f);

    const float factor = 1.f - sf::base::exp(-deltaTimeMs / speed);
    return current + (target - current) * factor;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const, gnu::always_inline]] inline constexpr sf::Vector2f exponentialApproach(
    const sf::Vector2f current,
    const sf::Vector2f target,
    const float        deltaTimeMs,
    const float        speed)
{
    return {exponentialApproach(current.x, target.x, deltaTimeMs, speed),
            exponentialApproach(current.y, target.y, deltaTimeMs, speed)};
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] TextShakeEffect
{
    float grow  = 0.f;
    float angle = 0.f;

    ////////////////////////////////////////////////////////////
    void bump(const float strength)
    {
        grow  = strength;
        angle = getRndFloat(-grow * 0.2f, grow * 0.2f);
    }

    ////////////////////////////////////////////////////////////
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
enum class [[nodiscard]] BubbleType : U8
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

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        position += velocity * deltaTime;
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToSprite(sf::Sprite& sprite) const
    {
        sprite.position = position;
        sprite.scale    = {scale, scale};
        sprite.rotation = sf::radians(rotation);
    }
};

////////////////////////////////////////////////////////////
enum class [[nodiscard]] ParticleType : U8
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

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        velocity.y += accelerationY * deltaTime;
        position += velocity * deltaTime;

        rotation += torque * deltaTime;

        opacity = sf::base::clamp(opacity - opacityDecay * deltaTime, 0.f, 1.f);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToTransformable(auto& transformable) const
    {
        transformable.position = position;
        transformable.scale    = {scale, scale};
        transformable.rotation = sf::radians(rotation);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure, gnu::always_inline]] inline U8 opacityAsAlpha() const
    {
        return static_cast<U8>(opacity * 255.f);
    }
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] Particle
{
    ParticleData data;
    ParticleType type;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        data.update(deltaTime);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToSprite(sf::Sprite& sprite) const
    {
        data.applyToTransformable(sprite);
        sprite.color.a = data.opacityAsAlpha();
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline Particle makeParticle(
    const sf::Vector2f position,
    const ParticleType particleType,
    const float        scaleMult,
    const float        speedMult,
    const float        opacity = 1.f)
{
    return {.data = {.position      = position,
                     .velocity      = getRndVector2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * speedMult,
                     .scale         = getRndFloat(0.08f, 0.27f) * scaleMult,
                     .accelerationY = 0.002f,
                     .opacity       = opacity,
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

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        data.update(deltaTime);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void applyToText(sf::Text& text) const
    {
        text.setString(buffer); // TODO P1: should find a way to assign directly to text buffer

        data.applyToTransformable(text);
        text.origin = text.getLocalBounds().size / 2.f;

        text.setFillColor(text.getFillColor().withAlpha(data.opacityAsAlpha()));
        text.setOutlineColor(text.getOutlineColor().withAlpha(data.opacityAsAlpha()));
    }
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline TextParticle makeTextParticle(const sf::Vector2f position, const int combo)
{
    return {.buffer = {},
            .data   = {.position      = {position.x, position.y - 10.f},
                       .velocity      = getRndVector2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.425f,
                       .scale         = sf::base::clamp(1.f + 0.1f * static_cast<float>(combo + 1) / 1.75f, 1.f, 3.0f),
                       .accelerationY = 0.0042f,
                       .opacity       = 1.f,
                       .opacityDecay  = 0.00175f,
                       .rotation      = 0.f,
                       .torque        = getRndFloat(-0.002f, 0.002f)}};
}

////////////////////////////////////////////////////////////
enum class [[nodiscard]] CatType : U8
{
    Normal = 0u,
    Uni    = 1u,
    Devil  = 2u,
    Witch  = 3u,
    Astro  = 4u,

    Count
};

////////////////////////////////////////////////////////////
constexpr auto nCatTypes = static_cast<SizeT>(CatType::Count);

////////////////////////////////////////////////////////////
struct [[nodiscard]] Cat
{
    CatType type;

    sf::Vector2f position;
    sf::Vector2f rangeOffset;

    float wobbleTimer = 0.f;
    float cooldown    = 0.f;

    sf::Vector2f drawPosition;

    sf::Vector2f pawPosition;
    sf::Angle    pawRotation;

    float mainOpacity = 255.f;
    float pawOpacity  = 255.f;

    float inspiredTimer = 0.f;

    SizeT nameIdx;

    TextShakeEffect textStatusShakeEffect;

    int hits = 0;

    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] AstroState
    {
        float startX;

        float velocityX     = 0.f;
        bool  wrapped       = false;
        float particleTimer = 0.f;
    };

    sf::base::Optional<AstroState> astroState;

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        textStatusShakeEffect.update(deltaTime);

        wobbleTimer += deltaTime * 0.002f;
        drawPosition.x = position.x;
        drawPosition.y = position.y + std::sin(wobbleTimer * 2.f) * 7.5f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool updateCooldown(const float maxCooldown, const float deltaTime)
    {
        if (inspiredTimer > 0.f)
            inspiredTimer -= deltaTime;

        const float cooldownSpeed = inspiredTimer > 0.f ? 2.f : 1.f;
        cooldown += deltaTime * cooldownSpeed;

        if (cooldown >= maxCooldown)
        {
            cooldown = maxCooldown;
            return true;
        }

        return false;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getRadius() const noexcept
    {
        return 64.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool isCloseToStartX() const noexcept
    {
        SFML_BASE_ASSERT(type == CatType::Astro);
        SFML_BASE_ASSERT(astroState.hasValue());

        return astroState->wrapped && sf::base::fabs(position.x - astroState->startX) < 400.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool isAstroAndInFlight() const noexcept
    {
        return type == CatType::Astro && astroState.hasValue();
    }
};

////////////////////////////////////////////////////////////
[[nodiscard]] Cat makeCat(const CatType catType, const sf::Vector2f& position, const sf::Vector2f rangeOffset, const SizeT nameIdx)
{
    return Cat{.type                  = catType,
               .position              = position,
               .rangeOffset           = rangeOffset,
               .drawPosition          = position,
               .pawPosition           = position,
               .pawRotation           = sf::radians(0.f),
               .nameIdx               = nameIdx,
               .textStatusShakeEffect = {},
               .astroState            = {}};
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] CollisionResolution
{
    sf::Vector2f iDisplacement;
    sf::Vector2f jDisplacement;
    sf::Vector2f iVelocityChange;
    sf::Vector2f jVelocityChange;
};

////////////////////////////////////////////////////////////
[[nodiscard]] bool detectCollision(const sf::Vector2f iPosition, const sf::Vector2f jPosition, const float iRadius, const float jRadius)
{
    const sf::Vector2f diff        = jPosition - iPosition;
    const float        squaredDiff = diff.lengthSquared();

    const sf::Vector2f radii{iRadius, jRadius};
    const float        squaredRadiiSum = radii.lengthSquared();

    return squaredDiff < squaredRadiiSum;
}

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
        const float j = -(1.f + e) * vRelDotNormal / ((1.f / m1) + (1.f / m2));

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
    const auto
        result = handleCollision(deltaTimeMs, iCat.position, jCat.position, {}, {}, iCat.getRadius(), jCat.getRadius(), 1.f, 1.f);

    if (!result.hasValue())
        return false;

    iCat.position += result->iDisplacement;
    jCat.position += result->jDisplacement;

    return true;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline Bubble makeRandomBubble(const float mapLimit, const float maxY)
{
    const float scaleFactor = getRndFloat(0.07f, 0.17f) * 0.65f;

    return Bubble{.position = getRndVector2f({mapLimit, maxY}),
                  .velocity = getRndVector2f({-0.1f, -0.1f}, {0.1f, 0.1f}),
                  .scale    = scaleFactor,
                  .radius   = 350.f * scaleFactor,
                  .rotation = 0.f,
                  .type     = BubbleType::Normal};
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] GrowthFactors
{
    float initial;
    float linear         = 0.f;
    float multiplicative = 0.f;
    float exponential    = 1.f;
    float flat           = 0.f;
    float finalMult      = 1.f;
};

////////////////////////////////////////////////////////////
[[nodiscard, gnu::pure, gnu::always_inline]] inline float computeGrowth(const GrowthFactors& factors, const float n)
{
    return ((factors.initial + n * factors.multiplicative) * std::pow(factors.exponential, n) + factors.linear * n +
            factors.flat) *
           factors.finalMult;
}

////////////////////////////////////////////////////////////
struct [[nodiscard]] PSVData
{
    const SizeT         nMaxPurchases;
    const GrowthFactors cost;
    const GrowthFactors value;
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] PurchasableScalingValue
{
    const PSVData* data;
    SizeT          nPurchases = 0u;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float nextCost() const
    {
        return computeGrowth(data->cost, static_cast<float>(nPurchases));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float currentValue() const
    {
        return computeGrowth(data->value, static_cast<float>(nPurchases));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] auto maxSubsequentPurchases(const MoneyType money, float globalMultiplier) const
    {
        struct Result
        {
            SizeT     times    = 0u;
            MoneyType maxCost  = 0u;
            MoneyType nextCost = 0u;
        } result;

        MoneyType cumulative = 0;

        for (SizeT i = nPurchases; i < data->nMaxPurchases; ++i)
        {
            const auto currentCost = static_cast<MoneyType>(
                computeGrowth(data->cost, static_cast<float>(i)) * globalMultiplier);

            // Check if we can afford to buy the next upgrade
            if (cumulative + currentCost > money)
                break;

            // Track cumulative cost and update results
            cumulative += currentCost;
            ++result.times;
            result.maxCost = cumulative;

            // Calculate the cumulative cost for the next potential purchase
            const auto nextCostCandidate = static_cast<MoneyType>(
                computeGrowth(data->cost, static_cast<float>(i + 1)) * globalMultiplier);
            result.nextCost = cumulative + nextCostCandidate;
        }

        // Handle edge case: no purchases possible, but next cost exists
        if (result.times == 0 && nPurchases < data->nMaxPurchases)
        {
            result.nextCost = static_cast<MoneyType>(
                computeGrowth(data->cost, static_cast<float>(nPurchases)) * globalMultiplier);
        }

        return result;
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
    //
    // Screen position of minimap's top-left corner
    constexpr sf::Vector2f minimapPos = {15.f, 15.f};

    //
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

////////////////////////////////////////////////////////////
void drawSplashScreen(sf::RenderWindow& window, const sf::Texture& txLogo, const float splashTimer)
{
    float fade = 255.f;

    if (splashTimer > 1500.f)
        fade = remap(splashTimer, 1500.f, 1750.f, 255.f, 0.f);
    else if (splashTimer < 255.f)
        fade = splashTimer;

    window.draw({.position    = resolution / 2.f,
                 .scale       = {0.7f, 0.7f},
                 .origin      = txLogo.getSize().toVector2f() / 2.f,
                 .textureRect = txLogo.getRect(),
                 .color       = sf::Color::White.withAlpha(static_cast<U8>(fade))},
                txLogo);
}

////////////////////////////////////////////////////////////
struct Sounds
{
    ////////////////////////////////////////////////////////////
    struct LoadedSound : private sf::SoundBuffer, public sf::Sound
    {
        ////////////////////////////////////////////////////////////
        explicit LoadedSound(const sf::Path& filename) :
        sf::SoundBuffer(sf::SoundBuffer::loadFromFile("resources/" / filename).value()),
        sf::Sound(static_cast<const sf::SoundBuffer&>(*this))
        {
        }

        ////////////////////////////////////////////////////////////
        LoadedSound(const LoadedSound&) = delete;
        LoadedSound(LoadedSound&&)      = delete;
    };

    ////////////////////////////////////////////////////////////
    LoadedSound pop{"pop.ogg"};
    LoadedSound reversePop{"reversepop.ogg"};
    LoadedSound shine{"shine.ogg"};
    LoadedSound click{"click2.ogg"};
    LoadedSound byteMeow{"bytemeow.ogg"};
    LoadedSound grab{"grab.ogg"};
    LoadedSound drop{"drop.ogg"};
    LoadedSound scratch{"scratch.ogg"};
    LoadedSound buy{"buy.ogg"};
    LoadedSound explosion{"explosion.ogg"};
    LoadedSound makeBomb{"makebomb.ogg"};
    LoadedSound hex{"hex.ogg"};
    LoadedSound byteSpeak{"bytespeak.ogg"};
    LoadedSound prestige{"prestige.ogg"};
    LoadedSound launch{"launch.ogg"};
    LoadedSound rocket{"rocket.ogg"};

    ////////////////////////////////////////////////////////////
    explicit Sounds()
    {
        constexpr float worldAttenuation = 0.0015f;
        pop.setAttenuation(worldAttenuation);
        reversePop.setAttenuation(worldAttenuation);
        shine.setAttenuation(worldAttenuation);
        explosion.setAttenuation(worldAttenuation);
        makeBomb.setAttenuation(worldAttenuation);
        hex.setAttenuation(worldAttenuation);
        launch.setAttenuation(worldAttenuation);
        rocket.setAttenuation(worldAttenuation);

        constexpr float uiAttenuation = 0.f;
        click.setAttenuation(uiAttenuation);
        byteMeow.setAttenuation(uiAttenuation);
        grab.setAttenuation(uiAttenuation);
        drop.setAttenuation(uiAttenuation);
        scratch.setAttenuation(uiAttenuation);
        buy.setAttenuation(uiAttenuation);
        byteSpeak.setAttenuation(uiAttenuation);
        prestige.setAttenuation(uiAttenuation);

        scratch.setVolume(35.f);
        buy.setVolume(75.f);
        explosion.setVolume(75.f);
        rocket.setVolume(75.f);
    }

    ////////////////////////////////////////////////////////////
    Sounds(const Sounds&) = delete;
    Sounds(Sounds&&)      = delete;
};

////////////////////////////////////////////////////////////
struct GameResources
{
    sf::PlaybackDevice&      playbackDevice;
    sf::GraphicsContext&     graphicsContext;
    sf::RenderWindow&        window;
    sf::ImGui::ImGuiContext& imGuiContext;
    const sf::TextureAtlas&  textureAtlas;
    const sf::Font&          fontSuperBakery;
    const ImFont&            fontImGuiSuperBakery;
    sf::Music&               musicBGM;
};

////////////////////////////////////////////////////////////
namespace PSVDataConstants
{

constexpr PSVData comboStartTime //
    {.nMaxPurchases = 20u,
     .cost          = {.initial = 35.f, .linear = 125.f, .exponential = 1.7f},
     .value         = {.initial = 0.55f, .linear = 0.04f, .exponential = 1.02f}};

constexpr PSVData bubbleCount //
    {.nMaxPurchases = 30u,
     .cost          = {.initial = 75.f, .linear = 1500.f, .exponential = 2.5f},
     .value         = {.initial = 500.f, .linear = 325.f, .exponential = 1.01f}};

constexpr PSVData bubbleValue //
    {.nMaxPurchases = 19u,
     .cost          = {.initial = 2500.f, .linear = 2500.f, .exponential = 2.f},
     .value         = {.initial = 0.f, .linear = 1.f}};

constexpr PSVData explosionRadiusMult //
    {.nMaxPurchases = 10u, .cost = {.initial = 15000.f, .exponential = 1.5f}, .value = {.initial = 1.f, .linear = 0.1f}};

constexpr PSVData catNormalCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 2000.f, .exponential = 1.68f, .flat = -1500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

constexpr PSVData catNormalRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 4000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

constexpr PSVData catUniCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 6000.f, .exponential = 1.68f, .flat = -2500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

constexpr PSVData catUniRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 4000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

constexpr PSVData catDevilCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 2000.f, .exponential = 1.68f, .flat = -1500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

constexpr PSVData catDevilRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 4000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

// TODO test
constexpr PSVData catWitchCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 2000.f, .exponential = 1.68f, .flat = -1500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

// TODO test
constexpr PSVData catWitchRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 4000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

// TODO test
constexpr PSVData catAstroCooldownMult //
    {.nMaxPurchases = 12u,
     .cost          = {.initial = 2000.f, .exponential = 1.68f, .flat = -1500.f},
     .value         = {.initial = 1.f, .linear = 0.015f, .multiplicative = 0.05f, .exponential = 0.8f}};

// TODO test
constexpr PSVData catAstroRangeDiv //
    {.nMaxPurchases = 9u,
     .cost          = {.initial = 4000.f, .exponential = 1.85f, .flat = -2500.f},
     .value         = {.initial = 0.6f, .multiplicative = -0.05f, .exponential = 0.75f, .flat = 0.4f}};

constexpr PSVData multiPopRange //
    {.nMaxPurchases = 24u,
     .cost = {.initial = 1.0f, .linear = 1.0f, .multiplicative = 0.0f, .exponential = 1.5f, .flat = 0.0f, .finalMult = 1.0f},
     .value = {.initial = 64.0f, .linear = 8.0f, .multiplicative = 0.0f, .exponential = 1.0f, .flat = 0.0f, .finalMult = 1.0f}};

} // namespace PSVDataConstants

////////////////////////////////////////////////////////////
struct Game
{
    //
    // PSV instances
    PurchasableScalingValue psvComboStartTime{&PSVDataConstants::comboStartTime};
    PurchasableScalingValue psvBubbleCount{&PSVDataConstants::bubbleCount};
    PurchasableScalingValue psvBubbleValue{&PSVDataConstants::bubbleValue};
    PurchasableScalingValue psvExplosionRadiusMult{&PSVDataConstants::explosionRadiusMult};

    PurchasableScalingValue psvCooldownMultsPerCatType[nCatTypes]{{&PSVDataConstants::catNormalCooldownMult},
                                                                  {&PSVDataConstants::catUniCooldownMult},
                                                                  {&PSVDataConstants::catDevilCooldownMult},
                                                                  {&PSVDataConstants::catWitchCooldownMult},
                                                                  {&PSVDataConstants::catAstroCooldownMult}};

    PurchasableScalingValue psvRangeDivsPerCatType[nCatTypes]{{&PSVDataConstants::catNormalRangeDiv},
                                                              {&PSVDataConstants::catUniRangeDiv},
                                                              {&PSVDataConstants::catDevilRangeDiv},
                                                              {&PSVDataConstants::catWitchRangeDiv},
                                                              {&PSVDataConstants::catAstroRangeDiv}};

    PurchasableScalingValue psvMultiPopRange{&PSVDataConstants::multiPopRange};

    //
    // Currencies
    MoneyType money = 0u;

    //
    // Permanent currencies
    U64 prestigePoints = 0u;

    //
    // Purchases
    bool comboPurchased    = false;
    bool mapPurchased      = false;
    U64  mapLimitIncreases = 0u;

    //
    // Permanent purchases
    bool smarterCatsPurchased = false;
    bool windPurchased        = false;

    //
    // Permanent purchases settings
    bool multiPopEnabled = true;
    bool windEnabled     = false;

    //
    // Object state
    std::vector<Bubble> bubbles;
    std::vector<Cat>    cats;

    //
    // Statistics
    struct [[nodiscard]] Stats
    {
        float secondsPlayed            = 0.f;
        U64   bubblesPopped            = 0u;
        U64   bubblesPoppedRevenue     = 0u;
        U64   bubblesHandPopped        = 0u;
        U64   bubblesHandPoppedRevenue = 0u;
        U64   explosionRevenue         = 0u;
    };

    Stats statsTotal;
    Stats statsSession;

    //
    // Settings
    float masterVolume          = 100.f;
    float musicVolume           = 100.f;
    bool  playAudioInBackground = true;
    bool  playComboEndSound     = true;
    float minimapScale          = 20.f;
    bool  tipsEnabled           = false; // TODO: should be true by default

    ////////////////////////////////////////////////////////////
    void onPrestige(const U64 prestigeCount)
    {
        psvComboStartTime.nPurchases      = 0u;
        psvBubbleCount.nPurchases         = 0u;
        psvExplosionRadiusMult.nPurchases = 0u;

        for (auto& psv : psvCooldownMultsPerCatType)
            psv.nPurchases = 0u;

        for (auto& psv : psvRangeDivsPerCatType)
            psv.nPurchases = 0u;

        money = 0u;
        prestigePoints += prestigeCount;

        comboPurchased    = false;
        mapPurchased      = false;
        mapLimitIncreases = 0u;

        windEnabled = false;

        statsSession = {};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getMapLimit() const
    {
        return resolution.x * static_cast<float>(mapLimitIncreases + 1u);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr float getBaseCooldownByCatType(const CatType type) const
    {
        constexpr float baseCooldowns[nCatTypes]{
            1000.f, // Normal
            3000.f, // Uni
            7000.f, // Devil
            2000.f, // Witch
            5000.f  // Astro
        };

        return baseCooldowns[static_cast<U8>(type)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr float getBaseRangeByCatType(const CatType type) const
    {
        constexpr float baseRanges[nCatTypes]{
            96.f,  // Normal
            64.f,  // Uni
            48.f,  // Devil
            256.f, // Witch
            48.f   // Astro
        };

        return baseRanges[static_cast<U8>(type)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline PurchasableScalingValue& getCooldownMultPSVByCatType(const CatType catType)
    {
        return psvCooldownMultsPerCatType[static_cast<U8>(catType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline PurchasableScalingValue& getRangeDivPSVByCatType(const CatType catType)
    {
        return psvRangeDivsPerCatType[static_cast<U8>(catType)];
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedCooldownByCatType(const CatType catType)
    {
        return getBaseCooldownByCatType(catType) * getCooldownMultPSVByCatType(catType).currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedRangeByCatType(const CatType catType)
    {
        return getBaseRangeByCatType(catType) / getRangeDivPSVByCatType(catType).currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getComputedBombExplosionRadius() const
    {
        return 200.f * psvExplosionRadiusMult.currentValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getComputedMultiPopRange() const
    {
        if (psvMultiPopRange.nPurchases == 0u)
            return 0.f;

        if (psvMultiPopRange.nPurchases == 1u)
            return 64.f;

        return computeGrowth(psvMultiPopRange.data->value, static_cast<float>(psvMultiPopRange.nPurchases - 1));
    }
};

////////////////////////////////////////////////////////////
enum class ControlFlow
{
    Continue,
    Break
};

////////////////////////////////////////////////////////////
class SpatialGrid
{
private:
    static inline constexpr float gridSize = 64.f;
    static inline constexpr SizeT nCellsX  = static_cast<SizeT>(boundaries.x / gridSize) + 1;
    static inline constexpr SizeT nCellsY  = static_cast<SizeT>(boundaries.y / gridSize) + 1;

    [[nodiscard, gnu::always_inline]] inline static constexpr SizeT convert2DTo1D(const SizeT x, const SizeT y, const SizeT width)
    {
        return y * width + x;
    }

    [[nodiscard, gnu::always_inline]] inline constexpr auto computeGridRange(const sf::Vector2f center, const float radius) const
    {
        const float minX = center.x - radius;
        const float minY = center.y - radius;
        const float maxX = center.x + radius;
        const float maxY = center.y + radius;

        struct Result
        {
            SizeT xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx;
        };

        return Result{static_cast<SizeT>(sf::base::max(0.f, minX / gridSize)),
                      static_cast<SizeT>(sf::base::max(0.f, minY / gridSize)),
                      static_cast<SizeT>(sf::base::clamp(maxX / gridSize, 0.f, static_cast<float>(nCellsX) - 1.f)),
                      static_cast<SizeT>(sf::base::clamp(maxY / gridSize, 0.f, static_cast<float>(nCellsY) - 1.f))};
    }


    std::vector<SizeT> m_objectIndices;          // Flat list of all bubble indices in all cells
    std::vector<SizeT> m_cellStartIndices;       // Tracks where each cell's data starts in `bubbleIndices`
    std::vector<SizeT> m_cellInsertionPositions; // Temporary copy of `cellStartIndices` to track insertion points

public:
    void forEachIndexInRadius(const sf::Vector2f center, const float radius, auto&& func)
    {
        const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(center, radius);

        // Check all candidate cells
        for (SizeT cellX = xCellStartIdx; cellX <= xCellEndIdx; ++cellX)
            for (SizeT cellY = yCellStartIdx; cellY <= yCellEndIdx; ++cellY)
            {
                const SizeT cellIdx = convert2DTo1D(cellX, cellY, nCellsX);

                // Get range of bubbles in this cell
                const SizeT start = m_cellStartIndices[cellIdx];
                const SizeT end   = m_cellStartIndices[cellIdx + 1];

                // Check each bubble in cell
                for (SizeT i = start; i < end; ++i)
                    if (func(m_objectIndices[i]) == ControlFlow::Break)
                        return;
            }
    }

    void forEachUniqueIndexPair(auto&& func)
    {
        for (SizeT ix = 0; ix < nCellsX; ++ix)
            for (SizeT iy = 0; iy < nCellsY; ++iy)
            {
                const SizeT cellIdx = convert2DTo1D(ix, iy, nCellsX);
                const SizeT start   = m_cellStartIndices[cellIdx];
                const SizeT end     = m_cellStartIndices[cellIdx + 1];

                for (SizeT i = start; i < end; ++i)
                    for (SizeT j = i + 1; j < end; ++j)
                        func(m_objectIndices[i], m_objectIndices[j]);
            }
    }

    void clear()
    {
        m_cellStartIndices.clear();
        m_cellStartIndices.resize(nCellsX * nCellsY + 1, 0); // +1 for prefix sum
    }

    void populate(const auto& bubbles)
    {
        //
        // First Pass (Counting):
        // - Calculate how many bubbles will be placed in each grid cell.
        for (auto& bubble : bubbles)
        {
            const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(bubble.position,
                                                                                                   bubble.radius);

            // For each cell the bubble covers, increment the count
            for (SizeT x = xCellStartIdx; x <= xCellEndIdx; ++x)
                for (SizeT y = yCellStartIdx; y <= yCellEndIdx; ++y)
                {
                    const SizeT cellIdx = convert2DTo1D(x, y, nCellsX) + 1; // +1 offsets for prefix sum
                    ++m_cellStartIndices[sf::base::min(cellIdx, m_cellStartIndices.size() - 1)];
                }
        }

        //
        // Second Pass (Prefix Sum):
        // - Calculate the starting index for each cell’s data in `m_objectIndices`.

        // Prefix sum to compute start indices
        for (SizeT i = 1; i < m_cellStartIndices.size(); ++i)
            m_cellStartIndices[i] += m_cellStartIndices[i - 1];

        m_objectIndices.resize(m_cellStartIndices.back()); // Total bubbles across all cells

        // Used to track where to insert the next bubble index into the `m_objectIndices` buffer for each cell
        m_cellInsertionPositions.assign(m_cellStartIndices.begin(), m_cellStartIndices.end());

        //
        // Third Pass (Populating):
        // - Place the bubble indices into the correct positions in the `m_objectIndices` buffer.
        for (SizeT i = 0; i < bubbles.size(); ++i)
        {
            const auto& bubble                                                  = bubbles[i];
            const auto [xCellStartIdx, yCellStartIdx, xCellEndIdx, yCellEndIdx] = computeGridRange(bubble.position,
                                                                                                   bubble.radius);

            // Insert the bubble index into all overlapping cells
            for (SizeT x = xCellStartIdx; x <= xCellEndIdx; ++x)
                for (SizeT y = yCellStartIdx; y <= yCellEndIdx; ++y)
                {
                    const SizeT cellIdx        = convert2DTo1D(x, y, nCellsX);
                    const SizeT insertPos      = m_cellInsertionPositions[cellIdx]++;
                    m_objectIndices[insertPos] = i;
                }
        }
    }
};

////////////////////////////////////////////////////////////
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Cat, type, position, rangeOffset, wobbleTimer, cooldown, inspiredTimer, nameIdx, hits);

////////////////////////////////////////////////////////////
void to_json(json& j, const PurchasableScalingValue& p)
{
    j = p.nPurchases;
}

////////////////////////////////////////////////////////////
void from_json(const json& j, PurchasableScalingValue& p)
{
    p.nPurchases = j;
}

////////////////////////////////////////////////////////////
template <SizeT N>
void to_json(json& j, const PurchasableScalingValue (&p)[N])
{
    j = json::array({p[0].nPurchases, p[1].nPurchases, p[2].nPurchases, p[3].nPurchases, p[4].nPurchases});
}

////////////////////////////////////////////////////////////
template <SizeT N>
void from_json(const json& j, PurchasableScalingValue (&p)[N])
{
    p[0].nPurchases = j[0];
    p[1].nPurchases = j[1];
    p[2].nPurchases = j[2];
    p[3].nPurchases = j[3];
    p[4].nPurchases = j[4];
}

////////////////////////////////////////////////////////////
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Game::Stats,
    secondsPlayed,
    bubblesPopped,
    bubblesPoppedRevenue,
    bubblesHandPopped,
    bubblesHandPoppedRevenue,
    explosionRevenue);

////////////////////////////////////////////////////////////
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Game,
    psvComboStartTime,
    psvBubbleCount,
    psvBubbleValue,
    psvExplosionRadiusMult,
    psvCooldownMultsPerCatType,
    psvRangeDivsPerCatType,
    psvMultiPopRange,
    money,
    prestigePoints,
    comboPurchased,
    mapPurchased,
    mapLimitIncreases,
    smarterCatsPurchased,
    windPurchased,
    multiPopEnabled,
    windEnabled,
    // bubbles,
    cats,
    statsTotal,
    statsSession,
    masterVolume,
    musicVolume,
    playAudioInBackground,
    playComboEndSound,
    minimapScale,
    tipsEnabled);

////////////////////////////////////////////////////////////
void saveGameToFile(const Game& game)
{
    const auto gameAsJsonString = nlohmann::json(game).dump(4);
    std::ofstream("save.json") << gameAsJsonString;
}

////////////////////////////////////////////////////////////
void loadGameFromFile(Game& game)
{
    std::ifstream gameAsJsonString("save.json");
    json::parse(gameAsJsonString).get_to(game);
}

} // namespace


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
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
    const auto       addImgResourceToAtlas = [&](const sf::Path& path)
    { return textureAtlas.add(sf::Image::loadFromFile("resources" / path).value()).value(); };

    //
    //
    // Load and initialize resources
    /* --- Fonts */
    const auto fontSuperBakery = sf::Font::openFromFile("resources/superbakery.ttf", &textureAtlas).value();

    /* --- ImGui fonts */
    ImFont* fontImGuiSuperBakery = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/superbakery.ttf", 26.f);

    /* --- Music */
    auto musicBGM = sf::Music::openFromFile("resources/hibiscus.mp3").value();

    /* --- Sounds */
    Sounds       sounds;
    sf::Listener listener;

    /* --- Textures */
    const auto txLogo       = sf::Texture::loadFromFile("resources/logo.png", {.smooth = true}).value();
    const auto txBackground = sf::Texture::loadFromFile("resources/background.jpg", {.smooth = true}).value();
    const auto txByteTip    = sf::Texture::loadFromFile("resources/bytetip.png", {.smooth = true}).value();

    /* --- Texture atlas rects */
    const auto txrWhiteDot     = textureAtlas.add(graphicsContext.getBuiltInWhiteDotTexture()).value();
    const auto txrBubble       = addImgResourceToAtlas("bubble2.png");
    const auto txrBubbleStar   = addImgResourceToAtlas("bubble3.png");
    const auto txrCat          = addImgResourceToAtlas("cat.png");
    const auto txrSmartCat     = addImgResourceToAtlas("smartcat.png");
    const auto txrUniCat       = addImgResourceToAtlas("unicat.png");
    const auto txrDevilCat     = addImgResourceToAtlas("devilcat.png");
    const auto txrCatPaw       = addImgResourceToAtlas("catpaw.png");
    const auto txrUniCatPaw    = addImgResourceToAtlas("unicatpaw.png");
    const auto txrDevilCatPaw  = addImgResourceToAtlas("devilcatpaw.png");
    const auto txrParticle     = addImgResourceToAtlas("particle.png");
    const auto txrStarParticle = addImgResourceToAtlas("starparticle.png");
    const auto txrFireParticle = addImgResourceToAtlas("fireparticle.png");
    const auto txrHexParticle  = addImgResourceToAtlas("hexparticle.png");
    const auto txrWitchCat     = addImgResourceToAtlas("witchcat.png");
    const auto txrWitchCatPaw  = addImgResourceToAtlas("witchcatpaw.png");
    const auto txrAstroCat     = addImgResourceToAtlas("astromeow.png");
    const auto txrBomb         = addImgResourceToAtlas("bomb.png");

    //
    //
    // Game
    GameResources gameResources{playbackDevice,
                                graphicsContext,
                                window,
                                imGuiContext,
                                textureAtlas,
                                fontSuperBakery,
                                *fontImGuiSuperBakery,
                                musicBGM};

    Game game;


    //
    //
    // UI Text
    sf::Text moneyText{fontSuperBakery,
                       {.position         = {15.f, 70.f},
                        .string           = "$0",
                        .characterSize    = 32u,
                        .fillColor        = sf::Color::White,
                        .outlineColor     = colorBlueOutline,
                        .outlineThickness = 2.f}};

    sf::Text comboText{fontSuperBakery,
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
    SpatialGrid spatialGrid;

    //
    //
    // Particles
    std::vector<Particle>     particles;
    std::vector<TextParticle> textParticles;

    const auto spawnParticles = [&](const SizeT n, auto&&... args)
    {
        for (SizeT i = 0; i < n; ++i)
            particles.emplace_back(makeParticle(SFML_BASE_FORWARD(args)...));
    };

    //
    //
    // Purchasables (persistent)
    const auto costFunction = [](const float baseCost, const float nOwned, const float growthFactor)
    { return baseCost * std::pow(growthFactor, nOwned); };


    int prestigeTransition = 0;

    //
    //
    // Scaling values (persistent)
    const MoneyType rewardPerType[3]{
        1u,  // Normal
        25u, // Star
        1u,  // Bomb
    };

    const auto getScaledReward = [&](const BubbleType type)
    { return rewardPerType[static_cast<SizeT>(type)] * static_cast<MoneyType>(game.psvBubbleValue.currentValue() + 1); };

    //
    //
    // Cat names
    // TODO: use seed for persistance
    const auto shuffledCatNames  = getShuffledCatNames(getRandomEngine());
    auto       getNextCatNameIdx = [&, nextCatName = 0u]() mutable { return nextCatName++ % shuffledCatNames.size(); };

    //
    //
    // Persistent game state
    game.bubbles.reserve(20000);
    game.cats.reserve(512);

    const auto forEachBubbleInRadius = [&](const sf::Vector2f center, const float radius, auto&& func)
    {
        const float radiusSq = radius * radius;

        spatialGrid.forEachIndexInRadius(center,
                                         radius,
                                         [&](const SizeT index)
                                         {
                                             auto& bubble = game.bubbles[index];

                                             if ((bubble.position - center).lengthSquared() > radiusSq)
                                                 return ControlFlow::Continue;

                                             return func(bubble);
                                         });
    };

    constexpr float bubbleSpawnDelay = 3.f;
    float           bubbleSpawnTimer = 0.f;

    //
    //
    // Transient game state
    int   combo      = 0;
    float comboTimer = 0.f;

    const auto comboValueMult = [&](const int n)
    {
        constexpr float initial = 1.f;
        constexpr float decay   = 0.95f;

        return initial * (1.f - std::pow(decay, static_cast<float>(n))) / (1.f - decay);
    };

    //
    //
    // Clocks
    sf::Clock     playedClock;
    sf::base::I64 playedUsAccumulator = 0;

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

    //
    //
    // Cat dragging
    Cat*  draggedCat           = nullptr;
    float catDragPressDuration = 0.f;

    //
    //
    // Touch state (TODO)
    std::vector<sf::base::Optional<sf::Vector2f>> fingerPositions;
    fingerPositions.resize(10);

    //
    //
    // Startup
    constexpr float splashTimerMax = 1750.f;
    float           splashTimer    = splashTimerMax;
    sounds.byteMeow.play(playbackDevice);

    //
    //
    // Tips
    float       tipTimer = 0.f;
    std::string tipString;

    //
    //
    // Background music
    musicBGM.setLooping(true);
    musicBGM.setAttenuation(0.f);
    musicBGM.play(playbackDevice);

    //
    //
    // Money helper functions
    const auto addReward = [&](const MoneyType reward)
    {
        game.money += reward;
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
                if (game.mapPurchased && dragPosition.hasValue())
                {
                    scroll = dragPosition->x - static_cast<float>(e->position.x);
                }
            }
#pragma clang diagnostic pop
        }

        const auto deltaTime   = deltaClock.restart();
        const auto deltaTimeMs = static_cast<float>(deltaTime.asMilliseconds());

        //
        // Cheats (TODO)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F4))
        {
            game.comboPurchased = true;
            game.mapPurchased   = true;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5))
        {
            game.money = 1'000'000'000u;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F6))
        {
            game.money += 15u;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F7))
        {
            game.prestigePoints += 15u;
        }

        //
        // Number of fingers
        const auto countFingersDown = std::count_if(fingerPositions.begin(),
                                                    fingerPositions.end(),
                                                    [](const auto& fingerPos) { return fingerPos.hasValue(); });

        //
        // Map scrolling via keyboard and touch
        if (game.mapPurchased)
        {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            {
                dragPosition.reset();
                scroll -= 2.f * deltaTimeMs;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            {
                dragPosition.reset();
                scroll += 2.f * deltaTimeMs;
            }
            else if (countFingersDown == 2)
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
        }

        //
        // Reset map scrolling
        if (dragPosition.hasValue() && countFingersDown != 2 && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            dragPosition.reset();
        }

        //
        // Scrolling
        scroll = sf::base::clamp(scroll,
                                 0.f,
                                 sf::base::min(game.getMapLimit() / 2.f - resolution.x / 2.f,
                                               (boundaries.x - resolution.x) / 2.f));

        actualScroll = exponentialApproach(actualScroll, scroll, deltaTimeMs, 75.f);

        const sf::View gameView //
            {.center = {sf::base::clamp(resolution.x / 2.f + actualScroll * 2.f,
                                        resolution.x / 2.f,
                                        boundaries.x - resolution.x / 2.f),
                        resolution.y / 2.f},
             .size   = resolution};

        const auto mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), gameView);

        //
        // Update listener position
        listener.position = {sf::base::clamp(mousePos.x, 0.f, game.getMapLimit()),
                             sf::base::clamp(mousePos.y, 0.f, boundaries.y),
                             0.f};
        musicBGM.setPosition(listener.position);
        (void)playbackDevice.updateListener(listener);

        //
        // Target bubble count
        const auto targetBubbleCountPerScreen = static_cast<SizeT>(
            game.psvBubbleCount.currentValue() / (boundaries.x / resolution.x));
        const auto nScreens          = static_cast<SizeT>(game.getMapLimit() / resolution.x) + 1;
        const auto targetBubbleCount = targetBubbleCountPerScreen * nScreens;

        //
        // Startup and bubble spawning
        const auto playReversePopAt = [&](const sf::Vector2f position)
        {
            if (sounds.reversePop.getStatus() == sf::Sound::Status::Playing)
                return;

            sounds.reversePop.setPosition({position.x, position.y});
            sounds.reversePop.play(playbackDevice);
        };

        if (splashTimer > 0.f)
            splashTimer -= deltaTimeMs;

        if (splashTimer <= 0.f)
        {
            if (bubbleSpawnTimer > 0.f)
                bubbleSpawnTimer -= deltaTimeMs;
            else
            {
                bubbleSpawnTimer = bubbleSpawnDelay;

                if (prestigeTransition == 1)
                {
                    if (!game.bubbles.empty())
                    {
                        const SizeT times = game.bubbles.size() > 1000 ? 25 : 1;

                        for (SizeT i = 0; i < times; ++i)
                        {
                            const auto& b = game.bubbles.back();
                            spawnParticles(8, b.position, ParticleType::Bubble, 0.5f, 0.5f);
                            game.bubbles.pop_back();
                            playReversePopAt(b.position);
                        }
                    }

                    if (!game.cats.empty()) // TODO add a longer delay here
                    {
                        const auto& c = game.cats.back();

                        spawnParticles(24, c.position, ParticleType::Star, 0.5f, 0.5f);
                        game.cats.pop_back();
                        playReversePopAt(c.position);
                    }

                    if (game.cats.empty() && game.bubbles.empty())
                    {
                        prestigeTransition = 0;

                        splashTimer = splashTimerMax;
                        sounds.byteMeow.play(playbackDevice);
                    }
                }
                else if (game.bubbles.size() < targetBubbleCount)
                {
                    const SizeT times = (targetBubbleCount - game.bubbles.size()) > 1000 ? 25 : 1;

                    for (SizeT i = 0; i < times; ++i)
                    {
                        const auto& b = game.bubbles.emplace_back(makeRandomBubble(game.getMapLimit(), boundaries.y));

                        spawnParticles(8, b.position, ParticleType::Bubble, 0.5f, 0.5f);
                        playReversePopAt(b.position);
                    }
                }
            }
        }

        //
        //
        // Update spatial partitioning
        spatialGrid.clear();
        spatialGrid.populate(game.bubbles);

        const auto popBubble =
            [&](auto               self,
                const bool         byHand,
                const BubbleType   bubbleType,
                const MoneyType    reward,
                const int          combo,
                const sf::Vector2f position) -> void
        {
            ++game.statsTotal.bubblesPopped;
            ++game.statsSession.bubblesPopped;
            game.statsTotal.bubblesPoppedRevenue += reward;
            game.statsSession.bubblesPoppedRevenue += reward;

            if (byHand)
            {
                ++game.statsTotal.bubblesHandPopped;
                ++game.statsSession.bubblesHandPopped;
                game.statsTotal.bubblesHandPoppedRevenue += reward;
                game.statsSession.bubblesHandPoppedRevenue += reward;
            }

            auto& tp = textParticles.emplace_back(makeTextParticle(position, combo));
            std::snprintf(tp.buffer, sizeof(tp.buffer), "+$%zu", reward);

            sounds.pop.setPosition({position.x, position.y});
            sounds.pop.setPitch(remap(static_cast<float>(combo), 1, 10, 1.f, 2.f));
            sounds.pop.play(playbackDevice);

            spawnParticles(32, position, ParticleType::Bubble, 0.5f, 0.5f);
            spawnParticles(8, position, ParticleType::Bubble, 1.2f, 0.25f);

            if (bubbleType == BubbleType::Star)
            {
                spawnParticles(16, position, ParticleType::Star, 0.25f, 0.35f);
            }
            else if (bubbleType == BubbleType::Bomb)
            {
                sounds.explosion.setPosition({position.x, position.y});
                sounds.explosion.play(playbackDevice);

                spawnParticles(32, position, ParticleType::Fire, 3.f, 1.f);

                forEachBubbleInRadius(position,
                                      game.getComputedBombExplosionRadius(),
                                      [&](Bubble& bubble)
                                      {
                                          if (bubble.type == BubbleType::Bomb)
                                              return ControlFlow::Continue;

                                          const SizeT newReward = getScaledReward(bubble.type) * 10u;

                                          game.statsTotal.explosionRevenue += newReward;
                                          game.statsSession.explosionRevenue += newReward;

                                          self(self, byHand, bubble.type, newReward, 1, bubble.position);
                                          addReward(newReward);
                                          bubble = makeRandomBubble(game.getMapLimit(), 0.f);
                                          bubble.position.y -= bubble.radius;

                                          return ControlFlow::Continue;
                                      });
            }
        };

        for (auto& bubble : game.bubbles)
        {
            if (bubble.type == BubbleType::Bomb)
                bubble.rotation += deltaTimeMs * 0.01f;

            auto& pos = bubble.position;

            if (game.windPurchased && game.windEnabled)
            {
                bubble.velocity.x += 0.00055f * deltaTimeMs;
                bubble.velocity.y += 0.00055f * deltaTimeMs;
            }

            pos += bubble.velocity * deltaTimeMs;

            if (pos.x - bubble.radius > game.getMapLimit())
                pos.x = -bubble.radius;
            else if (pos.x + bubble.radius < 0.f)
                pos.x = game.getMapLimit() + bubble.radius;

            if (pos.y - bubble.radius > boundaries.y)
            {
                pos.y             = -bubble.radius;
                bubble.velocity.y = game.windEnabled ? 0.2f : 0.f;

                if (sf::base::fabs(bubble.velocity.x) > 0.04f)
                    bubble.velocity.x = 0.04f;

                bubble.type = BubbleType::Normal;
            }
            else if (pos.y + bubble.radius < 0.f)
                pos.y = boundaries.y + bubble.radius;

            bubble.velocity.y += 0.00005f * deltaTimeMs;
        }

        const auto popWithRewardAndReplaceBubble = [&](const bool byHand, Bubble& bubble, int combo)
        {
            const auto reward = static_cast<MoneyType>(
                sf::base::ceil(static_cast<float>(getScaledReward(bubble.type)) * comboValueMult(combo)));

            popBubble(popBubble, byHand, bubble.type, reward, combo, bubble.position);
            addReward(reward);
            bubble = makeRandomBubble(game.getMapLimit(), 0.f);
            bubble.position.y -= bubble.radius;
        };

        bool anyBubblePoppedByClicking = false;

        if (clickPosition.hasValue())
        {
            const auto clickPos = window.mapPixelToCoords(clickPosition->toVector2i(), gameView);

            const auto clickAction = [&](Bubble& bubble)
            {
                if ((clickPos - bubble.position).length() > bubble.radius)
                    return ControlFlow::Continue;

                anyBubblePoppedByClicking = true;

                if (game.comboPurchased)
                {
                    if (combo == 0)
                    {
                        combo      = 1;
                        comboTimer = game.psvComboStartTime.currentValue() * 1000.f;
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

                popWithRewardAndReplaceBubble(/* byHand */ true, bubble, combo);

                if (game.multiPopEnabled && game.psvMultiPopRange.nPurchases > 0)
                    forEachBubbleInRadius(clickPos,
                                          game.getComputedMultiPopRange(),
                                          [&](Bubble& otherBubble)
                                          {
                                              if (&otherBubble != &bubble)
                                                  popWithRewardAndReplaceBubble(/* byHand */ true, otherBubble, combo);

                                              return ControlFlow::Continue;
                                          });


                return ControlFlow::Break;
            };

            forEachBubbleInRadius(clickPos, 128.f, clickAction);
        }

        //
        // Combo failure due to missed click
        if (!anyBubblePoppedByClicking && clickPosition.hasValue())
        {
            if (combo > 1)
                sounds.scratch.play(playbackDevice);

            combo      = 0;
            comboTimer = 0.f;
        }

        //
        // Bubble vs bubble collisions
        spatialGrid.forEachUniqueIndexPair(
            [&](const SizeT bubbleIdxI, const SizeT bubbleIdxJ)
            { handleBubbleCollision(deltaTimeMs, game.bubbles[bubbleIdxI], game.bubbles[bubbleIdxJ]); });

        //
        // Cat vs cat collisions
        for (SizeT i = 0u; i < game.cats.size(); ++i)
            for (SizeT j = i + 1; j < game.cats.size(); ++j)
            {
                Cat& iCat = game.cats[i];
                Cat& jCat = game.cats[j];

                if (draggedCat == &iCat || draggedCat == &jCat)
                    continue;

                if (iCat.isAstroAndInFlight() && jCat.type != CatType::Astro)
                {
                    if (detectCollision(iCat.position, jCat.position, iCat.getRadius(), jCat.getRadius()))
                        jCat.inspiredTimer = 2000.f; // TODO: repetition, upgrade to scale

                    continue;
                }

                if (jCat.isAstroAndInFlight() && iCat.type != CatType::Astro)
                {
                    // TODO: inspire cat
                    if (detectCollision(iCat.position, jCat.position, iCat.getRadius(), jCat.getRadius()))
                        iCat.inspiredTimer = 2000.f;

                    continue;
                }

                handleCatCollision(deltaTimeMs, game.cats[i], game.cats[j]);
            }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) // TODO: touch
        {
            if (draggedCat)
            {
                draggedCat->position = exponentialApproach(draggedCat->position, mousePos, deltaTimeMs, 50.f);
                draggedCat->cooldown = -250.f;
            }
            else
            {
                Cat* hoveredCat = nullptr;

                // Only check for hover targets during initial press phase
                if (catDragPressDuration <= 150.f)
                    for (Cat& cat : game.cats)
                        if ((mousePos - cat.position).length() <= cat.getRadius())
                            hoveredCat = &cat;

                if (hoveredCat)
                {
                    catDragPressDuration += deltaTimeMs;

                    if (catDragPressDuration >= 150.f)
                    {
                        draggedCat = hoveredCat;
                        sounds.grab.play(playbackDevice);
                    }
                }
            }
        }
        else
        {
            if (draggedCat)
            {
                sounds.drop.play(playbackDevice);
                draggedCat = nullptr;
            }

            catDragPressDuration = 0.f;
        }

        for (auto& cat : game.cats)
        {
            // Keep cat in boundaries
            const float catRadius = cat.getRadius();

            if (!cat.astroState.hasValue())
            {
                cat.position.x = sf::base::clamp(cat.position.x, catRadius, game.getMapLimit() - catRadius);
                cat.position.y = sf::base::clamp(cat.position.y, catRadius, boundaries.y - catRadius);
            }

            const auto maxCooldown = game.getComputedCooldownByCatType(cat.type);
            const auto range       = game.getComputedRangeByCatType(cat.type);

            auto diff = cat.pawPosition - cat.drawPosition - sf::Vector2f{-25.f, 25.f};
            cat.pawPosition -= diff * 0.01f * deltaTimeMs;
            cat.pawRotation = cat.pawRotation.rotatedTowards(sf::degrees(-45.f), deltaTimeMs * 0.005f);

            if (cat.cooldown < 0.f)
            {
                cat.pawOpacity  = 128.f;
                cat.mainOpacity = 128.f;
            }
            else
            {
                cat.mainOpacity = 255.f;
            }

            if (cat.cooldown == maxCooldown && cat.pawOpacity > 10.f)
                cat.pawOpacity -= 0.5f * deltaTimeMs;

            cat.update(deltaTimeMs);

            const auto [cx, cy] = cat.position + cat.rangeOffset;

            if (cat.inspiredTimer > 0.f)
            {
                if (getRndFloat(0.f, 1.f) > 0.5f)
                    particles.push_back(
                        {.data = {.position = cat.drawPosition + sf::Vector2f{getRndFloat(-catRadius, +catRadius), catRadius},
                                  .velocity      = getRndVector2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                                  .scale         = getRndFloat(0.08f, 0.27f) * 0.2f,
                                  .accelerationY = -0.002f,
                                  .opacity       = 255.f,
                                  .opacityDecay  = getRndFloat(0.00025f, 0.0015f),
                                  .rotation      = getRndFloat(0.f, sf::base::tau),
                                  .torque        = getRndFloat(-0.002f, 0.002f)},
                         .type = ParticleType::Star});
            }

            if (cat.type == CatType::Astro && cat.astroState.hasValue())
            {
                auto& [startX, velocityX, wrapped, particleTimer] = *cat.astroState;

                particleTimer += deltaTimeMs;

                if (particleTimer >= 3.f && !cat.isCloseToStartX())
                {
                    if (sounds.rocket.getStatus() != sf::Sound::Status::Playing)
                    {
                        sounds.rocket.setPosition({cx, cy});
                        sounds.rocket.play(playbackDevice);
                    }

                    spawnParticles(1, cat.drawPosition + sf::Vector2f{56.f, 45.f}, ParticleType::Fire, 1.5f, 0.25f, 0.65f);
                    particleTimer = 0.f;
                }

                const auto astroPopAction = [&](Bubble& bubble)
                {
                    const SizeT newReward = getScaledReward(bubble.type) * 20u;

                    popBubble(popBubble,
                              /* byHand */ false,
                              bubble.type,
                              newReward,
                              /* combo */ 1,
                              bubble.position);
                    addReward(newReward);
                    bubble = makeRandomBubble(game.getMapLimit(), 0.f);
                    bubble.position.y -= bubble.radius;

                    cat.textStatusShakeEffect.bump(1.5f);

                    return ControlFlow::Continue;
                };

                forEachBubbleInRadius({cx, cy}, range, astroPopAction);

                if (!cat.isCloseToStartX() && velocityX > -5.f)
                    velocityX -= 0.00025f * deltaTimeMs;

                if (!cat.isCloseToStartX())
                    cat.position.x += velocityX * deltaTimeMs;
                else
                    cat.position.x = exponentialApproach(cat.position.x, startX - 10.f, deltaTimeMs, 500.f);

                if (!wrapped && cat.position.x + catRadius < 0.f)
                {
                    cat.position.x = game.getMapLimit() + catRadius;
                    wrapped        = true;
                }

                if (wrapped && cat.position.x <= startX)
                {
                    cat.astroState.reset();
                    cat.position.x = startX;
                    cat.cooldown   = 0.f;
                }

                continue;
            }

            if (!cat.updateCooldown(maxCooldown, deltaTimeMs))
                continue;

            if (cat.type == CatType::Astro)
            {
                if (!cat.astroState.hasValue())
                {
                    sounds.launch.play(playbackDevice);

                    ++cat.hits;
                    cat.astroState.emplace(/* startX */ cat.position.x, /* velocityX */ 0.f, /* wrapped */ false);
                    --cat.position.x;
                }
            }

            if (cat.type == CatType::Witch) // TODO: change
            {
                int  witchHits = 0;
                bool pawSet    = false;

                for (auto& otherCat : game.cats)
                {
                    if (otherCat.type == CatType::Witch)
                        continue;

                    if ((otherCat.position - cat.position).length() > range)
                        continue;

                    otherCat.cooldown = game.getComputedCooldownByCatType(cat.type);
                    ++witchHits;

                    if (!pawSet && getRndFloat(0.f, 100.f) > 50.f)
                    {
                        pawSet = true;

                        cat.pawPosition = otherCat.position;
                        cat.pawOpacity  = 255.f;
                        cat.pawRotation = (otherCat.position - cat.position).angle() + sf::degrees(45);
                    }

                    spawnParticles(8, otherCat.position, ParticleType::Hex, 0.5f, 0.35f);
                }

                if (witchHits > 0)
                {
                    sounds.hex.setPosition({cx, cy});
                    sounds.hex.play(playbackDevice);

                    cat.textStatusShakeEffect.bump(1.5f);
                    cat.hits += witchHits;
                }

                cat.cooldown = 0.f;
                continue;
            }

            const auto action = [&](Bubble& bubble)
            {
                if (cat.type == CatType::Uni && bubble.type != BubbleType::Normal)
                    return ControlFlow::Continue;

                cat.pawPosition = bubble.position;
                cat.pawOpacity  = 255.f;
                cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

                if (cat.type == CatType::Uni)
                {
                    bubble.type       = BubbleType::Star;
                    bubble.velocity.y = getRndFloat(-0.1f, -0.05f);
                    sounds.shine.setPosition({bubble.position.x, bubble.position.y});
                    sounds.shine.play(playbackDevice);

                    spawnParticles(4, bubble.position, ParticleType::Star, 0.25f, 0.35f);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;
                }
                else if (cat.type == CatType::Normal)
                {
                    popWithRewardAndReplaceBubble(/* byHand */ false, bubble, /* combo */ 1);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;
                }
                else if (cat.type == CatType::Devil)
                {
                    bubble.type = BubbleType::Bomb;
                    bubble.velocity.y += getRndFloat(0.1f, 0.2f);
                    sounds.makeBomb.setPosition({bubble.position.x, bubble.position.y});
                    sounds.makeBomb.play(playbackDevice);

                    spawnParticles(8, bubble.position, ParticleType::Fire, 1.25f, 0.35f);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;
                }

                cat.cooldown = 0.f;
                return ControlFlow::Break;
            };

            if (cat.type == CatType::Normal && game.smarterCatsPurchased)
            {
                bool smarterActionSuccess = false;

                const auto smarterAction = [&](Bubble& bubble)
                {
                    if (bubble.type == BubbleType::Normal)
                        return ControlFlow::Continue;

                    cat.pawPosition = bubble.position;
                    cat.pawOpacity  = 255.f;
                    cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

                    smarterActionSuccess = true;

                    popWithRewardAndReplaceBubble(/* byHand */ false, bubble, /* combo */ 1);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;

                    cat.cooldown = 0.f;
                    return ControlFlow::Break;
                };

                forEachBubbleInRadius({cx, cy}, range, smarterAction);

                if (!smarterActionSuccess)
                    forEachBubbleInRadius({cx, cy}, range, action);
            }
            else
            {
                forEachBubbleInRadius({cx, cy}, range, action);
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
                combo      = 0;
                comboTimer = 0.f;
            }
        }

        const auto countCatsByType = [&](CatType type)
        {
            return static_cast<int>(
                std::count_if(game.cats.begin(), game.cats.end(), [type](const auto& cat) { return cat.type == type; }));
        };

        const auto nCatNormal = countCatsByType(CatType::Normal);
        const auto nCatUni    = countCatsByType(CatType::Uni);
        const auto nCatDevil  = countCatsByType(CatType::Devil);
        const auto nCatWitch  = countCatsByType(CatType::Witch);
        const auto nCatAstro  = countCatsByType(CatType::Astro);

        const auto globalCostMultiplier = [&]
        {
            // [ 0.25, 0.25 + 0.125, 0.25 + 0.125 + 0.0625, ... ]
            const auto geomSum = [](auto n)
            { return static_cast<float>(n) <= 0.f ? 0.f : 0.5f * (1.f - std::pow(0.5f, static_cast<float>(n) + 1.f)); };

            return 1.f +                                                 //
                   (geomSum(game.psvComboStartTime.nPurchases) * 0.1f) + //
                   (geomSum(game.psvBubbleCount.nPurchases) * 0.5f) +    //
                   (geomSum(game.psvBubbleValue.nPurchases) * 0.75f) +   //
                   (geomSum(nCatNormal) * 0.35f) +                       //
                   (geomSum(nCatUni) * 0.5f) +                           //
                   (geomSum(nCatDevil) * 0.75f) +                        //
                   (geomSum(nCatWitch) * 0.75f) +                        //
                   (geomSum(nCatAstro) * 0.75f);
        };

        const bool bubbleValueUnlocked = game.psvBubbleValue.nPurchases > 0 ||
                                         (game.psvBubbleCount.nPurchases > 0 && nCatUni >= 3);


        char buffer[256];
        char labelBuffer[512];

        const auto makeButtonLabel = [&](const char* label)
        {
            ImGui::Text("%s", label);
            ImGui::SameLine();
        };

        const auto makeButtonTopLabel = [&](const char* labelBuffer)
        {
            ImGui::SetWindowFontScale(0.5f);
            ImGui::Text("%s", labelBuffer);
            ImGui::SetWindowFontScale(1.f);
            ImGui::SameLine();
        };

        float buttonHueMod = 0.f;

        const auto pushButtonColors = [&]
        {
            const auto convertColor = [&](const auto colorId)
            {
                return sf::Color::fromVec4(ImGui::GetStyleColorVec4(colorId)).withHueMod(buttonHueMod).template toVec4<ImVec4>();
            };

            ImGui::PushStyleColor(ImGuiCol_Button, convertColor(ImGuiCol_Button));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, convertColor(ImGuiCol_ButtonHovered));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, convertColor(ImGuiCol_ButtonActive));
        };

        const auto popButtonColors = [&] { ImGui::PopStyleColor(3); };

        const auto makeButtonImpl = [&](const char* buffer)
        {
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 151.f);

            pushButtonColors();

            if (ImGui::Button(buffer, ImVec2(135.f, 0.f)))
            {
                sounds.buy.play(playbackDevice);

                popButtonColors();
                return true;
            }

            popButtonColors();
            return false;
        };

        const auto makeDoneButton = [&](const char* label)
        {
            ImGui::BeginDisabled(true);

            makeButtonLabel(label);
            makeButtonTopLabel(labelBuffer);

            makeButtonImpl("DONE");

            ImGui::EndDisabled();
        };

        const auto makePurchasableButton = [&](const char* label, float baseCost, float growthFactor, float count)
        {
            bool result = false;

            const auto cost = static_cast<MoneyType>(globalCostMultiplier() * costFunction(baseCost, count, growthFactor));
            std::sprintf(buffer, "$%zu##%s", cost, label);

            ImGui::BeginDisabled(game.money < cost);

            makeButtonLabel(label);
            makeButtonTopLabel(labelBuffer);

            if (makeButtonImpl(buffer))
            {
                result = true;
                game.money -= cost;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto makePurchasableButtonPSV = [&](const char* label, PurchasableScalingValue& psv)
        {
            const bool maxedOut = psv.nPurchases == psv.data->nMaxPurchases;

            bool result = false;

            const auto cost = static_cast<MoneyType>(globalCostMultiplier() * psv.nextCost());

            if (maxedOut)
                std::sprintf(buffer, "MAX");
            else
                std::sprintf(buffer, "$%zu##%s", cost, label);

            ImGui::BeginDisabled(maxedOut || game.money < cost);

            makeButtonLabel(label);
            makeButtonTopLabel(labelBuffer);

            if (makeButtonImpl(buffer))
            {
                result = true;
                game.money -= cost;

                ++psv.nPurchases;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto makePrestigePurchasableButtonPSV =
            [&](const char* label, PurchasableScalingValue& psv, const SizeT times, const MoneyType cost)
        {
            const bool maxedOut = psv.nPurchases == psv.data->nMaxPurchases;

            bool result = false;

            if (maxedOut)
                std::sprintf(buffer, "MAX");
            else if (cost == 0u)
                std::sprintf(buffer, "N/A");
            else
                std::sprintf(buffer, "$%zu##%s", cost, label);

            ImGui::BeginDisabled(maxedOut || game.money < cost || cost == 0u);

            makeButtonLabel(label);
            makeButtonTopLabel(labelBuffer);

            makeButtonImpl(buffer);

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                result = true;
                game.money -= cost;

                psv.nPurchases += times;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto makePurchasableButtonOneTime = [&](const char* label, const MoneyType xcost, bool& done)
        {
            bool result = false;

            const auto cost = static_cast<MoneyType>(globalCostMultiplier() * static_cast<float>(xcost));

            if (done)
                std::sprintf(buffer, "DONE");
            else
                std::sprintf(buffer, "$%zu##%s", cost, label);

            ImGui::BeginDisabled(done || game.money < cost);

            makeButtonLabel(label);
            makeButtonTopLabel(labelBuffer);

            if (makeButtonImpl(buffer))
            {
                result = true;
                game.money -= cost;

                done = true;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto makePurchasablePPButtonOneTime = [&](const char* label, const U64 prestigePointsCost, bool& done)
        {
            bool result = false;

            if (done)
                std::sprintf(buffer, "DONE");
            else
                std::sprintf(buffer, "%zu PPs##%s", prestigePointsCost, label);

            ImGui::BeginDisabled(done || game.prestigePoints < prestigePointsCost);

            makeButtonLabel(label);
            makeButtonTopLabel(labelBuffer);

            if (makeButtonImpl(buffer))
            {
                result = true;
                game.prestigePoints -= prestigePointsCost;

                done = true;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto makePrestigePurchasablePPButtonPSV = [&](const char* label, PurchasableScalingValue& psv)
        {
            const bool maxedOut           = psv.nPurchases == psv.data->nMaxPurchases;
            const U64  prestigePointsCost = static_cast<U64>(psv.nextCost());

            bool result = false;

            if (maxedOut)
                std::sprintf(buffer, "MAX");
            else
                std::sprintf(buffer, "%zu PPs##%s", prestigePointsCost, label);

            ImGui::BeginDisabled(maxedOut || game.prestigePoints < prestigePointsCost);

            makeButtonLabel(label);
            makeButtonTopLabel(labelBuffer);

            if (makeButtonImpl(buffer))
            {
                result = true;
                game.prestigePoints -= prestigePointsCost;

                ++psv.nPurchases;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto doTip = [&](const char* str, const SizeT maxPrestigeLevel = 0)
        {
            if (!game.tipsEnabled || game.psvBubbleValue.nPurchases > maxPrestigeLevel)
                return;

            sounds.byteMeow.play(playbackDevice);
            tipString = str;
            tipTimer  = 4500.f;
        };

        imGuiContext.update(window, deltaTime);

        ImGui::SetNextWindowPos({resolution.x - 15.f, 15.f}, 0, {1.f, 0.f});
        ImGui::SetNextWindowSizeConstraints(ImVec2(420.f, 0.f), ImVec2(1000.f, 600.f));
        ImGui::PushFont(fontImGuiSuperBakery);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f); // Set corner radius

        ImGuiStyle& style               = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.f, 0.f, 0.f, 0.65f); // 65% transparent black

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


                std::sprintf(labelBuffer, "");
                if (makePurchasableButtonOneTime("Combo", 15, game.comboPurchased))
                {
                    combo = 0;
                    doTip("Pop bubbles in quick successions to\nkeep your combo up and make money!");
                }

                if (game.comboPurchased)
                {
                    std::sprintf(labelBuffer, "%.2fs", static_cast<double>(game.psvComboStartTime.currentValue()));
                    makePurchasableButtonPSV("- Longer combo", game.psvComboStartTime);
                }

                if (nCatNormal > 0 && game.psvComboStartTime.nPurchases > 0)
                {
                    std::sprintf(labelBuffer, "");
                    if (makePurchasableButtonOneTime("Map scrolling", 250, game.mapPurchased))
                    {
                        ++game.mapLimitIncreases;
                        scroll = 0.f;

                        doTip("You can scroll the map with right click\nor by dragging with two fingers!");
                    }

                    if (game.mapPurchased)
                    {
                        if (game.getMapLimit() < boundaries.x - resolution.x)
                        {
                            std::sprintf(labelBuffer,
                                         "%.2f%%",
                                         static_cast<double>(remap(game.getMapLimit(), 0.f, boundaries.x, 0.f, 100.f) + 10.f));
                            if (makePurchasableButton("- Extend map", 100.f, 4.85f, game.getMapLimit() / resolution.x))
                            {
                                ++game.mapLimitIncreases;
                                game.mapPurchased = true;
                            }
                        }
                        else
                        {
                            std::sprintf(labelBuffer, "100%%");
                            makeDoneButton("- Extend map");
                        }
                    }

                    std::sprintf(labelBuffer, "%zu bubbles", static_cast<SizeT>(game.psvBubbleCount.currentValue()));
                    makePurchasableButtonPSV("More bubbles", game.psvBubbleCount);
                }

                const auto spawnCat = [&](const CatType catType, const sf::Vector2f rangeOffset) -> Cat&
                {
                    const auto pos = window.mapPixelToCoords((resolution / 2.f).toVector2i(), gameView);
                    spawnParticles(32, pos, ParticleType::Star, 0.25f, 0.75f);
                    return game.cats.emplace_back(makeCat(catType, pos, rangeOffset, getNextCatNameIdx()));
                };

                if (game.comboPurchased && game.psvComboStartTime.nPurchases > 0)
                {
                    std::sprintf(labelBuffer, "%d cats", nCatNormal);
                    if (makePurchasableButton("Cat", 35, 1.7f, static_cast<float>(nCatNormal)))
                    {
                        spawnCat(CatType::Normal, {0.f, 0.f});

                        if (nCatNormal == 0)
                        {
                            doTip("Cats periodically pop bubbles for you!\nYou can drag them around to position them.");
                        }
                    }
                }

                const auto makeCooldownButton = [&](const char* label, const CatType catType)
                {
                    auto& psv = game.getCooldownMultPSVByCatType(catType);

                    std::sprintf(labelBuffer,
                                 "%.2fs",
                                 static_cast<double>(game.getComputedCooldownByCatType(catType) / 1000.f));
                    makePurchasableButtonPSV(label, psv);
                };

                const auto makeRangeButton = [&](const char* label, const CatType catType)
                {
                    auto& psv = game.getRangeDivPSVByCatType(catType);

                    std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(game.getComputedRangeByCatType(catType)));
                    makePurchasableButtonPSV(label, psv);
                };

                const bool catUpgradesUnlocked = game.psvBubbleCount.nPurchases > 0 && nCatNormal >= 2 && nCatUni >= 1;
                if (catUpgradesUnlocked)
                {
                    makeCooldownButton("- Cat cooldown", CatType::Normal);
                    makeRangeButton("- Cat range", CatType::Normal);
                }

                // UNICORN CAT
                const bool catUnicornUnlocked         = game.psvBubbleCount.nPurchases > 0 && nCatNormal >= 3;
                const bool catUnicornUpgradesUnlocked = catUnicornUnlocked && nCatUni >= 2 && nCatDevil >= 1;
                if (catUnicornUnlocked)
                {
                    std::sprintf(labelBuffer, "%d unicats", nCatUni);
                    if (makePurchasableButton("Unicat", 250, 1.75f, static_cast<float>(nCatUni)))
                    {
                        spawnCat(CatType::Uni, {0.f, -100.f});

                        if (nCatUni == 0)
                        {
                            doTip(
                                "Unicats transform bubbles in star bubbles,\nwhich are worth much more!\nPop them at "
                                "the end of a combo for huge gains!");
                        }
                    }

                    if (catUnicornUpgradesUnlocked)
                    {
                        makeCooldownButton("- Unicat cooldown", CatType::Uni);
                        makeRangeButton("- Unicat range", CatType::Uni);
                    }
                }

                // DEVIL CAT
                const bool catDevilUnlocked = game.psvBubbleValue.nPurchases > 0 && nCatNormal >= 6 && nCatUni >= 4;
                const bool catDevilUpgradesUnlocked = catDevilUnlocked && nCatDevil >= 2 && nCatAstro >= 1;
                if (catDevilUnlocked)
                {
                    std::sprintf(labelBuffer, "%d devilcats", nCatDevil);
                    if (makePurchasableButton("Devilcat", 15000.f, 1.6f, static_cast<float>(nCatDevil)))
                    {
                        spawnCat(CatType::Devil, {0.f, 100.f});

                        if (nCatDevil == 0)
                        {
                            doTip(
                                "Devilcats transform bubbles in bombs!\nPop a bomb to explode all nearby bubbles\nwith "
                                "a huge x10 money multiplier!",
                                /* maxPrestigeLevel */ 1);
                        }
                    }

                    std::sprintf(labelBuffer, "x%.2f", static_cast<double>(game.psvExplosionRadiusMult.currentValue()));
                    makePurchasableButtonPSV("- Explosion radius", game.psvExplosionRadiusMult);

                    if (catDevilUpgradesUnlocked)
                    {
                        makeCooldownButton("- Devilcat cooldown", CatType::Devil);
                        makeRangeButton("- Devilcat range", CatType::Devil);
                    }
                }

                // ASTRO CAT
                const bool astroCatUnlocked         = nCatNormal >= 10 && nCatUni >= 5 && nCatDevil >= 2;
                const bool astroCatUpgradesUnlocked = astroCatUnlocked && nCatDevil >= 10 && nCatAstro >= 5;
                if (astroCatUnlocked)
                {
                    std::sprintf(labelBuffer, "%d astrocats", nCatAstro);
                    if (makePurchasableButton("astrocat", 150000.f, 1.5f, static_cast<float>(nCatAstro)))
                    {
                        spawnCat(CatType::Astro, {-64.f, 0.f});
                    }

                    if (astroCatUpgradesUnlocked)
                    {
                        makeCooldownButton("- astrocat cooldown", CatType::Astro);
                        makeRangeButton("- astrocat range", CatType::Astro);
                    }
                }

                const auto milestoneText = [&]() -> std::string
                {
                    if (!game.comboPurchased)
                        return "buy combo to earn money faster";

                    if (game.psvComboStartTime.nPurchases == 0)
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
                        if      (&count == &nCatNormal) name = "cat";
                        else if (&count == &nCatUni)    name = "unicat";
                        else if (&count == &nCatDevil)  name = "devilcat";
                        else if (&count == &nCatWitch)  name = "witchcat";
                        else if (&count == &nCatAstro)  name = "astrocat";
                        // clang-format on

                        if (count < needed)
                            result += "\n- buy " + std::to_string(needed - count) + " more " + name + "(s)";
                    };

                    if (!game.mapPurchased)
                    {
                        startList("to increase playing area:");
                        result += "\n- buy map scrolling";
                    }

                    if (!catUnicornUnlocked)
                    {
                        startList("to unlock unicats:");

                        if (game.psvBubbleCount.nPurchases == 0)
                            result += "\n- buy more bubbles";

                        needNCats(nCatNormal, 3);
                    }

                    if (!catUpgradesUnlocked && catUnicornUnlocked)
                    {
                        startList("to unlock cat upgrades:");

                        if (game.psvBubbleCount.nPurchases == 0)
                            result += "\n- buy more bubbles";

                        needNCats(nCatNormal, 2);
                        needNCats(nCatUni, 1);
                    }

                    // TODO: change dynamically
                    if (catUnicornUnlocked && !bubbleValueUnlocked)
                    {
                        startList("to unlock prestige:");

                        if (game.psvBubbleCount.nPurchases == 0)
                            result += "\n- buy more bubbles";

                        needNCats(nCatUni, 3);
                    }

                    if (catUnicornUnlocked && bubbleValueUnlocked && !catDevilUnlocked)
                    {
                        startList("to unlock devilcats:");

                        if (game.psvBubbleValue.nPurchases == 0)
                            result += "\n- prestige at least once";

                        needNCats(nCatNormal, 6);
                        needNCats(nCatUni, 4);
                    }

                    if (catUnicornUnlocked && catDevilUnlocked && !catUnicornUpgradesUnlocked)
                    {
                        startList("to unlock unicat upgrades:");
                        needNCats(nCatUni, 2);
                        needNCats(nCatDevil, 1);
                    }

                    if (catUnicornUnlocked && catDevilUnlocked && !astroCatUnlocked)
                    {
                        startList("to unlock astrocats:");
                        needNCats(nCatNormal, 10);
                        needNCats(nCatUni, 5);
                        needNCats(nCatDevil, 2);
                    }

                    if (catUnicornUnlocked && catDevilUnlocked && astroCatUnlocked && !catDevilUpgradesUnlocked)
                    {
                        startList("to unlock devilcat upgrades:");
                        needNCats(nCatDevil, 2);
                        needNCats(nCatAstro, 1);
                    }

                    if (catUnicornUnlocked && catDevilUnlocked && astroCatUnlocked && !astroCatUpgradesUnlocked)
                    {
                        startList("to unlock astrocat upgrades:");
                        needNCats(nCatDevil, 10);
                        needNCats(nCatAstro, 5);
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

            if (true || bubbleValueUnlocked)
            {
                if (ImGui::BeginTabItem(" Prestige "))
                {
                    ImGui::SetWindowFontScale(1.f);
                    ImGui::Text("Prestige will:");

                    ImGui::SetWindowFontScale(0.75f);
                    ImGui::Text("- increase bubble value permanently");
                    ImGui::Text("- reset all your other progress");
                    ImGui::Text("- award you prestige points");

                    ImGui::SetWindowFontScale(1.f);
                    ImGui::Separator();


                    std::sprintf(labelBuffer, "current bubble value x%zu", getScaledReward(BubbleType::Normal));

                    const auto [times,
                                maxCost,
                                nextCost] = game.psvBubbleValue.maxSubsequentPurchases(game.money, globalCostMultiplier());

                    buttonHueMod = 120.f;
                    if (makePrestigePurchasableButtonPSV("Prestige", game.psvBubbleValue, times, maxCost))
                    {
                        sounds.prestige.play(playbackDevice);
                        prestigeTransition = 1;

                        scroll = 0.f;

                        draggedCat           = nullptr;
                        catDragPressDuration = 0.f;

                        game.onPrestige(times);
                    }

                    buttonHueMod = 0.f;
                    ImGui::SetWindowFontScale(0.75f);

                    const auto currentMult = static_cast<SizeT>(game.psvBubbleValue.currentValue()) + 1;

                    ImGui::Text("(next prestige: $%llu)", nextCost);

                    if (maxCost == 0u)
                        ImGui::Text("- not enough money to prestige");
                    else
                    {
                        ImGui::Text("- increase bubble value from x%llu to x%llu\n- obtain %llu prestige points",
                                    currentMult,
                                    currentMult + times,
                                    times);
                    }

                    ImGui::SetWindowFontScale(1.f);
                    ImGui::Separator();

                    ImGui::Text("permanent upgrades");

                    ImGui::SetWindowFontScale(0.75f);
                    ImGui::Text("- prestige points: %llu PPs", game.prestigePoints);
                    ImGui::SetWindowFontScale(1.f);

                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    buttonHueMod = 190.f;

                    if (game.psvMultiPopRange.nPurchases > 0)
                    {
                        ImGui::Checkbox("##multipop", &game.multiPopEnabled);
                        ImGui::SameLine();
                    }

                    std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(game.getComputedMultiPopRange()));
                    if (makePrestigePurchasablePPButtonPSV("Multipop range", game.psvMultiPopRange))
                        if (game.psvMultiPopRange.nPurchases == 1)
                            doTip("Popping a bubble now also pops\nnearby bubbles automatically!",
                                  /* maxPrestigeLevel */ UINT_MAX);

                    // TODO:
                    if (!game.smarterCatsPurchased)
                        std::sprintf(labelBuffer, "pop randomly");
                    else
                        std::sprintf(labelBuffer, "pop smartly");

                    if (makePurchasablePPButtonOneTime("Smarter cats", 1u, game.smarterCatsPurchased))
                        doTip("Cats will now prioritize popping\nspecial bubbles over basic ones!",
                              /* maxPrestigeLevel */ UINT_MAX);

                    if (game.windPurchased)
                    {
                        ImGui::Checkbox("##wind", &game.windEnabled);
                        ImGui::SameLine();
                    }

                    std::sprintf(labelBuffer, "");
                    if (makePurchasablePPButtonOneTime("Windy season", 2u, game.windPurchased))
                        doTip("It's windy season!\nHold onto something!",
                              /* maxPrestigeLevel */ UINT_MAX);

                    buttonHueMod = 0.f;

                    ImGui::EndTabItem();
                }
            }

            playedUsAccumulator += playedClock.getElapsedTime().asMicroseconds();
            playedClock.restart();

            while (playedUsAccumulator > 1000000)
            {
                playedUsAccumulator -= 1'000'000;

                game.statsTotal.secondsPlayed += 1.f;
                game.statsSession.secondsPlayed += 1.f;
            }

            if (ImGui::BeginTabItem(" Stats "))
            {
                const auto displayStats = [&](const Game::Stats& stats)
                {
                    ImGui::Spacing();
                    ImGui::Spacing();

                    const auto secondsPlayedToDisplay = static_cast<U64>(std::fmod(stats.secondsPlayed, 60.f));
                    const auto minutesPlayedToDisplay = static_cast<U64>(stats.secondsPlayed / 60u);
                    ImGui::Text("Time played: %llu min %llu sec", minutesPlayedToDisplay, secondsPlayedToDisplay);

                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::Text("Bubbles popped: %llu", stats.bubblesPopped);
                    ImGui::Indent();
                    ImGui::Text("Clicked: %llu", stats.bubblesHandPopped);
                    ImGui::Text("By cats: %llu", stats.bubblesPopped - stats.bubblesHandPopped);
                    ImGui::Unindent();

                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::Text("Revenue: $%llu", stats.bubblesPoppedRevenue);
                    ImGui::Indent();
                    ImGui::Text("Clicked: $%llu", stats.bubblesHandPoppedRevenue);
                    ImGui::Text("By cats: $%llu", stats.bubblesPoppedRevenue - stats.bubblesHandPoppedRevenue);
                    ImGui::Text("Bombs:   $%llu", stats.explosionRevenue);
                    ImGui::Unindent();
                };

                ImGui::SetWindowFontScale(0.75f);

                ImGui::Text(" -- Total values -- ");
                displayStats(game.statsTotal);

                ImGui::Separator();

                ImGui::Text(" -- Prestige values -- ");
                displayStats(game.statsSession);

                ImGui::SetWindowFontScale(1.f);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(" Settings "))
            {
                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Master volume", &game.masterVolume, 0.f, 100.f, "%.0f%%");

                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Music volume", &game.musicVolume, 0.f, 100.f, "%.0f%%");

                ImGui::Checkbox("Play audio in background", &game.playAudioInBackground);
                ImGui::Checkbox("Enable combo scratch sound", &game.playComboEndSound);

                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Minimap Scale", &game.minimapScale, 5.f, 30.f, "%.2f");

                ImGui::Checkbox("Enable tips", &game.tipsEnabled);

                const float fps = 1.f / fpsClock.getElapsedTime().asSeconds();
                ImGui::Text("FPS: %.2f", static_cast<double>(fps));

                ImGui::Separator();

                if (ImGui::Button("Save game"))
                {
                    saveGameToFile(game);
                }

                ImGui::SameLine();

                if (ImGui::Button("Load game"))
                {
                    loadGameFromFile(game);
                }

                ImGui::SameLine();

                buttonHueMod = 120.f;
                pushButtonColors();

                if (ImGui::Button("Reset game"))
                {
                    game = Game{};
                }

                popButtonColors();
                buttonHueMod = 0.f;

                ImGui::EndTabItem();
            }

            const float volumeMult = game.playAudioInBackground || window.hasFocus() ? 1.f : 0.f;

            listener.volume = game.masterVolume * volumeMult;
            musicBGM.setVolume(game.musicVolume * volumeMult);

            if (sounds.prestige.getStatus() == sf::Sound::Status::Playing)
                musicBGM.setVolume(0.f);

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
        sf::Text catTextName{fontSuperBakery,
                             {.characterSize    = 24u,
                              .fillColor        = sf::Color::White,
                              .outlineColor     = colorBlueOutline,
                              .outlineThickness = 1.5f}};

        sf::Text catTextStatus{fontSuperBakery,
                               {.characterSize    = 16u,
                                .fillColor        = sf::Color::White,
                                .outlineColor     = colorBlueOutline,
                                .outlineThickness = 1.f}};

        const sf::FloatRect* const catTxrsByType[nCatTypes]{
            game.smarterCatsPurchased ? &txrSmartCat : &txrCat, // Normal
            &txrUniCat,                                         // Unicorn
            &txrDevilCat,                                       // Devil
            &txrWitchCat,                                       // Witch
            &txrAstroCat,                                       // Astro
        };

        const sf::FloatRect* const catPawTxrsByType[nCatTypes]{
            &txrCatPaw,      // Normal
            &txrUniCatPaw,   // Unicorn
            &txrDevilCatPaw, // Devil
            &txrWitchCatPaw, // Witch
            &txrWhiteDot     // Astro
        };

        bool anyCatHovered = false;

        for (auto& cat : game.cats)
        {
            float opacityMod = 1.f;
            if (!anyCatHovered && &cat != draggedCat && (mousePos - cat.position).length() <= cat.getRadius() &&
                !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
            {
                anyCatHovered = true;
                opacityMod    = 0.5f;
            }

            const auto& catTxr    = *catTxrsByType[static_cast<int>(cat.type)];
            const auto& catPawTxr = *catPawTxrsByType[static_cast<int>(cat.type)];

            const auto maxCooldown  = game.getComputedCooldownByCatType(cat.type);
            const auto cooldownDiff = (maxCooldown - cat.cooldown);

            float catRotation = 0.f;

            if (cat.type == CatType::Astro)
            {
                if (cat.astroState.hasValue() && cat.isCloseToStartX())
                    catRotation = remap(sf::base::fabs(cat.position.x - cat.astroState->startX), 0.f, 400.f, 0.f, 0.523599f);
                else if (cooldownDiff < 1000.f)
                    catRotation = remap(cooldownDiff, 0.f, 1000.f, 0.523599f, 0.f);
                else if (cat.astroState.hasValue())
                    catRotation = 0.523599f;
            }

            cpuDrawableBatch.add(
                sf::Sprite{.position    = cat.drawPosition,
                           .scale       = {0.2f, 0.2f},
                           .origin      = catTxr.size / 2.f,
                           .rotation    = sf::radians(catRotation),
                           .textureRect = catTxr,
                           .color       = sf::Color::White.withAlpha(static_cast<U8>(cat.mainOpacity * opacityMod))});

            cpuDrawableBatch.add(
                sf::Sprite{.position    = cat.pawPosition,
                           .scale       = {0.1f, 0.1f},
                           .origin      = catPawTxr.size / 2.f,
                           .rotation    = cat.pawRotation,
                           .textureRect = catPawTxr,
                           .color       = sf::Color::White.withAlpha(static_cast<U8>(cat.pawOpacity * opacityMod))});

            const auto range = game.getComputedRangeByCatType(cat.type);

            constexpr sf::Color colorsByType[nCatTypes]{
                sf::Color::Blue,   // Cat
                sf::Color::Purple, // Unicorn
                sf::Color::Red,    // Devil
                sf::Color::Green,  // Witch
                sf::Color::White,  // Astro
            };

            // TODO P1: make it possible to draw a circle directly via batching without any of this stuff,
            // no need to preallocate a circle shape before, have a reusable vertex buffer in the batch itself
            catRadiusCircle.position = cat.position + cat.rangeOffset;
            catRadiusCircle.origin   = {range, range};
            catRadiusCircle.setRadius(range);
            catRadiusCircle.setOutlineColor(colorsByType[static_cast<int>(cat.type)].withAlpha(
                cat.cooldown < 0.f ? static_cast<U8>(0u) : static_cast<U8>(cat.cooldown / maxCooldown * 128.f)));
            cpuDrawableBatch.add(catRadiusCircle);

            catTextName.setString(shuffledCatNames[cat.nameIdx]);
            catTextName.position = cat.position + sf::Vector2f{0.f, 48.f};
            catTextName.origin   = catTextName.getLocalBounds().size / 2.f;
            cpuDrawableBatch.add(catTextName);

            constexpr const char* catActions[nCatTypes]{"Pops", "Shines", "IEDs", "Hexes", "Flights"};
            catTextStatus.setString(std::to_string(cat.hits) + " " + catActions[static_cast<int>(cat.type)]);
            catTextStatus.position = cat.position + sf::Vector2f{0.f, 68.f};
            catTextStatus.origin   = catTextStatus.getLocalBounds().size / 2.f;
            cat.textStatusShakeEffect.applyToText(catTextStatus);
            cpuDrawableBatch.add(catTextStatus);
        };
        // ---

        sf::Sprite tempSprite;

        // ---
        const sf::FloatRect bubbleRects[3]{txrBubble, txrBubbleStar, txrBomb};

        for (const auto& bubble : game.bubbles)
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
        sf::Text tempText{fontSuperBakery,
                          {.characterSize    = 16u,
                           .fillColor        = sf::Color::White,
                           .outlineColor     = colorBlueOutline,
                           .outlineThickness = 1.f}};

        for (const auto& textParticle : textParticles)
        {
            textParticle.applyToText(tempText);
            cpuDrawableBatch.add(tempText);
        }
        // ---

        window.draw(cpuDrawableBatch, {.texture = &textureAtlas.getTexture()});

        const sf::View hudView{.center = {resolution.x / 2.f, resolution.y / 2.f}, .size = resolution};
        window.setView(hudView);

        moneyText.setString("$" + std::to_string(game.money));
        moneyText.scale  = {1.f, 1.f};
        moneyText.origin = moneyText.getLocalBounds().size / 2.f;

        moneyText.setTopLeft({15.f, 70.f});
        moneyTextShakeEffect.update(deltaTimeMs);
        moneyTextShakeEffect.applyToText(moneyText);

        const float yBelowMinimap = game.mapPurchased ? (boundaries.y / game.minimapScale) + 15.f : 0.f;

        moneyText.position.y = yBelowMinimap + 30.f;
        window.draw(moneyText);

        if (game.comboPurchased)
        {
            comboText.setString("x" + std::to_string(combo + 1));

            comboTextShakeEffect.update(deltaTimeMs);
            comboTextShakeEffect.applyToText(comboText);

            comboText.position.y = yBelowMinimap + 50.f;
            window.draw(comboText);
        }

        //
        // Combo bar
        window.draw(sf::RectangleShape{{.position = {comboText.getCenterRight().x + 3.f, game.mapPurchased ? 110.f : 55.f},
                                        .fillColor = sf::Color{255, 255, 255, 75},
                                        .size      = {100.f * comboTimer / 700.f, 20.f}}},
                    /* texture */ nullptr);

        //
        // Minimap
        if (game.mapPurchased)
            drawMinimap(game.minimapScale, game.getMapLimit(), gameView, hudView, window, txBackground, cpuDrawableBatch, textureAtlas);

        //
        // UI
        imGuiContext.render(window);

        //
        // Splash screen
        if (splashTimer > 0.f)
            drawSplashScreen(window, txLogo, splashTimer);

        //
        // Tips
        if (tipTimer > 0.f)
        {
            tipTimer -= deltaTimeMs;

            if (game.tipsEnabled)
            {
                float fade = 255.f;

                if (tipTimer > 4000.f)
                    fade = remap(tipTimer, 4000.f, 4500.f, 255.f, 0.f);
                else if (tipTimer < 500.f)
                    fade = remap(tipTimer, 0.f, 500.f, 0.f, 255.f);

                const auto alpha = static_cast<U8>(sf::base::clamp(fade, 0.f, 255.f));

                sounds.byteSpeak.setPitch(1.6f);

                sf::Text tipText{fontSuperBakery,
                                 {.position         = {195.f, 265.f},
                                  .string           = tipString.substr(0,
                                                             static_cast<SizeT>(
                                                                 sf::base::clamp((4100.f - tipTimer) / 25.f,
                                                                                 0.f,
                                                                                 static_cast<float>(tipString.size())))),
                                  .characterSize    = 32u,
                                  .fillColor        = sf::Color::White.withAlpha(alpha),
                                  .outlineColor     = colorBlueOutline.withAlpha(alpha),
                                  .outlineThickness = 2.f}};

                if (sounds.byteSpeak.getStatus() != sf::Sound::Status::Playing &&
                    tipText.getString().getSize() < tipString.size() && tipText.getString().getSize() > 0)
                    sounds.byteSpeak.play(playbackDevice);

                sf::Sprite tipSprite{.position    = resolution / 2.f,
                                     .scale       = {0.7f, 0.7f},
                                     .origin      = txByteTip.getSize().toVector2f() / 2.f,
                                     .textureRect = txByteTip.getRect(),
                                     .color       = sf::Color::White.withAlpha(alpha)};

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
// - leveling cat (2500 pops is a good milestone for 1st lvl up, 5000 for 2nd, 10000 for 3rd)
// - some normal cat buff around 17000 money as a milestone towards devilcats, maybe two paws?
// - change bg when unlocking new cat type or prestiging?
// - steam achievements
// - find better word for "prestige"
// - tooltips in menus
// - change cat names
// - smart cat name prefix
// - pp point ideas: start with stuff unlocked, start with a bit of money, etc
// - prestige should scale indefinitely...? or make PP costs scale linearly, max is 20 -- or maybe when we reach max bubble value just purchase prestige points
// x - astrocat should inspire cats touched while flying
// x - unicat cooldown scaling should be less powerful and capped
// x - cat cooldown scaling should be a bit more powerful at the beginning and capped around 0.4
// x - unicat range scaling should be much more exponential and be capped
// x - cat after devil should be at least 150k
// x - another prestige bonus could be toggleable wind
// x - add stats to astrocats
// x - stats tab
// x - timer
// x - bomb explosion should have circular range (slight nerf)
// x - bomb should have higher mass for bubble collisions ??
// x - cats can go out of bounds if pushed by other cats
// x - cats should not be allowed next to the bounds, should be gently pushed away
// x - gradually increase count of bubbles when purchasing and on startup
// x - make combo end on misclick and play scratch sound

// TODO PRE-RELEASE:
// - release trailer with Byte and real life bubbles!
// - check Google Keep
// - unlock hard mode and speedrun mode at the end

// TODO LOW PRIO:
// - make open source?
// - bubbles that need to be weakened
// - individual cat levels and upgrades
// - unlockable areas, different bubble types
