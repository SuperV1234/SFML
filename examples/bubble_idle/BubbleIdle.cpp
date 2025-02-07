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
#include "MathUtils.hpp"
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
#include "SFML/Base/Builtins/OffsetOf.hpp"
#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Macros.hpp"
#include "SFML/Base/Math/Ceil.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/TokenPaste.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>


////////////////////////////////////////////////////////////
#define BUBBLEBYTE_VERSION_STR "v0.0.5"


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
                                        iBubble.getRadius(),
                                        jBubble.getRadius(),
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
[[nodiscard, gnu::always_inline]] inline Bubble makeRandomBubble(RNG& rng, const float mapLimit, const float maxY)
{
    const float scaleFactor = rng.getF(0.07f, 0.16f);

    return Bubble{.position = rng.getVec2f({mapLimit, maxY}),
                  .velocity = rng.getVec2f({-0.1f, -0.1f}, {0.1f, 0.1f}),
                  .scale    = scaleFactor,
                  .rotation = 0.f,
                  .type     = BubbleType::Normal};
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr float getComboValueMult(const int n)
{
    constexpr float initial = 1.f;
    constexpr float decay   = 0.9f;

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
    window.setView(minimapView);                                                     // Use minimap projection
    window.draw(txBackground, {.scale = {hudScale, hudScale}}, {.shader = &shader}); // Draw world background
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
    const auto alpha = static_cast<U8>(easeInOutSine(splashCountdown.getProgressBounced(easeInOutCubic)) * 255.f);

    window.draw({.position = resolution / 2.f / hudScale,
                 .scale    = sf::Vector2f{0.7f, 0.7f} *
                          (0.35f + 0.65f * easeInOutCubic(splashCountdown.getProgressBounced(easeInOutCubic))) / hudScale,
                 .origin      = txLogo.getSize().toVector2f() / 2.f,
                 .textureRect = txLogo.getRect(),
                 .color       = sf::Color::White.withAlpha(alpha)},
                txLogo);
}

////////////////////////////////////////////////////////////
template <typename This, auto Fn>
struct MemberInitGuard
{
    This* thisPtr;

    MemberInitGuard(This* ptr) : thisPtr(ptr)
    {
        Fn(*thisPtr);
    }
};

////////////////////////////////////////////////////////////
#define MEMBER_INIT_GUARD(type, ...) \
    MemberInitGuard<type, [](auto& self) __VA_ARGS__> SFML_BASE_TOKEN_PASTE(_scopeGuard, __LINE__) { this }

////////////////////////////////////////////////////////////
template <typename This, auto Fn>
struct MemberScopeGuard
{
    This* thisPtr;

    MemberScopeGuard(This* ptr) : thisPtr{ptr}
    {
    }

    MemberScopeGuard(const MemberScopeGuard&) = delete;
    MemberScopeGuard(MemberScopeGuard&&)      = delete;

    ~MemberScopeGuard()
    {
        Fn(*thisPtr);
    }
};

////////////////////////////////////////////////////////////
#define MEMBER_SCOPE_GUARD(type, ...) \
    MemberScopeGuard<type, [](auto& self) __VA_ARGS__> SFML_BASE_TOKEN_PASTE(_scopeGuard, __LINE__) { this }

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
    // Render window
    sf::RenderWindow window{
        {.size           = {1920u, 1080u},
         .title          = "BubbleByte " BUBBLEBYTE_VERSION_STR,
         .resizable      = true,
         .vsync          = true,
         .frametimeLimit = 144u,
         .contextSettings = {.antiAliasingLevel = sf::base::min(16u, sf::RenderTexture::getMaximumAntiAliasingLevel())}}};

    ////////////////////////////////////////////////////////////
    // ImGui context and initialization
    sf::ImGui::ImGuiContext imGuiContext;
    MEMBER_INIT_GUARD(Main, {
        if (!self.imGuiContext.init(self.window))
            throw -1;
    });

    ////////////////////////////////////////////////////////////
    // Texture atlas
    sf::TextureAtlas textureAtlas{sf::Texture::create({4096u, 4096u}, {.smooth = true}).value()};

    ////////////////////////////////////////////////////////////
    // SFML fonts
    sf::Font fontSuperBakery{sf::Font::openFromFile("resources/superbakery.ttf", &textureAtlas).value()};

    ////////////////////////////////////////////////////////////
    // ImGui fonts
    ImFont* fontImGuiSuperBakery{ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/superbakery.ttf", 26.f)};
    ImFont* fontImGuiMouldyCheese{ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/mouldycheese.ttf", 26.f)};

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
    sf::Texture txBackground{sf::Texture::loadFromFile("resources/background.jpg", {.smooth = true}).value()};
    sf::Texture txByteTip{sf::Texture::loadFromFile("resources/bytetip.png", {.smooth = true}).value()};
    sf::Texture txTipBg{sf::Texture::loadFromFile("resources/tipbg.png", {.smooth = true}).value()};
    sf::Texture txTipByte{sf::Texture::loadFromFile("resources/tipbyte.png", {.smooth = true}).value()};

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
                               .outlineThickness = 2.f}};
    TextShakeEffect moneyTextShakeEffect;

    ////////////////////////////////////////////////////////////
    // HUD combo text
    sf::Text        comboText{fontSuperBakery,
                              {.position         = moneyText.position + sf::Vector2f{0.f, 35.f},
                               .string           = "x1",
                               .characterSize    = 48u,
                               .fillColor        = sf::Color::White,
                               .outlineColor     = colorBlueOutline,
                               .outlineThickness = 1.5f}};
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
    std::vector<std::string> shuffledCatNames{getShuffledCatNames(rng.getEngine())};

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
    // Cat dragging state
    Cat*  draggedCat{nullptr};
    float catDragPressDuration{0.f};

    ////////////////////////////////////////////////////////////
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
                             .outlineThickness = 1.5f}};
    sf::Text textStatusBuffer{fontSuperBakery,
                              {.characterSize    = 32u,
                               .fillColor        = sf::Color::White,
                               .outlineColor     = colorBlueOutline,
                               .outlineThickness = 1.f}};

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SizeT getNextCatNameIdx()
    {
        return pt.nextCatName++ % shuffledCatNames.size();
    }

    ////////////////////////////////////////////////////////////
    void spawnParticles(const SizeT n, auto&&... args)
    {
        for (SizeT i = 0; i < n; ++i)
            particles.emplace_back(makeParticle(rng, SFML_BASE_FORWARD(args)...));
    }

    ////////////////////////////////////////////////////////////
    void spawnParticlesWithHue(const float hue, const SizeT n, auto&&... args)
    {
        for (SizeT i = 0; i < n; ++i)
            particles.emplace_back(makeParticle(rng, SFML_BASE_FORWARD(args)...)).hue = hue;
    }

    ////////////////////////////////////////////////////////////
    void spawnParticlesNoGravity(const SizeT n, auto&&... args)
    {
        for (SizeT i = 0; i < n; ++i)
            particles.emplace_back(makeParticle(rng, SFML_BASE_FORWARD(args)...)).data.accelerationY = 0.f;
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
    [[nodiscard]] bool keyDown(const sf::Keyboard::Key key)
    {
        return window.hasFocus() && sf::Keyboard::isKeyPressed(key);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool mBtnDown(const sf::Mouse::Button button)
    {
        return window.hasFocus() && sf::Mouse::isButtonPressed(button);
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

        spatialGrid.forEachIndexInRadius(center,
                                         radius,
                                         [&](const SizeT index)
        {
            auto& bubble = pt.bubbles[index];

            if ((bubble.position - center).lengthSquared() > radiusSq)
                return ControlFlow::Continue;

            return func(bubble);
        });
    }

    ////////////////////////////////////////////////////////////
    void addReward(const MoneyType reward)
    {
        pt.money += reward;
        moneyTextShakeEffect.bump(rng, 1.f + static_cast<float>(combo) * 0.1f);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] sf::Vector2f getResolution() const
    {
        return window.getSize().toVector2f();
    }

    ////////////////////////////////////////////////////////////
    Cat& spawnCat(const sf::View& gameView, const CatType catType, const sf::Vector2f rangeOffset, const float hue)
    {
        const auto pos = window.mapPixelToCoords((getResolution() / 2.f).toVector2i(), gameView);
        spawnParticles(32, pos, ParticleType::Star, 0.5f, 0.75f);

        return pt.cats.emplace_back(
            Cat{.position              = pos,
                .rangeOffset           = rangeOffset,
                .wobbleRadians         = {},
                .cooldown              = {.value = pt.getComputedCooldownByCatType(catType)},
                .pawPosition           = pos,
                .pawRotation           = sf::radians(0.f),
                .hue                   = hue,
                .inspiredCountdown     = {},
                .nameIdx               = getNextCatNameIdx(),
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
    void uiMakeTooltip()
    {
        if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || std::strlen(tooltipBuffer) == 0u)
            return;

        const float tooltipWidth = 400.f;

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetMousePos().x - tooltipWidth, ImGui::GetMousePos().y + 20));
        ImGui::SetNextWindowSizeConstraints(ImVec2(tooltipWidth, 0), ImVec2(tooltipWidth, FLT_MAX));

        ImGui::BeginTooltip();
        ImGui::PushFont(fontImGuiMouldyCheese);
        ImGui::SetWindowFontScale(toolTipFontScale);

        ImGui::TextWrapped("%s", tooltipBuffer);

        ImGui::SetWindowFontScale(normalFontScale);
        ImGui::PopFont();
        ImGui::EndTooltip();
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
            std::sprintf(buffer, currencyFmt, toStringWithSeparators(cost).c_str(), widgetId++);
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
    static std::string toStringWithSeparators(const T value)
    {
        // TODO P1: optimize to use buffer and return const char*

        std::string str = std::to_string(value);

        for (int i = static_cast<int>(str.size()) - 3; i > 0; i -= 3)
            str.insert(static_cast<SizeT>(i), ".");

        return str;
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
            std::sprintf(buffer, currencyFmt, toStringWithSeparators(cost).c_str(), widgetId++);
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
    void uiDraw(const sf::View& gameView)
    {
        widgetId = 0u;

        ImGui::SetNextWindowPos({getResolution().x - 15.f, 15.f}, 0, {1.f, 0.f});
        ImGui::SetNextWindowSizeConstraints(ImVec2(windowWidth, 0.f), ImVec2(windowWidth, getResolution().y - 15.f));
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

        if (pt.magicUnlocked && ImGui::BeginTabItem("Magic"))
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
            std::sprintf(tooltipBuffer, "Increase combo duration.");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.psvComboStartTime.currentValue()));
            makePSVButton("- Longer combo", pt.psvComboStartTime);
        }

        ImGui::Separator();

        if (nCatNormal > 0 && pt.psvComboStartTime.nPurchases > 0)
        {
            std::sprintf(tooltipBuffer,
                         "Extend the map and enable scrolling (right click or drag with two fingers).\n\nExtending the "
                         "map will increase the total number of bubbles you can work with, and will also reveal "
                         "shrines that grant unique cats upon completion.");
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
                             "bubbles are absorbed, shrines will grant a unique cat.");
                std::sprintf(labelBuffer, "%zu shrines active", pt.psvShrineActivation.nPurchases);
                if (makePSVButton("- Activate next shrine", pt.psvShrineActivation))
                {
                    for (Shrine& shrine : pt.shrines)
                    {
                        if (shrine.tcActivation.hasValue())
                            continue;

                        shrine.tcActivation.emplace(TargetedCountdown{.startingValue = 2000.f});
                        shrine.tcActivation->restart();

                        sounds.earthquakeFast.setPosition({shrine.position.x, shrine.position.y});
                        playSound(sounds.earthquakeFast);

                        screenShakeAmount = 4.5f;
                        screenShakeTimer  = 1000.f;

                        break;
                    }
                }
                ImGui::EndDisabled();
            }

            ImGui::Separator();

            std::sprintf(tooltipBuffer,
                         "Increase the total number of bubbles. Scales with map size.\n\nMore bubbles, "
                         "more money!");
            std::sprintf(labelBuffer, "%zu bubbles", static_cast<SizeT>(pt.psvBubbleCount.currentValue()));
            makePSVButton("More bubbles", pt.psvBubbleCount);

            ImGui::Separator();
        }

        if (pt.comboPurchased && pt.psvComboStartTime.nPurchases > 0)
        {
            std::sprintf(tooltipBuffer,
                         "Cats periodically pop nearby bubbles or bombs. Their cooldown and range can be "
                         "upgraded. Their behavior can be permanently upgraded with prestige "
                         "points.\n\nCats "
                         "can be dragged around to position them strategically.\n\nNo, cats cannot be "
                         "removed "
                         "once purchased, you monster.");
            std::sprintf(labelBuffer, "%zu cats", nCatNormal);
            if (makePSVButton("Cat", pt.getPSVByCatType(CatType::Normal)))
            {
                spawnCat(gameView, CatType::Normal, {0.f, 0.f}, /* hue */ rng.getF(-20.f, 20.f));

                if (nCatNormal == 0)
                {
                    doTip(
                        "Cats periodically pop bubbles for you!\nYou can drag them around to position "
                        "them.");
                }
            }
        }

        const auto makeCooldownButton = [this](const char* label, const CatType catType)
        {
            auto& psv = pt.getCooldownMultPSVByCatType(catType);

            std::sprintf(tooltipBuffer, "Decrease cooldown.");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.getComputedCooldownByCatType(catType) / 1000.f));
            makePSVButton(label, psv);
        };

        const auto makeRangeButton = [this](const char* label, const CatType catType)
        {
            auto& psv = pt.getRangeDivPSVByCatType(catType);

            std::sprintf(tooltipBuffer, "Increase range.");
            std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(pt.getComputedRangeByCatType(catType)));
            makePSVButton(label, psv);
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
                         "your "
                         "cats pop them for you, or pop them near the end of a combo for huge rewards!");
            std::sprintf(labelBuffer, "%zu unicats", nCatUni);
            if (makePSVButton("Unicat", pt.getPSVByCatType(CatType::Uni)))
            {
                spawnCat(gameView, CatType::Uni, {0.f, -100.f}, /* hue */ rng.getF(0.f, 360.f));

                if (nCatUni == 0)
                {
                    doTip("Unicats transform bubbles in star bubbles,\nworth x25! Pop them at the end of a combo!");
                }
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
                         "Devilcats transform bubbles into bombs that explode when popped. Bubbles "
                         "affected by "
                         "the explosion are worth x10 more! Bomb explosion range can be upgraded.");
            std::sprintf(labelBuffer, "%zu devilcats", nCatDevil);
            if (makePSVButton("Devilcat", pt.getPSVByCatType(CatType::Devil)))
            {
                spawnCat(gameView, CatType::Devil, {0.f, 100.f}, /* hue */ rng.getF(-20.f, 20.f));

                if (nCatDevil == 0)
                {
                    doTip(
                        "Devilcats transform bubbles in bombs!\nExplode them to pop nearby "
                        "bubbles\nwith a x10 money multiplier!",
                        /* maxPrestigeLevel */ 1);
                }
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
                         "Astrocats periodically fly across the map, popping bubbles they hit with a huge "
                         "x20 "
                         "money multiplier!\n\nThey can be permanently upgraded with prestige points to "
                         "inspire cats watching them fly past to pop bubbles faster.");
            std::sprintf(labelBuffer, "%zu astrocats", nCatAstro);
            if (makePSVButton("Astrocat", pt.getPSVByCatType(CatType::Astro)))
            {
                spawnCat(gameView, CatType::Astro, {-64.f, 0.f}, /* hue */ rng.getF(-20.f, 20.f));

                if (nCatAstro == 0)
                {
                    doTip(
                        "Astrocats periodically fly across\nthe entire map, with a huge\nx20 "
                        "money multiplier!",
                        /* maxPrestigeLevel */ 1);
                }
            }

            if (astroCatUpgradesUnlocked)
            {
                makeCooldownButton("- cooldown", CatType::Astro);
                makeRangeButton("- range", CatType::Astro);
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

        uiBeginColumns();

        buttonHueMod = 120.f;
        if (makePSVButtonEx("Prestige", pt.psvBubbleValue, times, maxCost))
        {
            playSound(sounds.prestige);
            inPrestigeTransition = true;

            scroll = 0.f;

            draggedCat           = nullptr;
            catDragPressDuration = 0.f;

            pt.onPrestige(times);
        }

        ImGui::Columns(1);

        buttonHueMod = 0.f;
        ImGui::SetWindowFontScale(0.75f);

        const auto currentMult = static_cast<SizeT>(pt.psvBubbleValue.currentValue()) + 1u;

        ImGui::Text("(next prestige: $%s)", toStringWithSeparators(nextCost).c_str());

        if (maxCost == 0u)
            ImGui::Text("- not enough money to prestige");
        else
        {
            ImGui::Text("- increase bubble value from x%zu to x%zu\n- obtain %zu prestige points",
                        currentMult,
                        currentMult + times,
                        times);
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
                     "Cats have graduated!\n\nThey still cannot resist their popping insticts, but "
                     "they "
                     "will "
                     "go for star bubbles and bombs first, ensuring they are not wasted!");
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
            if (makePurchasablePPButtonOneTime("- genius cats", 3u, pt.geniusCatsPurchased))
                doTip("Genius cats prioritize bombs and\ncan be instructed to ignore certain bubbles!",
                      /* maxPrestigeLevel */ UINT_MAX);
        }

        if (pt.geniusCatsPurchased)
        {
            ImGui::Columns(1);
            ImGui::SetWindowFontScale(subBulletFontScale);

            ImGui::Text("- ignore: ");
            ImGui::SameLine();

            ImGui::Checkbox("normal", &pt.geniusCatIgnoreNormalBubbles);
            ImGui::SameLine();

            ImGui::Checkbox("star", &pt.geniusCatIgnoreStarBubbles);
            ImGui::SameLine();

            ImGui::Checkbox("bombs", &pt.geniusCatIgnoreBombBubbles);

            ImGui::SetWindowFontScale(normalFontScale);
            uiBeginColumns();
        }

        ImGui::Separator();

        std::sprintf(tooltipBuffer,
                     "A giant fan (off-screen) will produce an intense wind, making bubbles move and "
                     "flow much faster.\n\nNote: this effect can be toggled at will.");
        std::sprintf(labelBuffer, "");
        if (makePurchasablePPButtonOneTime("Giant fan", 2u, pt.windPurchased))
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
        if (makePurchasablePPButtonOneTime("Space propaganda", 3u, pt.astroCatInspirePurchased))
            doTip("Astrocats will inspire other cats\nto work faster when flying by!",
                  /* maxPrestigeLevel */ UINT_MAX);

        if (pt.astroCatInspirePurchased)
        {
            std::sprintf(tooltipBuffer, "Increase the duration of the inspiration effect.");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.getComputedInspirationDuration() / 1000.f));

            makePrestigePurchasablePPButtonPSV("- buff duration", pt.psvPPInspireDurationMult);
        }

        ImGui::Separator();

        if (pt.magicUnlocked)
        {
            std::sprintf(tooltipBuffer, "Increase the speed of mana generation.");
            std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.getComputedManaCooldown() / 1000.f));
            makePrestigePurchasablePPButtonPSV("Mana cooldown", pt.psvPPManaCooldownMult);

            std::sprintf(tooltipBuffer, "Increase the maximum mana.");
            std::sprintf(labelBuffer, "%llu mana", pt.getComputedMaxMana());
            makePrestigePurchasablePPButtonPSV("Mana limit", pt.psvPPManaMaxMult);
        }

        buttonHueMod = 0.f;

        ImGui::Columns(1);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isWizardBusy() const
    {
        return pt.absorbingWisdom;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* findWizard()
    {
        const auto wizardCatIt = sf::base::findIf(pt.cats.begin(),
                                                  pt.cats.end(),
                                                  [](const Cat& cat) { return cat.type == CatType::Wizard; });

        if (wizardCatIt == pt.cats.end())
            return nullptr;

        return &*wizardCatIt;
    }

    ////////////////////////////////////////////////////////////
    void uiTabBarMagic()
    {
        ImGui::SetWindowFontScale(normalFontScale);

        Cat* wizardCat = findWizard();

        if (wizardCat == nullptr)
        {
            ImGui::Text("The wizard cat is missing!");
            return;
        }

        const auto range = pt.getComputedRangeByCatType(CatType::Wizard);

        ImGui::Text("Your wizard is %s!", shuffledCatNames[wizardCat->nameIdx].c_str());

        ImGui::Separator();

        ImGui::Text("Wisdom points: %llu WP", pt.wisdom);

        ImGui::Checkbox("Absorb wisdom", &pt.absorbingWisdom);
        std::sprintf(tooltipBuffer, "TODO");
        uiMakeTooltip();

        uiBeginColumns();
        buttonHueMod = 45.f;

        std::sprintf(tooltipBuffer, "TODO");
        std::sprintf(labelBuffer, "TODO");
        (void)makePSVButtonExByCurrency("Discover spell",
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
            if (pt.psvSpellCount.nPurchases >= 0)
            {
                std::sprintf(tooltipBuffer, "TODO");
                std::sprintf(labelBuffer, "TODO");
                bool done = false;
                // TODO: give it a name, also make it ignore bombs
                if (makePurchasableButtonOneTimeByCurrency("Spell", done, ManaType{1}, pt.mana, "%s mana##%u"))
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
                        spawnParticles(4, bubble.position, ParticleType::Star, 0.5f, 0.35f);
                        bubble.type = BubbleType::Star;

                        return ControlFlow::Continue;
                    });

                    done = false;
                    ++wizardCat->hits;
                }
            }

            //
            // SPELL 0
            if (pt.psvSpellCount.nPurchases >= 2)
            {
                std::sprintf(tooltipBuffer, "TODO");
                std::sprintf(labelBuffer, "%.2fs", static_cast<double>(pt.arcaneAuraTimer / 1000.f));
                bool done = false;
                if (makePurchasableButtonOneTimeByCurrency("Arcane aura", done, ManaType{1}, pt.mana, "%s mana##%u"))
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
                }
            }

            buttonHueMod = 0.f;
            ImGui::Columns(1);
            ImGui::EndDisabled();
        }
    }

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

            const auto secondsToDisplay = sf::base::U64{stats.secondsPlayed % 60u};
            const auto minutesToDisplay = sf::base::U64{(stats.secondsPlayed / 60u) % 60u};
            const auto hoursToDisplay   = sf::base::U64{stats.secondsPlayed / 3600u};
            ImGui::Text("Time played: %lluh %llum %llus", hoursToDisplay, minutesToDisplay, secondsToDisplay);

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::Text("Bubbles popped: %s", toStringWithSeparators(stats.bubblesPopped).c_str());
            ImGui::Indent();
            ImGui::Text("Clicked: %s", toStringWithSeparators(stats.bubblesHandPopped).c_str());
            ImGui::Text("By cats: %s", toStringWithSeparators(stats.bubblesPopped - stats.bubblesHandPopped).c_str());
            ImGui::Unindent();

            ImGui::NextColumn();

            ImGui::Text("Revenue: $%s", toStringWithSeparators(stats.bubblesPoppedRevenue).c_str());
            ImGui::Indent();
            ImGui::Text("Clicked: $%s", toStringWithSeparators(stats.bubblesHandPoppedRevenue).c_str());
            ImGui::Text("By cats: $%s",
                        toStringWithSeparators(stats.bubblesPoppedRevenue - stats.bubblesHandPoppedRevenue).c_str());
            ImGui::Text("Bombs:  $%s", toStringWithSeparators(stats.explosionRevenue).c_str());
            ImGui::Text("Flights: $%s", toStringWithSeparators(stats.flightRevenue).c_str());
            ImGui::Unindent();

            ImGui::Columns(1);
        };

        if (ImGui::BeginTabBar("TabBarStats", ImGuiTabBarFlags_DrawSelectedOverline))
        {
            ImGui::SetWindowFontScale(0.75f);
            if (ImGui::BeginTabItem(" Statistics "))
            {
                ImGui::SetWindowFontScale(0.75f);

                uiCenteredText(" -- Lifetime values -- ");
                displayStats(profile.statsLifetime);

                ImGui::Separator();

                uiCenteredText(" -- Playthrough values -- ");
                displayStats(pt.statsTotal);

                ImGui::Separator();

                uiCenteredText(" -- Prestige values -- ");
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

                    const auto secondsToDisplay = sf::base::U64{value % 60u};
                    const auto minutesToDisplay = sf::base::U64{(value / 60u) % 60u};
                    const auto hoursToDisplay   = sf::base::U64{value / 3600u};

                    ImGui::Text("%s: %lluh %llum %llus", name, hoursToDisplay, minutesToDisplay, secondsToDisplay);
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

    [[nodiscard]] MoneyType computeFinalReward(const sf::Vector2f bubblePosition, const MoneyType computedReward, const bool applyCombo)
    {
        Cat* wizardCat = findWizard();

        const bool inRangeOfWizard = wizardCat != nullptr && (wizardCat->position - bubblePosition).length() <=
                                                                 pt.getComputedRangeByCatType(CatType::Wizard);

        const float comboValueMult = applyCombo ? getComboValueMult(combo) : 1.f;
        const float arcaneAuraMult = (pt.arcaneAuraTimer > 0.f && inRangeOfWizard) ? 5.f : 1.f;

        return static_cast<MoneyType>(sf::base::ceil(static_cast<float>(computedReward) * comboValueMult * arcaneAuraMult));
    }

    ////////////////////////////////////////////////////////////
    void uiTabBarSettings()
    {
        ImGui::SetNextItemWidth(210.f);
        ImGui::SliderFloat("Master volume", &profile.masterVolume, 0.f, 100.f, "%.0f%%");

        ImGui::SetNextItemWidth(210.f);
        ImGui::SliderFloat("Music volume", &profile.musicVolume, 0.f, 100.f, "%.0f%%");

        ImGui::Checkbox("Play audio in background", &profile.playAudioInBackground);
        ImGui::Checkbox("Enable combo scratch sound", &profile.playComboEndSound);

        ImGui::SetNextItemWidth(210.f);
        ImGui::SliderFloat("Minimap Scale", &profile.minimapScale, 5.f, 40.f, "%.2f");

        ImGui::SetNextItemWidth(210.f);
        ImGui::SliderFloat("HUD Scale", &profile.hudScale, 0.5f, 2.f, "%.2f");

        ImGui::Checkbox("Enable tips", &profile.tipsEnabled);

        const float fps = 1.f / fpsClock.getElapsedTime().asSeconds();
        ImGui::Text("FPS: %.2f", static_cast<double>(fps));

        ImGui::Separator();

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
            shuffledCatNames = getShuffledCatNames(rng.getEngine());

            pt      = Playthrough{};
            pt.seed = rng.getSeed();

            particles.clear();
            textParticles.clear();
        }

        uiPopButtonColors();
        buttonHueMod = 0.f;
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

            const auto explosionAction = [&](Bubble& bubble)
            {
                if (bubble.type == BubbleType::Bomb)
                    return ControlFlow::Continue;

                const MoneyType newReward = computeFinalReward(bubble.position,
                                                               pt.getComputedRewardByBubbleType(bubble.type) * 10u,
                                                               /* applyCombo */ false);

                statExplosionRevenue(newReward);

                popWithRewardAndReplaceBubble(newReward,
                                              byHand,
                                              bubble,
                                              /* combo */ 1,
                                              /* popSoundOverlap */ false);

                return ControlFlow::Continue;
            };

            forEachBubbleInRadius(position, pt.getComputedBombExplosionRadius(), explosionAction);
        }
    }

    ////////////////////////////////////////////////////////////
    void popWithRewardAndReplaceBubble(const MoneyType reward, const bool byHand, Bubble& bubble, int xCombo, bool popSoundOverlap)
    {
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

            particles.emplace_back(ParticleData{.position      = bubble.position,
                                                .velocity      = -diff.normalized() * 0.5f,
                                                .scale         = 1.5f,
                                                .accelerationY = 0.f,
                                                .opacity       = 1.f,
                                                .opacityDecay = 0.00135f + (shrine.getRange() - diff.length()) / 22000.f,
                                                .rotation = 0.f,
                                                .torque   = 0.f},
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
            addReward(reward);

        bubble = makeRandomBubble(rng, pt.getMapLimit(), 0.f);
        bubble.position.y -= bubble.getRadius();
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
    void gameLoopUpdateBubbles(const float deltaTimeMs)
    {
        for (Bubble& bubble : pt.bubbles)
        {
            if (bubble.type == BubbleType::Bomb)
                bubble.rotation += deltaTimeMs * 0.01f;

            auto& pos = bubble.position;

            if (pt.windPurchased && pt.windEnabled)
            {
                bubble.velocity.x += 0.00055f * deltaTimeMs;
                bubble.velocity.y += 0.00055f * deltaTimeMs;
            }

            pos += bubble.velocity * deltaTimeMs;

            const float radius = bubble.getRadius();

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

                bubble.type     = BubbleType::Normal;
                bubble.rotation = 0.f;
            }
            else if (pos.y + radius < 0.f)
                pos.y = boundaries.y + radius;

            bubble.velocity.y += 0.00005f * deltaTimeMs;
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActions(const float deltaTimeMs)
    {
        for (Cat& cat : pt.cats)
        {
            // Keep cat in boundaries
            const float catRadius = cat.getRadius();

            if (!cat.astroState.hasValue())
            {
                cat.position.x = sf::base::clamp(cat.position.x, catRadius, pt.getMapLimit() - catRadius);
                cat.position.y = sf::base::clamp(cat.position.y, catRadius, boundaries.y - catRadius);
            }

            const auto maxCooldown = pt.getComputedCooldownByCatType(cat.type);
            const auto range       = pt.getComputedRangeByCatType(cat.type);

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

            const auto [cx, cy] = cat.position + cat.rangeOffset;

            if (cat.inspiredCountdown.value > 0.f)
            {
                if (rng.getF(0.f, 1.f) > 0.5f)
                    particles.push_back(
                        {.data = {.position = drawPosition + sf::Vector2f{rng.getF(-catRadius, +catRadius), catRadius},
                                  .velocity = rng.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                                  .scale    = rng.getF(0.08f, 0.27f) * 0.2f,
                                  .accelerationY = -0.002f,
                                  .opacity       = 255.f,
                                  .opacityDecay  = rng.getF(0.00025f, 0.0015f),
                                  .rotation      = rng.getF(0.f, sf::base::tau),
                                  .torque        = rng.getF(-0.002f, 0.002f)},
                         .type = ParticleType::Star});
            }

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
                                                                   /* applyCombo */ false);

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
                    particles.push_back(
                        {.data = {.position = drawPosition + sf::Vector2f{rng.getF(-catRadius, +catRadius), catRadius},
                                  .velocity = rng.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                                  .scale    = rng.getF(0.08f, 0.27f) * 0.2f,
                                  .accelerationY = -0.002f,
                                  .opacity       = 255.f,
                                  .opacityDecay  = rng.getF(0.00025f, 0.0015f),
                                  .rotation      = rng.getF(0.f, sf::base::tau),
                                  .torque        = rng.getF(-0.002f, 0.002f)},
                         .hue  = 225.f,
                         .type = ParticleType::Star});
                }
            }

            if (!cat.updateCooldown(deltaTimeMs))
                continue;

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
                    bubble.velocity.y = rng.getF(-0.1f, -0.05f);
                    sounds.shine.setPosition({bubble.position.x, bubble.position.y});
                    playSound(sounds.shine);

                    spawnParticles(4, bubble.position, ParticleType::Star, 0.5f, 0.35f);

                    cat.textStatusShakeEffect.bump(rng, 1.5f);
                    ++cat.hits;
                }
                else if (cat.type == CatType::Normal)
                {
                    popWithRewardAndReplaceBubble(computeFinalReward(bubble.position,
                                                                     pt.getComputedRewardByBubbleType(bubble.type),
                                                                     /* applyCombo */ false),
                                                  /* byHand */ false,
                                                  bubble,
                                                  /* combo */ 1,
                                                  /* popSoundOverlap */ true);

                    cat.textStatusShakeEffect.bump(rng, 1.5f);
                    ++cat.hits;
                }
                else if (cat.type == CatType::Devil)
                {
                    bubble.type = BubbleType::Bomb;
                    bubble.velocity.y += rng.getF(0.1f, 0.2f);
                    sounds.makeBomb.setPosition({bubble.position.x, bubble.position.y});
                    playSound(sounds.makeBomb);

                    spawnParticles(8, bubble.position, ParticleType::Fire, 1.25f, 0.35f);

                    cat.textStatusShakeEffect.bump(rng, 1.5f);
                    ++cat.hits;
                }

                cat.cooldown.value = maxCooldown;
                return ControlFlow::Break;
            };

            if (cat.type == CatType::Wizard)
            {
                if (pt.absorbingWisdom)
                {
                    Bubble* starBubble = nullptr;

                    const auto findRotatedStarBubble = [&](Bubble& bubble)
                    {
                        if (bubble.type != BubbleType::Star || bubble.rotation == 0.f)
                            return ControlFlow::Continue;

                        starBubble = &bubble;
                        return ControlFlow::Break;
                    };

                    const auto findStarBubble = [&](Bubble& bubble)
                    {
                        if (bubble.type != BubbleType::Star)
                            return ControlFlow::Continue;

                        starBubble = &bubble;
                        return ControlFlow::Break;
                    };

                    forEachBubbleInRadius({cx, cy}, range, findRotatedStarBubble);

                    if (starBubble == nullptr)
                        forEachBubbleInRadius({cx, cy}, range, findStarBubble);

                    if (starBubble != nullptr)
                    {
                        Bubble& bubble = *starBubble;

                        cat.pawPosition = bubble.position;
                        cat.pawOpacity  = 255.f;
                        cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

                        bubble.rotation += deltaTimeMs * 0.025f;
                        spawnParticlesWithHue(230.f, 1, bubble.position, ParticleType::Star, 0.5f, 0.35f);

                        if (bubble.rotation >= sf::base::tau)
                        {
                            const auto wisdomReward = pt.getComputedRewardByBubbleType(bubble.type);

                            auto& tp = textParticles.emplace_back(
                                TextParticle{.buffer = {},
                                             .data   = {.position = {drawPosition.x, drawPosition.y - 10.f},
                                                        .velocity = rng.getVec2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.425f,
                                                        .scale         = 1.25f,
                                                        .accelerationY = 0.0042f,
                                                        .opacity       = 1.f,
                                                        .opacityDecay  = 0.00175f,
                                                        .rotation      = 0.f,
                                                        .torque        = rng.getF(-0.002f, 0.002f)}});

                            std::snprintf(tp.buffer, sizeof(tp.buffer), "+%llu WP", wisdomReward);

                            // TODO P1: change sound
                            sounds.pop.setPosition({bubble.position.x, bubble.position.y});
                            sounds.pop.setPitch(1.f);
                            playSound(sounds.pop);

                            spawnParticlesWithHue(230.f, 16, bubble.position, ParticleType::Star, 0.5f, 0.35f);

                            pt.wisdom += wisdomReward;
                            bubble.type     = BubbleType::Normal;
                            bubble.rotation = 0.f;

                            cat.cooldown.value = maxCooldown;
                        }
                    }
                }

                continue;
            }

            if (cat.type == CatType::Astro)
            {
                if (!cat.astroState.hasValue())
                {
                    sounds.launch.setPosition({cx, cy});
                    playSound(sounds.launch, /* overlap */ true);

                    ++cat.hits;
                    cat.astroState.emplace(/* startX */ cat.position.x, /* velocityX */ 0.f, /* wrapped */ false);
                    --cat.position.x;
                }
            }

            if (cat.type == CatType::Witch) // TODO P1: change
            {
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
                continue;
            }

            if (cat.type == CatType::Normal && pt.smartCatsPurchased)
            {
                Bubble* firstSpecialBubble = nullptr;
                Bubble* firstNormalBubble  = nullptr;
                Bubble* firstStarBubble    = nullptr;
                Bubble* firstBombBubble    = nullptr;

                const auto findBubblesByType = [&](Bubble& bubble)
                {
                    if (bubble.type == BubbleType::Normal && firstNormalBubble == nullptr)
                    {
                        firstNormalBubble = &bubble;
                    }
                    else if (bubble.type == BubbleType::Star && firstStarBubble == nullptr)
                    {
                        firstStarBubble = &bubble;
                        if (firstSpecialBubble == nullptr)
                            firstSpecialBubble = &bubble;
                    }
                    else if (bubble.type == BubbleType::Bomb && firstBombBubble == nullptr)
                    {
                        firstBombBubble = &bubble;
                        if (firstSpecialBubble == nullptr)
                            firstSpecialBubble = &bubble;
                    }

                    return ControlFlow::Continue;
                };

                forEachBubbleInRadius({cx, cy}, range, findBubblesByType);

                if (!pt.geniusCatsPurchased)
                {
                    if (firstSpecialBubble != nullptr)
                        (void)action(*firstSpecialBubble);
                    else if (firstNormalBubble != nullptr)
                        (void)action(*firstNormalBubble);
                }
                else
                {
                    if (firstBombBubble != nullptr && !pt.geniusCatIgnoreBombBubbles)
                        (void)action(*firstBombBubble);
                    else if (firstStarBubble != nullptr && !pt.geniusCatIgnoreStarBubbles)
                        (void)action(*firstStarBubble);
                    else if (firstNormalBubble != nullptr && !pt.geniusCatIgnoreNormalBubbles)
                        (void)action(*firstNormalBubble);
                }
            }
            else
            {
                forEachBubbleInRadius({cx, cy}, range, action);
            }
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

                        // TODO P0: handle all shrine death rewards
                        if (shrine.type == ShrineType::Magic)
                        {
                            auto& c    = spawnCat(gameView, CatType::Wizard, {0.f, 0.f}, /* hue */ 0.f);
                            c.position = shrine.position;

                            pt.magicUnlocked = true;
                            doTip("TODO: wizard tip");
                        }
                        else if (shrine.type == ShrineType::Clicking)
                        {
                            auto& c    = spawnCat(gameView, CatType::Mouse, {0.f, 0.f}, /* hue */ 0.f);
                            c.position = shrine.position;

                            doTip("TODO: mouse tip");
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
        if (!pt.magicUnlocked)
            return;

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

        if (pt.arcaneAuraTimer > 0.f)
        {
            pt.arcaneAuraTimer -= deltaTimeMs;
            pt.arcaneAuraTimer = sf::base::max(pt.arcaneAuraTimer, 0.f);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateMilestones()
    {
        const auto updateMilestone = [&](const char* name, sf::base::U64& milestone, bool showTip = true)
        {
            const auto oldMilestone = milestone;

            milestone = sf::base::min(milestone, pt.statsTotal.secondsPlayed);

            if (showTip && milestone != oldMilestone)
                doTip("Milestone reached!\n -> " + std::string{name}, /* maxPrestigeLevel */ UINT_MAX);
        };

        const auto nCatNormal = pt.getCatCountByType(CatType::Normal);
        const auto nCatUni    = pt.getCatCountByType(CatType::Uni);
        const auto nCatDevil  = pt.getCatCountByType(CatType::Devil);
        const auto nCatAstro  = pt.getCatCountByType(CatType::Astro);

        if (nCatNormal >= 1)
            updateMilestone("1st Cat", pt.milestones.firstCat, /* showTip */ false);

        if (nCatUni >= 1)
            updateMilestone("1st Unicat", pt.milestones.firstUnicat, /* showTip */ false);

        if (nCatDevil >= 1)
            updateMilestone("1st Devilcat", pt.milestones.firstDevilcat, /* showTip */ false);

        if (nCatAstro >= 1)
            updateMilestone("1st Astrocat", pt.milestones.firstAstrocat, /* showTip */ false);

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
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawBubbles()
    {
        sf::Sprite tempSprite;

        const sf::FloatRect bubbleRects[3]{txrBubble, txrBubbleStar, txrBomb};

        for (SizeT i = 0u; i < pt.bubbles.size(); ++i)
        {
            const Bubble& bubble = pt.bubbles[i];

            bubble.applyToSprite(tempSprite);

            tempSprite.textureRect = bubbleRects[static_cast<U8>(bubble.type)];
            tempSprite.origin      = tempSprite.textureRect.size / 2.f;
            tempSprite.scale *= bubble.type == BubbleType::Bomb ? 1.65f : 1.f;

            constexpr float hueRange = 60.f;

            tempSprite.color = hueColor(wrapHue(sf::base::fmod(static_cast<float>(i) * 2.f - hueRange / 2.f, hueRange)), 255);

            cpuDrawableBatch.add(tempSprite);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawCats(const float deltaTimeMs, const sf::Vector2f mousePos)
    {
        const sf::FloatRect* const normalCatTxr = pt.geniusCatsPurchased  ? &txrGeniusCat
                                                  : pt.smartCatsPurchased ? &txrSmartCat
                                                                          : &txrCat;

        const sf::FloatRect* const astroCatTxr = pt.astroCatInspirePurchased ? &txrAstroCatWithFlag : &txrAstroCat;

        const sf::FloatRect* const catTxrsByType[nCatTypes]{
            normalCatTxr, // Normal
            &txrUniCat,   // Uni
            &txrDevilCat, // Devil
            &txrWitchCat, // Witch
            astroCatTxr,  // Astro

            &txrWizardCat, // Wizard
            &txrMouseCat,  // Mouse
        };

        const sf::FloatRect* const catPawTxrsByType[nCatTypes]{
            &txrCatPaw,      // Normal
            &txrUniCatPaw,   // Uni
            &txrDevilCatPaw, // Devil
            &txrWitchCatPaw, // Witch
            &txrWhiteDot,    // Astro

            &txrWizardCatPaw, // Wizard
            &txrMouseCatPaw,  // Mouse
        };

        bool anyCatHovered = false;

        for (Cat& cat : pt.cats)
        {
            float opacityMod      = 1.f;
            U8    rangeInnerAlpha = 0u;

            if (!anyCatHovered && &cat != draggedCat && !cat.isAstroAndInFlight() &&
                (mousePos - cat.position).length() <= cat.getRadius() && !mBtnDown(sf::Mouse::Button::Left))
            {
                anyCatHovered   = true;
                opacityMod      = 0.5f;
                rangeInnerAlpha = 75u;
            }

            const auto& catTxr    = *catTxrsByType[static_cast<U8>(cat.type)];
            const auto& catPawTxr = *catPawTxrsByType[static_cast<U8>(cat.type)];

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
            const auto alpha    = static_cast<U8>(cat.mainOpacity * opacityMod);
            const auto catColor = hueColor(wrapHue(cat.hue), alpha);

            const auto circleAlpha = cat.cooldown.value < 0.f
                                         ? static_cast<U8>(0u)
                                         : static_cast<U8>(255.f - (cat.cooldown.value / maxCooldown * 225.f));

            const auto circleColor = CatConstants::colors[static_cast<U8>(cat.type)].withHueMod(cat.hue).withLightness(0.75f);
            const sf::Color outlineColor = circleColor.withLightness(0.25f);

            // TODO P2: (lib) make it possible to draw a circle directly via batching without any of this stuff,
            // no need to preallocate a circle shape before, have a reusable vertex buffer in the batch itself
            circleShapeBuffer.position = cat.position + cat.rangeOffset;
            circleShapeBuffer.origin   = {range, range};
            circleShapeBuffer.setPointCount(static_cast<unsigned int>(range / 3.f));
            circleShapeBuffer.setRadius(range);
            circleShapeBuffer.setOutlineColor(circleColor.withAlpha(rangeInnerAlpha == 0u ? circleAlpha : 255u));
            circleShapeBuffer.setFillColor(circleShapeBuffer.getOutlineColor().withAlpha(rangeInnerAlpha));
            cpuDrawableBatch.add(circleShapeBuffer);

            if (cat.type == CatType::Uni)
                cat.hue += deltaTimeMs * 0.1f;

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
                           .rotation    = cat.pawRotation,
                           .textureRect = catPawTxr,
                           .color       = catColor.withAlpha(static_cast<U8>(cat.pawOpacity))});

            textNameBuffer.setString(shuffledCatNames[cat.nameIdx]);
            textNameBuffer.position = cat.position + sf::Vector2f{0.f, 48.f};
            textNameBuffer.origin   = textNameBuffer.getLocalBounds().size / 2.f;
            textNameBuffer.scale    = sf::Vector2f{0.5f, 0.5f};
            textNameBuffer.setOutlineColor(outlineColor);
            cpuDrawableBatch.add(textNameBuffer);

            textStatusBuffer.setString(
                std::to_string(cat.hits) + " " + CatConstants::actionNames[static_cast<U8>(cat.type)]);
            textStatusBuffer.position = cat.position + sf::Vector2f{0.f, 68.f};
            textStatusBuffer.origin   = textStatusBuffer.getLocalBounds().size / 2.f;
            textStatusBuffer.setOutlineColor(outlineColor);
            cat.textStatusShakeEffect.applyToText(textStatusBuffer);
            textStatusBuffer.scale *= 0.5f;
            cpuDrawableBatch.add(textStatusBuffer);
        };
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawShrines()
    {
        for (Shrine& shrine : pt.shrines)
        {
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
            circleShapeBuffer.setFillColor(sf::Color::Transparent);
            cpuDrawableBatch.add(circleShapeBuffer);

            textNameBuffer.setString(shrineNames[static_cast<U8>(shrine.type)]);
            textNameBuffer.position = shrine.position + sf::Vector2f{0.f, 48.f};
            textNameBuffer.origin   = textNameBuffer.getLocalBounds().size / 2.f;
            textNameBuffer.scale    = sf::Vector2f{0.5f, 0.5f} * invDeathProgress;
            textNameBuffer.setOutlineColor(outlineColor);
            cpuDrawableBatch.add(textNameBuffer);

            if (shrine.isActive())
            {
                textStatusBuffer.setString("$" + toStringWithSeparators(shrine.collectedReward) + " / $" +
                                           toStringWithSeparators(pt.getComputedRequiredRewardByShrineType(shrine.type)));
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
    void gameLoopDrawParticles()
    {
        sf::Sprite tempSprite;

        const sf::FloatRect
            particleRects[nParticleTypes]{txrParticle, txrStarParticle, txrFireParticle, txrHexParticle, txrShrineParticle};

        for (const auto& particle : particles)
        {
            particle.applyToSprite(tempSprite);
            tempSprite.textureRect = particleRects[static_cast<U8>(particle.type)];
            tempSprite.origin      = tempSprite.textureRect.size / 2.f;

            cpuDrawableBatch.add(tempSprite);
        }
    }

    ////////////////////////////////////////////////////////////
    void gameLoopDrawTextParticles()
    {
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
        window.draw(tipSprite, txTipBg);

        sf::Sprite tipByteSprite{.position    = {},
                                 .scale       = {0.7f, 0.7f},
                                 .origin      = txTipByte.getSize().toVector2f() / 2.f,
                                 .textureRect = txTipByte.getRect(),
                                 .color       = sf::Color::White.withAlpha(alpha)};

        tipByteSprite.setCenter(tipSprite.getCenterRight() + sf::Vector2f{-40.f, 0.f});
        window.draw(tipByteSprite, txTipByte);

        tipText.setTopLeft(tipSprite.getTopLeft() + sf::Vector2f{45.f, 65.f});
        window.draw(tipText);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool gameLoop()
    {
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

        const sf::Vector2f viewCenter{sf::base::clamp(gameScreenSize.x / 2.f + actualScroll * 2.f,
                                                      gameScreenSize.x / 2.f,
                                                      boundaries.x - gameScreenSize.x / 2.f),
                                      gameScreenSize.y / 2.f};

        sf::View gameView            = createScaledView(gameScreenSize, resolution);
        gameView.viewport.position.x = 0.f;
        gameView.center              = viewCenter + screenShake;

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
                else if (pt.bubbles.size() < targetBubbleCount)
                {
                    const SizeT times = (targetBubbleCount - pt.bubbles.size()) > 500 ? 25 : 1;

                    for (SizeT i = 0; i < times; ++i)
                    {
                        const auto bPos = pt.bubbles.emplace_back(makeRandomBubble(rng, pt.getMapLimit(), boundaries.y)).position;

                        spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
                        playReversePopAt(bPos);
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
                if ((clickPos - bubble.position).length() > bubble.getRadius())
                    return ControlFlow::Continue;

                anyBubblePoppedByClicking = true;

                if (pt.comboPurchased)
                {
                    if (combo == 0)
                    {
                        combo                = 1;
                        comboCountdown.value = pt.psvComboStartTime.currentValue() * 1000.f;
                    }
                    else
                    {
                        combo += 1;
                        comboCountdown.value += 150.f - sf::base::clamp(static_cast<float>(combo) * 10.f, 0.f, 100.f);

                        comboTextShakeEffect.bump(rng, 1.f + static_cast<float>(combo) * 0.2f);
                    }
                }
                else
                {
                    combo = 1;
                }

                const auto reward = computeFinalReward(bubble.position,
                                                       pt.getComputedRewardByBubbleType(bubble.type),
                                                       /* applyCombo */ true);
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
        spatialGrid.forEachUniqueIndexPair([&](const SizeT bubbleIdxI, const SizeT bubbleIdxJ)
        { handleBubbleCollision(deltaTimeMs, pt.bubbles[bubbleIdxI], pt.bubbles[bubbleIdxJ]); });

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
                particleLike.update(deltaTimeMs);

            std::erase_if(particleLikeVec, [](const auto& particleLike) { return particleLike.data.opacity <= 0.f; });
        };

        updateParticleLike(particles);
        updateParticleLike(textParticles);

        if (comboCountdown.updateAndStop(deltaTimeMs) == CountdownStatusStop::AlreadyFinished)
            combo = 0;

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
            uiDraw(gameView);

        window.clear(sf::Color{157, 171, 191});

        window.setView(gameView);
        window.draw(txBackground, {.shader = &shader});

        cpuDrawableBatch.clear();

        gameLoopDrawBubbles();
        gameLoopDrawCats(deltaTimeMs, mousePos);
        gameLoopDrawShrines();
        gameLoopDrawParticles();
        gameLoopDrawTextParticles();

        window.draw(cpuDrawableBatch, {.texture = &textureAtlas.getTexture(), .shader = &shader});

        const auto createScaledHudView = [](const sf::Vector2f& resolution, float scale) -> sf::View
        { return {.center = {resolution.x / (2.f * scale), resolution.y / (2.f * scale)}, .size = resolution / scale}; };

        const sf::View hudView = createScaledHudView(resolution, profile.hudScale);
        window.setView(hudView);

        moneyText.setString("$" + toStringWithSeparators(pt.money));
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
        // UI
        imGuiContext.render(window);

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
        shuffledCatNames = getShuffledCatNames(rng.getEngine());
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
// - make window non resizable or make game scale with window size proportionally
// - leveling cat (2000-2500 pops is a good milestone for 1st lvl up, 5000 for 2nd, 10000 for 3rd), level up should increase reward by 2...1.75...1.5, etc
// - maybe unlock leveling via prestige
// - some normal cat buff around 17000 money as a milestone towards devilcats, maybe two paws?
// - change bg when unlocking new cat type or prestiging?
// - steam achievements
// - find better word for "prestige"
// - change cat names
// - smart/genius cat name prefix
// - pp point ideas: start with stuff unlocked, start with a bit of money, etc
// - prestige should scale indefinitely...? or make PP costs scale linearly, max is 20 -- or maybe when we reach max bubble value just purchase prestige points
// - other prestige ideas: cat multipop, unicat multitransform, unicat trasnform twice in a row, unlock random special bubbles
// - make bombs less affected by wind
// - balance until 3rd prestige + 2 astrocats seems pretty good
// - consider allowing menu to be outside game view when resizing or in separate widnow
// - astrocats collide with each other when one flies but the other doesn't
// - milestone system with time per milestone, also achievements for speedrunning milestones
// - map expansion special bubble find
// - another source of prestige points at prestige max level?
// - mouse cat could keep up his own combo, and his paw should be a cursor

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
