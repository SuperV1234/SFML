#include "Aliases.hpp"
#include "Bubble.hpp"
#include "Cat.hpp"
#include "CatConstants.hpp"
#include "CatNames.hpp"
#include "Collision.hpp"
#include "Constants.hpp"
#include "ControlFlow.hpp"
#include "Countdown.hpp"
#include "Easing.hpp"
#include "HueColor.hpp"
#include "ImGuiNotify.hpp"
#include "MathUtils.hpp"
#include "MemberGuard.hpp"
#include "Particle.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "PurchasableScalingValue.hpp"
#include "RNG.hpp"
#include "Serialization.hpp"
#include "Shrine.hpp"
#include "Sounds.hpp"
#include "SpatialGrid.hpp"
#include "Stats.hpp"
#include "TextParticle.hpp"
#include "TextShakeEffect.hpp"
#include "ThreadPool/Pool.hpp"

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
#include "SFML/Graphics/Shader.hpp"
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

#include "SFML/Window/ContextSettings.hpp"
#include "SFML/Window/EventUtils.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"
#include "SFML/Window/VideoMode.hpp"
#include "SFML/Window/VideoModeUtils.hpp"
#include "SFML/Window/WindowSettings.hpp"

#include "SFML/System/Angle.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/System/Path.hpp"
#include "SFML/System/Rect.hpp"
#include "SFML/System/RectUtils.hpp"
#include "SFML/System/Vector2.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Math/Ceil.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/ScopeGuard.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <latch>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <cmath>
#include <cstdio>
#include <cstring>


////////////////////////////////////////////////////////////
#define BUBBLEBYTE_VERSION_STR "v0.0.6"


namespace
{
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
bool handleCatShrineCollision(const float deltaTimeMs, Cat& cat, Shrine& shrine)
{
    const auto result = handleCollision(deltaTimeMs, cat.position, shrine.position, {}, {}, cat.getRadius(), 64.f, 1.f, 1.f);

    if (!result.hasValue())
        return false;

    cat.position += result->iDisplacement;
    return true;
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline Bubble makeRandomBubble(Playthrough& pt, RNG& rng, const float mapLimit, const float maxY)
{
    return {.position = rng.getVec2f({mapLimit, maxY}),
            .velocity = rng.getVec2f({-0.1f, -0.1f}, {0.1f, 0.1f}),
            .radius   = rng.getF(0.07f, 0.16f) * 256.f *
                      remap(static_cast<float>(pt.psvBubbleCount.nPurchases), 0.f, 30.f, 1.1f, 0.8f),
            .rotation = 0.f,
            .hueMod   = 0.f,
            .type     = BubbleType::Normal};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr float getComboValueMult(const int n)
{
    constexpr float initial = 1.f;
    constexpr float decay   = 0.95f;

    return initial * (1.f - std::pow(decay, static_cast<float>(n))) / (1.f - decay);
}

////////////////////////////////////////////////////////////
void drawMinimap(sf::Shader&             shader,
                 const float             minimapScale,
                 const float             mapLimit,
                 const sf::View&         gameView,
                 const sf::View&         hudView,
                 sf::RenderTarget&       window,
                 const sf::Texture&      txBackground,
                 sf::CPUDrawableBatch&   cpuDrawableBatch,
                 const sf::TextureAtlas& textureAtlas,
                 const sf::Vector2f      resolution,
                 const float             hudScale)
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
         .size             = sf::Vector2f{mapLimit / minimapScale, minimapSize.y}}};

    //
    // Blue rectangle showing current visible area
    const sf::RectangleShape minimapIndicator{
        {.position     = minimapPos + sf::Vector2f{(gameView.center.x - gameScreenSize.x / 2.f) / minimapScale, 0.f},
         .fillColor    = sf::Color::Transparent,
         .outlineColor = sf::Color::Blue,
         .outlineThickness = 2.f,
         .size             = sf::Vector2f{gameScreenSize / minimapScale}}};

    //
    // Convert minimap dimensions to normalized `[0, 1]` range for scissor rectangle
    const float progressRatio = sf::base::clamp(mapLimit / boundaries.x, 0.f, 1.f);

    auto minimapScaledPosition = minimapPos.componentWiseDiv(resolution / hudScale);
    auto minimapScaledSize     = minimapSize.componentWiseDiv(resolution / hudScale);

    minimapScaledPosition.x = sf::base::clamp(minimapScaledPosition.x, 0.f, 1.f);
    minimapScaledPosition.y = sf::base::clamp(minimapScaledPosition.y, 0.f, 1.f);
    minimapScaledSize.y     = sf::base::clamp(minimapScaledSize.y, 0.f, 1.f - minimapScaledPosition.y);

    if (progressRatio * minimapScaledSize.x > 1.f - minimapScaledPosition.x)
        minimapScaledSize.x = (1.f - minimapScaledPosition.x) / progressRatio;

    //
    // Special view that renders the world scaled down for minimap
    const sf::View minimapView                                                  //
        {.center  = (resolution * 0.5f - minimapPos * hudScale) * minimapScale, // Offset center to align minimap
         .size    = resolution * minimapScale,                                  // Zoom out to show scaled-down world
         .scissor = {minimapScaledPosition, // Scissor rectangle position (normalized)
                     {
                         progressRatio * minimapScaledSize.x, // Only show accessible width
                         minimapScaledSize.y                  // Full height
                     }}};

    //
    // Draw minimap contents
    window.setView(minimapView); // Use minimap projection
    window.draw(sf::RectangleShape{{.fillColor = sf::Color::Black, .size = boundaries * hudScale}}, /* texture */ nullptr);
    window.draw(txBackground,
                {.scale = {hudScale, hudScale}, .color = sf::Color::White.withAlpha(128)},
                {.shader = &shader}); // Draw world background
    cpuDrawableBatch.scale = {hudScale, hudScale};
    window.draw(cpuDrawableBatch, {.texture = &textureAtlas.getTexture(), .shader = &shader}); // Draw game objects
    cpuDrawableBatch.scale = {1.f, 1.f};

    //
    // Switch back to HUD view and draw overlay elements
    window.setView(hudView);
    window.draw(minimapBorder, /* texture */ nullptr);    // Draw border frame
    window.draw(minimapIndicator, /* texture */ nullptr); // Draw current view indicator
}

////////////////////////////////////////////////////////////
void drawSplashScreen(sf::RenderWindow&        window,
                      const sf::Texture&       txLogo,
                      const TargetedCountdown& splashCountdown,
                      const sf::Vector2f       resolution,
                      const float              hudScale)
{
    const auto progress = splashCountdown.getProgressBounced(easeInOutCubic);

    window.draw({.position    = resolution / 2.f / hudScale,
                 .scale       = sf::Vector2f{0.7f, 0.7f} * (0.35f + 0.65f * easeInOutCubic(progress)) / hudScale,
                 .origin      = txLogo.getSize().toVector2f() / 2.f,
                 .textureRect = txLogo.getRect(),
                 .color       = sf::Color::White.withAlpha(static_cast<U8>(easeInOutSine(progress) * 255.f))},
                txLogo);
}

} // namespace


////////////////////////////////////////////////////////////
/// Main struct
///
////////////////////////////////////////////////////////////
struct Main
{
    ////////////////////////////////////////////////////////////
    // Resource contexts
    sf::AudioContext    audioContext{sf::AudioContext::create().value()};
    sf::PlaybackDevice  playbackDevice{sf::PlaybackDevice::createDefault(audioContext).value()};
    sf::GraphicsContext graphicsContext{sf::GraphicsContext::create().value()};

    ////////////////////////////////////////////////////////////
    // Shader with hue support
    sf::Shader shader{sf::Shader::loadFromMemory(sf::GraphicsContext::getBuiltInShaderVertexSrc(), fragmentSrc).value()};

    ////////////////////////////////////////////////////////////
    // Context settings
    const sf::ContextSettings contextSettings{
        .antiAliasingLevel = sf::base::min(16u, sf::RenderTexture::getMaximumAntiAliasingLevel())};

    ////////////////////////////////////////////////////////////
    // Render window
    sf::base::Optional<sf::RenderWindow> optWindow;
    bool                                 mustRecreateWindow = true;

    ////////////////////////////////////////////////////////////
    // ImGui context
    sf::ImGui::ImGuiContext imGuiContext;

    ////////////////////////////////////////////////////////////
    // Texture atlas
    sf::TextureAtlas textureAtlas{sf::Texture::create({4096u, 4096u}, {.smooth = true}).value()};

    ////////////////////////////////////////////////////////////
    // SFML fonts
    sf::Font fontSuperBakery{sf::Font::openFromFile("resources/superbakery.ttf", &textureAtlas).value()};

    ////////////////////////////////////////////////////////////
    // ImGui fonts
    ImFont* fontImGuiSuperBakery{nullptr};
    ImFont* fontImGuiMouldyCheese{nullptr};

    ////////////////////////////////////////////////////////////
    // Music
    sf::Music musicBGM{sf::Music::openFromFile("resources/hibiscus.mp3").value()};

    ////////////////////////////////////////////////////////////
    // Sound management
    Sounds       sounds;
    sf::Listener listener;

    ////////////////////////////////////////////////////////////
    // Textures (not in atlas)
    sf::Texture txLogo{sf::Texture::loadFromFile("resources/logo.png", {.smooth = true}).value()};
    sf::Texture txBackground{sf::Texture::loadFromFile("resources/background.png", {.smooth = true}).value()};
    sf::Texture txByteTip{sf::Texture::loadFromFile("resources/bytetip.png", {.smooth = true}).value()};
    sf::Texture txTipBg{sf::Texture::loadFromFile("resources/tipbg.png", {.smooth = true}).value()};
    sf::Texture txTipByte{sf::Texture::loadFromFile("resources/tipbyte.png", {.smooth = true}).value()};
    sf::Texture txCursor{sf::Texture::loadFromFile("resources/cursor.png", {.smooth = true}).value()};

    ////////////////////////////////////////////////////////////
    // Texture atlas rects
    sf::FloatRect txrWhiteDot{textureAtlas.add(graphicsContext.getBuiltInWhiteDotTexture()).value()};
    sf::FloatRect txrBubble{addImgResourceToAtlas("bubble2.png")};
    sf::FloatRect txrBubbleStar{addImgResourceToAtlas("bubble3.png")};
    sf::FloatRect txrCat{addImgResourceToAtlas("cat.png")};
    sf::FloatRect txrSmartCat{addImgResourceToAtlas("smartcat.png")};
    sf::FloatRect txrGeniusCat{addImgResourceToAtlas("geniuscat.png")};
    sf::FloatRect txrUniCat{addImgResourceToAtlas("unicat.png")};
    sf::FloatRect txrDevilCat{addImgResourceToAtlas("devilcat.png")};
    sf::FloatRect txrCatPaw{addImgResourceToAtlas("catpaw.png")};
    sf::FloatRect txrUniCatPaw{addImgResourceToAtlas("unicatpaw.png")};
    sf::FloatRect txrDevilCatPaw{addImgResourceToAtlas("devilcatpaw.png")};
    sf::FloatRect txrParticle{addImgResourceToAtlas("particle.png")};
    sf::FloatRect txrStarParticle{addImgResourceToAtlas("starparticle.png")};
    sf::FloatRect txrFireParticle{addImgResourceToAtlas("fireparticle.png")};
    sf::FloatRect txrHexParticle{addImgResourceToAtlas("hexparticle.png")};
    sf::FloatRect txrShrineParticle{addImgResourceToAtlas("shrineparticle.png")};
    sf::FloatRect txrCogParticle{addImgResourceToAtlas("cogparticle.png")};
    sf::FloatRect txrWitchCat{addImgResourceToAtlas("witchcat.png")};
    sf::FloatRect txrWitchCatPaw{addImgResourceToAtlas("witchcatpaw.png")};
    sf::FloatRect txrAstroCat{addImgResourceToAtlas("astromeow.png")};
    sf::FloatRect txrAstroCatWithFlag{addImgResourceToAtlas("astromeowwithflag.png")};
    sf::FloatRect txrBomb{addImgResourceToAtlas("bomb.png")};
    sf::FloatRect txrShrine{addImgResourceToAtlas("shrine.png")};
    sf::FloatRect txrWizardCat{addImgResourceToAtlas("wizardcat.png")};
    sf::FloatRect txrWizardCatPaw{addImgResourceToAtlas("wizardcatpaw.png")};
    sf::FloatRect txrMouseCat{addImgResourceToAtlas("mousecat.png")};
    sf::FloatRect txrMouseCatPaw{addImgResourceToAtlas("mousecatpaw.png")};
    sf::FloatRect txrEngiCat{addImgResourceToAtlas("engicat.png")};
    sf::FloatRect txrEngiCatPaw{addImgResourceToAtlas("engicatpaw.png")};
    sf::FloatRect txrRepulsoCat{addImgResourceToAtlas("repulsocat.png")};
    sf::FloatRect txrRepulsoCatPaw{addImgResourceToAtlas("repulsocatpaw.png")};

    ///////////////////////////////////////////////////////////
    // Profile (stores settings)
    Profile profile;
    MEMBER_SCOPE_GUARD(Main, {
        std::cout << "Saving profile to file on exit\n";
        saveProfileToFile(self.profile);
    });

    ////////////////////////////////////////////////////////////
    // Playthrough (game state)
    Playthrough pt;
    MEMBER_SCOPE_GUARD(Main, {
        std::cout << "Saving playthrough to file on exit\n";
        savePlaythroughToFile(self.pt);
    });

    ////////////////////////////////////////////////////////////
    // HUD money text
    sf::Text        moneyText{fontSuperBakery,
                              {.position         = {15.f, 70.f},
                               .string           = "$0",
                               .characterSize    = 64u,
                               .fillColor        = sf::Color::White,
                               .outlineColor     = colorBlueOutline,
                               .outlineThickness = 4.f}};
    TextShakeEffect moneyTextShakeEffect;

    ////////////////////////////////////////////////////////////
    // HUD combo text
    sf::Text        comboText{fontSuperBakery,
                              {.position         = moneyText.position + sf::Vector2f{0.f, 35.f},
                               .string           = "x1",
                               .characterSize    = 48u,
                               .fillColor        = sf::Color::White,
                               .outlineColor     = colorBlueOutline,
                               .outlineThickness = 3.f}};
    TextShakeEffect comboTextShakeEffect;

    ////////////////////////////////////////////////////////////
    SpatialGrid spatialGrid;

    ////////////////////////////////////////////////////////////
    std::vector<Particle>     particles;
    std::vector<TextParticle> textParticles;

    ////////////////////////////////////////////////////////////
    // Random number generation
    RNG rng{std::random_device{}()};

    ////////////////////////////////////////////////////////////
    // Cat names
    [[nodiscard]] static std::vector<std::vector<std::string>> makeShuffledCatNames(RNG& rng)
    {
        std::vector<std::vector<std::string>> result(nCatTypes);

        for (SizeT i = 0u; i < nCatTypes; ++i)
            result[i] = getShuffledCatNames(static_cast<CatType>(i), rng.getEngine());

        return result;
    }

    std::vector<std::vector<std::string>> shuffledCatNamesPerType = makeShuffledCatNames(rng);

    ////////////////////////////////////////////////////////////
    // Prestige transition
    bool inPrestigeTransition{false};

    ////////////////////////////////////////////////////////////
    // Times for transitions
    TargetedCountdown bubbleSpawnTimer{.startingValue = 3.f};
    TargetedCountdown catRemoveTimer{.startingValue = 100.f};

    ////////////////////////////////////////////////////////////
    // Combo state
    int       combo{0u};
    Countdown comboCountdown;

    ////////////////////////////////////////////////////////////
    // Clock and accumulator for played time
    sf::Clock     playedClock;
    sf::base::I64 playedUsAccumulator{0};
    sf::base::I64 autosaveUsAccumulator{0};

    ////////////////////////////////////////////////////////////
    // FPS and delta time clocks
    sf::Clock fpsClock;
    sf::Clock deltaClock;

    ////////////////////////////////////////////////////////////
    // Batch for drawing
    sf::CPUDrawableBatch bubbleDrawableBatch;
    sf::CPUDrawableBatch cpuDrawableBatch;

    ////////////////////////////////////////////////////////////
    // Scrolling state
    sf::base::Optional<sf::Vector2f> dragPosition;
    float                            scroll{0.f};
    float                            actualScroll{0.f};

    ////////////////////////////////////////////////////////////
    // Screen shake effect state
    float screenShakeAmount{0.f};
    float screenShakeTimer{0.f};

    ////////////////////////////////////////////////////////////
    // Cached culling boundaries
    struct CullingBoundaries
    {
        float left;
        float right;
        float top;
        float bottom;

        [[nodiscard, gnu::always_inline, gnu::flatten]] inline bool isInside(const sf::Vector2f point) const
        {
            return (point.x >= left) & (point.x <= right) & (point.y >= top) & (point.y <= bottom);
        }
    };

    CullingBoundaries particleCullingBoundaries{};
    CullingBoundaries bubbleCullingBoundaries{};

    ////////////////////////////////////////////////////////////
    // Cat dragging state
    Cat*  draggedCat{nullptr};
    float catDragPressDuration{0.f};

    ////////////////////////////////////////////////////////////
    // Touch state
    std::vector<sf::base::Optional<sf::Vector2f>> fingerPositions;

    ////////////////////////////////////////////////////////////
    // Splash screen state
    TargetedCountdown splashCountdown{.startingValue = 1750.f};

    ////////////////////////////////////////////////////////////
    // Tip state
    float       tipTimer{0.f};
    std::string tipString;

    ////////////////////////////////////////////////////////////
    // Shape buffers
    sf::CircleShape circleShapeBuffer{{.outlineTextureRect = txrWhiteDot, .outlineThickness = 1.f}};

    ////////////////////////////////////////////////////////////
    // Text buffers
    sf::Text textNameBuffer{fontSuperBakery,
                            {.characterSize    = 48u,
                             .fillColor        = sf::Color::White,
                             .outlineColor     = colorBlueOutline,
                             .outlineThickness = 3.f}};
    sf::Text textStatusBuffer{fontSuperBakery,
                              {.characterSize    = 32u,
                               .fillColor        = sf::Color::White,
                               .outlineColor     = colorBlueOutline,
                               .outlineThickness = 2.f}};

    ////////////////////////////////////////////////////////////
    // Thread pool
    hg::ThreadPool::Pool threadPool{getTPWorkerCount()};

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getTPWorkerCount()
    {
        const unsigned int numThreads = std::thread::hardware_concurrency();
        return (numThreads == 0u) ? 4u : numThreads;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SizeT getNextCatNameIdx(const CatType catType)
    {
        return pt.nextCatNamePerType[asIdx(catType)]++ % shuffledCatNamesPerType[asIdx(catType)].size();
    }

    ////////////////////////////////////////////////////////////
    Particle& implEmplaceParticle(const sf::Vector2f position,
                                  const ParticleType particleType,
                                  const float        scaleMult,
                                  const float        speedMult,
                                  const float        opacity = 1.f,
                                  const float        hue     = 0.f)
    {
        return particles.emplace_back(ParticleData{.position = position,
                                                   .velocity = rng.getVec2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * speedMult,
                                                   .scale         = rng.getF(0.08f, 0.27f) * scaleMult,
                                                   .accelerationY = 0.002f,
                                                   .opacity       = opacity,
                                                   .opacityDecay  = rng.getF(0.00025f, 0.0015f),
                                                   .rotation      = rng.getF(0.f, sf::base::tau),
                                                   .torque        = rng.getF(-0.002f, 0.002f)},
                                      hue,
                                      particleType);
    }

    ////////////////////////////////////////////////////////////
    void spawnParticle(const ParticleData& particleData, const float hue, const ParticleType particleType)
    {
        if (!profile.showParticles || !particleCullingBoundaries.isInside(particleData.position))
            return;

        particles.emplace_back(particleData, hue, particleType);
    }

    ////////////////////////////////////////////////////////////
    void spawnParticles(const SizeT n, const sf::Vector2f position, const auto... args)
    {
        if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
            return;

        for (SizeT i = 0; i < n; ++i)
            implEmplaceParticle(position, args...);
    }

    ////////////////////////////////////////////////////////////
    void spawnParticlesWithHue(const float hue, const SizeT n, const sf::Vector2f position, const auto... args)
    {
        if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
            return;

        for (SizeT i = 0; i < n; ++i)
            implEmplaceParticle(position, args...).hue = hue;
    }

    ////////////////////////////////////////////////////////////
    void spawnParticlesNoGravity(const SizeT n, const sf::Vector2f position, const auto... args)
    {
        if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
            return;

        for (SizeT i = 0; i < n; ++i)
            implEmplaceParticle(position, args...).data.accelerationY = 0.f;
    }

    ////////////////////////////////////////////////////////////
    void spawnParticlesWithHueNoGravity(const float hue, const SizeT n, const sf::Vector2f position, const auto... args)
    {
        if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
            return;

        for (SizeT i = 0; i < n; ++i)
        {
            auto& p              = implEmplaceParticle(position, args...);
            p.data.accelerationY = 0.f;
            p.hue                = hue;
        }
    }

    ////////////////////////////////////////////////////////////
    void withAllStats(auto&& func)
    {
        func(profile.statsLifetime);
        func(pt.statsTotal);
        func(pt.statsSession);
    }

    ////////////////////////////////////////////////////////////
    void statBubblePopped(const bool byHand, const MoneyType reward)
    {
        withAllStats([&](Stats& stats)
        {
            stats.bubblesPopped += 1;
            stats.bubblesPoppedRevenue += reward;
        });

        if (byHand)
        {
            withAllStats([&](Stats& stats)
            {
                stats.bubblesHandPopped += 1;
                stats.bubblesHandPoppedRevenue += reward;
            });
        }
    }

    ////////////////////////////////////////////////////////////
    void statExplosionRevenue(const MoneyType reward)
    {
        withAllStats([&](Stats& stats) { stats.explosionRevenue += reward; });
    }

    ////////////////////////////////////////////////////////////
    void statFlightRevenue(const MoneyType reward)
    {
        withAllStats([&](Stats& stats) { stats.flightRevenue += reward; });
    }

    ////////////////////////////////////////////////////////////
    void statSecondsPlayed()
    {
        withAllStats([&](Stats& stats) { stats.secondsPlayed += 1u; });
    }

    ////////////////////////////////////////////////////////////
    sf::RenderWindow& getWindow()
    {
        SFML_BASE_ASSERT(optWindow.hasValue());
        return *optWindow;
    }

    ////////////////////////////////////////////////////////////
    const sf::RenderWindow& getWindow() const
    {
        SFML_BASE_ASSERT(optWindow.hasValue());
        return *optWindow;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool keyDown(const sf::Keyboard::Key key) const
    {
        return getWindow().hasFocus() && sf::Keyboard::isKeyPressed(key);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool mBtnDown(const sf::Mouse::Button button) const
    {
        return getWindow().hasFocus() && sf::Mouse::isButtonPressed(button);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] sf::FloatRect addImgResourceToAtlas(const sf::Path& path)
    {
        return textureAtlas.add(sf::Image::loadFromFile("resources" / path).value()).value();
    };

    ////////////////////////////////////////////////////////////
    void playSound(const Sounds::LoadedSound& ls, bool overlap = true)
    {
        sounds.playPooled(playbackDevice, ls, overlap);
    }

    ////////////////////////////////////////////////////////////
    void forEachBubbleInRadius(const sf::Vector2f center, const float radius, auto&& func)
    {
        const float radiusSq = radius * radius;

        for (Bubble& bubble : pt.bubbles)
            if ((bubble.position - center).lengthSquared() <= radiusSq)
                if (func(bubble) == ControlFlow::Break)
                    break;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Bubble* pickRandomBubbleInRadiusMatching(const sf::Vector2f center, const float radius, auto&& predicate)
    {
        const float radiusSq = radius * radius;

        SizeT   count    = 0u;
        Bubble* selected = nullptr;

        for (Bubble& bubble : pt.bubbles)
            if (predicate(bubble) && (bubble.position - center).lengthSquared() <= radiusSq)
            {
                ++count;

                // Select the current bubble with probability `1/count` (reservoir sampling)
                if (rng.getI<SizeT>(0, count - 1) == 0)
                    selected = &bubble;
            }

        return (count == 0u) ? nullptr : selected;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Bubble* pickRandomBubbleInRadius(const sf::Vector2f center, const float radius)
    {
        return pickRandomBubbleInRadiusMatching(center,
                                                radius,
                                                [](const Bubble&) __attribute__((always_inline, flatten))
        { return true; });
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] sf::Vector2f getResolution() const
    {
        return getWindow().getSize().toVector2f();
    }

    ////////////////////////////////////////////////////////////
    Cat& spawnCat(const sf::View& gameView, const CatType catType, const float hue)
    {
        const auto pos = getWindow().mapPixelToCoords((getResolution() / 2.f).toVector2i(), gameView);
        spawnParticles(32, pos, ParticleType::Star, 0.5f, 0.75f);

        return pt.cats.emplace_back(
            Cat{.position              = pos,
                .wobbleRadians         = {},
                .cooldown              = {.value = pt.getComputedCooldownByCatType(catType)},
                .pawPosition           = pos,
                .pawRotation           = sf::radians(0.f),
                .hue                   = hue,
                .inspiredCountdown     = {},
                .boostCountdown        = {},
                .nameIdx               = getNextCatNameIdx(catType),
                .textStatusShakeEffect = {},
                .type                  = catType,
                .astroState            = {}});
    }

    ////////////////////////////////////////////////////////////
    void doTip(const std::string& str, const SizeT maxPrestigeLevel = 0u)
    {
        if (!profile.tipsEnabled || pt.psvBubbleValue.nPurchases > maxPrestigeLevel)
            return;

        playSound(sounds.byteMeow);
        tipString = str;
        tipTimer  = 5000.f;
    }

    ////////////////////////////////////////////////////////////
    static inline constexpr float normalFontScale    = 1.f;
    static inline constexpr float subBulletFontScale = 0.8f;
    static inline constexpr float toolTipFontScale   = 0.65f;
    static inline constexpr float windowWidth        = 425.f;
    static inline constexpr float buttonWidth        = 150.f;

    ////////////////////////////////////////////////////////////
    char         buffer[256]{};
    char         labelBuffer[512]{};
    char         tooltipBuffer[1024]{};
    float        buttonHueMod = 0.f;
    unsigned int widgetId     = 0u;

    ////////////////////////////////////////////////////////////
    void uiMakeButtonLabels(const char* label, const char* xLabelBuffer)
    {
        // button label
        ImGui::SetWindowFontScale((label[0] == '-' ? subBulletFontScale : normalFontScale) * 1.15f);
        ImGui::Text("%s", label);
        ImGui::SameLine();

        // button top label
        ImGui::SetWindowFontScale(0.5f);
        ImGui::Text("%s", xLabelBuffer);
        ImGui::SetWindowFontScale(label[0] == '-' ? subBulletFontScale : normalFontScale);
        ImGui::SameLine();

        ImGui::NextColumn();
    }

    ////////////////////////////////////////////////////////////
    void uiPushButtonColors()
    {
        const auto convertColor = [&](const auto colorId)
        {
            return sf::Color::fromVec4(ImGui::GetStyleColorVec4(colorId)).withHueMod(buttonHueMod).template toVec4<ImVec4>();
        };

        ImGui::PushStyleColor(ImGuiCol_Button, convertColor(ImGuiCol_Button));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, convertColor(ImGuiCol_ButtonHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, convertColor(ImGuiCol_ButtonActive));
        ImGui::PushStyleColor(ImGuiCol_Border, colorBlueOutline.withHueMod(buttonHueMod).toVec4<ImVec4>());
    }

    ////////////////////////////////////////////////////////////
    void uiPopButtonColors()
    {
        ImGui::PopStyleColor(4);
    }

    ////////////////////////////////////////////////////////////
    void uiBeginTooltip() const
    {
        const float tooltipWidth = 400.f;

        ImGui::SetNextWindowSizeConstraints(ImVec2(tooltipWidth, 0), ImVec2(tooltipWidth, FLT_MAX));

        ImGui::BeginTooltip();
        ImGui::PushFont(fontImGuiMouldyCheese);
        ImGui::SetWindowFontScale(toolTipFontScale);
    }

    ////////////////////////////////////////////////////////////
    void uiEndTooltip() const
    {
        ImGui::SetWindowFontScale(normalFontScale);
        ImGui::PopFont();
        ImGui::EndTooltip();
    }

    ////////////////////////////////////////////////////////////
    void uiMakeTooltip()
    {
        if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || std::strlen(tooltipBuffer) == 0u)
            return;

        const float tooltipWidth = 400.f;

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetMousePos().x - tooltipWidth, ImGui::GetMousePos().y + 20));
        uiBeginTooltip();

        ImGui::TextWrapped("%s", tooltipBuffer);

        uiEndTooltip();
    }

    ////////////////////////////////////////////////////////////
    void uiMakeCatTooltip(const sf::Vector2f mousePos)
    {
        Shrine* hoveredShrine = nullptr;

        for (Shrine& shrine : pt.shrines)
            if ((mousePos - shrine.position).lengthSquared() <= shrine.getRadiusSquared())
            {
                hoveredShrine = &shrine;
                break;
            }

        Cat* hoveredCat = nullptr;

        if (hoveredShrine == nullptr)
            for (Cat& cat : pt.cats)
                if ((mousePos - cat.position).lengthSquared() <= cat.getRadiusSquared())
                {
                    hoveredCat = &cat;
                    break;
                }

        if ((hoveredShrine == nullptr && hoveredCat == nullptr) || std::strlen(tooltipBuffer) == 0u)
            return;

        ImGui::SetNextWindowPos(ImVec2(getResolution().x - 15.f, getResolution().y - 15.f), 0, ImVec2(1, 1));
        uiBeginTooltip();

        if (hoveredShrine != nullptr)
        {
            constexpr const char* shrineTooltipsByType[]{
                R"(
~~ Shrine Of Magic ~~

The legend says that a powerful Wizardcat is sealed inside, capable of absorbing wisdom from star bubbles and casting spells on demand. Guess it wasn't powerful enough to avoid being sealed...

The magic of star bubbles seem to be absorbed by the shrine, nullifying their benefits.)",
                R"(
~~ Shrine Of Clicking ~~

Rumor has it that a sneaky Mousecat is sealed inside, capable of clicking bubbles, keeping up its own combos, empowering nearby cats, and providing a global click reward value multiplier by merely existing. That's a lot.

The shrine repels cats, wanting you to prove your worth by clicking bubbles.)",
                R"(
~~ Shrine Of Automation ~~

Stories of a crafty Engicat being sealed inside this shrine are told, capable of performing engine maintenance on nearby cats, temporarily increasing their speed, and providing a global cat reward value multiplier by merely existing.

Bubbles in the shrine's range are immune to clicks, as the shrine wants you to prove your automation skils.)",
                R"(
~~ Shrine Of Repulsion ~~

Experiments reveal that a Repulsocat is sealed inside, who continuously pushes bubbles away from itself with his portable USB fan. Thankfully, other cats are too fat to be affected by the wind.

Seems like the wind is powerful enough to repel bubbles even from within the shrine.)",
                R"(
~~ Shrine Of Attraction ~~

Electromagnetism readings suggest that an Attractocat is sealed inside, who continuously attracts bubbles with his huge magnet. Despite cats having an engine, they are not affected by the magnet -- can't explain that one.

The magnet is so powerful that its effects are felt even near the shrine.)",
                R"(
~~ Shrine Of Decay ~~

Effects: TODO P1: complete

Rewards: TODO P1: complete)",
                R"(
~~ Shrine Of Chaos ~~

Effects: TODO P1: complete

Rewards: TODO P1: complete)",
                R"(
~~ Shrine Of Transmutation ~~

Effects: TODO P1: complete

Rewards: TODO P1: complete)",
                R"(
~~ Shrine Of Victory ~~

Effects: TODO P1: complete

Rewards: TODO P1: complete)",
            };

            static_assert(sf::base::getArraySize(shrineTooltipsByType) == nShrineTypes);

            std::sprintf(tooltipBuffer, "%s", shrineTooltipsByType[static_cast<SizeT>(hoveredShrine->type)] + 1);
        }
        else
        {
            const char* catNormalTooltip = R"(
~~ Cat ~~

Pops bubbles or bombs, whatever comes first. Not the brightest, despite not being orange.

Prestige points can be spent for its college tuition, making it more cleverer.)";

            if (pt.geniusCatsPurchased)
            {
                catNormalTooltip =
                    R"(
~~ Genius Cat ~~

A truly intelligent being: prioritizes popping bombs first, then star bubbles, then normal bubbles. Can be instructed to ignore specific bubble types.

We do not speak of the origin of the large brain attached to its body.)";
            }
            else if (pt.smartCatsPurchased)
            {
                catNormalTooltip =
                    R"(
~~ Smart Cat ~~

Pops bubbles or bombs. Smart enough to prioritizes bombs and star bubbles over normal bubbles, but can't really tell those two apart.

We do not speak of the tuition fees.)";
            }

            const char* catAstroTooltip = R"(
~~ Astrocat ~~

Pride of the NCSA, a highly trained feline astronaut that continuously flies across the map, popping bubbles with a x20 multiplier.

Desperately trying to get funding from the government for a mission on the cheese moon. Perhaps some prestige points could help?)";

            if (pt.astroCatInspirePurchased)
            {
                catAstroTooltip =
                    R"(
~~ Propagandist Astrocat ~~

Pride of the NCSA, a highly trained feline astronaut that continuously flies across the map, popping bubbles with a x20 multiplier.

Finally financed by the NB (NOBUBBLES) political party to inspire other cats to work faster when flying by.)";
            }

            const char* catTooltipsByType[]{
                catNormalTooltip,
                R"(
~~ Unicat ~~

Imbued with the power of stars and rainbows, transforms bubbles (or bombs) into star bubbles, worth x25 more.

Must have eaten something it wasn't supposed to, because it keeps changing color.
)",
                R"(
~~ Devilcat ~~

Hired diplomat of the NB (NOBUBBLES) political party. Convinces bubbles to turn into bombs and explode for the rightful cause. Bubbles caught in explosions are worth x10 more.)",
                R"(
~~ Witchcat ~~
(unique cat)

TODO P1: complete)",
                catAstroTooltip,
                R"(
~~ Wizardcat ~~
(unique cat)

Ancient arcane feline capable of unleashing powerful spells, if only it could remember them.
Can absorb the magic of star bubbles to recall its past lives and remember spells.

The scriptures say that it "unlocks a Magic menu", but nobody knows what that means.)",
                R"(
~~ Mousecat ~~
(unique cat)

It stole a Logicat gaming mouse and it's now on the run. Surprisingly, the mouse still works even though it's not plugged in to anything.

Able to keep up a combo like for manual popping, and empowers nearby cats to pop bubbles with its current combo multiplier.

Provides a global click reward value multiplier (upgradable via PPs) by merely existing... Logicat does know how to make a good mouse.
)",
                R"(
~~ Engicat ~~
(unique cat)

Periodically performs maintenance on all nearby cats, temporarily increasing their engine efficiency and making them faster. (Note: this buff stacks with inspirational NB propaganda.)

Provides a global cat reward value multiplier (upgradable via PPs) by merely existing... guess it's a "10x engineer"?
)",
                R"(
~~ Repulsocat ~~
(unique cat)

Continuously pushes bubbles away from itself with his powerful USB fan, powered by God knows what kind of batteries. (Note: this effect is applied even while Repulsocat is being dragged.)

Using prestige points, the fan can be upgraded to filter specific bubble types and/or convert a percentage of bubbles to star bubbles.
)",
            };

            static_assert(sf::base::getArraySize(catTooltipsByType) == nCatTypes);

            SFML_BASE_ASSERT(hoveredCat != nullptr);
            std::sprintf(tooltipBuffer, "%s", catTooltipsByType[static_cast<SizeT>(hoveredCat->type)] + 1);
        }

        ImGui::TextWrapped("%s", tooltipBuffer);

        uiEndTooltip();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool uiMakeButtonImpl(const char* label, const char* xBuffer)
    {
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - buttonWidth - 2.5f, 0.f)); // Push to right
        ImGui::SameLine();

        uiPushButtonColors();

        bool clicked = false;
        if (ImGui::Button(xBuffer, ImVec2(buttonWidth, 0.f)))
        {
            playSound(sounds.buy);
            clicked = true;
        }

        uiMakeTooltip();

        uiPopButtonColors();

        if (label[0] == '-')
            ImGui::SetWindowFontScale(normalFontScale);

        ImGui::NextColumn();
        return clicked;
    }

    ////////////////////////////////////////////////////////////
    template <typename TCost>
    [[nodiscard]] bool makePSVButtonExByCurrency(
        const char*              label,
        PurchasableScalingValue& psv,
        const SizeT              times,
        const TCost              cost,
        TCost&                   availability,
        const char*              currencyFmt)
    {
        const bool maxedOut = psv.nPurchases == psv.data->nMaxPurchases;
        bool       result   = false;

        if (maxedOut)
            std::sprintf(buffer, "MAX##%u", widgetId++);
        else if (cost == 0u)
            std::sprintf(buffer, "N/A##%u", widgetId++);
        else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
            std::sprintf(buffer, currencyFmt, toStringWithSeparators(cost), widgetId++);
#pragma clang diagnostic pop

        ImGui::BeginDisabled(maxedOut || availability < cost || cost == 0u);

        uiMakeButtonLabels(label, labelBuffer);
        if (uiMakeButtonImpl(label, buffer))
        {
            result = true;
            availability -= cost;
            psv.nPurchases += times;
        }

        ImGui::EndDisabled();
        return result;
    };

    ////////////////////////////////////////////////////////////
    template <typename T>
    static const char* toStringWithSeparators(const T value)
    {
        // Thread-local buffer to store the result
        // Size should be 27 (max 20 digits for 64-bit integer + up to 6 separators + null terminator)
        // Using 32 for addinional safety just in case
        static thread_local char buffer[32];

        // First, convert to string from right to left
        char* end = buffer + sizeof(buffer) - 1;
        char* ptr = end;
        *ptr      = '\0';

        // Handle negative numbers
        const bool isNegative = value < 0;
        T          absValue   = isNegative ? -value : value;

        // Handle zero specially
        if (absValue == 0)
        {
            *--ptr = '0';
            return ptr;
        }

        // Convert digits and add separators
        int digitCount = 0;
        while (absValue > 0)
        {
            if (digitCount > 0 && digitCount % 3 == 0)
                *--ptr = '.';

            *--ptr = '0' + (absValue % 10);
            absValue /= 10;
            digitCount++;
        }

        if (isNegative)
            *--ptr = '-';

        return ptr;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makePSVButtonEx(const char* label, PurchasableScalingValue& psv, const SizeT times, const MoneyType cost)
    {
        return makePSVButtonExByCurrency(label, psv, times, cost, pt.money, "$%s##%u");
    }

    ////////////////////////////////////////////////////////////
    bool makePSVButton(const char* label, PurchasableScalingValue& psv)
    {
        return makePSVButtonEx(label, psv, 1u, static_cast<MoneyType>(pt.getComputedGlobalCostMultiplier() * psv.nextCost()));
    }

    ////////////////////////////////////////////////////////////
    template <typename TCost>
    [[nodiscard]] bool makePurchasableButtonOneTimeByCurrency(
        const char* label,
        bool&       done,
        const TCost cost,
        TCost&      availability,
        const char* currencyFmt)
    {
        bool result = false;

        if (done)
            std::sprintf(buffer, "DONE##%u", widgetId++);
        else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
            std::sprintf(buffer, currencyFmt, toStringWithSeparators(cost), widgetId++);
#pragma clang diagnostic pop

        ImGui::BeginDisabled(done || availability < cost);

        uiMakeButtonLabels(label, labelBuffer);
        if (uiMakeButtonImpl(label, buffer))
        {
            result = true;
            availability -= cost;
            done = true;
        }

        ImGui::EndDisabled();
        return result;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makePurchasableButtonOneTime(const char* label, const MoneyType cost, bool& done)
    {
        return makePurchasableButtonOneTimeByCurrency(
            label,
            done,
            /* cost */ static_cast<MoneyType>(pt.getComputedGlobalCostMultiplier() * static_cast<float>(cost)),
            /* availability */ pt.money,
            "$%s##%u");
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makePurchasablePPButtonOneTime(const char* label, const PrestigePointsType prestigePointsCost, bool& done)
    {
        return makePurchasableButtonOneTimeByCurrency(label,
                                                      done,
                                                      /* cost */ prestigePointsCost,
                                                      /* availability */ pt.prestigePoints,
                                                      "%s PPs##%u");
    }

    ////////////////////////////////////////////////////////////
    bool makePrestigePurchasablePPButtonPSV(const char* label, PurchasableScalingValue& psv)
    {
        return makePSVButtonExByCurrency(label,
                                         psv,
                                         /* times */ 1u,
                                         /* cost */ static_cast<PrestigePointsType>(psv.nextCost()),
                                         /* availability */ pt.prestigePoints,
                                         "%s PPs##%u");
    }

    ////////////////////////////////////////////////////////////
    void uiBeginColumns()
    {
        ImGui::Columns(2, "twoColumns", false);
        ImGui::SetColumnWidth(0, windowWidth - buttonWidth - 20.f);
        ImGui::SetColumnWidth(1, buttonWidth + 10.f);
    }

    ////////////////////////////////////////////////////////////
    void uiCenteredText(const char* str)
    {
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(str).x) * 0.5f);
        ImGui::Text("%s", str);
    }

    ////////////////////////////////////////////////////////////
    void uiDraw(const sf::View& gameView, const sf::Vector2f mousePos)
    {
        widgetId = 0u;

        ImGui::SetNextWindowPos({getResolution().x - 15.f, 15.f}, 0, {1.f, 0.f});
        ImGui::SetNextWindowSizeConstraints(ImVec2(windowWidth, 0.f), ImVec2(windowWidth, getResolution().y - 30.f));
        ImGui::PushFont(fontImGuiSuperBakery);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f); // Set corner radius

        ImGuiStyle& style               = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.f, 0.f, 0.f, 0.65f); // 65% transparent black
        style.Colors[ImGuiCol_Border]   = colorBlueOutline.toVec4<ImVec4>();
        style.FrameBorderSize           = 2.f;
        style.FrameRounding             = 10.f;
        style.WindowRounding            = 5.f;

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

        ImGui::Begin("##menu",
                     nullptr,
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoTitleBar);

        ImGui::PopStyleVar();

        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_DrawSelectedOverline))
        {
            uiTabBar(gameView);
            ImGui::EndTabBar();
        }

        uiMakeCatTooltip(mousePos);

        ImGui::End();
        ImGui::PopFont();
    }

    void uiTabBar(const sf::View& gameView)
    {
        if (ImGui::BeginTabItem("X"))
            ImGui::EndTabItem();

        static auto selectOnce = ImGuiTabItemFlags_SetSelected;
        if (ImGui::BeginTabItem("Shop", nullptr, selectOnce))
        {
            selectOnce = {};
            uiTabBarShop(gameView);
            ImGui::EndTabItem();
        }

        if (findCatWizard() != nullptr && ImGui::BeginTabItem("Magic"))
        {
            uiTabBarMagic();
            ImGui::EndTabItem();
        }

        if (pt.isBubbleValueUnlocked())
        {
            if (!pt.prestigeTipShown)
            {
                pt.prestigeTipShown = true;
                doTip("Prestige to increase bubble value\nand unlock permanent upgrades!");
            }

            if (ImGui::BeginTabItem("Prestige"))
            {
                uiTabBarPrestige();
                ImGui::EndTabItem();
            }
        }

        if (ImGui::BeginTabItem("Stats"))
        {
            uiTabBarStats();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Options"))
        {
            uiTabBarSettings();
            ImGui::EndTabItem();
        }
    }

    ////////////////////////////////////////////////////////////
    void uiTabBarShop(const sf::View& gameView)
    {
        const auto nCatNormal = pt.getCatCountByType(CatType::Normal);
        const auto nCatUni    = pt.getCatCountByType(CatType::Uni);
        const auto nCatDevil  = pt.getCatCountByType(CatType::Devil);
        const auto nCatWitch  = pt.getCatCountByType(CatType::Witch);
        const auto nCatAstro  = pt.getCatCountByType(CatType::Astro);

        Cat* catWizard  = findCatWizard();
        Cat* catMouse   = findCatMouse();
        Cat* catEngi    = findCatEngi();
        Cat* catRepulso = findCatRepulso();

        const bool anyUniqueCat = catWizard != nullptr || catMouse != nullptr || catEngi != nullptr || catRepulso != nullptr;

        uiBeginColumns();

        std::sprintf(tooltipBuffer,
                     "Build your combo by popping bubbles quickly, increasing the value of each subsequent "
                     "one.\n\nCombos expires on misclicks and over time, but can be upgraded to last "
                     "longer.\n\nStar bubbles are affected -- pop them while your multiplier is high!");
        std::sprintf(labelBuffer, "");
        if (makePurchasableButtonOneTime("Combo", 15, pt.comboPurchased))
        {
            combo = 0;
            doTip("Pop bubbles in quick succession to\nkeep your combo up and make money!");
        }

        if (pt.comboPurchased)
        {
            std::sprintf(tooltipBuffer, "Increase combo duration. We are in it for the long run.");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.psvComboStartTime.currentValue()));
            makePSVButton("- Longer combo", pt.psvComboStartTime);
        }

        ImGui::Separator();

        if (nCatNormal > 0 && pt.psvComboStartTime.nPurchases > 0)
        {
            std::sprintf(tooltipBuffer,
                         "Extend the map and enable scrolling (right click or drag with two fingers).\n\nExtending the "
                         "map will increase the total number of bubbles you can work with, and will also reveal "
                         "magical shrines that grant unique cats upon completion.");
            std::sprintf(labelBuffer, "");
            if (makePurchasableButtonOneTime("Map scrolling", 250, pt.mapPurchased))
            {
                scroll = 0.f;
                doTip("You can scroll the map with right click\nor by dragging with two fingers!");
            }

            if (pt.mapPurchased)
            {
                std::sprintf(tooltipBuffer, "Extend the map further by one screen.");
                std::sprintf(labelBuffer, "%.2f%%", static_cast<double>(pt.getMapLimit() / boundaries.x * 100.f));
                makePSVButton("- Extend map", pt.psvMapExtension);

                ImGui::BeginDisabled(pt.psvShrineActivation.nPurchases > pt.psvMapExtension.nPurchases);
                std::sprintf(tooltipBuffer,
                             "Activates the next shrine, enabling it to absorb nearby popped bubbles. Once enough "
                             "bubbles are absorbed by a shrine, it will grant a unique cat.");
                std::sprintf(labelBuffer, "%zu/9", pt.psvShrineActivation.nPurchases);
                makePSVButton("- Activate next shrine", pt.psvShrineActivation);
                ImGui::EndDisabled();
            }

            ImGui::Separator();

            std::sprintf(tooltipBuffer,
                         "Increase the total number of bubbles. Scales with map size.\n\nMore bubbles, "
                         "more money, fewer FPS!");
            std::sprintf(labelBuffer, "%zu bubbles", static_cast<SizeT>(pt.psvBubbleCount.currentValue()));
            makePSVButton("More bubbles", pt.psvBubbleCount);

            ImGui::Separator();
        }

        if (pt.comboPurchased && pt.psvComboStartTime.nPurchases > 0)
        {
            std::sprintf(tooltipBuffer,
                         "Cats pop nearby bubbles or bombs. Their cooldown and range can be upgraded. Their behavior "
                         "can be permanently upgraded with prestige points.\n\nCats can be dragged around to position "
                         "them strategically.\n\nNo, you can't get rid of a cat once purchased, you monster.");
            std::sprintf(labelBuffer, "%zu cats", nCatNormal);
            if (makePSVButton("Cat", pt.psvPerCatType[asIdx(CatType::Normal)]))
            {
                spawnCat(gameView, CatType::Normal, /* hue */ rng.getF(-20.f, 20.f));

                if (nCatNormal == 0)
                    doTip("Cats periodically pop bubbles for you!\nYou can drag them around to position them.");
            }
        }

        const auto makeCooldownButton = [this](const char* label, const CatType catType)
        {
            auto& psv = pt.psvCooldownMultsPerCatType[asIdx(catType)];

            std::sprintf(tooltipBuffer,
                         "Decrease cooldown.\n\nNote: can be reverted by right-clicking, but no refunds!");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.getComputedCooldownByCatType(catType) / 1000.f));
            makePSVButton(label, psv);

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Right) && psv.nPurchases > 0u)
            {
                --psv.nPurchases;
                playSound(sounds.buy);
            }
        };

        const auto makeRangeButton = [this](const char* label, const CatType catType)
        {
            auto& psv = pt.psvRangeDivsPerCatType[asIdx(catType)];

            std::sprintf(tooltipBuffer, "Increase range.\n\nNote: can be reverted by right-clicking, but no refunds!");
            std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(pt.getComputedRangeByCatType(catType)));
            makePSVButton(label, psv);

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Right) && psv.nPurchases > 0u)
            {
                --psv.nPurchases;
                playSound(sounds.buy);
            }
        };

        const bool catUpgradesUnlocked = pt.psvBubbleCount.nPurchases > 0 && nCatNormal >= 2 && nCatUni >= 1;
        if (catUpgradesUnlocked)
        {
            makeCooldownButton("- cooldown", CatType::Normal);
            makeRangeButton("- range", CatType::Normal);
        }

        ImGui::Separator();

        // UNICAT
        const bool catUnicornUnlocked         = pt.psvBubbleCount.nPurchases > 0 && nCatNormal >= 3;
        const bool catUnicornUpgradesUnlocked = catUnicornUnlocked && nCatUni >= 2 && nCatDevil >= 1;
        if (catUnicornUnlocked)
        {
            std::sprintf(tooltipBuffer,
                         "Unicats transform bubbles into star bubbles, which are worth x25 more!\n\nHave "
                         "your cats pop them for you, or pop them towards the end of a combo for huge rewards!");
            std::sprintf(labelBuffer, "%zu unicats", nCatUni);
            if (makePSVButton("Unicat", pt.psvPerCatType[asIdx(CatType::Uni)]))
            {
                spawnCat(gameView, CatType::Uni, /* hue */ rng.getF(0.f, 360.f));

                if (nCatUni == 0)
                    doTip("Unicats transform bubbles in star bubbles,\nworth x25! Pop them at the end of a combo!");
            }

            if (catUnicornUpgradesUnlocked)
            {
                makeCooldownButton("- cooldown", CatType::Uni);
                makeRangeButton("- range", CatType::Uni);
            }

            ImGui::Separator();
        }

        // DEVILCAT
        const bool catDevilUnlocked         = pt.psvBubbleValue.nPurchases > 0 && nCatNormal >= 6 && nCatUni >= 4;
        const bool catDevilUpgradesUnlocked = catDevilUnlocked && nCatDevil >= 2 && nCatAstro >= 1;
        if (catDevilUnlocked)
        {
            std::sprintf(tooltipBuffer,
                         "Devilcats transform bubbles into bombs that explode when popped. Bubbles affected by the "
                         "explosion are worth x10 more! Bomb explosion range can be upgraded.");
            std::sprintf(labelBuffer, "%zu devilcats", nCatDevil);
            if (makePSVButton("Devilcat", pt.psvPerCatType[asIdx(CatType::Devil)]))
            {
                spawnCat(gameView, CatType::Devil, /* hue */ rng.getF(-20.f, 20.f));

                if (nCatDevil == 0)
                    doTip(
                        "Devilcats transform bubbles in bombs!\nExplode them to pop nearby "
                        "bubbles\nwith a x10 money multiplier!",
                        /* maxPrestigeLevel */ 1);
            }

            std::sprintf(tooltipBuffer, "Increase bomb explosion radius.");
            std::sprintf(labelBuffer, "x%.2f", static_cast<double>(pt.psvExplosionRadiusMult.currentValue()));
            makePSVButton("- Explosion radius", pt.psvExplosionRadiusMult);

            if (catDevilUpgradesUnlocked)
            {
                makeCooldownButton("- cooldown", CatType::Devil);
                makeRangeButton("- range", CatType::Devil);
            }

            ImGui::Separator();
        }

        // ASTROCAT
        const bool astroCatUnlocked         = nCatNormal >= 10 && nCatUni >= 5 && nCatDevil >= 2;
        const bool astroCatUpgradesUnlocked = astroCatUnlocked && nCatDevil >= 9 && nCatAstro >= 5;
        if (astroCatUnlocked)
        {
            std::sprintf(tooltipBuffer,
                         "Astrocats periodically fly across the map, popping bubbles they hit with a huge x20 money "
                         "multiplier!\n\nThey can be permanently upgraded with prestige points to inspire cats "
                         "watching them fly past to pop bubbles faster.");
            std::sprintf(labelBuffer, "%zu astrocats", nCatAstro);
            if (makePSVButton("Astrocat", pt.psvPerCatType[asIdx(CatType::Astro)]))
            {
                spawnCat(gameView, CatType::Astro, /* hue */ rng.getF(-20.f, 20.f));

                if (nCatAstro == 0)
                    doTip(
                        "Astrocats periodically fly across\nthe entire map, with a huge\nx20 "
                        "money multiplier!",
                        /* maxPrestigeLevel */ 1);
            }

            if (astroCatUpgradesUnlocked)
            {
                makeCooldownButton("- cooldown", CatType::Astro);
                makeRangeButton("- range", CatType::Astro);
            }

            ImGui::Separator();
        }

        // UNIQUE CAT BONUSES
        if (anyUniqueCat)
        {
            ImGui::Text("Unique cats");

            ImGui::NextColumn();
            ImGui::NextColumn();

            if (catWizard != nullptr)
            {
                makeCooldownButton("- wizardcat cooldown", CatType::Wizard);
                makeRangeButton("- wizardcat range", CatType::Wizard);
            }

            if (catMouse != nullptr)
            {
                makeCooldownButton("- mousecat cooldown", CatType::Mouse);
                makeRangeButton("- mousecat range", CatType::Mouse);
            }

            if (catEngi != nullptr)
            {
                makeCooldownButton("- engicat cooldown", CatType::Engi);
                makeRangeButton("- engicat range", CatType::Engi);
            }

            if (catRepulso != nullptr)
            {
                // makeCooldownButton("- repulsocat cooldown", CatType::Repulso);
                makeRangeButton("- repulsocat range", CatType::Repulso);
            }

            ImGui::Separator();
        }

        const auto nextGoalsText = [&]() -> std::string
        {
            if (!pt.comboPurchased)
                return "buy combo to earn money faster";

            if (pt.psvComboStartTime.nPurchases == 0)
                return "buy longer combo to unlock cats";

            if (nCatNormal == 0)
                return "buy a cat";

            std::string result;
            const auto  startList = [&](const char* s)
            {
                result += result.empty() ? "" : "\n\n";
                result += s;
            };

            const auto needNCats = [&](const SizeT& count, const SizeT needed)
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

            if (!pt.mapPurchased)
            {
                startList("to increase playing area:");
                result += "\n- buy map scrolling";
            }

            if (!catUnicornUnlocked)
            {
                startList("to unlock unicats:");

                if (pt.psvBubbleCount.nPurchases == 0)
                    result += "\n- buy more bubbles";

                needNCats(nCatNormal, 3);
            }

            if (!catUpgradesUnlocked && catUnicornUnlocked)
            {
                startList("to unlock cat upgrades:");

                if (pt.psvBubbleCount.nPurchases == 0)
                    result += "\n- buy more bubbles";

                needNCats(nCatNormal, 2);
                needNCats(nCatUni, 1);
            }

            // TODO P1: change dynamically
            if (catUnicornUnlocked && !pt.isBubbleValueUnlocked())
            {
                startList("to unlock prestige:");

                if (pt.psvBubbleCount.nPurchases == 0)
                    result += "\n- buy more bubbles";

                needNCats(nCatUni, 3);
            }

            if (catUnicornUnlocked && pt.isBubbleValueUnlocked() && !catDevilUnlocked)
            {
                startList("to unlock devilcats:");

                if (pt.psvBubbleValue.nPurchases == 0)
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
                needNCats(nCatDevil, 9);
                needNCats(nCatAstro, 5);
            }

            return result;
        }();

        if (nextGoalsText != "")
        {
            ImGui::SetWindowFontScale(toolTipFontScale);
            ImGui::Text("%s", nextGoalsText.c_str());
            ImGui::SetWindowFontScale(normalFontScale);
        }

        ImGui::Columns(1);
    }

    ////////////////////////////////////////////////////////////
    void uiTabBarPrestige()
    {
        ImGui::SetWindowFontScale(normalFontScale);
        ImGui::Text("Prestige to:");

        ImGui::SetWindowFontScale(0.75f);
        ImGui::Text("- increase bubble value permanently");
        ImGui::Text("- reset all your other progress");
        ImGui::Text("- obtain prestige points");

        ImGui::SetWindowFontScale(normalFontScale);
        ImGui::Separator();

        std::sprintf(tooltipBuffer,
                     "WARNING: this will reset your progress!\n\nPrestige to increase bubble value permanently and "
                     "obtain prestige points. Prestige points can be used to unlock powerful permanent "
                     "upgrades.\n\nYou will sacrifice all your cats, bubbles, and money, but you will keep your "
                     "prestige points and permanent upgrades, and the value of bubbles will be permanently "
                     "increased.\n\nDo not be afraid to prestige -- it is what enables you to progress further!");
        std::sprintf(labelBuffer, "current bubble value x%llu", pt.getComputedRewardByBubbleType(BubbleType::Normal));

        const auto [times,
                    maxCost,
                    nextCost] = pt.psvBubbleValue.maxSubsequentPurchases(pt.money, pt.getComputedGlobalCostMultiplier());

        const auto ppReward = pt.calculatePrestigePointReward(times);

        uiBeginColumns();

        buttonHueMod = 120.f;
        if (makePSVButtonEx("Prestige", pt.psvBubbleValue, times, maxCost))
        {
            playSound(sounds.prestige);
            inPrestigeTransition = true;

            scroll = 0.f;

            draggedCat           = nullptr;
            catDragPressDuration = 0.f;

            pt.onPrestige(ppReward);
        }

        ImGui::Columns(1);

        buttonHueMod = 0.f;
        ImGui::SetWindowFontScale(0.75f);

        const auto currentMult = static_cast<SizeT>(pt.psvBubbleValue.currentValue()) + 1u;

        ImGui::Text("(next prestige: $%s)", toStringWithSeparators(nextCost));

        if (maxCost == 0u)
            ImGui::Text("- not enough money to prestige");
        else
        {
            ImGui::Text("- increase bubble value from x%zu to x%zu\n- obtain %zu prestige points",
                        currentMult,
                        currentMult + times,
                        ppReward);
        }

        ImGui::SetWindowFontScale(normalFontScale);
        ImGui::Separator();

        ImGui::Text("permanent upgrades");

        ImGui::SetWindowFontScale(0.75f);
        ImGui::Text("- prestige points: %llu PPs", pt.prestigePoints);
        ImGui::SetWindowFontScale(normalFontScale);

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        uiBeginColumns();

        buttonHueMod = 190.f;

        std::sprintf(tooltipBuffer,
                     "Manually popping a bubble now also pops nearby bubbles automatically!\n\nNote that combo "
                     "multiplier still only increases once per successful click.\n\nNote: this effect can be toggled "
                     "at will.");
        std::sprintf(labelBuffer, "");
        if (makePurchasablePPButtonOneTime("Multipop click", 1u, pt.multiPopPurchased))
            doTip("Popping a bubble now also pops\nnearby bubbles automatically!",
                  /* maxPrestigeLevel */ UINT_MAX);

        if (pt.multiPopPurchased)
        {
            std::sprintf(tooltipBuffer, "Increase the range of the multipop effect.");
            std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(pt.getComputedMultiPopRange()));
            makePrestigePurchasablePPButtonPSV("- range", pt.psvPPMultiPopRange);

            ImGui::SetWindowFontScale(subBulletFontScale);
            ImGui::Checkbox("enable ##multipop", &pt.multiPopEnabled);
            ImGui::SetWindowFontScale(normalFontScale);
            ImGui::NextColumn();
            ImGui::NextColumn();
        }

        ImGui::Separator();

        std::sprintf(tooltipBuffer,
                     "Cats have graduated!\n\nThey still cannot resist their popping insticts, but they will go for "
                     "star bubbles and bombs first, ensuring they are not wasted!");
        std::sprintf(labelBuffer, "");
        if (makePurchasablePPButtonOneTime("Smart cats", 1u, pt.smartCatsPurchased))
            doTip("Cats will now prioritize popping\nspecial bubbles over basic ones!",
                  /* maxPrestigeLevel */ UINT_MAX);

        if (pt.smartCatsPurchased)
        {
            std::sprintf(tooltipBuffer,
                         "Embrace the glorious evolution!\n\nCats have ascended beyond their primal "
                         "insticts and will now prioritize bombs, then star bubbles, then normal "
                         "bubbles!\n\nThey will also ignore any bubble type of your choosing.\n\nNote: "
                         "this effect can be toggled at will.");
            std::sprintf(labelBuffer, "");
            if (makePurchasablePPButtonOneTime("- genius cats", 16u, pt.geniusCatsPurchased))
                doTip("Genius cats prioritize bombs and\ncan be instructed to ignore certain bubbles!",
                      /* maxPrestigeLevel */ UINT_MAX);
        }

        if (pt.geniusCatsPurchased)
        {
            ImGui::Columns(1);
            ImGui::SetWindowFontScale(subBulletFontScale);

            ImGui::Text("- ignore: ");
            ImGui::SameLine();

            ImGui::Checkbox("normal##genius", &pt.geniusCatIgnoreNormalBubbles);
            ImGui::SameLine();

            ImGui::Checkbox("star##genius", &pt.geniusCatIgnoreStarBubbles);
            ImGui::SameLine();

            ImGui::Checkbox("bombs##genius", &pt.geniusCatIgnoreBombBubbles);

            ImGui::SetWindowFontScale(normalFontScale);
            uiBeginColumns();
        }

        ImGui::Separator();

        std::sprintf(tooltipBuffer,
                     "A giant fan (off-screen) will produce an intense wind, making bubbles move and "
                     "flow much faster.\n\nNote: this effect can be toggled at will.");
        std::sprintf(labelBuffer, "");
        if (makePurchasablePPButtonOneTime("Giant fan", 8u, pt.windPurchased))
            doTip("Hold onto something!", /* maxPrestigeLevel */ UINT_MAX);

        if (pt.windPurchased)
        {
            ImGui::SetWindowFontScale(subBulletFontScale);
            ImGui::Checkbox("enable ##wind", &pt.windEnabled);
            ImGui::SetWindowFontScale(normalFontScale);
            ImGui::NextColumn();
            ImGui::NextColumn();
        }

        ImGui::Separator();

        std::sprintf(tooltipBuffer,
                     "Astrocats are now equipped with fancy patriotic flags, inspiring cats watching "
                     "them fly by to work faster!");
        std::sprintf(labelBuffer, "");
        if (makePurchasablePPButtonOneTime("Space propaganda", 32u, pt.astroCatInspirePurchased))
            doTip("Astrocats will inspire other cats\nto work faster when flying by!",
                  /* maxPrestigeLevel */ UINT_MAX);

        if (pt.astroCatInspirePurchased)
        {
            std::sprintf(tooltipBuffer, "Increase the duration of the inspiration effect.");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.getComputedInspirationDuration() / 1000.f));

            makePrestigePurchasablePPButtonPSV("- buff duration", pt.psvPPInspireDurationMult);
        }

        if (findCatWizard() != nullptr || pt.psvBubbleValue.nPurchases >= 3)
        {
            ImGui::Separator();

            ImGui::Columns(1);
            ImGui::Text("Wizardcat");
            uiBeginColumns();

            // TODO P1: unshackle
            // TODO P1: autocast

            std::sprintf(tooltipBuffer, "Increase the speed of mana generation.");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.getComputedManaCooldown() / 1000.f));
            makePrestigePurchasablePPButtonPSV("- Mana cooldown", pt.psvPPManaCooldownMult);

            std::sprintf(tooltipBuffer, "Increase the maximum mana.");
            std::sprintf(labelBuffer, "%llu mana", pt.getComputedMaxMana());
            makePrestigePurchasablePPButtonPSV("- Mana limit", pt.psvPPManaMaxMult);

            std::sprintf(tooltipBuffer,
                         "Starpaw conversion ignores bombs, transforming only normal bubbles around the wizard into "
                         "star bubbles.");
            std::sprintf(labelBuffer, "");
            (void)makePurchasablePPButtonOneTime("- Selective starpaw", 64u, pt.astroCatInspirePurchased);
        }

        if (findCatMouse() != nullptr || pt.psvBubbleValue.nPurchases >= 4)
        {
            ImGui::Separator();

            ImGui::Columns(1);
            ImGui::Text("Mousecat");
            uiBeginColumns();

            // TODO P1: unshackle

            std::sprintf(tooltipBuffer, "Increase the global click reward value multiplier.");
            std::sprintf(labelBuffer, "x%.2f", static_cast<double>(pt.psvPPMouseCatGlobalBonusMult.currentValue()));
            makePrestigePurchasablePPButtonPSV("- Global click mult", pt.psvPPMouseCatGlobalBonusMult);
        }

        if (findCatEngi() != nullptr || pt.psvBubbleValue.nPurchases >= 5)
        {
            ImGui::Separator();

            ImGui::Columns(1);
            ImGui::Text("Engicat");
            uiBeginColumns();

            // TODO P1: unshackle

            std::sprintf(tooltipBuffer, "Increase the global cat reward value multiplier.");
            std::sprintf(labelBuffer, "x%.2f", static_cast<double>(pt.psvPPEngiCatGlobalBonusMult.currentValue()));
            makePrestigePurchasablePPButtonPSV("- Global cat mult", pt.psvPPEngiCatGlobalBonusMult);
        }

        if (findCatRepulso() != nullptr || pt.psvBubbleValue.nPurchases >= 5)
        {
            ImGui::Separator();

            ImGui::Columns(1);
            ImGui::Text("Repulsocat");
            uiBeginColumns();

            // TODO P1: unshackle

            std::sprintf(tooltipBuffer,
                         "The Repulsocat does some quantum science stuff to its magnet to allow filtering of repelled "
                         "bubbles by type.");
            std::sprintf(labelBuffer, "");
            (void)makePurchasablePPButtonOneTime("- Repulsion filter", 64u, pt.repulsoCatFilterPurchased);

            if (pt.repulsoCatFilterPurchased)
            {
                ImGui::Columns(1);
                ImGui::SetWindowFontScale(subBulletFontScale);

                ImGui::Text("- ignore: ");
                ImGui::SameLine();

                ImGui::Checkbox("normal##repulso", &pt.repulsoCatIgnoreNormalBubbles);
                ImGui::SameLine();

                ImGui::Checkbox("star##repulso", &pt.repulsoCatIgnoreStarBubbles);
                ImGui::SameLine();

                ImGui::Checkbox("bombs##repulso", &pt.repulsoCatIgnoreBombBubbles);

                ImGui::SetWindowFontScale(normalFontScale);
                uiBeginColumns();
            }

            std::sprintf(tooltipBuffer,
                         "The Repulsocat does some more weird science to its magnet, giving it a chance to convert "
                         "repelled bubbles to star bubbles.");
            std::sprintf(labelBuffer, "");
            (void)makePurchasablePPButtonOneTime("- Conversion field", 256u, pt.repulsoCatConverterPurchased);

            if (pt.repulsoCatConverterPurchased)
            {
                ImGui::SetWindowFontScale(subBulletFontScale);
                ImGui::Checkbox("enable ##repulsoconv", &pt.repulsoCatConverterEnabled);
                ImGui::SetWindowFontScale(normalFontScale);
                ImGui::NextColumn();
                ImGui::NextColumn();

                std::sprintf(tooltipBuffer, "Increase the repelled bubble conversion chance.");
                std::sprintf(labelBuffer, "%.2f%%", static_cast<double>(pt.psvPPRepulsoCatConverterChance.currentValue()));
                makePrestigePurchasablePPButtonPSV("- Conversion chance", pt.psvPPRepulsoCatConverterChance);
            }
        }

        buttonHueMod = 0.f;

        ImGui::Columns(1);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isWizardBusy() const
    {
        const Cat* wizardCat = findCatWizard();

        if (wizardCat == nullptr)
            return false;

        return pt.absorbingWisdom || wizardCat->cooldown.value != 0.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* findFirstCatByType(const CatType& catType)
    {
        const auto it = sf::base::findIf(pt.cats.begin(),
                                         pt.cats.end(),
                                         [&](const Cat& cat) { return cat.type == catType; });

        if (it == pt.cats.end())
            return nullptr;

        return &*it;
    }

    ////////////////////////////////////////////////////////////
#define DEFINE_CAT_FINDER_BY_TYPE(type)                                    \
    [[nodiscard]] Cat* findCat##type()                                     \
    {                                                                      \
        return findFirstCatByType(CatType::type);                          \
    }                                                                      \
                                                                           \
    [[nodiscard]] const Cat* findCat##type() const                         \
    {                                                                      \
        return const_cast<Main*>(this)->findFirstCatByType(CatType::type); \
    }

    ////////////////////////////////////////////////////////////
    DEFINE_CAT_FINDER_BY_TYPE(Wizard);
    DEFINE_CAT_FINDER_BY_TYPE(Mouse);
    DEFINE_CAT_FINDER_BY_TYPE(Engi);
    DEFINE_CAT_FINDER_BY_TYPE(Repulso);

    ////////////////////////////////////////////////////////////
    void addCombo(int& xCombo, Countdown& xComboCountdown) const
    {
        if (xCombo == 0)
        {
            xCombo                = 1;
            xComboCountdown.value = pt.psvComboStartTime.currentValue() * 1000.f;
        }
        else
        {
            xCombo += 1;
            xComboCountdown.value += 150.f - sf::base::clamp(static_cast<float>(xCombo) * 10.f, 0.f, 100.f);
        }
    }

    ////////////////////////////////////////////////////////////
    static void checkComboEnd(const float deltaTimeMs, int& xCombo, Countdown& xComboCountdown)
    {
        if (xComboCountdown.updateAndStop(deltaTimeMs) == CountdownStatusStop::AlreadyFinished)
            xCombo = 0;
    }

    ////////////////////////////////////////////////////////////
    void uiTabBarMagic()
    {
        ImGui::SetWindowFontScale(normalFontScale);

        Cat* wizardCat = findCatWizard();

        if (wizardCat == nullptr)
        {
            ImGui::Text("The wizardcat is missing!");
            return;
        }

        const auto range       = pt.getComputedRangeByCatType(CatType::Wizard);
        const auto maxCooldown = pt.getComputedCooldownByCatType(CatType::Wizard);

        ImGui::Text("Your wizard is %s!", shuffledCatNamesPerType[asIdx(CatType::Wizard)][wizardCat->nameIdx].c_str());

        ImGui::Separator();

        ImGui::Text("Wisdom points: %llu WP", pt.wisdom);

        ImGui::Checkbox("Absorb wisdom", &pt.absorbingWisdom);
        std::sprintf(tooltipBuffer,
                     "The Wizardcat concentrates, absorbing wisdom points from nearby star bubbles. While the "
                     "Wizardcat is concentrating, it cannot cast spells nor be moved around.");
        uiMakeTooltip();

        uiBeginColumns();
        buttonHueMod = 45.f;

        std::sprintf(tooltipBuffer, "The Wizardcat taps into memories of past lives, remembering a powerful spell.");
        std::sprintf(labelBuffer, "%zu/%zu", pt.psvSpellCount.nPurchases, pt.psvSpellCount.data->nMaxPurchases);
        (void)makePSVButtonExByCurrency("Remember spell",
                                        pt.psvSpellCount,
                                        1u,
                                        static_cast<MoneyType>(
                                            pt.getComputedGlobalCostMultiplier() * pt.psvSpellCount.nextCost()),
                                        pt.wisdom,
                                        "%s WP##%u");

        buttonHueMod = 0.f;
        ImGui::Columns(1);

        ImGui::Separator();

        ImGui::Text("Mana: %llu / %llu", pt.mana, pt.getComputedMaxMana());

        ImGui::Text("Next mana:");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(157, 0, 255, 128));
        ImGui::ProgressBar(pt.manaTimer / pt.getComputedManaCooldown());
        ImGui::PopStyleColor();

        ImGui::Text("Wizard cooldown: %.2fs", static_cast<double>(wizardCat->cooldown.value / 1000.f));

        ImGui::Separator();

        if (pt.psvSpellCount.nPurchases == 0)
        {
            ImGui::Text("No spells revealed yet...");
        }
        else
        {
            ImGui::BeginDisabled(isWizardBusy());
            uiBeginColumns();
            buttonHueMod = 45.f;

            //
            // SPELL 0
            if (pt.psvSpellCount.nPurchases >= 1)
            {
                std::sprintf(tooltipBuffer,
                             "Transforms a percentage of bubbles around the Wizardcat into star bubbles "
                             "immediately.\n\nCan be upgraded to ignore bombs with prestige points.");
                std::sprintf(labelBuffer, "");
                bool done = false;

                if (makePurchasableButtonOneTimeByCurrency("Starpaw Conversion",
                                                           done,
                                                           ManaType{10u},
                                                           pt.mana,
                                                           "%s mana##%u"))
                {
                    playSound(sounds.cast0);
                    spawnParticlesNoGravity(256,
                                            wizardCat->position,
                                            ParticleType::Star,
                                            rng.getF(0.25f, 1.25f),
                                            rng.getF(0.50f, 3.f));

                    forEachBubbleInRadius(wizardCat->position,
                                          range,
                                          [&](Bubble& bubble)
                    {
                        if (pt.starpawConversionIgnoreBombs && bubble.type != BubbleType::Normal)
                            return ControlFlow::Continue;

                        if (rng.getF(0.f, 99.f) > pt.psvStarpawPercentage.currentValue())
                            return ControlFlow::Continue;

                        bubble.type   = BubbleType::Star;
                        bubble.hueMod = rng.getF(0.f, 360.f);
                        bubble.velocity.y -= rng.getF(0.025f, 0.05f);

                        spawnParticles(1, bubble.position, ParticleType::Star, 0.5f, 0.35f);

                        return ControlFlow::Continue;
                    });

                    done = false;
                    ++wizardCat->hits;
                    wizardCat->cooldown.value = maxCooldown * 2.f;
                }

                std::sprintf(tooltipBuffer, "Increase the percentage of bubbles converted into star bubbles.");
                std::sprintf(labelBuffer, "%.2f%%", static_cast<double>(pt.psvStarpawPercentage.currentValue()));
                (void)makePSVButtonExByCurrency("- higher percentage",
                                                pt.psvStarpawPercentage,
                                                1u,
                                                static_cast<MoneyType>(pt.getComputedGlobalCostMultiplier() *
                                                                       pt.psvStarpawPercentage.nextCost()),
                                                pt.wisdom,
                                                "%s WP##%u");
            }

            //
            // SPELL 1
            if (pt.psvSpellCount.nPurchases >= 2)
            {
                ImGui::Separator();

                std::sprintf(tooltipBuffer,
                             "Creates a value multiplier aura around the Wizardcat that affects all cats and bubbles. "
                             "Lasts 5 seconds.\n\nCasting this spell multiple times will accumulate the aura "
                             "duration.");
                std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.arcaneAuraTimer / 1000.f));
                bool done = false;
                if (makePurchasableButtonOneTimeByCurrency("Mewltiplier Aura",
                                                           done,
                                                           ManaType{20u},
                                                           pt.mana,
                                                           "%s mana##%u"))
                {
                    playSound(sounds.cast0);
                    spawnParticlesNoGravity(256,
                                            wizardCat->position,
                                            ParticleType::Star,
                                            rng.getF(0.25f, 1.25f),
                                            rng.getF(0.50f, 3.f));

                    pt.arcaneAuraTimer += 5000.f;

                    done = false;
                    ++wizardCat->hits;
                    wizardCat->cooldown.value = maxCooldown * 2.f;
                }

                std::sprintf(tooltipBuffer, "Increase the multiplier applied while the aura is active.");
                std::sprintf(labelBuffer, "x%.2f", static_cast<double>(pt.psvMewltiplierMult.currentValue()));
                (void)makePSVButtonExByCurrency("- higher multiplier",
                                                pt.psvMewltiplierMult,
                                                1u,
                                                static_cast<MoneyType>(pt.getComputedGlobalCostMultiplier() *
                                                                       pt.psvMewltiplierMult.nextCost()),
                                                pt.wisdom,
                                                "%s WP##%u");
            }

            //
            // SPELL 2 (TODO P0: should cost 30 mana, and is usually unlocked around prestige 4, so both devils and astros exist)
            // idea: stasis, makes all bubbles stationary and when popped they do not disappear (except bombs maybe)
            if (pt.psvSpellCount.nPurchases >= 3)
            {
                ImGui::Separator();

                std::sprintf(tooltipBuffer, "TODO");
                std::sprintf(labelBuffer, "TODO");

                bool done = false;
                if (makePurchasableButtonOneTimeByCurrency("TODO", done, ManaType{30u}, pt.mana, "%s mana##%u"))
                {
                    playSound(sounds.cast0);

                    // TODO: effect

                    done = false;
                    ++wizardCat->hits;
                    wizardCat->cooldown.value = maxCooldown * 2.f;
                }
            }

            buttonHueMod = 0.f;
            ImGui::Columns(1);
            ImGui::EndDisabled();
        }
    }

    ////////////////////////////////////////////////////////////
    static constexpr auto formatTime(const sf::base::U64 seconds)
    {
        struct Result
        {
            sf::base::U64 h;
            sf::base::U64 m;
            sf::base::U64 s;
        };

        return Result{seconds / 3600u, (seconds / 60u) % 60u, seconds % 60u};
    };

    ////////////////////////////////////////////////////////////
    void uiTabBarStats()
    {
        const auto displayStats = [&](const Stats& stats)
        {
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::Columns(2, "twoColumnsStats", false);
            ImGui::SetColumnWidth(0, windowWidth / 2.f);
            ImGui::SetColumnWidth(1, windowWidth / 2.f);

            const auto [h, m, s] = formatTime(stats.secondsPlayed);
            ImGui::Text("Time played: %lluh %llum %llus", h, m, s);

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::Text("Bubbles popped: %s", toStringWithSeparators(stats.bubblesPopped));
            ImGui::Indent();
            ImGui::Text("Clicked: %s", toStringWithSeparators(stats.bubblesHandPopped));
            ImGui::Text("By cats: %s", toStringWithSeparators(stats.bubblesPopped - stats.bubblesHandPopped));
            ImGui::Unindent();

            ImGui::NextColumn();

            ImGui::Text("Revenue: $%s", toStringWithSeparators(stats.bubblesPoppedRevenue));
            ImGui::Indent();
            ImGui::Text("Clicked: $%s", toStringWithSeparators(stats.bubblesHandPoppedRevenue));
            ImGui::Text("By cats: $%s",
                        toStringWithSeparators(stats.bubblesPoppedRevenue - stats.bubblesHandPoppedRevenue));
            ImGui::Text("Bombs:  $%s", toStringWithSeparators(stats.explosionRevenue));
            ImGui::Text("Flights: $%s", toStringWithSeparators(stats.flightRevenue));
            ImGui::Unindent();

            ImGui::Columns(1);
        };

        if (ImGui::BeginTabBar("TabBarStats", ImGuiTabBarFlags_DrawSelectedOverline))
        {
            ImGui::SetWindowFontScale(0.75f);
            if (ImGui::BeginTabItem(" Statistics "))
            {
                ImGui::SetWindowFontScale(0.75f);

                uiCenteredText(" ~~ Lifetime ~~ ");
                displayStats(profile.statsLifetime);

                ImGui::Separator();

                uiCenteredText(" ~~ This playthrough ~~ ");
                displayStats(pt.statsTotal);

                ImGui::Separator();

                uiCenteredText(" ~~ This prestige ~~ ");
                displayStats(pt.statsSession);

                ImGui::SetWindowFontScale(normalFontScale);

                ImGui::EndTabItem();
            }

            ImGui::SetWindowFontScale(0.75f);
            if (ImGui::BeginTabItem(" Milestones "))
            {
                ImGui::SetWindowFontScale(0.75f);

                const auto doMilestone = [&](const char* name, const MilestoneTimestamp value)
                {
                    if (value == maxMilestone)
                    {
                        ImGui::Text("%s: N/A", name);
                        return;
                    }

                    const auto [h, m, s] = formatTime(value);
                    ImGui::Text("%s: %lluh %llum %llus", name, h, m, s);
                };

                doMilestone("1st Cat", pt.milestones.firstCat);
                doMilestone("5th Cat", pt.milestones.fiveCats);
                doMilestone("10th Cat", pt.milestones.tenCats);

                ImGui::Separator();

                doMilestone("1st Unicat", pt.milestones.firstUnicat);
                doMilestone("5th Unicat", pt.milestones.fiveUnicats);
                doMilestone("10th Unicat", pt.milestones.tenUnicats);

                ImGui::Separator();

                doMilestone("1st Devilcat", pt.milestones.firstDevilcat);
                doMilestone("5th Devilcat", pt.milestones.fiveDevilcats);
                doMilestone("10th Devilcat", pt.milestones.tenDevilcats);

                ImGui::Separator();

                doMilestone("1st Astrocat", pt.milestones.firstAstrocat);
                doMilestone("5th Astrocat", pt.milestones.fiveAstrocats);
                doMilestone("10th Astrocat", pt.milestones.tenAstrocats);

                ImGui::Separator();

                doMilestone("Prestige Level 1", pt.milestones.prestigeLevel1);
                doMilestone("Prestige Level 2", pt.milestones.prestigeLevel2);
                doMilestone("Prestige Level 3", pt.milestones.prestigeLevel3);
                doMilestone("Prestige Level 4", pt.milestones.prestigeLevel4);
                doMilestone("Prestige Level 5", pt.milestones.prestigeLevel5);
                doMilestone("Prestige Level 10", pt.milestones.prestigeLevel10);
                doMilestone("Prestige Level 15", pt.milestones.prestigeLevel15);
                doMilestone("Prestige Level 20", pt.milestones.prestigeLevel20);

                ImGui::Separator();

                doMilestone("$10.000 Revenue", pt.milestones.revenue10000);
                doMilestone("$100.000 Revenue", pt.milestones.revenue100000);
                doMilestone("$1.000.000 Revenue", pt.milestones.revenue1000000);
                doMilestone("$10.000.000 Revenue", pt.milestones.revenue10000000);
                doMilestone("$100.000.000 Revenue", pt.milestones.revenue100000000);
                doMilestone("$1.000.000.000 Revenue", pt.milestones.revenue1000000000);

                ImGui::Separator();

                for (SizeT i = 0u; i < nShrineTypes; ++i)
                {
                    const char* shrineName = i >= pt.getMapLimitIncreases() ? "Shrine Of ???" : shrineNames[i];
                    doMilestone(shrineName, pt.milestones.shrineCompletions[i]);
                }

                ImGui::EndTabItem();
            }

            ImGui::SetWindowFontScale(0.75f);
            if (ImGui::BeginTabItem(" Achievements "))
            {
                // TODO P1: add achievements

                ImGui::EndTabItem();
            }

            ImGui::SetWindowFontScale(normalFontScale);
            ImGui::EndTabBar();
        }

        ImGui::SetWindowFontScale(normalFontScale);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] MoneyType computeFinalReward(const sf::Vector2f bubblePosition, const MoneyType computedReward, const int xCombo)
    {
        Cat* wizardCat = findCatWizard();

        const bool inRangeOfWizard = wizardCat != nullptr && (wizardCat->position - bubblePosition).lengthSquared() <=
                                                                 pt.getComputedSquaredRangeByCatType(CatType::Wizard);

        const float arcaneAuraMult = (pt.arcaneAuraTimer > 0.f && inRangeOfWizard) ? pt.psvMewltiplierMult.currentValue() : 1.f;

        return static_cast<MoneyType>(
            sf::base::ceil(static_cast<float>(computedReward) * getComboValueMult(xCombo) * arcaneAuraMult));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static sf::Vector2u getReasonableWindowSize(const float scalingFactorMult = 1.f)
    {
        constexpr float gameRatio = gameScreenSize.x / gameScreenSize.y;

        const auto  fullscreenSize = sf::VideoModeUtils::getDesktopMode().size.toVector2f();
        const float aspectRatio    = fullscreenSize.x / fullscreenSize.y;

        const bool isUltrawide = aspectRatio >= 2.0f;
        const bool isWide      = aspectRatio >= 1.6f && aspectRatio < 2.0f;

        const float scalingFactor = isUltrawide ? 0.9f : isWide ? 0.8f : 0.7f;

        const auto windowSize = fullscreenSize * scalingFactor * scalingFactorMult;

        const auto windowedWidth = windowSize.y * gameRatio + (windowWidth + 35.f);

        return (sf::Vector2f{windowedWidth, windowSize.y}).toVector2u();
    }

    ////////////////////////////////////////////////////////////
    void uiTabBarSettings()
    {
        bool sgActive = false;
        SFML_BASE_SCOPE_GUARD({
            if (sgActive)
                ImGui::EndTabBar();
        });
        sgActive = ImGui::BeginTabBar("TabBarSettings", ImGuiTabBarFlags_DrawSelectedOverline);

        ImGui::SetWindowFontScale(0.75f);
        if (ImGui::BeginTabItem("Audio"))
        {
            ImGui::SetWindowFontScale(normalFontScale);

            ImGui::SetNextItemWidth(210.f);
            ImGui::SliderFloat("Master volume", &profile.masterVolume, 0.f, 100.f, "%.0f%%");

            ImGui::SetNextItemWidth(210.f);
            ImGui::SliderFloat("Music volume", &profile.musicVolume, 0.f, 100.f, "%.0f%%");

            ImGui::Checkbox("Play audio in background", &profile.playAudioInBackground);
            ImGui::Checkbox("Enable combo scratch sound", &profile.playComboEndSound);

            ImGui::EndTabItem();
        }

        ImGui::SetWindowFontScale(0.75f);
        if (ImGui::BeginTabItem("Interface"))
        {
            ImGui::SetWindowFontScale(normalFontScale);

            ImGui::SetNextItemWidth(210.f);
            ImGui::SliderFloat("Minimap Scale", &profile.minimapScale, 5.f, 40.f, "%.2f");

            ImGui::SetNextItemWidth(210.f);
            ImGui::SliderFloat("HUD Scale", &profile.hudScale, 0.5f, 2.f, "%.2f");

            ImGui::Checkbox("Enable tips", &profile.tipsEnabled);

            ImGui::Separator();

            ImGui::Checkbox("High-visibility cursor", &profile.highVisibilityCursor);

            ImGui::BeginDisabled(!profile.highVisibilityCursor);
            {
                ImGui::SetWindowFontScale(0.75f);

                ImGui::Checkbox("Multicolor", &profile.multicolorCursor);

                ImGui::BeginDisabled(profile.multicolorCursor);
                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Hue", &profile.cursorHue, 0.f, 360.f, "%.2f");
                ImGui::EndDisabled();

                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Scale", &profile.cursorScale, 0.3f, 1.5f, "%.2f");

                ImGui::SetWindowFontScale(normalFontScale);
            }
            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }

        ImGui::SetWindowFontScale(0.75f);
        if (ImGui::BeginTabItem("Graphics"))
        {
            ImGui::SetWindowFontScale(normalFontScale);

            ImGui::SetNextItemWidth(210.f);
            ImGui::SliderFloat("Background Opacity", &profile.backgroundOpacity, 0.f, 100.f, "%.0f%%");

            ImGui::Checkbox("Show cat text", &profile.showCatText);
            ImGui::Checkbox("Show particles", &profile.showParticles);
            ImGui::Checkbox("Show text particles", &profile.showTextParticles);

            ImGui::EndTabItem();
        }

        ImGui::SetWindowFontScale(0.75f);
        if (ImGui::BeginTabItem("Display"))
        {
            ImGui::SetWindowFontScale(normalFontScale);

            ImGui::Text("Auto resolution");

            ImGui::SetWindowFontScale(0.85f);
            ImGui::Text("Windowed");

            ImGui::SameLine();

            if (ImGui::Button("Large"))
            {
                profile.resWidth = getReasonableWindowSize(1.f);
                profile.windowed = true;

                mustRecreateWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Medium"))
            {
                profile.resWidth = getReasonableWindowSize(0.9f);
                profile.windowed = true;

                mustRecreateWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Small"))
            {
                profile.resWidth = getReasonableWindowSize(0.8f);
                profile.windowed = true;

                mustRecreateWindow = true;
            }

            ImGui::Text("Fullscreen");

            ImGui::SameLine();

            if (ImGui::Button("Borderless"))
            {
                profile.resWidth = sf::VideoModeUtils::getDesktopMode().size;
                profile.windowed = true;

                mustRecreateWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Exclusive"))
            {
                profile.resWidth = sf::VideoModeUtils::getDesktopMode().size;
                profile.windowed = false;

                mustRecreateWindow = true;
            }

            ImGui::Separator();

            if (ImGui::Checkbox("VSync", &profile.vsync))
            {
                optWindow->setVerticalSyncEnabled(profile.vsync);
            }

            static auto fpsLimit = static_cast<float>(profile.frametimeLimit);
            ImGui::SetNextItemWidth(210.f);
            if (ImGui::DragFloat("FPS Limit", &fpsLimit, 1.f, 60.f, 144.f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
            {
                profile.frametimeLimit = static_cast<unsigned int>(fpsLimit);
                optWindow->setFramerateLimit(profile.frametimeLimit);
            }

            ImGui::SetWindowFontScale(normalFontScale);

            ImGui::EndTabItem();
        }

        ImGui::SetWindowFontScale(0.75f);
        if (true && ImGui::BeginTabItem("Developer") /* TODO P0: cheats */)
        {
            if (ImGui::Button("Save game"))
                savePlaythroughToFile(pt);

            ImGui::SameLine();

            if (ImGui::Button("Load game"))
                loadPlaythroughFromFileAndReseed();

            ImGui::SameLine();

            buttonHueMod = 120.f;
            uiPushButtonColors();

            if (ImGui::Button("Reset game"))
            {
                rng.reseed(std::random_device{}());
                shuffledCatNamesPerType = makeShuffledCatNames(rng);

                pt      = Playthrough{};
                pt.seed = rng.getSeed();

                particles.clear();
                textParticles.clear();
            }

            uiPopButtonColors();
            buttonHueMod = 0.f;

            ImGui::Separator();

            ImGui::PushFont(fontImGuiMouldyCheese);
            ImGui::SetWindowFontScale(toolTipFontScale);

            SizeT step    = 1u;
            SizeT counter = 0u;

            static char filenameBuf[128] = "custom.json";

            ImGui::SetNextItemWidth(320.f);
            ImGui::InputText("##Filename", filenameBuf, sizeof(filenameBuf));

            if (ImGui::Button("Custom save"))
                savePlaythroughToFile(pt, filenameBuf);

            ImGui::SameLine();

            if (ImGui::Button("Custom load"))
                loadPlaythroughFromFile(pt, filenameBuf);

            ImGui::Separator();

            if (ImGui::Button("Feed next shrine"))
            {
                for (Shrine& shrine : pt.shrines)
                {
                    if (!shrine.isActive() || shrine.tcDeath.hasValue())
                        continue;

                    const auto requiredReward = pt.getComputedRequiredRewardByShrineType(shrine.type);
                    shrine.collectedReward += requiredReward / 10u;
                    break;
                }
            }

            ImGui::Separator();

            ImGui::SetNextItemWidth(240.f);
            ImGui::InputScalar("Money", ImGuiDataType_U64, &pt.money, &step, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal);

            ImGui::SetNextItemWidth(240.f);
            ImGui::InputScalar("PPs", ImGuiDataType_U64, &pt.prestigePoints, &step, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal);

            ImGui::SetNextItemWidth(240.f);
            ImGui::InputScalar("WPs", ImGuiDataType_U64, &pt.wisdom, &step, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal);

            ImGui::Separator();

            const auto psvScalarInput = [&](const char* label, PurchasableScalingValue& psv)
            {
                if (psv.data->nMaxPurchases == 0u)
                    return;

                std::string lbuf = label;
                lbuf += "##";
                lbuf += std::to_string(counter++);

                ImGui::SetNextItemWidth(140.f);
                if (ImGui::InputScalar(lbuf.c_str(), ImGuiDataType_U64, &psv.nPurchases, &step, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal))
                    psv.nPurchases = sf::base::clamp(psv.nPurchases, SizeT{0u}, psv.data->nMaxPurchases);
            };

            psvScalarInput("ComboStartTime", pt.psvComboStartTime);
            psvScalarInput("MapExtension", pt.psvMapExtension);
            psvScalarInput("ShrineActivation", pt.psvShrineActivation);
            psvScalarInput("BubbleCount", pt.psvBubbleCount);
            psvScalarInput("SpellCount", pt.psvSpellCount);
            psvScalarInput("BubbleValue", pt.psvBubbleValue);
            psvScalarInput("ExplosionRadiusMult", pt.psvExplosionRadiusMult);

            ImGui::Separator();

            for (SizeT i = 0u; i < nCatTypes; ++i)
            {
                constexpr const char* catNames[] = {
                    "Normal",
                    "Uni",
                    "Devil",
                    "Witch",
                    "Astro",
                    "Wizard",
                    "Mouse",
                    "Engi",
                    "Repulso",
                };

                static_assert(sf::base::getArraySize(catNames) == nCatTypes);

                ImGui::Text("%s", catNames[i]);
                psvScalarInput("PerCatType", pt.psvPerCatType[i]);
                psvScalarInput("CooldownMultsPerCatType", pt.psvCooldownMultsPerCatType[i]);
                psvScalarInput("RangeDivsPerCatType", pt.psvRangeDivsPerCatType[i]);

                ImGui::Separator();
            }

            psvScalarInput("PPMultiPopRange", pt.psvPPMultiPopRange);
            psvScalarInput("PPInspireDurationMult", pt.psvPPInspireDurationMult);
            psvScalarInput("PPManaCooldownMult", pt.psvPPManaCooldownMult);
            psvScalarInput("PPManaMaxMult", pt.psvPPManaMaxMult);
            psvScalarInput("PPMouseCatGlobalBonusMult", pt.psvPPMouseCatGlobalBonusMult);
            psvScalarInput("PPEngiCatGlobalBonusMult", pt.psvPPEngiCatGlobalBonusMult);
            psvScalarInput("PPRepulsoCatConverterChance", pt.psvPPRepulsoCatConverterChance);

            ImGui::SetWindowFontScale(normalFontScale);
            ImGui::PopFont();

            ImGui::EndTabItem();
        }

        ImGui::Separator();
        ImGui::SetWindowFontScale(normalFontScale);

        const float fps = 1.f / fpsClock.getElapsedTime().asSeconds();
        ImGui::Text("FPS: %.2f", static_cast<double>(fps));
    }

    ////////////////////////////////////////////////////////////
    void popBubbleImpl(const bool         byHand,
                       const BubbleType   bubbleType,
                       const MoneyType    reward,
                       const int          xCombo,
                       const sf::Vector2f position,
                       const sf::Vector2f tpPosition,
                       bool               popSoundOverlap)
    {
        statBubblePopped(byHand, reward);

        if (profile.showTextParticles)
        {
            auto& tp = textParticles.emplace_back(
                TextParticle{.buffer = {},
                             .data   = {.position = {tpPosition.x, tpPosition.y - 10.f},
                                        .velocity = rng.getVec2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.425f,
                                        .scale = sf::base::clamp(1.f + 0.1f * static_cast<float>(combo + 1) / 1.75f, 1.f, 3.0f),
                                        .accelerationY = 0.0042f,
                                        .opacity       = 1.f,
                                        .opacityDecay  = 0.00175f,
                                        .rotation      = 0.f,
                                        .torque        = rng.getF(-0.002f, 0.002f)}});

            std::snprintf(tp.buffer, sizeof(tp.buffer), "+$%llu", reward);
        }

        sounds.pop.setPosition({position.x, position.y});
        sounds.pop.setPitch(remap(static_cast<float>(xCombo), 1, 10, 1.f, 2.f));

        playSound(sounds.pop, popSoundOverlap);

        spawnParticles(32, position, ParticleType::Bubble, 0.5f, 0.5f);
        spawnParticles(8, position, ParticleType::Bubble, 1.2f, 0.25f);

        if (bubbleType == BubbleType::Star)
        {
            spawnParticles(16, position, ParticleType::Star, 0.5f, 0.35f);
        }
        else if (bubbleType == BubbleType::Bomb)
        {
            sounds.explosion.setPosition({position.x, position.y});
            playSound(sounds.explosion);

            spawnParticles(32, position, ParticleType::Fire, 3.f, 1.f);

            forEachBubbleInRadius(position,
                                  pt.getComputedBombExplosionRadius(),
                                  [&](Bubble& bubble)
            {
                if (bubble.type == BubbleType::Bomb)
                    return ControlFlow::Continue;

                const MoneyType newReward = computeFinalReward(bubble.position,
                                                               pt.getComputedRewardByBubbleType(bubble.type) * 10u,
                                                               /* combo */ 1);

                statExplosionRevenue(newReward);

                popWithRewardAndReplaceBubble(newReward,
                                              /* byHand */ false,
                                              bubble,
                                              /* combo */ 1,
                                              /* popSoundOverlap */ false);

                return ControlFlow::Continue;
            });
        }
    }

    ////////////////////////////////////////////////////////////
    void popWithRewardAndReplaceBubble(MoneyType reward, const bool byHand, Bubble& bubble, int xCombo, bool popSoundOverlap)
    {
        if (byHand && findCatMouse() != nullptr)
            reward = static_cast<MoneyType>(
                sf::base::ceil(static_cast<float>(reward) * pt.psvPPMouseCatGlobalBonusMult.currentValue()));
        else if (!byHand && findCatEngi() != nullptr)
            reward = static_cast<MoneyType>(
                sf::base::ceil(static_cast<float>(reward) * pt.psvPPEngiCatGlobalBonusMult.currentValue()));

        Shrine* collectorShrine = nullptr;
        for (Shrine& shrine : pt.shrines)
        {
            const auto diff = bubble.position - shrine.position;

            // if bubble is not in shrine range, continue
            if (diff.length() > shrine.getRange())
                continue;

            collectorShrine = &shrine;
            shrine.collectedReward += reward;
            shrine.textStatusShakeEffect.bump(rng, 1.5f);

            spawnParticlesWithHue(wrapHue(shrine.getHue() + 40.f),
                                  6,
                                  shrine.getDrawPosition(),
                                  ParticleType::Fire,
                                  rng.getF(0.25f, 0.6f),
                                  0.75f);

            spawnParticlesWithHue(shrine.getHue(), 6, shrine.getDrawPosition(), ParticleType::Shrine, rng.getF(0.6f, 1.f), 0.5f);

            spawnParticle(ParticleData{.position      = bubble.position,
                                       .velocity      = -diff.normalized() * 0.5f,
                                       .scale         = 1.5f,
                                       .accelerationY = 0.f,
                                       .opacity       = 1.f,
                                       .opacityDecay  = 0.00135f + (shrine.getRange() - diff.length()) / 22000.f,
                                       .rotation      = 0.f,
                                       .torque        = 0.f},
                          /* hue */ 0.f,
                          ParticleType::Bubble);
        }

        popBubbleImpl(byHand,
                      bubble.type,
                      reward,
                      xCombo,
                      bubble.position,
                      collectorShrine == nullptr ? bubble.position : collectorShrine->getDrawPosition(),
                      popSoundOverlap);

        if (collectorShrine == nullptr)
        {
            pt.money += reward;
            moneyTextShakeEffect.bump(rng, 1.f + static_cast<float>(combo) * 0.1f);
        }

        bubble = makeRandomBubble(pt, rng, pt.getMapLimit(), 0.f);
        bubble.position.y -= bubble.radius;
    };

    ////////////////////////////////////////////////////////////
    void gameLoopCheats()
    {
        // TODO P1: remove or enable via flag

        if (keyDown(sf::Keyboard::Key::F4))
        {
            pt.comboPurchased = true;
            pt.mapPurchased   = true;
        }
        else if (keyDown(sf::Keyboard::Key::F5))
        {
            pt.money = 1'000'000'000u;
        }
        else if (keyDown(sf::Keyboard::Key::F6))
        {
            pt.money += 15u;
        }
        else if (keyDown(sf::Keyboard::Key::F7))
        {
            pt.prestigePoints += 15u;
        }
    }

    ////////////////////////////////////////////////////////////
    void turnBubbleNormal(Bubble& bubble)
    {
        bubble.type     = BubbleType::Normal;
        bubble.rotation = 0.f;
        bubble.hueMod   = 0.f;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateBubbles(const float deltaTimeMs)
    {
        for (Bubble& bubble : pt.bubbles)
        {
            if (bubble.type == BubbleType::Bomb)
                bubble.rotation += deltaTimeMs * 0.01f;

            if (bubble.type == BubbleType::Star)
                bubble.hueMod += deltaTimeMs * 0.1f;

            auto& pos = bubble.position;

            if (pt.windPurchased && pt.windEnabled)
            {
                bubble.velocity.x += 0.00055f * deltaTimeMs;
                bubble.velocity.y += 0.00055f * deltaTimeMs;
            }

            pos += bubble.velocity * deltaTimeMs;

            const float radius = bubble.radius;

            if (pos.x - radius > pt.getMapLimit())
                pos.x = -radius;
            else if (pos.x + radius < 0.f)
                pos.x = pt.getMapLimit() + radius;

            if (pos.y - radius > boundaries.y)
            {
                pos.x = rng.getF(0.f, pt.getMapLimit());
                pos.y = -radius;

                bubble.velocity.y = pt.windEnabled ? 0.2f : 0.05f;

                if (sf::base::fabs(bubble.velocity.x) > 0.04f)
                    bubble.velocity.x = 0.04f;

                turnBubbleNormal(bubble);
            }
            else if (pos.y + radius < 0.f)
            {
                turnBubbleNormal(bubble);
            }

            bubble.velocity.y += 0.00005f * deltaTimeMs;
        }
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static sf::Vector2f getCatRangeCenter(const Cat& cat)
    {
        return cat.position + CatConstants::rangeOffsets[asIdx(cat.type)];
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionNormal(const float /* deltaTimeMs */, Cat& cat)
    {
        const auto maxCooldown = pt.getComputedCooldownByCatType(cat.type);
        const auto range       = pt.getComputedRangeByCatType(cat.type);
        const auto [cx, cy]    = getCatRangeCenter(cat);

        const auto normalCatPopBubble = [&](Bubble& bubble)
        {
            cat.pawPosition = bubble.position;
            cat.pawOpacity  = 255.f;
            cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

            const Cat* mouseCat = findCatMouse();

            const bool inMouseCatRange = mouseCat != nullptr && (mouseCat->position - cat.position).lengthSquared() <=
                                                                    pt.getComputedSquaredRangeByCatType(CatType::Mouse);

            const int comboMult = inMouseCatRange ? pt.mouseCatCombo : 1;

            popWithRewardAndReplaceBubble(computeFinalReward(bubble.position,
                                                             pt.getComputedRewardByBubbleType(bubble.type),
                                                             /* combo */ comboMult),
                                          /* byHand */ false,
                                          bubble,
                                          /* combo */ comboMult,
                                          /* popSoundOverlap */ true);

            cat.textStatusShakeEffect.bump(rng, 1.5f);
            ++cat.hits;

            cat.cooldown.value = maxCooldown;
        };

        if (!pt.smartCatsPurchased)
        {
            if (Bubble* b = pickRandomBubbleInRadius({cx, cy}, range))
                normalCatPopBubble(*b);

            return;
        }

        const auto pickAny = [&](const auto... types) -> Bubble*
        {
            return pickRandomBubbleInRadiusMatching({cx, cy},
                                                    range,
                                                    [&](Bubble& b) { return ((b.type == types) || ...); });
        };

        if (!pt.geniusCatsPurchased)
        {
            if (Bubble* specialBubble = pickAny(BubbleType::Star, BubbleType::Bomb))
                normalCatPopBubble(*specialBubble);
            else if (Bubble* b = pickRandomBubbleInRadius({cx, cy}, range))
                normalCatPopBubble(*b);

            return;
        }

        if (Bubble* bBomb = pickAny(BubbleType::Bomb); bBomb != nullptr && !pt.geniusCatIgnoreBombBubbles)
            normalCatPopBubble(*bBomb);
        else if (Bubble* bStar = pickAny(BubbleType::Star); bStar != nullptr && !pt.geniusCatIgnoreStarBubbles)
            normalCatPopBubble(*bStar);
        else if (Bubble* bNormal = pickAny(BubbleType::Normal); bNormal != nullptr && !pt.geniusCatIgnoreNormalBubbles)
            normalCatPopBubble(*bNormal);
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionUni(const float /* deltaTimeMs */, Cat& cat)
    {
        const auto maxCooldown = pt.getComputedCooldownByCatType(cat.type);
        const auto range       = pt.getComputedRangeByCatType(cat.type);

        Bubble* b = pickRandomBubbleInRadiusMatching(getCatRangeCenter(cat),
                                                     range,
                                                     [&](Bubble& bubble) { return bubble.type == BubbleType::Normal; });

        if (b == nullptr)
            return;

        Bubble& bubble = *b;

        cat.pawPosition = bubble.position;
        cat.pawOpacity  = 255.f;
        cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

        bubble.type       = BubbleType::Star;
        bubble.hueMod     = rng.getF(0.f, 360.f);
        bubble.velocity.y = rng.getF(-0.1f, -0.05f);
        sounds.shine.setPosition({bubble.position.x, bubble.position.y});
        playSound(sounds.shine);

        spawnParticles(4, bubble.position, ParticleType::Star, 0.5f, 0.35f);

        cat.textStatusShakeEffect.bump(rng, 1.5f);
        ++cat.hits;

        cat.cooldown.value = maxCooldown;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionDevil(const float /* deltaTimeMs */, Cat& cat)
    {
        const auto maxCooldown = pt.getComputedCooldownByCatType(cat.type);
        const auto range       = pt.getComputedRangeByCatType(cat.type);

        Bubble* b = pickRandomBubbleInRadius(getCatRangeCenter(cat), range);
        if (b == nullptr)
            return;

        Bubble& bubble = *b;

        cat.pawPosition = bubble.position;
        cat.pawOpacity  = 255.f;
        cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

        bubble.type = BubbleType::Bomb;
        bubble.velocity.y += rng.getF(0.1f, 0.2f);
        sounds.makeBomb.setPosition({bubble.position.x, bubble.position.y});
        playSound(sounds.makeBomb);

        spawnParticles(8, bubble.position, ParticleType::Fire, 1.25f, 0.35f);

        cat.textStatusShakeEffect.bump(rng, 1.5f);
        ++cat.hits;

        cat.cooldown.value = maxCooldown;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionWitch(const float /* deltaTimeMs */, Cat& cat) // TODO P1: change
    {
        const auto maxCooldown = pt.getComputedCooldownByCatType(cat.type);
        const auto range       = pt.getComputedRangeByCatType(cat.type);
        const auto [cx, cy]    = getCatRangeCenter(cat);

        sf::base::U32 witchHits = 0u;
        bool          pawSet    = false;

        for (auto& otherCat : pt.cats)
        {
            if (otherCat.type == CatType::Witch)
                continue;

            if ((otherCat.position - cat.position).length() > range)
                continue;

            otherCat.cooldown.value = pt.getComputedCooldownByCatType(cat.type);
            ++witchHits;

            if (!pawSet && rng.getF(0.f, 100.f) > 50.f)
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
            playSound(sounds.hex);

            cat.textStatusShakeEffect.bump(rng, 1.5f);
            cat.hits += witchHits;
        }

        cat.cooldown.value = maxCooldown;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionAstro(const float /* deltaTimeMs */, Cat& cat)
    {
        const auto [cx, cy] = getCatRangeCenter(cat);

        if (cat.astroState.hasValue())
            return;

        sounds.launch.setPosition({cx, cy});
        playSound(sounds.launch, /* overlap */ true);

        ++cat.hits;
        cat.astroState.emplace(/* startX */ cat.position.x, /* velocityX */ 0.f, /* wrapped */ false);
        --cat.position.x;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionWizard(const float deltaTimeMs, Cat& cat)
    {
        if (!pt.absorbingWisdom)
            return;

        const auto maxCooldown  = pt.getComputedCooldownByCatType(cat.type);
        const auto range        = pt.getComputedRangeByCatType(cat.type);
        const auto [cx, cy]     = getCatRangeCenter(cat);
        const auto drawPosition = cat.getDrawPosition();

        Bubble* starBubble = nullptr;

        const auto findRotatedStarBubble = [&](Bubble& bubble)
        {
            if (bubble.type != BubbleType::Star || bubble.rotation == 0.f)
                return ControlFlow::Continue;

            starBubble = &bubble;
            return ControlFlow::Break;
        };

        forEachBubbleInRadius({cx, cy}, range, findRotatedStarBubble);

        if (starBubble == nullptr)
            starBubble = pickRandomBubbleInRadiusMatching({cx, cy},
                                                          range,
                                                          [&](Bubble& bubble) { return bubble.type == BubbleType::Star; });

        if (starBubble == nullptr)
            return;

        Bubble& bubble = *starBubble;

        cat.pawPosition = bubble.position;
        cat.pawOpacity  = 255.f;
        cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

        bubble.rotation += deltaTimeMs * 0.025f;
        spawnParticlesWithHue(230.f, 1, bubble.position, ParticleType::Star, 0.5f, 0.35f);

        if (bubble.rotation >= sf::base::tau)
        {
            const auto wisdomReward = pt.getComputedRewardByBubbleType(bubble.type);

            if (profile.showTextParticles)
            {
                auto& tp = textParticles.emplace_back(
                    TextParticle{.buffer = {},
                                 .data   = {.position      = {drawPosition.x, drawPosition.y - 10.f},
                                            .velocity      = rng.getVec2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.425f,
                                            .scale         = 1.25f,
                                            .accelerationY = 0.0042f,
                                            .opacity       = 1.f,
                                            .opacityDecay  = 0.00175f,
                                            .rotation      = 0.f,
                                            .torque        = rng.getF(-0.002f, 0.002f)}});

                std::snprintf(tp.buffer, sizeof(tp.buffer), "+%llu WP", wisdomReward);
            }

            // TODO P1: change sound
            sounds.pop.setPosition({bubble.position.x, bubble.position.y});
            sounds.pop.setPitch(1.f);
            playSound(sounds.pop);

            spawnParticlesWithHue(230.f, 16, bubble.position, ParticleType::Star, 0.5f, 0.35f);

            pt.wisdom += wisdomReward;
            turnBubbleNormal(bubble);

            cat.cooldown.value = maxCooldown;
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionMouse(const float /* deltaTimeMs */, Cat& cat)
    {
        const auto maxCooldown = pt.getComputedCooldownByCatType(cat.type);
        const auto range       = pt.getComputedRangeByCatType(cat.type);

        Bubble* b = pickRandomBubbleInRadius(cat.position, range);
        if (b == nullptr)
            return;

        Bubble& bubble = *b;

        cat.pawPosition = bubble.position;
        cat.pawOpacity  = 255.f;

        addCombo(pt.mouseCatCombo, pt.mouseCatComboCountdown);

        const auto savedBubblePos = bubble.position;
        const auto reward         = computeFinalReward(savedBubblePos,
                                               pt.getComputedRewardByBubbleType(bubble.type),
                                               /* combo */ pt.mouseCatCombo);

        popWithRewardAndReplaceBubble(reward,
                                      /* byHand */ true,
                                      bubble,
                                      /* combo */ pt.mouseCatCombo,
                                      /* popSoundOverlap */ true);

        if (pt.multiPopEnabled)
            forEachBubbleInRadius(savedBubblePos,
                                  pt.getComputedMultiPopRange(),
                                  [&](Bubble& otherBubble)
            {
                if (&otherBubble != &bubble)
                    popWithRewardAndReplaceBubble(reward,
                                                  /* byHand */ true,
                                                  otherBubble,
                                                  /* combo */ pt.mouseCatCombo,
                                                  /* popSoundOverlap */ false);

                return ControlFlow::Continue;
            });

        cat.textStatusShakeEffect.bump(rng, 1.5f);
        ++cat.hits;

        cat.cooldown.value = maxCooldown;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionEngi(const float /* deltaTimeMs */, Cat& cat)
    {
        const auto maxCooldown  = pt.getComputedCooldownByCatType(cat.type);
        const auto range        = pt.getComputedRangeByCatType(cat.type);
        const auto rangeSquared = range * range;

        bool anyCatHit = false;

        for (Cat& otherCat : pt.cats)
        {
            if (otherCat.type == CatType::Engi)
                continue;

            if ((otherCat.position - cat.position).lengthSquared() > rangeSquared)
                continue;

            anyCatHit = true;

            spawnParticles(8, otherCat.getDrawPosition(), ParticleType::Cog, 0.25f, 0.5f);

            // TODO P1: change sound
            sounds.rocket.setPosition({otherCat.position.x, otherCat.position.y});
            playSound(sounds.woosh, /* overlap */ false);

            otherCat.boostCountdown.value = 1500.f;
        }

        if (anyCatHit)
        {
            cat.textStatusShakeEffect.bump(rng, 1.5f);
            ++cat.hits;
        }

        cat.cooldown.value = maxCooldown;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActionRepulso(const float /* deltaTimeMs */, Cat& cat)
    {
        const auto maxCooldown = pt.getComputedCooldownByCatType(cat.type);
        const auto range       = pt.getComputedRangeByCatType(cat.type);

        if (pt.repulsoCatConverterEnabled && !pt.repulsoCatIgnoreNormalBubbles)
        {
            Bubble* b = pickRandomBubbleInRadiusMatching(cat.position,
                                                         range,
                                                         [&](Bubble& bubble) { return bubble.type != BubbleType::Star; });

            if (b != nullptr && rng.getF(0.f, 100.f) < pt.psvPPRepulsoCatConverterChance.currentValue())
            {
                b->type   = BubbleType::Star;
                b->hueMod = rng.getF(0.f, 360.f);
                spawnParticles(2, b->position, ParticleType::Star, 0.5f, 0.35f);

                cat.textStatusShakeEffect.bump(rng, 1.5f);
                ++cat.hits;
            }
        }

        cat.cooldown.value = maxCooldown;
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActions(const float deltaTimeMs)
    {
        for (Cat& cat : pt.cats)
        {
            // Keep cat in boundaries
            const float catRadius = cat.getRadius();

            // Keep cats away from shrine of clicking
            // Buff cats in shrine of automation
            for (Shrine& shrine : pt.shrines)
            {
                if (shrine.type == ShrineType::Clicking && shrine.isActive())
                {
                    const auto diff = (shrine.position - cat.position);
                    if (diff.length() < shrine.getRange() * 1.35f)
                    {
                        const auto strength = (shrine.getRange() * 1.35f - diff.length()) * 0.00125f * deltaTimeMs;
                        cat.position -= diff.normalized() * strength;
                    }
                }
                else if (shrine.type == ShrineType::Automation && shrine.isActive())
                {
                    const auto diff = (shrine.position - cat.position);
                    if (diff.length() < shrine.getRange())
                    {
                        cat.boostCountdown.value = 250.f;
                    }
                }
            }

            if (!cat.astroState.hasValue())
            {
                cat.position.x = sf::base::clamp(cat.position.x, catRadius, pt.getMapLimit() - catRadius);
                cat.position.y = sf::base::clamp(cat.position.y, catRadius, boundaries.y - catRadius);
            }

            const auto maxCooldown  = pt.getComputedCooldownByCatType(cat.type);
            const auto range        = pt.getComputedRangeByCatType(cat.type);
            const auto rangeSquared = range * range;

            const auto drawPosition = cat.getDrawPosition();

            auto diff = cat.pawPosition - drawPosition - sf::Vector2f{-25.f, 25.f};
            cat.pawPosition -= diff * 0.01f * deltaTimeMs;
            cat.pawRotation = cat.pawRotation.rotatedTowards(sf::degrees(-45.f), deltaTimeMs * 0.005f);

            if (cat.cooldown.value == INFINITY)
            {
                cat.pawOpacity  = 128.f;
                cat.mainOpacity = 128.f;
            }
            else
            {
                cat.mainOpacity = 255.f;
            }

            if (cat.cooldown.value == 0.f && cat.pawOpacity > 10.f)
                cat.pawOpacity -= 0.5f * deltaTimeMs;

            cat.update(deltaTimeMs);

            const auto [cx, cy] = getCatRangeCenter(cat);

            if (cat.inspiredCountdown.value > 0.f && rng.getF(0.f, 1.f) > 0.5f)
            {
                spawnParticle({.position = drawPosition + sf::Vector2f{rng.getF(-catRadius, +catRadius), catRadius},
                               .velocity = rng.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                               .scale    = rng.getF(0.08f, 0.27f) * 0.2f,
                               .accelerationY = -0.002f,
                               .opacity       = 255.f,
                               .opacityDecay  = rng.getF(0.00025f, 0.0015f),
                               .rotation      = rng.getF(0.f, sf::base::tau),
                               .torque        = rng.getF(-0.002f, 0.002f)},
                              /* hue */ 0.f,
                              ParticleType::Star);
            }

            if (cat.boostCountdown.value > 0.f && rng.getF(0.f, 1.f) > 0.75f)
            {
                spawnParticle({.position = drawPosition + sf::Vector2f{rng.getF(-catRadius, +catRadius), catRadius - 25.f},
                               .velocity      = rng.getVec2f({-0.025f, -0.015f}, {0.025f, 0.015f}),
                               .scale         = rng.getF(0.08f, 0.27f) * 0.15f,
                               .accelerationY = -0.0015f,
                               .opacity       = 255.f,
                               .opacityDecay  = rng.getF(0.00055f, 0.0045f),
                               .rotation      = rng.getF(0.f, sf::base::tau),
                               .torque        = rng.getF(-0.002f, 0.002f)},
                              /* hue */ 0.f,
                              ParticleType::Cog);
            }

            if (cat.type == CatType::Uni)
                cat.hue += deltaTimeMs * 0.1f;

            if (cat.type == CatType::Astro && cat.astroState.hasValue())
            {
                auto& [startX, velocityX, particleTimer, wrapped] = *cat.astroState;

                particleTimer += deltaTimeMs;

                if (particleTimer >= 3.f && !cat.isCloseToStartX())
                {
                    sounds.rocket.setPosition({cx, cy});
                    playSound(sounds.rocket, /* overlap */ false);

                    spawnParticles(1, drawPosition + sf::Vector2f{56.f, 45.f}, ParticleType::Fire, 1.5f, 0.25f, 0.65f);
                    particleTimer = 0.f;
                }

                const auto astroPopAction = [&](Bubble& bubble)
                {
                    const MoneyType newReward = computeFinalReward(bubble.position,
                                                                   pt.getComputedRewardByBubbleType(bubble.type) * 20u,
                                                                   /* combo */ 1);

                    statFlightRevenue(newReward);

                    popWithRewardAndReplaceBubble(newReward,
                                                  /* byHand */ false,
                                                  bubble,
                                                  /* combo */ 1,
                                                  /* popSoundOverlap */ rng.getF(0.f, 1.f) > 0.75f);

                    cat.textStatusShakeEffect.bump(rng, 1.5f);

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
                    cat.position.x = pt.getMapLimit() + catRadius;
                    wrapped        = true;
                }

                if (wrapped && cat.position.x <= startX)
                {
                    cat.astroState.reset();
                    cat.position.x     = startX;
                    cat.cooldown.value = maxCooldown;
                }

                continue;
            }

            if (cat.type == CatType::Wizard)
            {
                if (isWizardBusy() && rng.getF(0.f, 1.f) > 0.5f)
                {
                    spawnParticle({.position = drawPosition + sf::Vector2f{rng.getF(-catRadius, +catRadius), catRadius},
                                   .velocity = rng.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                                   .scale    = rng.getF(0.08f, 0.27f) * 0.2f,
                                   .accelerationY = -0.002f,
                                   .opacity       = 255.f,
                                   .opacityDecay  = rng.getF(0.00025f, 0.0015f),
                                   .rotation      = rng.getF(0.f, sf::base::tau),
                                   .torque        = rng.getF(-0.002f, 0.002f)},
                                  /* hue */ 225.f,
                                  ParticleType::Star);
                }
            }

            if (cat.type == CatType::Mouse)
            {
                for (const Cat& otherCat : pt.cats)
                {
                    if (otherCat.type != CatType::Normal)
                        continue;

                    if ((otherCat.position - cat.position).lengthSquared() > rangeSquared)
                        continue;

                    if (rng.getF(0.f, 1.f) > 0.85f)
                        spawnParticle({.position = otherCat.getDrawPosition() +
                                                   sf::Vector2f{rng.getF(-catRadius, +catRadius), catRadius - 25.f},
                                       .velocity      = rng.getVec2f({-0.01f, -0.05f}, {0.01f, 0.05f}),
                                       .scale         = rng.getF(0.08f, 0.27f) * 0.2f,
                                       .accelerationY = -0.00015f,
                                       .opacity       = 255.f,
                                       .opacityDecay  = rng.getF(0.0003f, 0.002f),
                                       .rotation      = -0.6f,
                                       .torque        = 0.f},
                                      /* hue */ 0.f,
                                      ParticleType::Cursor);
                }
            }

            if (cat.type == CatType::Repulso)
            {
                forEachBubbleInRadius(cat.position,
                                      pt.getComputedRangeByCatType(cat.type),
                                      [&](Bubble& bubble)
                {
                    if (pt.repulsoCatIgnoreNormalBubbles && bubble.type == BubbleType::Normal)
                        return ControlFlow::Continue;

                    if (pt.repulsoCatIgnoreStarBubbles && bubble.type == BubbleType::Star)
                        return ControlFlow::Continue;

                    if (pt.repulsoCatIgnoreBombBubbles && bubble.type == BubbleType::Bomb)
                        return ControlFlow::Continue;

                    const auto diff = (cat.position - bubble.position);
                    const auto strength = (pt.getComputedRangeByCatType(cat.type) - diff.length()) * 0.00001f * deltaTimeMs;
                    bubble.velocity -= diff.normalized() * strength * (pt.windEnabled ? 10.f : 1.f);

                    return ControlFlow::Continue;
                });
            }

            if (!cat.updateCooldown(deltaTimeMs))
                continue;

            using FnPtr = void (Main::*)(const float, Cat&);

            const FnPtr fnPtrs[]{
                &Main::gameLoopUpdateCatActionNormal,
                &Main::gameLoopUpdateCatActionUni,
                &Main::gameLoopUpdateCatActionDevil,
                &Main::gameLoopUpdateCatActionWitch,
                &Main::gameLoopUpdateCatActionAstro,
                &Main::gameLoopUpdateCatActionWizard,
                &Main::gameLoopUpdateCatActionMouse,
                &Main::gameLoopUpdateCatActionEngi,
                &Main::gameLoopUpdateCatActionRepulso,
            };

            static_assert(sf::base::getArraySize(fnPtrs) == nCatTypes);

            (this->*fnPtrs[asIdx(cat.type)])(deltaTimeMs, cat);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatDragging(const float deltaTimeMs, const SizeT countFingersDown, const sf::Vector2f mousePos)
    {
        if (!mBtnDown(sf::Mouse::Button::Left) && countFingersDown != 1)
        {
            if (draggedCat)
            {
                draggedCat->cooldown.value = pt.getComputedCooldownByCatType(draggedCat->type);

                playSound(sounds.drop);
                draggedCat = nullptr;
            }

            catDragPressDuration = 0.f;
            return;
        }

        if (draggedCat)
        {
            draggedCat->position       = exponentialApproach(draggedCat->position, mousePos, deltaTimeMs, 50.f);
            draggedCat->cooldown.value = INFINITY;
            return;
        }

        constexpr float catDragPressDurationMax = 100.f;

        Cat* hoveredCat = nullptr;

        // Only check for hover targets during initial press phase
        if (catDragPressDuration <= catDragPressDurationMax)
            for (Cat& cat : pt.cats)
            {
                if (cat.isAstroAndInFlight())
                    continue;

                if (cat.type == CatType::Wizard && isWizardBusy())
                    continue;

                if ((mousePos - cat.position).length() > cat.getRadius())
                    continue;

                hoveredCat = &cat;
            }

        if (hoveredCat)
        {
            catDragPressDuration += deltaTimeMs;

            if (catDragPressDuration >= catDragPressDurationMax)
            {
                draggedCat = hoveredCat;
                playSound(sounds.grab);
            }
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateShrines(const float deltaTimeMs, const sf::View& gameView)
    {
        for (SizeT i = 0u; i < pt.psvShrineActivation.nPurchases; ++i)
        {
            for (Shrine& shrine : pt.shrines)
            {
                if (shrine.tcActivation.hasValue() || shrine.type != static_cast<ShrineType>(i))
                    continue;

                shrine.tcActivation.emplace(TargetedCountdown{.startingValue = 2000.f});
                shrine.tcActivation->restart();

                sounds.earthquakeFast.setPosition({shrine.position.x, shrine.position.y});
                playSound(sounds.earthquakeFast);

                screenShakeAmount = 4.5f;
                screenShakeTimer  = 1000.f;
            }
        }

        // Should only be triggered in testing via cheats
        for (SizeT i = pt.psvShrineActivation.nPurchases; i < nShrineTypes; ++i)
            for (Shrine& shrine : pt.shrines)
                if (shrine.type == static_cast<ShrineType>(i))
                    shrine.tcActivation.reset();

        for (Shrine& shrine : pt.shrines)
        {
            if (shrine.tcActivation.hasValue())
            {
                const auto cdStatus = shrine.tcActivation->updateAndStop(deltaTimeMs);

                if (cdStatus == CountdownStatusStop::Running)
                {
                    spawnParticlesWithHue(wrapHue(shrine.getHue() + 40.f),
                                          static_cast<SizeT>(1 + 12 * (1.f - shrine.tcActivation->getProgress())),
                                          shrine.getDrawPosition() + rng.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                          ParticleType::Fire,
                                          rng.getF(0.25f, 1.f),
                                          0.75f);

                    spawnParticlesWithHue(shrine.getHue(),
                                          static_cast<SizeT>(4 + 36 * (1.f - shrine.tcActivation->getProgress())),
                                          shrine.getDrawPosition() + rng.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                          ParticleType::Shrine,
                                          rng.getF(0.35f, 1.2f),
                                          0.5f);
                }
                else if (cdStatus == CountdownStatusStop::JustFinished)
                {
                    playSound(sounds.woosh);
                }
            }

            shrine.update(deltaTimeMs);

            if (!shrine.isActive())
                continue;

            forEachBubbleInRadius(shrine.position,
                                  shrine.getRange(),
                                  [&](Bubble& bubble)
            {
                const auto diff = (shrine.position - bubble.position);

                if (shrine.type == ShrineType::Magic)
                {
                    if (bubble.type == BubbleType::Star)
                    {
                        if (rng.getF(0.f, 1.f) > 0.85f)
                            spawnParticlesWithHue(230.f, 1, bubble.position, ParticleType::Star, 0.5f, 0.35f);

                        bubble.rotation += deltaTimeMs * 0.0025f;

                        if (bubble.rotation >= sf::base::tau)
                            turnBubbleNormal(bubble);
                    }
                }
                else if (shrine.type == ShrineType::Repulsion)
                {
                    const auto strength = (shrine.getRange() - diff.length()) * 0.0000015f * deltaTimeMs;
                    bubble.velocity -= diff.normalized() * strength * (pt.windEnabled ? 10.f : 1.f);
                }
                else if (shrine.type == ShrineType::Attraction)
                {
                    const auto strength = (shrine.getRange() - diff.length()) * 0.0000025f * deltaTimeMs;
                    bubble.velocity += diff.normalized() * strength * (pt.windEnabled ? 10.f : 1.f);
                }

                // magnetism
                // bubble.velocity += diff * 0.0000005f * deltaTimeMs;

                // repulsion
                // if (diff.length() < 128.f)
                //     bubble.velocity -= diff * 0.0000025f * deltaTimeMs;

                // scattering
                // bubble.velocity.x += (diff.x * 0.0000055f * deltaTimeMs);

                // other ideas:
                // - turns all bubbles into normal bubbles
                // - periodically pushes cats/bubbles away
                // - debuffs/buffs clicks/cats/combos/stars/etc
                // - only gets rewards from stars/bombs

                return ControlFlow::Continue;
            });

            if (shrine.collectedReward >= pt.getComputedRequiredRewardByShrineType(shrine.type))
            {
                if (!shrine.tcDeath.hasValue())
                {
                    shrine.tcDeath.emplace(TargetedCountdown{.startingValue = 5000.f});
                    shrine.tcDeath->restart();

                    sounds.earthquake.setPosition({shrine.position.x, shrine.position.y});
                    playSound(sounds.earthquake);

                    screenShakeAmount = 4.5f;
                }
                else
                {
                    const auto cdStatus = shrine.tcDeath->updateAndStop(deltaTimeMs);

                    if (cdStatus == CountdownStatusStop::JustFinished)
                    {
                        playSound(sounds.woosh);
                        ++pt.nShrinesCompleted;

                        const auto spawnSpecialCat = [&](const CatType catType)
                        {
                            auto& c    = spawnCat(gameView, catType, /* hue */ 0.f);
                            c.position = shrine.position;
                            ++pt.psvPerCatType[static_cast<SizeT>(catType)].nPurchases;
                        };

                        // TODO P0: handle all shrine death rewards
                        if (shrine.type == ShrineType::Magic)
                        {
                            spawnSpecialCat(CatType::Wizard);
                            doTip("TODO: wizard tip", /* maxPrestigeLevel */ UINT_MAX);
                        }
                        else if (shrine.type == ShrineType::Clicking)
                        {
                            spawnSpecialCat(CatType::Mouse);
                            doTip("TODO: mouse tip", /* maxPrestigeLevel */ UINT_MAX);
                        }
                        else if (shrine.type == ShrineType::Automation)
                        {
                            spawnSpecialCat(CatType::Engi);
                            doTip("TODO: engi tip", /* maxPrestigeLevel */ UINT_MAX);
                        }
                        else if (shrine.type == ShrineType::Repulsion)
                        {
                            spawnSpecialCat(CatType::Repulso);
                            doTip("TODO: repulso tip", /* maxPrestigeLevel */ UINT_MAX);
                        }
                    }
                    else if (cdStatus == CountdownStatusStop::Running)
                    {
                        spawnParticlesWithHue(wrapHue(shrine.getHue() + 40.f),
                                              static_cast<SizeT>(1 + 12 * shrine.getDeathProgress()),
                                              shrine.getDrawPosition() + rng.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                              ParticleType::Fire,
                                              sf::base::max(0.25f, 1.f - shrine.getDeathProgress()),
                                              0.75f);

                        spawnParticlesWithHue(shrine.getHue(),
                                              static_cast<SizeT>(4 + 36 * shrine.getDeathProgress()),
                                              shrine.getDrawPosition() + rng.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                              ParticleType::Shrine,
                                              sf::base::max(0.35f, 1.2f - shrine.getDeathProgress()),
                                              0.5f);
                    }
                }
            }
        }

        std::erase_if(pt.shrines, [](const Shrine& shrine) { return shrine.getDeathProgress() >= 1.f; });
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateMana(const float deltaTimeMs)
    {
        Cat* wizardCat = findCatWizard();

        if (wizardCat == nullptr)
            return;

        //
        // Mana
        if (pt.mana < pt.getComputedMaxMana())
            pt.manaTimer += deltaTimeMs;
        else
            pt.manaTimer = 0.f;

        if (pt.manaTimer >= pt.getComputedManaCooldown())
        {
            pt.manaTimer = 0.f;

            if (pt.mana < pt.getComputedMaxMana())
                pt.mana += 1u;
        }

        //
        // Arcane aura spell
        if (pt.arcaneAuraTimer > 0.f)
        {
            pt.arcaneAuraTimer -= deltaTimeMs;
            pt.arcaneAuraTimer = sf::base::max(pt.arcaneAuraTimer, 0.f);

            for (SizeT i = 0u; i < 8u; ++i)
                spawnParticlesWithHueNoGravity(230.f,
                                               1,
                                               rng.getPointInCircle(wizardCat->position,
                                                                    pt.getComputedRangeByCatType(CatType::Wizard)),
                                               ParticleType::Star,
                                               0.15f,
                                               0.05f);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateMilestones()
    {
        const auto updateMilestone = [&](const char* name, sf::base::U64& milestone)
        {
            const auto oldMilestone = milestone;

            milestone = sf::base::min(milestone, pt.statsTotal.secondsPlayed);

            if (milestone != oldMilestone)
            {
                const auto [h, m, s] = formatTime(milestone);

                ImGuiToast toast{ImGuiToastType::None, 3000};
                toast.setTitle("Milestone reached!");
                toast.setContent("'%s' at %lluh %llum %llus", name, h, m, s);

                ImGui::InsertNotification(toast);
                playSound(sounds.notification);
            }
        };

        const auto nCatNormal = pt.getCatCountByType(CatType::Normal);
        const auto nCatUni    = pt.getCatCountByType(CatType::Uni);
        const auto nCatDevil  = pt.getCatCountByType(CatType::Devil);
        const auto nCatAstro  = pt.getCatCountByType(CatType::Astro);

        if (nCatNormal >= 1)
            updateMilestone("1st Cat", pt.milestones.firstCat);

        if (nCatUni >= 1)
            updateMilestone("1st Unicat", pt.milestones.firstUnicat);

        if (nCatDevil >= 1)
            updateMilestone("1st Devilcat", pt.milestones.firstDevilcat);

        if (nCatAstro >= 1)
            updateMilestone("1st Astrocat", pt.milestones.firstAstrocat);

        if (nCatNormal >= 5)
            updateMilestone("5th Cat", pt.milestones.fiveCats);

        if (nCatUni >= 5)
            updateMilestone("5th Unicat", pt.milestones.fiveUnicats);

        if (nCatDevil >= 5)
            updateMilestone("5th Devilcat", pt.milestones.fiveDevilcats);

        if (nCatAstro >= 5)
            updateMilestone("5th Astrocat", pt.milestones.fiveAstrocats);

        if (nCatNormal >= 10)
            updateMilestone("10th Cat", pt.milestones.tenCats);

        if (nCatUni >= 10)
            updateMilestone("10th Unicat", pt.milestones.tenUnicats);

        if (nCatDevil >= 10)
            updateMilestone("10th Devilcat", pt.milestones.tenDevilcats);

        if (nCatAstro >= 10)
            updateMilestone("10th Astrocat", pt.milestones.tenAstrocats);

        if (pt.psvBubbleValue.nPurchases >= 1)
            updateMilestone("Prestige Level 1", pt.milestones.prestigeLevel1);

        if (pt.psvBubbleValue.nPurchases >= 2)
            updateMilestone("Prestige Level 2", pt.milestones.prestigeLevel2);

        if (pt.psvBubbleValue.nPurchases >= 3)
            updateMilestone("Prestige Level 3", pt.milestones.prestigeLevel3);

        if (pt.psvBubbleValue.nPurchases >= 4)
            updateMilestone("Prestige Level 4", pt.milestones.prestigeLevel4);

        if (pt.psvBubbleValue.nPurchases >= 5)
            updateMilestone("Prestige Level 5", pt.milestones.prestigeLevel5);

        if (pt.psvBubbleValue.nPurchases >= 10)
            updateMilestone("Prestige Level 10", pt.milestones.prestigeLevel10);

        if (pt.psvBubbleValue.nPurchases >= 15)
            updateMilestone("Prestige Level 15", pt.milestones.prestigeLevel15);

        if (pt.psvBubbleValue.nPurchases >= 19)
            updateMilestone("Prestige Level 19 (MAX)", pt.milestones.prestigeLevel20);

        if (pt.statsTotal.bubblesPoppedRevenue >= 10'000)
            updateMilestone("$10.000 Revenue", pt.milestones.revenue10000);

        if (pt.statsTotal.bubblesPoppedRevenue >= 100'000)
            updateMilestone("$100.000 Revenue", pt.milestones.revenue100000);

        if (pt.statsTotal.bubblesPoppedRevenue >= 1'000'000)
            updateMilestone("$1.000.000 Revenue", pt.milestones.revenue1000000);

        if (pt.statsTotal.bubblesPoppedRevenue >= 10'000'000)
            updateMilestone("$10.000.000 Revenue", pt.milestones.revenue10000000);

        if (pt.statsTotal.bubblesPoppedRevenue >= 100'000'000)
            updateMilestone("$100.000.000 Revenue", pt.milestones.revenue100000000);

        if (pt.statsTotal.bubblesPoppedRevenue >= 1'000'000'000)
            updateMilestone("$1.000.000.000 Revenue", pt.milestones.revenue1000000000);

        for (SizeT i = 0u; i < pt.nShrinesCompleted; ++i)
        {
            const char* shrineName = i >= pt.getMapLimitIncreases() ? "Shrine Of ???" : shrineNames[i];
            updateMilestone(shrineName, pt.milestones.shrineCompletions[i]);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawBubbles()
    {
        sf::Sprite tempSprite;

        const sf::FloatRect bubbleRects[3]{txrBubble, txrBubbleStar, txrBomb};

        for (SizeT i = 0u; i < pt.bubbles.size(); ++i)
        {
            Bubble& bubble = pt.bubbles[i];

            if (!bubbleCullingBoundaries.isInside(bubble.position))
                continue;

            constexpr float radiusToScale = 1.f / 256.f;

            tempSprite.position = bubble.position;
            tempSprite.scale    = {bubble.radius * radiusToScale, bubble.radius * radiusToScale};
            tempSprite.rotation = sf::radians(bubble.rotation);

            tempSprite.textureRect = bubbleRects[asIdx(bubble.type)];
            tempSprite.origin      = tempSprite.textureRect.size / 2.f;
            tempSprite.scale *= bubble.type == BubbleType::Bomb ? 1.65f : 1.f;

            if (bubble.type == BubbleType::Star)
            {
                tempSprite.color = hueColor(wrapHue(bubble.hueMod), 255);
            }
            else
            {
                constexpr float hueRange = 60.f;
                tempSprite.color = hueColor(wrapHue(sf::base::fmod(static_cast<float>(i) * 2.f - hueRange / 2.f, hueRange)),
                                            255);
            }

            bubbleDrawableBatch.add(tempSprite);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawCats(const sf::Vector2f mousePos)
    {
        const sf::FloatRect* const normalCatTxr = pt.geniusCatsPurchased  ? &txrGeniusCat
                                                  : pt.smartCatsPurchased ? &txrSmartCat
                                                                          : &txrCat;

        const sf::FloatRect* const astroCatTxr = pt.astroCatInspirePurchased ? &txrAstroCatWithFlag : &txrAstroCat;

        const sf::FloatRect* const catTxrsByType[] = {
            normalCatTxr, // Normal
            &txrUniCat,   // Uni
            &txrDevilCat, // Devil
            &txrWitchCat, // Witch
            astroCatTxr,  // Astro

            &txrWizardCat,  // Wizard
            &txrMouseCat,   // Mouse
            &txrEngiCat,    // Engi
            &txrRepulsoCat, // Repulso
        };

        static_assert(sf::base::getArraySize(catTxrsByType) == nCatTypes);

        const sf::FloatRect* const catPawTxrsByType[] = {
            &txrCatPaw,      // Normal
            &txrUniCatPaw,   // Uni
            &txrDevilCatPaw, // Devil
            &txrWitchCatPaw, // Witch
            &txrWhiteDot,    // Astro

            &txrWizardCatPaw,  // Wizard
            &txrMouseCatPaw,   // Mouse
            &txrEngiCatPaw,    // Engi
            &txrRepulsoCatPaw, // Repulso
        };

        static_assert(sf::base::getArraySize(catPawTxrsByType) == nCatTypes);

        bool anyCatHovered = false;

        for (Cat& cat : pt.cats)
        {
            U8 rangeInnerAlpha = 0u;

            if (!anyCatHovered && &cat != draggedCat && !cat.isAstroAndInFlight() &&
                (mousePos - cat.position).lengthSquared() <= cat.getRadiusSquared() && !mBtnDown(sf::Mouse::Button::Left))
            {
                anyCatHovered   = true;
                rangeInnerAlpha = 75u;
            }

            const auto& catTxr    = *catTxrsByType[asIdx(cat.type)];
            const auto& catPawTxr = *catPawTxrsByType[asIdx(cat.type)];

            const auto maxCooldown  = pt.getComputedCooldownByCatType(cat.type);
            const auto cooldownDiff = cat.cooldown.value;

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

            const auto range    = pt.getComputedRangeByCatType(cat.type);
            const auto alpha    = static_cast<U8>(cat.mainOpacity);
            const auto catColor = hueColor(wrapHue(cat.hue), alpha);

            const auto circleAlpha = cat.cooldown.value < 0.f
                                         ? static_cast<U8>(0u)
                                         : static_cast<U8>(255.f - (cat.cooldown.value / maxCooldown * 225.f));

            const auto circleColor = CatConstants::colors[asIdx(cat.type)].withHueMod(cat.hue).withLightness(0.75f);
            const sf::Color outlineColor = circleColor.withLightness(0.25f);

            // TODO P2: (lib) make it possible to draw a circle directly via batching without any of this stuff,
            // no need to preallocate a circle shape before, have a reusable vertex buffer in the batch itself
            circleShapeBuffer.position = getCatRangeCenter(cat);
            circleShapeBuffer.origin   = {range, range};
            circleShapeBuffer.setPointCount(static_cast<unsigned int>(range / 3.f));
            circleShapeBuffer.setRadius(range);
            circleShapeBuffer.setOutlineColor(circleColor.withAlpha(rangeInnerAlpha == 0u ? circleAlpha : 255u));
            circleShapeBuffer.setFillColor(circleShapeBuffer.getOutlineColor().withAlpha(rangeInnerAlpha));
            cpuDrawableBatch.add(circleShapeBuffer);

            cpuDrawableBatch.add(
                sf::Sprite{.position    = cat.getDrawPosition(),
                           .scale       = {0.2f, 0.2f},
                           .origin      = catTxr.size / 2.f,
                           .rotation    = sf::radians(catRotation),
                           .textureRect = catTxr,
                           .color       = catColor});

            cpuDrawableBatch.add(
                sf::Sprite{.position    = cat.pawPosition,
                           .scale       = {0.1f, 0.1f},
                           .origin      = catPawTxr.size / 2.f,
                           .rotation    = cat.type == CatType::Mouse ? sf::radians(-0.6f) : cat.pawRotation,
                           .textureRect = catPawTxr,
                           .color       = catColor.withAlpha(static_cast<U8>(cat.pawOpacity))});

            if (profile.showCatText)
            {
                textNameBuffer.setString(shuffledCatNamesPerType[asIdx(cat.type)][cat.nameIdx]);
                textNameBuffer.position = cat.position + sf::Vector2f{0.f, 48.f};
                textNameBuffer.origin   = textNameBuffer.getLocalBounds().size / 2.f;
                textNameBuffer.scale    = sf::Vector2f{0.5f, 0.5f};
                textNameBuffer.setOutlineColor(outlineColor);
                cpuDrawableBatch.add(textNameBuffer);

                std::string actionString = std::to_string(cat.hits) + " " + CatConstants::actionNames[asIdx(cat.type)];
                if (cat.type == CatType::Mouse)
                    actionString += " (x" + std::to_string(pt.mouseCatCombo + 1) + ")";

                textStatusBuffer.setString(actionString);
                textStatusBuffer.position = cat.position + sf::Vector2f{0.f, 68.f};
                textStatusBuffer.origin   = textStatusBuffer.getLocalBounds().size / 2.f;
                textStatusBuffer.setOutlineColor(outlineColor);
                cat.textStatusShakeEffect.applyToText(textStatusBuffer);
                textStatusBuffer.scale *= 0.5f;
                cpuDrawableBatch.add(textStatusBuffer);
            }
        };
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawShrines(const sf::Vector2f mousePos)
    {
        Shrine* hoveredShrine = nullptr;

        for (Shrine& shrine : pt.shrines)
        {
            U8 rangeInnerAlpha = 0u;

            if (hoveredShrine == nullptr && (mousePos - shrine.position).lengthSquared() <= shrine.getRadiusSquared() &&
                !mBtnDown(sf::Mouse::Button::Left))
            {
                hoveredShrine   = &shrine;
                rangeInnerAlpha = 75u;
            }

            const float invDeathProgress = 1.f - shrine.getDeathProgress();

            const auto shrineAlpha = static_cast<U8>(remap(shrine.getActivationProgress(), 0.f, 1.f, 128.f, 255.f));
            const auto shrineColor = hueColor(shrine.getHue(), shrineAlpha);

            const auto      circleColor  = sf::Color{231u, 198u, 39u}.withHueMod(shrine.getHue()).withLightness(0.75f);
            const sf::Color outlineColor = circleColor.withLightness(0.25f);

            cpuDrawableBatch.add(
                sf::Sprite{.position = shrine.getDrawPosition(),
                           .scale    = sf::Vector2f{0.2f, 0.2f} * invDeathProgress +
                                    sf::Vector2f{1.f, 1.f} * shrine.textStatusShakeEffect.grow * 0.025f,
                           .origin      = txrShrine.size / 2.f,
                           .textureRect = txrShrine,
                           .color       = shrineColor});

            const auto range = shrine.getRange();

            // TODO P2: (lib) make it possible to draw a circle directly via batching without any of this stuff,
            // no need to preallocate a circle shape before, have a reusable vertex buffer in the batch itself
            circleShapeBuffer.position = shrine.position;
            circleShapeBuffer.origin   = {range, range};
            circleShapeBuffer.setPointCount(64);
            circleShapeBuffer.setRadius(range);
            circleShapeBuffer.setOutlineColor(circleColor);
            circleShapeBuffer.setFillColor(circleShapeBuffer.getOutlineColor().withAlpha(rangeInnerAlpha));
            cpuDrawableBatch.add(circleShapeBuffer);

            textNameBuffer.setString(shrineNames[asIdx(shrine.type)]);
            textNameBuffer.position = shrine.position + sf::Vector2f{0.f, 48.f};
            textNameBuffer.origin   = textNameBuffer.getLocalBounds().size / 2.f;
            textNameBuffer.scale    = sf::Vector2f{0.5f, 0.5f} * invDeathProgress;
            textNameBuffer.setOutlineColor(outlineColor);
            cpuDrawableBatch.add(textNameBuffer);

            if (shrine.isActive())
            {
                // TODO P1: move to member data
                static thread_local std::string shrineStatus;

                shrineStatus = "$";
                shrineStatus += toStringWithSeparators(shrine.collectedReward);
                shrineStatus += " / $";
                shrineStatus += toStringWithSeparators(pt.getComputedRequiredRewardByShrineType(shrine.type));

                textStatusBuffer.setString(shrineStatus);
            }
            else
            {
                textStatusBuffer.setString("Inactive");
            }

            textStatusBuffer.position = shrine.position + sf::Vector2f{0.f, 68.f};
            textStatusBuffer.origin   = textStatusBuffer.getLocalBounds().size / 2.f;
            textStatusBuffer.setOutlineColor(outlineColor);
            shrine.textStatusShakeEffect.applyToText(textStatusBuffer);
            textStatusBuffer.scale *= invDeathProgress;
            textStatusBuffer.scale *= 0.5f;
            cpuDrawableBatch.add(textStatusBuffer);
        };
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] sf::Vector2f getViewCenter() const
    {
        return {sf::base::clamp(gameScreenSize.x / 2.f + actualScroll * 2.f,
                                gameScreenSize.x / 2.f,
                                boundaries.x - gameScreenSize.x / 2.f),
                gameScreenSize.y / 2.f};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] CullingBoundaries getViewCullingBoundaries(const float offset) const
    {
        const sf::Vector2f viewCenter{getViewCenter()};

        return {viewCenter.x - gameScreenSize.x / 2.f + offset,
                viewCenter.x + gameScreenSize.x / 2.f - offset,
                viewCenter.y - gameScreenSize.y / 2.f + offset,
                viewCenter.y + gameScreenSize.y / 2.f - offset};
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawParticles()
    {
        if (!profile.showParticles)
            return;

        sf::Sprite tempSprite;

        const sf::FloatRect particleRects[] =
            {txrParticle, txrStarParticle, txrFireParticle, txrHexParticle, txrShrineParticle, txrMouseCatPaw, txrCogParticle};

        static_assert(sf::base::getArraySize(particleRects) == nParticleTypes);

        for (const auto& particle : particles)
        {
            if (!particleCullingBoundaries.isInside(particle.data.position))
                continue;

            particle.data.applyToTransformable(tempSprite);

            tempSprite.color       = hueColor(particle.hue, particle.data.opacityAsAlpha());
            tempSprite.textureRect = particleRects[asIdx(particle.type)];
            tempSprite.origin      = tempSprite.textureRect.size / 2.f;

            cpuDrawableBatch.add(tempSprite);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawTextParticles()
    {
        if (!profile.showTextParticles)
            return;

        sf::Text tempText{fontSuperBakery,
                          {.characterSize    = 16u,
                           .fillColor        = sf::Color::White,
                           .outlineColor     = colorBlueOutline,
                           .outlineThickness = 1.f}};

        for (const auto& textParticle : textParticles)
        {
            if (!particleCullingBoundaries.isInside(textParticle.data.position))
                continue;

            textParticle.applyToText(tempText);
            cpuDrawableBatch.add(tempText);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopTips(const float deltaTimeMs)
    {
        if (tipTimer <= 0.f)
            return;

        tipTimer -= deltaTimeMs;

        if (!profile.tipsEnabled)
            return;

        float fade = 255.f;

        if (tipTimer > 4500.f)
            fade = remap(tipTimer, 4500.f, 5000.f, 255.f, 0.f);
        else if (tipTimer < 500.f)
            fade = remap(tipTimer, 0.f, 500.f, 0.f, 255.f);

        const auto alpha = static_cast<U8>(sf::base::clamp(fade, 0.f, 255.f));

        sounds.byteSpeak.setPitch(1.6f);

        sf::Text tipText{fontSuperBakery,
                         {.position         = {},
                          .scale            = {0.5f, 0.5f},
                          .string           = tipString.substr(0,
                                                     static_cast<SizeT>(
                                                         sf::base::clamp((4100.f - tipTimer) / 25.f,
                                                                         0.f,
                                                                         static_cast<float>(tipString.size())))),
                          .characterSize    = 60u,
                          .fillColor        = sf::Color::White.withAlpha(alpha),
                          .outlineColor     = colorBlueOutline.withAlpha(alpha),
                          .outlineThickness = 2.f}};

        if (tipText.getString().getSize() < tipString.size() && tipText.getString().getSize() > 0)
            playSound(sounds.byteSpeak, /* overlap */ false);

        sf::Sprite tipSprite{.position    = {},
                             .scale       = {0.8f, 0.8f},
                             .origin      = txTipBg.getSize().toVector2f() / 2.f,
                             .textureRect = txTipBg.getRect(),
                             .color       = sf::Color::White.withAlpha(static_cast<U8>(alpha * 0.85f))};

        tipSprite.setBottomCenter({getResolution().x / 2.f / profile.hudScale, getResolution().y / profile.hudScale - 50.f});
        getWindow().draw(tipSprite, txTipBg);

        sf::Sprite tipByteSprite{.position    = {},
                                 .scale       = {0.7f, 0.7f},
                                 .origin      = txTipByte.getSize().toVector2f() / 2.f,
                                 .textureRect = txTipByte.getRect(),
                                 .color       = sf::Color::White.withAlpha(alpha)};

        tipByteSprite.setCenter(tipSprite.getCenterRight() + sf::Vector2f{-40.f, 0.f});
        getWindow().draw(tipByteSprite, txTipByte);

        tipText.setTopLeft(tipSprite.getTopLeft() + sf::Vector2f{45.f, 65.f});
        getWindow().draw(tipText);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool gameLoop()
    {
        if (mustRecreateWindow)
        {
            mustRecreateWindow = false;

            const sf::Vector2u newResolution = profile.resWidth == sf::Vector2u{} ? getReasonableWindowSize(0.9f)
                                                                                  : profile.resWidth;

            const bool takesAllScreen = newResolution == sf::VideoModeUtils::getDesktopMode().size;

            optWindow.emplace(
                sf::WindowSettings{.size            = newResolution,
                                   .title           = "BubbleByte " BUBBLEBYTE_VERSION_STR,
                                   .fullscreen      = !profile.windowed,
                                   .resizable       = !takesAllScreen,
                                   .closable        = !takesAllScreen,
                                   .hasTitlebar     = !takesAllScreen,
                                   .vsync           = profile.vsync,
                                   .frametimeLimit  = sf::base::clamp(profile.frametimeLimit, 60u, 144u),
                                   .contextSettings = contextSettings});

            static bool imguiInit = false;
            if (!imguiInit)
            {
                imguiInit = true;

                if (!imGuiContext.init(*optWindow))
                {
                    std::cout << "Error: ImGui context initialization failed\n";
                    return false;
                }

                fontImGuiSuperBakery  = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/superbakery.ttf", 26.f);
                fontImGuiMouldyCheese = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/mouldycheese.ttf", 26.f);
            }
        }

        auto& window = *optWindow;

        fpsClock.restart();

        sf::base::Optional<sf::Vector2f> clickPosition;

        while (const sf::base::Optional event = window.pollEvent())
        {
            imGuiContext.processEvent(window, *event);

            if (sf::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return false;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
            if (const auto* e = event->getIf<sf::Event::TouchBegan>())
            {
                fingerPositions[e->finger].emplace(e->position.toVector2f());

                if (!clickPosition.hasValue())
                    clickPosition.emplace(e->position.toVector2f());
            }
            else if (const auto* e = event->getIf<sf::Event::TouchEnded>())
            {
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
                if (pt.mapPurchased && dragPosition.hasValue())
                {
                    scroll = dragPosition->x - static_cast<float>(e->position.x);
                }
            }
#pragma clang diagnostic pop
        }

        const auto deltaTime   = deltaClock.restart();
        const auto deltaTimeMs = static_cast<float>(deltaTime.asMilliseconds());

        gameLoopCheats();

        //
        // Number of fingers
        std::vector<sf::Vector2f> downFingers;
        for (const auto maybeFinger : fingerPositions)
            if (maybeFinger.hasValue())
                downFingers.push_back(*maybeFinger);

        const auto countFingersDown = downFingers.size();

        //
        // Map scrolling via keyboard and touch
        if (pt.mapPurchased)
        {
            if (keyDown(sf::Keyboard::Key::Left))
            {
                dragPosition.reset();
                scroll -= 2.f * deltaTimeMs;
            }
            else if (keyDown(sf::Keyboard::Key::Right))
            {
                dragPosition.reset();
                scroll += 2.f * deltaTimeMs;
            }
            else if (countFingersDown == 2)
            {
                // TODO P1: check fingers distance
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
        if (dragPosition.hasValue() && countFingersDown != 2u && !mBtnDown(sf::Mouse::Button::Right))
            dragPosition.reset();

        //
        // Scrolling
        scroll = sf::base::clamp(scroll,
                                 0.f,
                                 sf::base::min(pt.getMapLimit() / 2.f - gameScreenSize.x / 2.f,
                                               (boundaries.x - gameScreenSize.x) / 2.f));

        actualScroll = exponentialApproach(actualScroll, scroll, deltaTimeMs, 75.f);

        const auto screenShake = rng.getVec2f({-screenShakeAmount, -screenShakeAmount},
                                              {screenShakeAmount, screenShakeAmount});

        const sf::Vector2f resolution = getResolution();

        const auto createScaledView = [&](const sf::Vector2f& originalSize, const sf::Vector2f& windowSize) -> sf::View
        {
            // Calculate the scale factors for both dimensions
            const float scaleX = windowSize.x / originalSize.x;
            const float scaleY = windowSize.y / originalSize.y;

            // Use the smaller scale factor to maintain aspect ratio
            const float scale = std::min(scaleX, scaleY);

            // Calculate the scaled dimensions
            const sf::Vector2f scaledSize = originalSize * scale;

            return {.center   = originalSize / 2.f,
                    .size     = originalSize,
                    .viewport = {(windowSize - scaledSize).componentWiseDiv(windowSize * 2.f),
                                 scaledSize.componentWiseDiv(windowSize)}};
        };

        sf::View gameView            = createScaledView(gameScreenSize, resolution);
        gameView.viewport.position.x = 0.f;
        gameView.center              = getViewCenter() + screenShake;

        particleCullingBoundaries = getViewCullingBoundaries(/* offset */ 0.f);
        bubbleCullingBoundaries   = getViewCullingBoundaries(/* offset */ -64.f);

        const auto windowSpaceMouseOrFingerPos = countFingersDown == 1u ? downFingers[0].toVector2i()
                                                                        : sf::Mouse::getPosition(window);

        const auto mousePos = window.mapPixelToCoords(windowSpaceMouseOrFingerPos, gameView);

        //
        // Update listener position
        listener.position = {sf::base::clamp(mousePos.x, 0.f, pt.getMapLimit()),
                             sf::base::clamp(mousePos.y, 0.f, boundaries.y),
                             0.f};
        musicBGM.setPosition(listener.position);
        (void)playbackDevice.updateListener(listener);

        //
        // Target bubble count
        const auto targetBubbleCountPerScreen = static_cast<SizeT>(
            pt.psvBubbleCount.currentValue() / (boundaries.x / gameScreenSize.x));
        const auto nScreens          = static_cast<SizeT>(pt.getMapLimit() / gameScreenSize.x) + 1;
        const auto targetBubbleCount = targetBubbleCountPerScreen * nScreens;

        //
        // Startup and bubble spawning
        const auto playReversePopAt = [this](const sf::Vector2f position)
        {
            sounds.reversePop.setPosition({position.x, position.y});
            playSound(sounds.reversePop, /* overlap */ false);
        };

        if (splashCountdown.updateAndStop(deltaTimeMs) == CountdownStatusStop::AlreadyFinished)
        {
            if (catRemoveTimer.updateAndLoop(deltaTimeMs) == CountdownStatusLoop::Looping)
            {
                if (inPrestigeTransition && !pt.cats.empty())
                {
                    const auto cPos = pt.cats.back().position;
                    pt.cats.pop_back();

                    spawnParticles(24, cPos, ParticleType::Star, 1.f, 0.5f);
                    playReversePopAt(cPos);
                }

                if (inPrestigeTransition && !pt.shrines.empty())
                {
                    const auto cPos = pt.shrines.back().position;
                    pt.shrines.pop_back();

                    spawnParticles(24, cPos, ParticleType::Star, 1.f, 0.5f);
                    playReversePopAt(cPos);
                }
            }

            if (bubbleSpawnTimer.updateAndLoop(deltaTimeMs) == CountdownStatusLoop::Looping)
            {
                if (inPrestigeTransition)
                {
                    if (!pt.bubbles.empty())
                    {
                        const SizeT times = pt.bubbles.size() > 500 ? 25 : 1;

                        for (SizeT i = 0; i < times; ++i)
                        {
                            const auto bPos = pt.bubbles.back().position;
                            pt.bubbles.pop_back();

                            spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
                            playReversePopAt(bPos);
                        }
                    }
                }
                else
                {
                    if (pt.bubbles.size() < targetBubbleCount)
                    {
                        const SizeT times = (targetBubbleCount - pt.bubbles.size()) > 500 ? 25 : 1;

                        for (SizeT i = 0; i < times; ++i)
                        {
                            const auto bPos = pt.bubbles
                                                  .emplace_back(makeRandomBubble(pt, rng, pt.getMapLimit(), boundaries.y))
                                                  .position;

                            spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
                            playReversePopAt(bPos);
                        }
                    }
                    else if (pt.bubbles.size() > targetBubbleCount)
                    {
                        // Should only be triggered in testing via cheats
                        pt.bubbles.resize(targetBubbleCount);
                    }
                }
            }

            // End prestige transition
            if (inPrestigeTransition && pt.cats.empty() && pt.shrines.empty() && pt.bubbles.empty())
            {
                inPrestigeTransition = false;
                pt.money             = 0u;
                splashCountdown.restart();
                playSound(sounds.byteMeow);
            }

            // Spawn shrines if required
            if (!inPrestigeTransition)
                pt.spawnAllShrinesIfNeeded();
        }

        //
        //
        // Update spatial partitioning
        spatialGrid.clear();
        spatialGrid.populate(pt.bubbles);

        gameLoopUpdateBubbles(deltaTimeMs);

        bool anyBubblePoppedByClicking = false;

        if (clickPosition.hasValue())
        {
            const auto clickPos = window.mapPixelToCoords(clickPosition->toVector2i(), gameView);

            const auto clickAction = [&](Bubble& bubble)
            {
                if ((clickPos - bubble.position).lengthSquared() > bubble.getRadiusSquared())
                    return ControlFlow::Continue;

                // Prevent clicks around shrine of automation
                // Boosts clicks around shrine of clicking
                bool clickingBoost = false;
                {
                    for (Shrine& shrine : pt.shrines)
                    {
                        if (shrine.type == ShrineType::Automation && shrine.isActive() &&
                            (clickPos - shrine.position).length() < shrine.getRange())
                        {
                            sounds.failpop.setPosition({clickPos.x, clickPos.y});
                            playSound(sounds.failpop);

                            return ControlFlow::Break;
                        }

                        if (shrine.type == ShrineType::Clicking && shrine.isActive() &&
                            (clickPos - shrine.position).length() < shrine.getRange())
                        {
                            clickingBoost = true;
                            break;
                        }
                    }
                }

                anyBubblePoppedByClicking = true;

                if (pt.comboPurchased)
                {
                    addCombo(combo, comboCountdown);
                    comboTextShakeEffect.bump(rng, 1.f + static_cast<float>(combo) * 0.2f);
                }
                else
                {
                    combo = 1;
                }

                MoneyType reward = computeFinalReward(bubble.position, pt.getComputedRewardByBubbleType(bubble.type), combo);
                if (clickingBoost)
                    reward *= 5;

                popWithRewardAndReplaceBubble(reward, /* byHand */ true, bubble, combo, /* popSoundOverlap */ true);

                if (pt.multiPopEnabled)
                    forEachBubbleInRadius(clickPos,
                                          pt.getComputedMultiPopRange(),
                                          [&](Bubble& otherBubble)
                    {
                        if (&otherBubble != &bubble)
                            popWithRewardAndReplaceBubble(reward,
                                                          /* byHand */ true,
                                                          otherBubble,
                                                          combo,
                                                          /* popSoundOverlap */ false);

                        return ControlFlow::Continue;
                    });


                return ControlFlow::Break;
            };

            forEachBubbleInRadius(clickPos, 128.f, clickAction);
        }

        //
        // Combo failure due to timer end
        checkComboEnd(deltaTimeMs, combo, comboCountdown);
        checkComboEnd(deltaTimeMs, pt.mouseCatCombo, pt.mouseCatComboCountdown);

        //
        // Combo failure due to missed click
        if (!anyBubblePoppedByClicking && clickPosition.hasValue())
        {
            if (combo > 1)
                playSound(sounds.scratch);

            combo                = 0;
            comboCountdown.value = 0.f;
        }

        //
        // Bubble vs bubble collisions
        const unsigned int nWorkers = threadPool.getWorkerCount();
        std::latch         latch{nWorkers};

        spatialGrid.forEachUniqueIndexPair(nWorkers,
                                           latch,
                                           threadPool,
                                           [&](const SizeT bubbleIdxI, const SizeT bubbleIdxJ) __attribute__((always_inline))
        { handleBubbleCollision(deltaTimeMs, pt.bubbles[bubbleIdxI], pt.bubbles[bubbleIdxJ]); });

        latch.wait();

        //
        // Cat vs cat collisions
        for (SizeT i = 0u; i < pt.cats.size(); ++i)
            for (SizeT j = i + 1; j < pt.cats.size(); ++j)
            {
                Cat& iCat = pt.cats[i];
                Cat& jCat = pt.cats[j];

                if (draggedCat == &iCat || draggedCat == &jCat)
                    continue;

                const auto checkAstro = [this](auto& catA, auto& catB)
                {
                    if (catA.isAstroAndInFlight() && catB.type != CatType::Astro)
                    {
                        if (pt.astroCatInspirePurchased &&
                            detectCollision(catA.position, catB.position, catA.getRadius(), catB.getRadius()))
                            catB.inspiredCountdown.value = pt.getComputedInspirationDuration();

                        return true;
                    }

                    return false;
                };

                if (checkAstro(iCat, jCat))
                    continue;

                // NOLINTNEXTLINE(readability-suspicious-call-argument)
                if (checkAstro(jCat, iCat))
                    continue;

                handleCatCollision(deltaTimeMs, pt.cats[i], pt.cats[j]);
            }

        //
        // Cat vs shrine collisions
        for (Cat& cat : pt.cats)
            for (Shrine& shrine : pt.shrines)
                handleCatShrineCollision(deltaTimeMs, cat, shrine);

        gameLoopUpdateCatDragging(deltaTimeMs, countFingersDown, mousePos);
        gameLoopUpdateCatActions(deltaTimeMs);
        gameLoopUpdateShrines(deltaTimeMs, gameView);
        gameLoopUpdateMana(deltaTimeMs);

        if (screenShakeTimer > 0.f)
        {
            screenShakeTimer -= deltaTimeMs;
            screenShakeTimer = sf::base::max(0.f, screenShakeTimer);
        }

        const bool anyShrineDying = sf::base::anyOf(pt.shrines.begin(),
                                                    pt.shrines.end(),
                                                    [](const Shrine& shrine) { return shrine.tcDeath.hasValue(); });

        if (!anyShrineDying && screenShakeTimer <= 0.f && screenShakeAmount > 0.f)
        {
            screenShakeAmount -= deltaTimeMs * 0.05f;
            screenShakeAmount = sf::base::max(0.f, screenShakeAmount);
        }

        const auto updateParticleLike = [&](auto& particleLikeVec)
        {
            for (auto& particleLike : particleLikeVec)
                particleLike.data.update(deltaTimeMs);

            // TODO P2: (lib) to lib
            std::erase_if(particleLikeVec, [](const auto& particleLike) { return particleLike.data.opacity <= 0.f; });
        };

        updateParticleLike(particles);
        updateParticleLike(textParticles);

        const float volumeMult = profile.playAudioInBackground || window.hasFocus() ? 1.f : 0.f;

        listener.volume = profile.masterVolume * volumeMult;
        musicBGM.setVolume(profile.musicVolume * volumeMult);

        if (sounds.isPlayingPooled(sounds.prestige))
            musicBGM.setVolume(0.f);

        playedUsAccumulator += playedClock.getElapsedTime().asMicroseconds();
        autosaveUsAccumulator += playedClock.getElapsedTime().asMicroseconds();
        playedClock.restart();

        while (playedUsAccumulator > 1'000'000)
        {
            playedUsAccumulator -= 1'000'000;
            statSecondsPlayed();
        }

        if (autosaveUsAccumulator >= 180'000'000) // 3 min
        {
            autosaveUsAccumulator = 0;

            std::cout << "Autosaving...\n";
            savePlaythroughToFile(pt);
        }

        gameLoopUpdateMilestones();

        imGuiContext.update(window, deltaTime);

        const bool shouldDrawUI = inPrestigeTransition == 0 && splashCountdown.value <= 0.f;

        if (shouldDrawUI)
            uiDraw(gameView, mousePos);

        window.clear(sf::Color{157, 171, 191});

        window.setView(gameView);
        window.draw(sf::RectangleShape{{.fillColor = sf::Color::Black, .size = boundaries}}, /* texture */ nullptr);
        window.draw(txBackground,
                    {.color = sf::Color::White.withAlpha(static_cast<U8>(profile.backgroundOpacity / 100.f * 255.f))},
                    {.shader = &shader});

        bubbleDrawableBatch.clear();
        gameLoopDrawBubbles();
        window.draw(bubbleDrawableBatch, {.texture = &textureAtlas.getTexture(), .shader = &shader});

        cpuDrawableBatch.clear();
        gameLoopDrawCats(mousePos);
        gameLoopDrawShrines(mousePos);
        gameLoopDrawParticles();
        gameLoopDrawTextParticles();
        window.draw(cpuDrawableBatch, {.texture = &textureAtlas.getTexture(), .shader = &shader});

        //
        // Draw border around gameview
        window.setView(sf::View{.center = resolution / 2.f, .size = resolution});

        // TODO P2: (lib) make it possible to draw a rectangle directly via batching without any of this stuff
        window.draw(sf::RectangleShape{{.position         = gameView.viewport.position.componentWiseMul(resolution),
                                        .fillColor        = sf::Color::Transparent,
                                        .outlineColor     = colorBlueOutline,
                                        .outlineThickness = 4.f,
                                        .size             = gameView.viewport.size.componentWiseMul(resolution)}},
                    /* texture */ nullptr);

        const auto createScaledHudView = [](const sf::Vector2f& resolution, float scale) -> sf::View
        { return {.center = {resolution.x / (2.f * scale), resolution.y / (2.f * scale)}, .size = resolution / scale}; };

        const sf::View hudView = createScaledHudView(resolution, profile.hudScale);
        window.setView(hudView);

        moneyText.setString("$" + std::string(toStringWithSeparators(pt.money)));
        moneyText.scale  = {0.5f, 0.5f};
        moneyText.origin = moneyText.getLocalBounds().size / 2.f;

        moneyText.setTopLeft({15.f, 70.f});
        moneyTextShakeEffect.update(deltaTimeMs);
        moneyTextShakeEffect.applyToText(moneyText);
        moneyText.scale *= 0.5f;

        const float yBelowMinimap = pt.mapPurchased ? (boundaries.y / profile.minimapScale) + 12.f : 0.f;

        moneyText.position.y = yBelowMinimap + 30.f;

        if (shouldDrawUI)
            window.draw(moneyText);

        if (pt.comboPurchased)
        {
            comboText.setString("x" + std::to_string(combo + 1));

            comboTextShakeEffect.update(deltaTimeMs);
            comboTextShakeEffect.applyToText(comboText);
            comboText.scale *= 0.5f;

            comboText.position.y = yBelowMinimap + 50.f;

            if (shouldDrawUI)
                window.draw(comboText);
        }

        //
        // Combo bar
        if (shouldDrawUI)
            window.draw(sf::RectangleShape{{.position  = {comboText.getCenterRight().x + 3.f, yBelowMinimap + 56.f},
                                            .fillColor = sf::Color{255, 255, 255, 75},
                                            .size      = {100.f * comboCountdown.value / 700.f, 20.f}}},
                        /* texture */ nullptr);

        //
        // Minimap
        if (shouldDrawUI && pt.mapPurchased)
            drawMinimap(shader,
                        profile.minimapScale,
                        pt.getMapLimit(),
                        gameView,
                        hudView,
                        window,
                        txBackground,
                        cpuDrawableBatch,
                        textureAtlas,
                        resolution,
                        profile.hudScale);

        //
        // UI Toasts
        ImGui::RenderNotifications(
            [&]
        {
            ImGui::PushFont(fontImGuiMouldyCheese);
            ImGui::SetWindowFontScale(toolTipFontScale);
        },
            [&]
        {
            ImGui::SetWindowFontScale(normalFontScale);
            ImGui::PopFont();
        });


        //
        // UI
        imGuiContext.render(window);

        //
        // High visibility cursor
        window.setMouseCursorVisible(!profile.highVisibilityCursor);

        if (profile.highVisibilityCursor)
        {
            if (profile.multicolorCursor)
            {
                profile.cursorHue += deltaTimeMs * 0.5f;
                profile.cursorHue = wrapHue(profile.cursorHue);
            }

            window.setView({.center = resolution / 2.f, .size = resolution});
            window.draw(txCursor,
                        {.position = sf::Mouse::getPosition(window).toVector2f(),
                         .scale    = {profile.cursorScale, profile.cursorScale},
                         .origin   = {5.f, 5.f},
                         .color    = hueColor(wrapHue(profile.cursorHue), 255u)},
                        {.shader = &shader});
        }

        //
        // Splash screen
        if (splashCountdown.value > 0.f)
            drawSplashScreen(window, txLogo, splashCountdown, resolution, profile.hudScale);

        //
        // Tips
        gameLoopTips(deltaTimeMs);

        //
        // Display
        window.display();

        return true;
    }

    static inline constexpr const char* fragmentSrc = R"glsl(

layout(location = 2) uniform sampler2D sf_u_texture;

in vec4 sf_v_color;
in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    vec2 texCoord = sf_v_texCoord / vec2(textureSize(sf_u_texture, 0));
    vec4 texColor = texture(sf_u_texture, texCoord);

    const vec2 flagTarget = vec2(1.0/255.0);
    const vec2 epsilon = vec2(0.001);
    bool hueDriven = all(lessThanEqual(abs(sf_v_color.rg - flagTarget), epsilon));

    if (!hueDriven)
    {
        sf_fragColor = sf_v_color * texColor;
        return;
    }

    vec3 hsv = rgb2hsv(texColor.rgb);
    hsv.x = mod(hsv.x + sf_v_color.b, 360.0);
    sf_fragColor = vec4(hsv2rgb(hsv), sf_v_color.a * texColor.a);
}

)glsl";

    ////////////////////////////////////////////////////////////
    void loadPlaythroughFromFileAndReseed()
    {
        loadPlaythroughFromFile(pt);
        rng.reseed(pt.seed);
        shuffledCatNamesPerType = makeShuffledCatNames(rng);
    }

    ////////////////////////////////////////////////////////////
    Main()
    {
        //
        // Profile
        if (sf::Path{"profile.json"}.exists())
        {
            loadProfileFromFile(profile);
            std::cout << "Loaded profile from file on startup\n";
        }

        //
        // Playthrough
        if (sf::Path{"playthrough.json"}.exists())
        {
            loadPlaythroughFromFileAndReseed();
            std::cout << "Loaded playthrough from file on startup\n";
        }
        else
        {
            pt.seed = rng.getSeed();
        }

        //
        // Reserve memory
        particles.reserve(512);
        textParticles.reserve(256);
        pt.bubbles.reserve(32768);
        pt.cats.reserve(512);

        //
        // Touch state
        fingerPositions.resize(10);
    }

    ////////////////////////////////////////////////////////////
    void run()
    {
        //
        // Startup (splash screen and meow)
        splashCountdown.restart();
        playSound(sounds.byteMeow);

        //
        //
        // Background music
        musicBGM.setLooping(true);
        musicBGM.setAttenuation(0.f);
        musicBGM.setSpatializationEnabled(false);
        musicBGM.play(playbackDevice);

        //
        // Game loop
        playedClock.start();

        while (true)
            if (!gameLoop())
                return;
    }
};

////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    Main{}.run();
}

// TODO IDEAS:
// - disable exiting with escape key, or add popup to confirm
// - leveling cat (2000-2500 pops is a good milestone for 1st lvl up, 5000 for 2nd, 10000 for 3rd), level up should increase reward by 2...1.75...1.5, etc
// - maybe unlock leveling via prestige
// - change bg when unlocking new cat type or prestiging?
// - steam achievements
// - find better word for "prestige"
// - change cat names
// - smart/genius cat name prefix
// - pp point ideas: start with stuff unlocked, start with a bit of money, start with special cats, etc
// - prestige should scale indefinitely...? or make PP costs scale linearly, max is 20 -- or maybe when we reach max bubble value just purchase prestige points
// - other prestige ideas: cat multipop, unicat multitransform, unicat trasnform twice in a row, unlock random special bubbles
// - make bombs less affected by wind
// - balance until 3rd prestige + 2 astrocats seems pretty good
// - astrocats collide with each other when one flies but the other doesn't
// - achievements for speedrunning milestones
// - track generated revenue per cat, enable via PPs (maybe)
// - track pops/actions per cat should enabled via PPs (maybe)
// - ignore clicks done on top of imgui
// - "resting powerup" via PPs that increases cat multiplier when not clicking for a while, maybe around 16PPs
// - maybe make "autocast spell selector" a PP upgrade for around 128PPs

// x - "tweaks menu" unlockable with PP that allows any purchased cooldown/range upgrade to be tweaked, or just allow selling via right click...?
// x - resolution should be calculated from user desktop size
// x - setting to disable draw particles
// x - consider allowing menu to be outside game view when resizing or in separate widnow
// x - milestone system with time per milestone,
// x - mousecat global click multiplier should be upgradable with PPs, start around 24PPs
// x - engicat global cat multiplier should be upgradable with PPs, start around 32PPs
// x - maybe make "starpaw conversion ignore bombs" a PP upgrade for around 64PPs
// x - mouse cat could keep up his own combo, and his paw should be a cursor
// x - setting to show/hide cat text
// x - decouple resolution and "map chunk size"
// x - genius cats should also be able to only hit bombs
// x - tooltips in menus
// x - always use events to avoid out of focus keypresses
// x - make astrocat unselectable during flight
// x - pp upgrade "genius cats" always prioritize bombs
// x - add astrocat inspiration time multiplier upgrade
// x - astrocat inspiration could be purchased with PPs and add flag to the sprite
// x - genius cat pp upgrade could add brain in the jar to the sprite (TODO: redo art)
// x - genius cat pp upgrade should also add "pop bombs only" or "ignore normal bubbles" options
// x - astrocat stats
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
