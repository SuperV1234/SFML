#include "Aliases.hpp"
#include "Cat.hpp"
#include "Common.hpp"
#include "Constants.hpp"
#include "MathUtils.hpp"
#include "Particle.hpp"
#include "PurchasableScalingValue.hpp"
#include "RNG.hpp"
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
#include "SFML/Base/IntTypes.hpp"
#include "SFML/Base/Macros.hpp"
#include "SFML/Base/Math/Ceil.hpp"
#include "SFML/Base/Math/Sqrt.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/ScopeGuard.hpp"

#include <imgui.h>

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include <cstdio>
#include <cstring>


////////////////////////////////////////////////////////////
#define BUBBLEBYTE_VERSION_STR "v0.0.5"


namespace
{


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
    const sf::Vector2f diff            = jPosition - iPosition;
    const float        squaredDistance = diff.lengthSquared();

    const float sumRadii        = iRadius + jRadius;
    const float squaredRadiiSum = sumRadii * sumRadii;

    if (squaredDistance >= squaredRadiiSum)
        return sf::base::nullOpt;

    // Calculate the overlap between the bubbles
    const float distance = sf::base::sqrt(squaredDistance); // Distance between centers
    const float overlap  = sumRadii - distance;             // Amount of overlap

    // Calculate the normal between the bubbles
    const sf::Vector2f normal = (distance == 0.f) ? sf::Vector2f{1.f, 0.f} : diff / distance;

    // Move the bubbles apart based on their masses (heavier bubbles move less)
    const float m1        = iRadius * iRadius * iMassMult; // Mass of bubble i (quadratic scaling)
    const float m2        = jRadius * jRadius * jMassMult; // Mass of bubble j (quadratic scaling)
    const float totalMass = m1 + m2;

    // Velocity resolution calculations
    const float vRelDotNormal = (iVelocity - jVelocity).dot(normal);

    sf::Vector2f velocityChangeI;
    sf::Vector2f velocityChangeJ;

    // Only apply impulse if bubbles are moving towards each other
    if (vRelDotNormal > 0.f)
    {
        constexpr float e = 0.65f; // Coefficient of restitution (1.0 = perfectly elastic)
        const float     j = -(1.f + e) * vRelDotNormal / ((1.f / m1) + (1.f / m2));

        const sf::Vector2f impulse = normal * j;

        velocityChangeI = impulse / m1;
        velocityChangeJ = -impulse / m2;
    }

    // Define a "softness" factor to control how quickly the overlap is resolved
    const float softnessFactor = 0.0005f * deltaTimeMs;

    // Calculate the displacement needed to resolve the overlap
    const sf::Vector2f displacement = normal * overlap * softnessFactor;

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
[[nodiscard, gnu::always_inline]] inline Bubble makeRandomBubble(const float mapLimit, const float maxY)
{
    const float scaleFactor = getRndFloat(0.07f, 0.16f);

    return Bubble{.position = getRndVector2f({mapLimit, maxY}),
                  .velocity = getRndVector2f({-0.1f, -0.1f}, {0.1f, 0.1f}),
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
void drawSplashScreen(sf::RenderWindow& window, const sf::Texture& txLogo, const TargetedCountdown& splashCountdown)
{
    float fade = 255.f;

    const float m0 = splashCountdown.startingValue;
    const float m1 = splashCountdown.startingValue - 255.f;

    if (splashCountdown.value > m1)
        fade = remap(splashCountdown.value, m1, m0, 255.f, 0.f);
    else if (splashCountdown.value < 255.f)
        fade = splashCountdown.value;

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
    struct LoadedSound : private sf::SoundBuffer, public sf::Sound // TODO: eww
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

        ////////////////////////////////////////////////////////////
        const sf::Sound& asSound() const
        {
            return *this;
        }

        ////////////////////////////////////////////////////////////
        const sf::SoundBuffer& asBuffer() const
        {
            return *this;
        }
    };

    ////////////////////////////////////////////////////////////
    LoadedSound pop{"pop.ogg"};
    LoadedSound reversePop{"reversePop.ogg"};
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
    std::vector<sf::Sound> soundsBeingPlayed;

    ////////////////////////////////////////////////////////////
    explicit Sounds()
    {
        const auto setupWorldSound = [&](auto& sound)
        {
            sound.setAttenuation(0.0025f);
            sound.setSpatializationEnabled(true);
        };

        const auto setupUISound = [&](auto& sound)
        {
            sound.setAttenuation(0.f);
            sound.setSpatializationEnabled(false);
        };

        setupWorldSound(pop);
        setupWorldSound(reversePop);
        setupWorldSound(shine);
        setupWorldSound(explosion);
        setupWorldSound(makeBomb);
        setupWorldSound(hex);
        setupWorldSound(launch);
        setupWorldSound(rocket);

        setupUISound(click);
        setupUISound(byteMeow);
        setupUISound(grab);
        setupUISound(drop);
        setupUISound(scratch);
        setupUISound(buy);
        setupUISound(byteSpeak);
        setupUISound(prestige);

        scratch.setVolume(35.f);
        buy.setVolume(75.f);
        explosion.setVolume(75.f);
    }

    ////////////////////////////////////////////////////////////
    Sounds(const Sounds&) = delete;
    Sounds(Sounds&&)      = delete;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isPlayingPooled(const LoadedSound& ls) const
    {
        return sf::base::anyOf( //
            soundsBeingPlayed.begin(),
            soundsBeingPlayed.end(),
            [&ls](const sf::Sound& sound)
            { return sound.getStatus() == sf::Sound::Status::Playing && &sound.getBuffer() == &ls.asBuffer(); });
    }

    ////////////////////////////////////////////////////////////
    bool playPooled(sf::PlaybackDevice& playbackDevice, const LoadedSound& ls, const bool overlap)
    {
        // TODO P1: improve in library

        if (!overlap && isPlayingPooled(ls))
            return false;

        const auto it = sf::base::findIf( //
            soundsBeingPlayed.begin(),
            soundsBeingPlayed.end(),
            [](const sf::Sound& sound) { return sound.getStatus() == sf::Sound::Status::Stopped; });

        if (it != soundsBeingPlayed.end())
        {
            *it = ls.asSound(); // assigment does not reallocate `m_impl`
            it->play(playbackDevice);

            return true;
        }

        // TODO: to sf base, also not needed
        // std::erase_if(soundsBeingPlayed,
        //               [](const sf::Sound& sound) { return sound.getStatus() == sf::Sound::Status::Stopped; });

        soundsBeingPlayed.emplace_back(ls.asSound()).play(playbackDevice);
        return true;
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
                                                                                                   bubble.getRadius());

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
                                                                                                   bubble.getRadius());

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
// TODO: save seed
// TODO: autosave
// TODO: multiple save slots

} // namespace

extern void saveProfileToFile(const Profile& profile);
extern void loadProfileFromFile(Profile& profile);
extern void saveGameToFile(const Game& game);
extern void loadGameFromFile(Game& game);


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
        {.size           = resolutionUInt,
         .title          = "BubbleByte " BUBBLEBYTE_VERSION_STR,
         .resizable      = false, // TODO: reconsider
         .vsync          = true,
         .frametimeLimit = 144u,
         .contextSettings = {.antiAliasingLevel = sf::base::min(16u, sf::RenderTexture::getMaximumAntiAliasingLevel())}});

    const auto keyDown = [&](const sf::Keyboard::Key key)
    { return window.hasFocus() && sf::Keyboard::isKeyPressed(key); };

    const auto mBtnDown = [&](const sf::Mouse::Button button)
    { return window.hasFocus() && sf::Mouse::isButtonPressed(button); };

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
    ImFont* fontImGuiSuperBakery  = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/superbakery.ttf", 26.f);
    ImFont* fontImGuiMouldyCheese = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/mouldycheese.ttf", 26.f);

    /* --- Music */
    auto musicBGM = sf::Music::openFromFile("resources/hibiscus.mp3").value();

    /* --- Sounds */
    Sounds       sounds;
    sf::Listener listener;

    const auto playSound = [&](const Sounds::LoadedSound& ls, bool overlap = true)
    { sounds.playPooled(playbackDevice, ls, overlap); };

    /* --- Textures */
    const auto txLogo       = sf::Texture::loadFromFile("resources/logo.png", {.smooth = true}).value();
    const auto txBackground = sf::Texture::loadFromFile("resources/background.jpg", {.smooth = true}).value();
    const auto txByteTip    = sf::Texture::loadFromFile("resources/bytetip.png", {.smooth = true}).value();
    const auto txTipBg      = sf::Texture::loadFromFile("resources/tipbg.png", {.smooth = true}).value();
    const auto txTipByte    = sf::Texture::loadFromFile("resources/tipbyte.png", {.smooth = true}).value();

    /* --- Texture atlas rects */
    const auto txrWhiteDot         = textureAtlas.add(graphicsContext.getBuiltInWhiteDotTexture()).value();
    const auto txrBubble           = addImgResourceToAtlas("bubble2.png");
    const auto txrBubbleStar       = addImgResourceToAtlas("bubble3.png");
    const auto txrCat              = addImgResourceToAtlas("cat.png");
    const auto txrSmartCat         = addImgResourceToAtlas("smartcat.png");
    const auto txrGeniusCat        = addImgResourceToAtlas("geniuscat.png");
    const auto txrUniCat           = addImgResourceToAtlas("unicat.png");
    const auto txrDevilCat         = addImgResourceToAtlas("devilcat.png");
    const auto txrCatPaw           = addImgResourceToAtlas("catpaw.png");
    const auto txrUniCatPaw        = addImgResourceToAtlas("unicatpaw.png");
    const auto txrDevilCatPaw      = addImgResourceToAtlas("devilcatpaw.png");
    const auto txrParticle         = addImgResourceToAtlas("particle.png");
    const auto txrStarParticle     = addImgResourceToAtlas("starparticle.png");
    const auto txrFireParticle     = addImgResourceToAtlas("fireparticle.png");
    const auto txrHexParticle      = addImgResourceToAtlas("hexparticle.png");
    const auto txrWitchCat         = addImgResourceToAtlas("witchcat.png");
    const auto txrWitchCatPaw      = addImgResourceToAtlas("witchcatpaw.png");
    const auto txrAstroCat         = addImgResourceToAtlas("astromeow.png");
    const auto txrAstroCatWithFlag = addImgResourceToAtlas("astromeowwithflag.png");
    const auto txrBomb             = addImgResourceToAtlas("bomb.png");

    //
    //
    // Profile
    Profile profile;
    SFML_BASE_SCOPE_GUARD({
        std::cout << "Saving profile to file on exit\n";
        saveProfileToFile(profile);
    });

    if (sf::Path{"profile.json"}.exists())
    {
        std::cout << "Loaded profile from file on startup\n";
        loadProfileFromFile(profile);
    }

    //
    //
    // Game
    Game game;
    SFML_BASE_SCOPE_GUARD({
        std::cout << "Saving game to file on exit\n";
        saveGameToFile(game);
    });

    if (sf::Path{"game.json"}.exists())
    {
        std::cout << "Loaded game from file on startup\n";
        loadGameFromFile(game);
    }

    //
    //
    // Stat helpers
    const auto withAllStats = [&](auto&& func)
    {
        func(profile.statsLifetime);
        func(game.statsTotal);
        func(game.statsSession);
    };

    const auto statBubblePopped = [&](const bool byHand, const MoneyType reward)
    {
        withAllStats(
            [&](auto& stats)
            {
                stats.bubblesPopped += 1;
                stats.bubblesPoppedRevenue += reward;
            });

        if (byHand)
        {
            withAllStats(
                [&](auto& stats)
                {
                    stats.bubblesHandPopped += 1;
                    stats.bubblesHandPoppedRevenue += reward;
                });
        }
    };

    const auto statExplosionRevenue = [&](const MoneyType reward)
    { withAllStats([&](auto& stats) { stats.explosionRevenue += reward; }); };

    const auto statFlightRevenue = [&](const MoneyType reward)
    { withAllStats([&](auto& stats) { stats.flightRevenue += reward; }); };

    const auto statSecondsPlayed = [&] { withAllStats([&](auto& stats) { stats.secondsPlayed += 1u; }); };

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
                       {.position         = moneyText.position + sf::Vector2f{0.f, 35.f},
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
    std::vector<Particle> particles;
    particles.reserve(512);

    std::vector<TextParticle> textParticles;
    textParticles.reserve(256);

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

    // TODO:
    bool inPrestigeTransition = false;

    //
    //
    // Scaling values (persistent)
    const MoneyType rewardPerType[3]{
        1u,  // Normal
        25u, // Star
        1u,  // Bomb
    };

    const auto getScaledReward = [&](const BubbleType type) -> MoneyType
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

    TargetedLoopingTimer bubbleSpawnTimer{.target = 3.f};
    TargetedLoopingTimer catRemoveTimer{.target = 100.f};

    //
    //
    // Transient game state
    int       combo = 0;
    Countdown comboCountdown;

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
    // Touch state
    std::vector<sf::base::Optional<sf::Vector2f>> fingerPositions;
    fingerPositions.resize(10);

    //
    //
    // Startup
    TargetedCountdown splashCountdown{.startingValue = 1750.f};
    splashCountdown.reset();
    playSound(sounds.byteMeow);

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
    musicBGM.setSpatializationEnabled(false);
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
    // UI
    const auto spawnCat = [&](const sf::View& gameView, const CatType catType, const sf::Vector2f rangeOffset) -> Cat&
    {
        const auto pos = window.mapPixelToCoords((resolution / 2.f).toVector2i(), gameView);
        spawnParticles(32, pos, ParticleType::Star, 0.5f, 0.75f);
        return game.cats.emplace_back(makeCat(catType, pos, rangeOffset, getNextCatNameIdx()));
    };

    const auto doTip = [&](const std::string& str, const SizeT maxPrestigeLevel = 0)
    {
        if (!profile.tipsEnabled || game.psvBubbleValue.nPurchases > maxPrestigeLevel)
            return;

        playSound(sounds.byteMeow);
        tipString = str;
        tipTimer  = 5000.f;
    };

    const auto drawUI =
        [&profile,
         &game,
         &playSound,
         &costFunction,
         &sounds,
         &inPrestigeTransition,
         &scroll,
         &draggedCat,
         &catDragPressDuration,
         &particles,
         &textParticles,
         &fontImGuiSuperBakery,
         &fontImGuiMouldyCheese,
         &combo,
         &doTip,
         &spawnCat,
         &getScaledReward,
         &fpsClock](const sf::View& gameView)
    {
        const auto nCatNormal = game.getCatCountByType(CatType::Normal);
        const auto nCatUni    = game.getCatCountByType(CatType::Uni);
        const auto nCatDevil  = game.getCatCountByType(CatType::Devil);
        const auto nCatWitch  = game.getCatCountByType(CatType::Witch);
        const auto nCatAstro  = game.getCatCountByType(CatType::Astro);

        const auto globalCostMultiplier = game.getComputedGlobalCostMultiplier();

        constexpr float normalFontScale    = 1.f;
        constexpr float subBulletFontScale = 0.8f;
        constexpr float toolTipFontScale   = 0.65f;

        char buffer[256]{};
        char labelBuffer[512]{};
        char tooltipBuffer[1024]{};

        const auto makeButtonLabel = [&](const char* label)
        {
            ImGui::SetWindowFontScale((label[0] == '-' ? subBulletFontScale : normalFontScale) * 1.15f);
            ImGui::Text("%s", label);
            ImGui::SameLine();
        };

        const auto makeButtonTopLabel = [&](const char* label, const char* labelBuffer)
        {
            ImGui::SetWindowFontScale(0.5f);
            ImGui::Text("%s", labelBuffer);

            ImGui::SetWindowFontScale(label[0] == '-' ? subBulletFontScale : normalFontScale);
            ImGui::SameLine();
        };

        const auto makeLabelsImpl = [&](const char* label, const char* labelBuffer)
        {
            makeButtonLabel(label);
            makeButtonTopLabel(label, labelBuffer);
            ImGui::NextColumn();
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
            ImGui::PushStyleColor(ImGuiCol_Border, colorBlueOutline.withHueMod(buttonHueMod).toVec4<ImVec4>());
        };

        const auto popButtonColors = [&] { ImGui::PopStyleColor(4); };

        constexpr float buttonWidth = 150.f;

        const auto makeTooltip = [&]()
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
        };

        auto makeButtonImpl = [&](const char* label, const char* buffer)
        {
            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - buttonWidth - 2.5f, 0.f)); // Push to right
            ImGui::SameLine();

            pushButtonColors();

            if (ImGui::Button(buffer, ImVec2(buttonWidth, 0.f)))
            {
                playSound(sounds.buy);

                popButtonColors();
                return true;
            }

            makeTooltip();

            popButtonColors();

            if (label[0] == '-')
                ImGui::SetWindowFontScale(normalFontScale);

            ImGui::NextColumn();
            return false;
        };

        const auto makeDoneButton = [&](const char* label)
        {
            ImGui::BeginDisabled(true);

            makeLabelsImpl(label, labelBuffer);
            makeButtonImpl(label, "DONE");

            ImGui::EndDisabled();
        };

        unsigned int widgetId = 0;

        const auto makePurchasableButton = [&](const char* label, float baseCost, float growthFactor, float count)
        {
            bool result = false;

            const auto cost = static_cast<MoneyType>(globalCostMultiplier * costFunction(baseCost, count, growthFactor));
            std::sprintf(buffer, "$%llu##%u", cost, widgetId++);

            ImGui::BeginDisabled(game.money < cost);

            makeLabelsImpl(label, labelBuffer);
            if (makeButtonImpl(label, buffer))
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

            const auto cost = static_cast<MoneyType>(globalCostMultiplier * psv.nextCost());

            if (maxedOut)
                std::sprintf(buffer, "MAX");
            else
                std::sprintf(buffer, "$%llu##%u", cost, widgetId++);

            ImGui::BeginDisabled(maxedOut || game.money < cost);

            makeLabelsImpl(label, labelBuffer);
            if (makeButtonImpl(label, buffer))
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
                std::sprintf(buffer, "$%llu##%u", cost, widgetId++);

            ImGui::BeginDisabled(maxedOut || game.money < cost || cost == 0u);

            makeLabelsImpl(label, labelBuffer);
            if (makeButtonImpl(label, buffer))
            {
                result = true;
                game.money -= cost;
                psv.nPurchases += times;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto makePurchasableButtonOneTimeByCurrency =
            [&](const char* label, bool& done, const auto computedCost, auto& availability, const char* currencyFmt)
        {
            bool result = false;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
            if (done)
                std::sprintf(buffer, "DONE");
            else
                std::sprintf(buffer, currencyFmt, computedCost, widgetId++);
#pragma clang diagnostic pop

            ImGui::BeginDisabled(done || availability < computedCost);

            makeLabelsImpl(label, labelBuffer);
            if (makeButtonImpl(label, buffer))
            {
                result = true;
                availability -= computedCost;
                done = true;
            }

            ImGui::EndDisabled();
            return result;
        };

        const auto makePurchasableButtonOneTime = [&](const char* label, const MoneyType xcost, bool& done)
        {
            const auto cost = static_cast<MoneyType>(globalCostMultiplier * static_cast<float>(xcost));
            return makePurchasableButtonOneTimeByCurrency(label, done, cost, game.money, "$%zu##%u");
        };

        const auto makePurchasablePPButtonOneTime = [&](const char* label, const U64 prestigePointsCost, bool& done)
        {
            return makePurchasableButtonOneTimeByCurrency(label,
                                                          done,
                                                          prestigePointsCost,
                                                          game.prestigePoints,
                                                          "%zu PPs##%u");
        };

        const auto makePrestigePurchasablePPButtonPSV = [&](const char* label, PurchasableScalingValue& psv)
        {
            const bool maxedOut           = psv.nPurchases == psv.data->nMaxPurchases;
            const U64  prestigePointsCost = static_cast<U64>(psv.nextCost());

            bool result = false;

            if (maxedOut)
                std::sprintf(buffer, "MAX");
            else
                std::sprintf(buffer, "%llu PPs##%u", prestigePointsCost, widgetId++);

            ImGui::BeginDisabled(maxedOut || game.prestigePoints < prestigePointsCost);

            makeLabelsImpl(label, labelBuffer);
            if (makeButtonImpl(label, buffer))
            {
                result = true;
                game.prestigePoints -= prestigePointsCost;
                ++psv.nPurchases;
            }

            ImGui::EndDisabled();
            return result;
        };

        constexpr float windowWidth = 420.f;

        ImGui::SetNextWindowPos({resolution.x - 15.f, 15.f}, 0, {1.f, 0.f});
        ImGui::SetNextWindowSizeConstraints(ImVec2(windowWidth, 0.f), ImVec2(windowWidth, resolution.y - 15.f));
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

        const auto beginColumns = []
        {
            ImGui::Columns(2, "twoColumns", false);
            ImGui::SetColumnWidth(0, windowWidth - buttonWidth - 20.f);
            ImGui::SetColumnWidth(1, buttonWidth + 10.f);
        };

        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_DrawSelectedOverline))
        {
            if (ImGui::BeginTabItem(" X "))
                ImGui::EndTabItem();

            static auto selectOnce = ImGuiTabItemFlags_SetSelected;
            if (ImGui::BeginTabItem(" Shop ", nullptr, selectOnce))
            {
                selectOnce = {};

                beginColumns();

                std::sprintf(labelBuffer, "");
                std::sprintf(tooltipBuffer,
                             "Build your combo by popping bubbles quickly, increasing the value of each subsequent "
                             "one.\n\nCombos expires on misclicks and over time, but can be upgraded to last "
                             "longer.\n\nStar bubbles are affected -- pop them while your multiplier is high!");
                if (makePurchasableButtonOneTime("Combo", 15, game.comboPurchased))
                {
                    combo = 0;
                    doTip("Pop bubbles in quick succession to\nkeep your combo up and make money!");
                }

                if (game.comboPurchased)
                {
                    std::sprintf(tooltipBuffer, "Increase combo duration.");
                    std::sprintf(labelBuffer, "%.2fs", static_cast<double>(game.psvComboStartTime.currentValue()));
                    makePurchasableButtonPSV("- Longer combo", game.psvComboStartTime);
                }

                ImGui::Separator();

                if (nCatNormal > 0 && game.psvComboStartTime.nPurchases > 0)
                {
                    std::sprintf(tooltipBuffer,
                                 "Extend the map and enable scrolling (right click or drag with two fingers).");
                    std::sprintf(labelBuffer, "");
                    if (makePurchasableButtonOneTime("Map scrolling", 250, game.mapPurchased))
                    {
                        ++game.mapLimitIncreases;
                        scroll = 0.f;

                        doTip("You can scroll the map with right click\nor by dragging with two fingers!");
                    }

                    if (game.mapPurchased)
                    {
                        if (game.getMapLimit() < boundaries.x - gameScreenSize.x)
                        {
                            std::sprintf(tooltipBuffer, "Extend the map further by one screen.");
                            std::sprintf(labelBuffer,
                                         "%.2f%%",
                                         static_cast<double>(remap(game.getMapLimit(), 0.f, boundaries.x, 0.f, 100.f) + 10.f));
                            if (makePurchasableButton("- Extend map", 100.f, 4.85f, game.getMapLimit() / gameScreenSize.x))
                            {
                                ++game.mapLimitIncreases;
                                game.mapPurchased = true;
                            }
                        }
                        else
                        {
                            std::sprintf(tooltipBuffer, "Extend the map further by one screen.");
                            std::sprintf(labelBuffer, "100%%");
                            makeDoneButton("- Extend map");
                        }
                    }

                    ImGui::Separator();

                    std::sprintf(tooltipBuffer,
                                 "Increase the total number of bubbles. Scales with map size.\n\nMore bubbles, more "
                                 "money!");
                    std::sprintf(labelBuffer, "%zu bubbles", static_cast<SizeT>(game.psvBubbleCount.currentValue()));
                    makePurchasableButtonPSV("More bubbles", game.psvBubbleCount);

                    ImGui::Separator();
                }

                if (game.comboPurchased && game.psvComboStartTime.nPurchases > 0)
                {
                    std::sprintf(tooltipBuffer,
                                 "Cats periodically pop nearby bubbles or bombs. Their cooldown and range can be "
                                 "upgraded. Their behavior can be permanently upgraded with prestige points.\n\nCats "
                                 "can be dragged around to position them strategically.\n\nNo, cats cannot be removed "
                                 "once purchased, you monster.");
                    std::sprintf(labelBuffer, "%zu cats", nCatNormal);
                    if (makePurchasableButton("Cat", 35, 1.7f, static_cast<float>(nCatNormal)))
                    {
                        spawnCat(gameView, CatType::Normal, {0.f, 0.f});

                        if (nCatNormal == 0)
                        {
                            doTip(
                                "Cats periodically pop bubbles for you!\nYou can drag them around to position "
                                "them.");
                        }
                    }
                }

                const auto makeCooldownButton = [&](const char* label, const CatType catType)
                {
                    auto& psv = game.getCooldownMultPSVByCatType(catType);

                    std::sprintf(tooltipBuffer, "Decrease cooldown.");
                    std::sprintf(labelBuffer,
                                 "%.2fs",
                                 static_cast<double>(game.getComputedCooldownByCatType(catType) / 1000.f));
                    makePurchasableButtonPSV(label, psv);
                };

                const auto makeRangeButton = [&](const char* label, const CatType catType)
                {
                    auto& psv = game.getRangeDivPSVByCatType(catType);

                    std::sprintf(tooltipBuffer, "Increase range.");
                    std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(game.getComputedRangeByCatType(catType)));
                    makePurchasableButtonPSV(label, psv);
                };

                const bool catUpgradesUnlocked = game.psvBubbleCount.nPurchases > 0 && nCatNormal >= 2 && nCatUni >= 1;
                if (catUpgradesUnlocked)
                {
                    makeCooldownButton("- cooldown", CatType::Normal);
                    makeRangeButton("- range", CatType::Normal);
                }

                ImGui::Separator();

                // UNICORN CAT
                const bool catUnicornUnlocked         = game.psvBubbleCount.nPurchases > 0 && nCatNormal >= 3;
                const bool catUnicornUpgradesUnlocked = catUnicornUnlocked && nCatUni >= 2 && nCatDevil >= 1;
                if (catUnicornUnlocked)
                {
                    std::sprintf(tooltipBuffer,
                                 "Unicats transform bubbles into star bubbles, which are worth x25 more!\n\nHave your "
                                 "cats pop them for you, or pop them near the end of a combo for huge rewards!");
                    std::sprintf(labelBuffer, "%zu unicats", nCatUni);
                    if (makePurchasableButton("Unicat", 250, 1.75f, static_cast<float>(nCatUni)))
                    {
                        spawnCat(gameView, CatType::Uni, {0.f, -100.f});

                        if (nCatUni == 0)
                        {
                            doTip(
                                "Unicats transform bubbles in star bubbles,\nworth x25! Pop them at the end of a "
                                "combo!");
                        }
                    }

                    if (catUnicornUpgradesUnlocked)
                    {
                        makeCooldownButton("- cooldown", CatType::Uni);
                        makeRangeButton("- range", CatType::Uni);
                    }

                    ImGui::Separator();
                }

                // DEVIL CAT
                const bool catDevilUnlocked = game.psvBubbleValue.nPurchases > 0 && nCatNormal >= 6 && nCatUni >= 4;
                const bool catDevilUpgradesUnlocked = catDevilUnlocked && nCatDevil >= 2 && nCatAstro >= 1;
                if (catDevilUnlocked)
                {
                    std::sprintf(tooltipBuffer,
                                 "Devilcats transform bubbles into bombs that explode when popped. Bubbles affected by "
                                 "the explosion are worth x10 more! Bomb explosion range can be upgraded.");
                    std::sprintf(labelBuffer, "%zu devilcats", nCatDevil);
                    if (makePurchasableButton("Devilcat", 15000.f, 1.6f, static_cast<float>(nCatDevil)))
                    {
                        spawnCat(gameView, CatType::Devil, {0.f, 100.f});

                        if (nCatDevil == 0)
                        {
                            doTip(
                                "Devilcats transform bubbles in bombs!\nExplode them to pop nearby "
                                "bubbles\nwith a x10 money multiplier!",
                                /* maxPrestigeLevel */ 1);
                        }
                    }

                    std::sprintf(tooltipBuffer, "Increase bomb explosion radius.");
                    std::sprintf(labelBuffer, "x%.2f", static_cast<double>(game.psvExplosionRadiusMult.currentValue()));
                    makePurchasableButtonPSV("- Explosion radius", game.psvExplosionRadiusMult);

                    if (catDevilUpgradesUnlocked)
                    {
                        makeCooldownButton("- cooldown", CatType::Devil);
                        makeRangeButton("- range", CatType::Devil);
                    }

                    ImGui::Separator();
                }

                // ASTRO CAT
                const bool astroCatUnlocked         = nCatNormal >= 10 && nCatUni >= 5 && nCatDevil >= 2;
                const bool astroCatUpgradesUnlocked = astroCatUnlocked && nCatDevil >= 9 && nCatAstro >= 5;
                if (astroCatUnlocked)
                {
                    std::sprintf(tooltipBuffer,
                                 "Astrocats periodically fly across the map, popping bubbles they hit with a huge x20 "
                                 "money multiplier!\n\nThey can be permanently upgraded with prestige points to "
                                 "inspire cats watching them fly past to pop bubbles faster.");
                    std::sprintf(labelBuffer, "%zu astrocats", nCatAstro);
                    if (makePurchasableButton("astrocat", 150000.f, 1.5f, static_cast<float>(nCatAstro)))
                    {
                        spawnCat(gameView, CatType::Astro, {-64.f, 0.f});

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
                    if (catUnicornUnlocked && !game.isBubbleValueUnlocked())
                    {
                        startList("to unlock prestige:");

                        if (game.psvBubbleCount.nPurchases == 0)
                            result += "\n- buy more bubbles";

                        needNCats(nCatUni, 3);
                    }

                    if (catUnicornUnlocked && game.isBubbleValueUnlocked() && !catDevilUnlocked)
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
                        needNCats(nCatDevil, 9);
                        needNCats(nCatAstro, 5);
                    }

                    return result;
                }();

                if (milestoneText != "")
                {
                    ImGui::SetWindowFontScale(0.65f);
                    ImGui::Text("%s", milestoneText.c_str());
                    ImGui::SetWindowFontScale(normalFontScale);
                }

                ImGui::Columns(1);
                ImGui::EndTabItem();
            }

            if (game.isBubbleValueUnlocked())
            {
                if (!game.prestigeTipShown)
                {
                    game.prestigeTipShown = true;
                    doTip("Prestige to increase bubble value\nand unlock permanent upgrades!");
                }

                if (ImGui::BeginTabItem(" Prestige "))
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
                                 "WARNING: this will reset your progress!\n\nPrestige to increase bubble value "
                                 "permanently and obtain prestige points. Prestige points can be used to unlock "
                                 "powerful permanent upgrades.\n\nYou will sacrifice all your cats, bubbles, and "
                                 "money, but you will keep your prestige points and permanent upgrades, and the value "
                                 "of bubbles will be permanently increased.\n\nDo not be afraid to prestige -- it is "
                                 "what enables you to progress further!");
                    std::sprintf(labelBuffer, "current bubble value x%llu", getScaledReward(BubbleType::Normal));

                    const auto [times,
                                maxCost,
                                nextCost] = game.psvBubbleValue.maxSubsequentPurchases(game.money, globalCostMultiplier);

                    beginColumns();

                    buttonHueMod = 120.f;
                    if (makePrestigePurchasableButtonPSV("Prestige", game.psvBubbleValue, times, maxCost))
                    {
                        playSound(sounds.prestige);
                        inPrestigeTransition = true;

                        scroll = 0.f;

                        draggedCat           = nullptr;
                        catDragPressDuration = 0.f;

                        game.onPrestige(times);
                    }

                    ImGui::Columns(1);

                    buttonHueMod = 0.f;
                    ImGui::SetWindowFontScale(0.75f);

                    const auto currentMult = static_cast<SizeT>(game.psvBubbleValue.currentValue()) + 1;

                    ImGui::Text("(next prestige: $%llu)", nextCost);

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
                    ImGui::Text("- prestige points: %llu PPs", game.prestigePoints);
                    ImGui::SetWindowFontScale(normalFontScale);

                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    beginColumns();

                    buttonHueMod = 190.f;

                    std::sprintf(tooltipBuffer,
                                 "Manually popping a bubble now also pops nearby bubbles automatically!\n\nNote that "
                                 "combo multiplier still only increases once per successful click.\n\nNote: this "
                                 "effect can be toggled at will.");
                    std::sprintf(labelBuffer, "");
                    if (makePurchasablePPButtonOneTime("Multipop click", 1u, game.multiPopPurchased))
                        doTip("Popping a bubble now also pops\nnearby bubbles automatically!",
                              /* maxPrestigeLevel */ UINT_MAX);

                    if (game.multiPopPurchased)
                    {
                        std::sprintf(tooltipBuffer, "Increase the range of the multipop effect.");
                        std::sprintf(labelBuffer, "%.2fpx", static_cast<double>(game.getComputedMultiPopRange()));
                        makePrestigePurchasablePPButtonPSV("- range", game.psvMultiPopRange);

                        ImGui::SetWindowFontScale(subBulletFontScale);
                        ImGui::Checkbox("enable ##multipop", &game.multiPopEnabled);
                        ImGui::SetWindowFontScale(normalFontScale);
                        ImGui::NextColumn();
                        ImGui::NextColumn();
                    }

                    ImGui::Separator();

                    std::sprintf(tooltipBuffer,
                                 "Cats have graduated!\n\nThey still cannot resist their popping insticts, but they "
                                 "will "
                                 "go for star bubbles and bombs first, ensuring they are not wasted!");
                    std::sprintf(labelBuffer, "");
                    if (makePurchasablePPButtonOneTime("Smart cats", 1u, game.smartCatsPurchased))
                        doTip("Cats will now prioritize popping\nspecial bubbles over basic ones!",
                              /* maxPrestigeLevel */ UINT_MAX);

                    if (game.smartCatsPurchased)
                    {
                        std::sprintf(tooltipBuffer,
                                     "Embrace the glorious evolution!\n\nCats have ascended beyond their primal "
                                     "insticts and will now prioritize bombs, then star bubbles, then normal "
                                     "bubbles!\n\nThey will also ignore any bubble type of your choosing.\n\nNote: "
                                     "this effect can be toggled at will.");
                        std::sprintf(labelBuffer, "");
                        if (makePurchasablePPButtonOneTime("- genius cats", 3u, game.geniusCatsPurchased))
                            doTip("Genius cats prioritize bombs and\ncan be instructed to ignore certain bubbles!",
                                  /* maxPrestigeLevel */ UINT_MAX);
                    }

                    if (game.geniusCatsPurchased)
                    {
                        ImGui::Columns(1);
                        ImGui::SetWindowFontScale(subBulletFontScale);

                        ImGui::Text("- ignore: ");
                        ImGui::SameLine();

                        ImGui::Checkbox("normal", &game.geniusCatIgnoreNormalBubbles);
                        ImGui::SameLine();

                        ImGui::Checkbox("star", &game.geniusCatIgnoreStarBubbles);
                        ImGui::SameLine();

                        ImGui::Checkbox("bombs", &game.geniusCatIgnoreBombBubbles);

                        ImGui::SetWindowFontScale(normalFontScale);
                        beginColumns();
                    }

                    ImGui::Separator();

                    std::sprintf(tooltipBuffer,
                                 "A giant fan (off-screen) will produce an intense wind, making bubbles move and flow "
                                 "much faster.\n\nNote: this effect can be toggled at will.");
                    std::sprintf(labelBuffer, "");
                    if (makePurchasablePPButtonOneTime("Giant fan", 2u, game.windPurchased))
                        doTip("Hold onto something!", /* maxPrestigeLevel */ UINT_MAX);

                    if (game.windPurchased)
                    {
                        ImGui::SetWindowFontScale(subBulletFontScale);
                        ImGui::Checkbox("enable ##wind", &game.windEnabled);
                        ImGui::SetWindowFontScale(normalFontScale);
                        ImGui::NextColumn();
                        ImGui::NextColumn();
                    }

                    ImGui::Separator();

                    std::sprintf(tooltipBuffer,
                                 "Astrocats are now equipped with fancy patriotic flags, inspiring cats watching them "
                                 "fly by to work faster!");
                    std::sprintf(labelBuffer, "");
                    if (makePurchasablePPButtonOneTime("Space propaganda", 3u, game.astroCatInspirePurchased))
                        doTip("Astrocats will inspire other cats\nto work faster when flying by!",
                              /* maxPrestigeLevel */ UINT_MAX);

                    if (game.astroCatInspirePurchased)
                    {
                        std::sprintf(tooltipBuffer, "Increase the duration of the inspiration effect.");
                        std::sprintf(labelBuffer,
                                     "%.2fs",
                                     static_cast<double>(game.getComputedInspirationDuration() / 1000.f));

                        makePrestigePurchasablePPButtonPSV("- buff duration", game.psvInspireDurationMult);
                    }

                    buttonHueMod = 0.f;

                    ImGui::Columns(1);

                    ImGui::EndTabItem();
                }
            }

            if (ImGui::BeginTabItem(" Stats "))
            {
                const auto displayStats = [&](const Stats& stats)
                {
                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::Columns(2, "twoColumnsStats", false);
                    ImGui::SetColumnWidth(0, windowWidth / 2.f);
                    ImGui::SetColumnWidth(1, windowWidth / 2.f);

                    const auto secondsToDisplay = U64{stats.secondsPlayed % 60u};
                    const auto minutesToDisplay = U64{(stats.secondsPlayed / 60u) % 60u};
                    const auto hoursToDisplay   = U64{stats.secondsPlayed / 3600u};
                    ImGui::Text("Time played: %lluh %llum %llus", hoursToDisplay, minutesToDisplay, secondsToDisplay);

                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::Text("Bubbles popped: %llu", stats.bubblesPopped);
                    ImGui::Indent();
                    ImGui::Text("Clicked: %llu", stats.bubblesHandPopped);
                    ImGui::Text("By cats: %llu", stats.bubblesPopped - stats.bubblesHandPopped);
                    ImGui::Unindent();

                    ImGui::NextColumn();

                    ImGui::Text("Revenue: $%llu", stats.bubblesPoppedRevenue);
                    ImGui::Indent();
                    ImGui::Text("Clicked: $%llu", stats.bubblesHandPoppedRevenue);
                    ImGui::Text("By cats: $%llu", stats.bubblesPoppedRevenue - stats.bubblesHandPoppedRevenue);
                    ImGui::Text("Bombs:  $%llu", stats.explosionRevenue);
                    ImGui::Text("Flights: $%llu", stats.flightRevenue);
                    ImGui::Unindent();

                    ImGui::Columns(1);
                };

                const auto centeredText = [&](const char* str)
                {
                    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(str).x) * 0.5f);
                    ImGui::Text("%s", str);
                };

                if (ImGui::BeginTabBar("TabBarStats", ImGuiTabBarFlags_DrawSelectedOverline))
                {
                    ImGui::SetWindowFontScale(0.75f);
                    if (ImGui::BeginTabItem(" Statistics "))
                    {
                        ImGui::SetWindowFontScale(0.75f);

                        centeredText(" -- Lifetime values -- ");
                        displayStats(profile.statsLifetime);

                        ImGui::Separator();

                        centeredText(" -- Game values -- ");
                        displayStats(game.statsTotal);

                        ImGui::Separator();

                        centeredText(" -- Prestige values -- ");
                        displayStats(game.statsSession);

                        ImGui::SetWindowFontScale(normalFontScale);

                        ImGui::EndTabItem();
                    }

                    ImGui::SetWindowFontScale(0.75f);
                    if (ImGui::BeginTabItem(" Milestones "))
                    {
                        ImGui::SetWindowFontScale(0.75f);

                        const auto doMilestone = [&](const char* name, const U64 value)
                        {
                            if (value == std::numeric_limits<U64>::max())
                            {
                                ImGui::Text("%s: N/A", name);
                                return;
                            }

                            const auto secondsToDisplay = U64{value % 60u};
                            const auto minutesToDisplay = U64{(value / 60u) % 60u};
                            const auto hoursToDisplay   = U64{value / 3600u};

                            ImGui::Text("%s: %lluh %llum %llus", name, hoursToDisplay, minutesToDisplay, secondsToDisplay);
                        };

                        doMilestone("1st Cat", game.milestones.firstCat);
                        doMilestone("5th Cat", game.milestones.fiveCats);
                        doMilestone("10th Cat", game.milestones.tenCats);

                        ImGui::Separator();

                        doMilestone("1st Unicat", game.milestones.firstUnicat);
                        doMilestone("5th Unicat", game.milestones.fiveUnicats);
                        doMilestone("10th Unicat", game.milestones.tenUnicats);

                        ImGui::Separator();

                        doMilestone("1st Devilcat", game.milestones.firstDevilcat);
                        doMilestone("5th Devilcat", game.milestones.fiveDevilcats);
                        doMilestone("10th Devilcat", game.milestones.tenDevilcats);

                        ImGui::Separator();

                        doMilestone("1st Astrocat", game.milestones.firstAstrocat);
                        doMilestone("5th Astrocat", game.milestones.fiveAstrocats);
                        doMilestone("10th Astrocat", game.milestones.tenAstrocats);

                        ImGui::Separator();

                        doMilestone("Prestige Level 1", game.milestones.prestigeLevel1);
                        doMilestone("Prestige Level 2", game.milestones.prestigeLevel2);
                        doMilestone("Prestige Level 3", game.milestones.prestigeLevel3);
                        doMilestone("Prestige Level 4", game.milestones.prestigeLevel4);
                        doMilestone("Prestige Level 5", game.milestones.prestigeLevel5);
                        doMilestone("Prestige Level 10", game.milestones.prestigeLevel10);
                        doMilestone("Prestige Level 15", game.milestones.prestigeLevel15);
                        doMilestone("Prestige Level 20", game.milestones.prestigeLevel20);

                        ImGui::Separator();

                        doMilestone("$10.000 Revenue", game.milestones.revenue10000);
                        doMilestone("$100.000 Revenue", game.milestones.revenue100000);
                        doMilestone("$1.000.000 Revenue", game.milestones.revenue1000000);
                        doMilestone("$10.000.000 Revenue", game.milestones.revenue10000000);
                        doMilestone("$100.000.000 Revenue", game.milestones.revenue100000000);
                        doMilestone("$1.000.000.000 Revenue", game.milestones.revenue1000000000);

                        ImGui::EndTabItem();
                    }

                    ImGui::SetWindowFontScale(0.75f);
                    if (ImGui::BeginTabItem(" Achievements "))
                    {
                        // TODO:

                        ImGui::EndTabItem();
                    }

                    ImGui::SetWindowFontScale(normalFontScale);
                    ImGui::EndTabBar();
                }

                ImGui::SetWindowFontScale(normalFontScale);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(" Settings "))
            {
                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Master volume", &profile.masterVolume, 0.f, 100.f, "%.0f%%");

                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Music volume", &profile.musicVolume, 0.f, 100.f, "%.0f%%");

                ImGui::Checkbox("Play audio in background", &profile.playAudioInBackground);
                ImGui::Checkbox("Enable combo scratch sound", &profile.playComboEndSound);

                ImGui::SetNextItemWidth(210.f);
                ImGui::SliderFloat("Minimap Scale", &profile.minimapScale, 10.f, 30.f, "%.2f");

                ImGui::Checkbox("Enable tips", &profile.tipsEnabled);

                const float fps = 1.f / fpsClock.getElapsedTime().asSeconds();
                ImGui::Text("FPS: %.2f", static_cast<double>(fps));

                ImGui::Separator();

                if (ImGui::Button("Save game"))
                    saveGameToFile(game);

                ImGui::SameLine();

                if (ImGui::Button("Load game"))
                    loadGameFromFile(game);

                ImGui::SameLine();

                buttonHueMod = 120.f;
                pushButtonColors();

                if (ImGui::Button("Reset game"))
                {
                    game = Game{};

                    particles.clear();
                    textParticles.clear();
                }

                popButtonColors();
                buttonHueMod = 0.f;

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }


        ImGui::End();
        ImGui::PopFont();
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

                if (!clickPosition.hasValue())
                    clickPosition.emplace(e->position.toVector2f());
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
        if (keyDown(sf::Keyboard::Key::F4))
        {
            game.comboPurchased = true;
            game.mapPurchased   = true;
        }
        else if (keyDown(sf::Keyboard::Key::F5))
        {
            game.money = 1'000'000'000u;
        }
        else if (keyDown(sf::Keyboard::Key::F6))
        {
            game.money += 15u;
        }
        else if (keyDown(sf::Keyboard::Key::F7))
        {
            game.prestigePoints += 15u;
        }

        //
        // Number of fingers
        std::vector<sf::Vector2f> downFingers;
        for (const auto maybeFinger : fingerPositions)
            if (maybeFinger.hasValue())
                downFingers.push_back(*maybeFinger);

        const auto countFingersDown = downFingers.size();

        //
        // Map scrolling via keyboard and touch
        if (game.mapPurchased)
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
        if (dragPosition.hasValue() && countFingersDown != 2u && !mBtnDown(sf::Mouse::Button::Right))
        {
            dragPosition.reset();
        }

        //
        // Scrolling
        scroll = sf::base::clamp(scroll,
                                 0.f,
                                 sf::base::min(game.getMapLimit() / 2.f - gameScreenSize.x / 2.f,
                                               (boundaries.x - gameScreenSize.x) / 2.f));

        actualScroll = exponentialApproach(actualScroll, scroll, deltaTimeMs, 75.f);

        const sf::View gameView //
            {.center = {sf::base::clamp(gameScreenSize.x / 2.f + actualScroll * 2.f,
                                        gameScreenSize.x / 2.f,
                                        boundaries.x - gameScreenSize.x / 2.f),
                        gameScreenSize.y / 2.f},
             .size   = gameScreenSize};

        // TODO: if windows is resized this needs to be adapted
        const auto windowSpaceMouseOrFingerPos = countFingersDown == 1u ? downFingers[0].toVector2i()
                                                                        : sf::Mouse::getPosition(window);

        const auto mousePos = window.mapPixelToCoords(windowSpaceMouseOrFingerPos, gameView);

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
            game.psvBubbleCount.currentValue() / (boundaries.x / gameScreenSize.x));
        const auto nScreens          = static_cast<SizeT>(game.getMapLimit() / gameScreenSize.x) + 1;
        const auto targetBubbleCount = targetBubbleCountPerScreen * nScreens;

        //
        // Startup and bubble spawning
        const auto playReversePopAt = [&](const sf::Vector2f position)
        {
            sounds.reversePop.setPosition({position.x, position.y});
            playSound(sounds.reversePop, /* overlap */ false);
        };

        if (splashCountdown.updateAndStop(deltaTimeMs))
        {
            if (catRemoveTimer.updateAndLoop(deltaTimeMs))
            {
                if (inPrestigeTransition && !game.cats.empty())
                {
                    const auto cPos = game.cats.back().position;
                    game.cats.pop_back();

                    spawnParticles(24, cPos, ParticleType::Star, 1.f, 0.5f);
                    playReversePopAt(cPos);
                }
            }

            if (bubbleSpawnTimer.updateAndLoop(deltaTimeMs))
            {
                if (inPrestigeTransition)
                {
                    if (!game.bubbles.empty())
                    {
                        const SizeT times = game.bubbles.size() > 500 ? 25 : 1;

                        for (SizeT i = 0; i < times; ++i)
                        {
                            const auto bPos = game.bubbles.back().position;
                            game.bubbles.pop_back();

                            spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
                            playReversePopAt(bPos);
                        }
                    }
                }
                else if (game.bubbles.size() < targetBubbleCount)
                {
                    const SizeT times = (targetBubbleCount - game.bubbles.size()) > 500 ? 25 : 1;

                    for (SizeT i = 0; i < times; ++i)
                    {
                        const auto bPos = game.bubbles.emplace_back(makeRandomBubble(game.getMapLimit(), boundaries.y)).position;

                        spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
                        playReversePopAt(bPos);
                    }
                }
            }

            // End prestige transition
            if (inPrestigeTransition && game.cats.empty() && game.bubbles.empty())
            {
                inPrestigeTransition = false;
                game.money           = 0u;
                splashCountdown.reset();
                playSound(sounds.byteMeow);
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
                const sf::Vector2f position,
                bool               popSoundOverlap) -> void
        {
            statBubblePopped(byHand, reward);

            auto& tp = textParticles.emplace_back(makeTextParticle(position, combo));
            std::snprintf(tp.buffer, sizeof(tp.buffer), "+$%llu", reward);

            sounds.pop.setPosition({position.x, position.y});
            sounds.pop.setPitch(remap(static_cast<float>(combo), 1, 10, 1.f, 2.f));

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

                    const MoneyType newReward = getScaledReward(bubble.type) * 10u;

                    statExplosionRevenue(newReward);

                    self(self, byHand, bubble.type, newReward, 1, bubble.position, /* popSoundOverlap */ false);
                    addReward(newReward);
                    bubble = makeRandomBubble(game.getMapLimit(), 0.f);
                    bubble.position.y -= bubble.getRadius();

                    return ControlFlow::Continue;
                };

                forEachBubbleInRadius(position, game.getComputedBombExplosionRadius(), explosionAction);
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

            const float radius = bubble.getRadius();

            if (pos.x - radius > game.getMapLimit())
                pos.x = -radius;
            else if (pos.x + radius < 0.f)
                pos.x = game.getMapLimit() + radius;

            if (pos.y - radius > boundaries.y)
            {
                pos.x = getRndFloat(0.f, game.getMapLimit());
                pos.y = -radius;

                bubble.velocity.y = game.windEnabled ? 0.2f : 0.05f;

                if (sf::base::fabs(bubble.velocity.x) > 0.04f)
                    bubble.velocity.x = 0.04f;

                bubble.type = BubbleType::Normal;
            }
            else if (pos.y + radius < 0.f)
                pos.y = boundaries.y + radius;

            bubble.velocity.y += 0.00005f * deltaTimeMs;
        }

        const auto popWithRewardAndReplaceBubble =
            [&getScaledReward,
             &popBubble,
             &addReward](const Game& game, const bool byHand, Bubble& bubble, int combo, bool popSoundOverlap)
        {
            const auto reward = static_cast<MoneyType>(
                sf::base::ceil(static_cast<float>(getScaledReward(bubble.type)) * getComboValueMult(combo)));

            popBubble(popBubble, byHand, bubble.type, reward, combo, bubble.position, popSoundOverlap);
            addReward(reward);
            bubble = makeRandomBubble(game.getMapLimit(), 0.f);
            bubble.position.y -= bubble.getRadius();
        };

        bool anyBubblePoppedByClicking = false;

        if (clickPosition.hasValue())
        {
            const auto clickPos = window.mapPixelToCoords(clickPosition->toVector2i(), gameView);

            const auto clickAction = [&](Bubble& bubble)
            {
                if ((clickPos - bubble.position).length() > bubble.getRadius())
                    return ControlFlow::Continue;

                anyBubblePoppedByClicking = true;

                if (game.comboPurchased)
                {
                    if (combo == 0)
                    {
                        combo                = 1;
                        comboCountdown.value = game.psvComboStartTime.currentValue() * 1000.f;
                    }
                    else
                    {
                        combo += 1;
                        comboCountdown.value += 150.f - sf::base::clamp(static_cast<float>(combo) * 10.f, 0.f, 100.f);

                        comboTextShakeEffect.bump(1.f + static_cast<float>(combo) * 0.2f);
                    }
                }
                else
                {
                    combo = 1;
                }

                popWithRewardAndReplaceBubble(game, /* byHand */ true, bubble, combo, /* popSoundOverlap */ true);

                if (game.multiPopEnabled)
                    forEachBubbleInRadius(clickPos,
                                          game.getComputedMultiPopRange(),
                                          [&](Bubble& otherBubble)
                                          {
                                              if (&otherBubble != &bubble)
                                                  popWithRewardAndReplaceBubble(game,
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

                const auto checkAstro = [&game](auto& catA, auto& catB)
                {
                    if (catA.isAstroAndInFlight() && catB.type != CatType::Astro)
                    {
                        if (game.astroCatInspirePurchased &&
                            detectCollision(catA.position, catB.position, catA.getRadius(), catB.getRadius()))
                            catB.inspiredCountdown.value = game.getComputedInspirationDuration();

                        return true;
                    }

                    return false;
                };

                if (checkAstro(iCat, jCat))
                    continue;

                // NOLINTNEXTLINE(readability-suspicious-call-argument)
                if (checkAstro(jCat, iCat))
                    continue;

                handleCatCollision(deltaTimeMs, game.cats[i], game.cats[j]);
            }

        if (mBtnDown(sf::Mouse::Button::Left) || (countFingersDown == 1))
        {
            if (draggedCat)
            {
                draggedCat->position = exponentialApproach(draggedCat->position, mousePos, deltaTimeMs, 50.f);
                draggedCat->cooldownTimer.value = -250.f;
            }
            else
            {
                constexpr float catDragPressDurationMax = 100.f;

                Cat* hoveredCat = nullptr;

                // Only check for hover targets during initial press phase
                if (catDragPressDuration <= catDragPressDurationMax)
                    for (Cat& cat : game.cats)
                        if (!cat.isAstroAndInFlight() && (mousePos - cat.position).length() <= cat.getRadius())
                            hoveredCat = &cat;

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
        }
        else
        {
            if (draggedCat)
            {
                playSound(sounds.drop);
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

            const auto drawPosition = cat.getDrawPosition();

            auto diff = cat.pawPosition - drawPosition - sf::Vector2f{-25.f, 25.f};
            cat.pawPosition -= diff * 0.01f * deltaTimeMs;
            cat.pawRotation = cat.pawRotation.rotatedTowards(sf::degrees(-45.f), deltaTimeMs * 0.005f);

            if (cat.cooldownTimer.value < 0.f)
            {
                cat.pawOpacity  = 128.f;
                cat.mainOpacity = 128.f;
            }
            else
            {
                cat.mainOpacity = 255.f;
            }

            if (cat.cooldownTimer.value == maxCooldown && cat.pawOpacity > 10.f)
                cat.pawOpacity -= 0.5f * deltaTimeMs;

            cat.update(deltaTimeMs);

            const auto [cx, cy] = cat.position + cat.rangeOffset;

            if (cat.inspiredCountdown.value > 0.f)
            {
                if (getRndFloat(0.f, 1.f) > 0.5f)
                    particles.push_back(
                        {.data = {.position = drawPosition + sf::Vector2f{getRndFloat(-catRadius, +catRadius), catRadius},
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
                    const MoneyType newReward = getScaledReward(bubble.type) * 20u;

                    statFlightRevenue(newReward);

                    popBubble(popBubble,
                              /* byHand */ false,
                              bubble.type,
                              newReward,
                              /* combo */ 1,
                              bubble.position,
                              /* popSoundOverlap */ getRndFloat(0.f, 1.f) > 0.75f);
                    addReward(newReward);
                    bubble = makeRandomBubble(game.getMapLimit(), 0.f);
                    bubble.position.y -= bubble.getRadius();

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
                    cat.position.x          = startX;
                    cat.cooldownTimer.value = 0.f;
                }

                continue;
            }

            if (!cat.updateCooldown(maxCooldown, deltaTimeMs))
                continue;

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

            if (cat.type == CatType::Witch) // TODO: change
            {
                sf::base::U32 witchHits = 0u;
                bool          pawSet    = false;

                for (auto& otherCat : game.cats)
                {
                    if (otherCat.type == CatType::Witch)
                        continue;

                    if ((otherCat.position - cat.position).length() > range)
                        continue;

                    otherCat.cooldownTimer.value = game.getComputedCooldownByCatType(cat.type);
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
                    playSound(sounds.hex);

                    cat.textStatusShakeEffect.bump(1.5f);
                    cat.hits += witchHits;
                }

                cat.cooldownTimer.value = 0.f;
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
                    playSound(sounds.shine);

                    spawnParticles(4, bubble.position, ParticleType::Star, 0.5f, 0.35f);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;
                }
                else if (cat.type == CatType::Normal)
                {
                    popWithRewardAndReplaceBubble(game, /* byHand */ false, bubble, /* combo */ 1, /* popSoundOverlap */ true);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;
                }
                else if (cat.type == CatType::Devil)
                {
                    bubble.type = BubbleType::Bomb;
                    bubble.velocity.y += getRndFloat(0.1f, 0.2f);
                    sounds.makeBomb.setPosition({bubble.position.x, bubble.position.y});
                    playSound(sounds.makeBomb);

                    spawnParticles(8, bubble.position, ParticleType::Fire, 1.25f, 0.35f);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;
                }

                cat.cooldownTimer.value = 0.f;
                return ControlFlow::Break;
            };

            const auto makeFilteredPopAction =
                [&popWithRewardAndReplaceBubble](Game&      game,
                                                 Cat&       cat,
                                                 bool&      successFlag,
                                                 const bool ignoreNormal,
                                                 const bool ignoreStar,
                                                 const bool ignoreBomb)
            {
                return [&, ignoreNormal, ignoreStar, ignoreBomb](Bubble& bubble)
                {
                    if (ignoreNormal && bubble.type == BubbleType::Normal)
                        return ControlFlow::Continue;

                    if (ignoreStar && bubble.type == BubbleType::Star)
                        return ControlFlow::Continue;

                    if (ignoreBomb && bubble.type == BubbleType::Bomb)
                        return ControlFlow::Continue;

                    cat.pawPosition = bubble.position;
                    cat.pawOpacity  = 255.f;
                    cat.pawRotation = (bubble.position - cat.position).angle() + sf::degrees(45);

                    successFlag = true;

                    popWithRewardAndReplaceBubble(game, /* byHand */ false, bubble, /* combo */ 1, /* popSoundOverlap */ true);

                    cat.textStatusShakeEffect.bump(1.5f);
                    ++cat.hits;

                    cat.cooldownTimer.value = 0.f;
                    return ControlFlow::Break;
                };
            };

            if (cat.type == CatType::Normal)
            {
                if (game.geniusCatsPurchased)
                {
                    bool geniusActionSuccess = false;

                    const auto geniusAction0 = makeFilteredPopAction(game,
                                                                     cat,
                                                                     geniusActionSuccess,
                                                                     /* ignoreNormal */ true,
                                                                     /* ignoreStar */ true,
                                                                     /* ignoreBomb */ game.geniusCatIgnoreBombBubbles);

                    forEachBubbleInRadius({cx, cy}, range, geniusAction0);

                    if (!geniusActionSuccess)
                    {
                        const auto geniusAction1 = makeFilteredPopAction(game,
                                                                         cat,
                                                                         geniusActionSuccess,
                                                                         /* ignoreNormal */ true,
                                                                         /* ignoreStar */ game.geniusCatIgnoreStarBubbles,
                                                                         /* ignoreBomb */ game.geniusCatIgnoreBombBubbles);

                        forEachBubbleInRadius({cx, cy}, range, geniusAction1);

                        if (!geniusActionSuccess)
                        {
                            const auto
                                geniusAction2 = makeFilteredPopAction(game,
                                                                      cat,
                                                                      geniusActionSuccess,
                                                                      /* ignoreNormal */ game.geniusCatIgnoreNormalBubbles,
                                                                      /* ignoreStar */ game.geniusCatIgnoreStarBubbles,
                                                                      /* ignoreBomb */ game.geniusCatIgnoreBombBubbles);

                            forEachBubbleInRadius({cx, cy}, range, geniusAction2);
                        }
                    }
                }
                else if (game.smartCatsPurchased)
                {
                    bool       smartActionSuccess = false;
                    const auto smartAction        = makeFilteredPopAction(game,
                                                                   cat,
                                                                   smartActionSuccess,
                                                                   /* ignoreNormal */ true,
                                                                   /* ignoreStar */ false,
                                                                   /* ignoreBomb */ false);

                    forEachBubbleInRadius({cx, cy}, range, smartAction);

                    if (!smartActionSuccess)
                        forEachBubbleInRadius({cx, cy}, range, action);
                }
                else
                {
                    forEachBubbleInRadius({cx, cy}, range, action);
                }
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

        if (comboCountdown.updateAndStop(deltaTimeMs))
            combo = 0;

        const float volumeMult = profile.playAudioInBackground || window.hasFocus() ? 1.f : 0.f;

        listener.volume = profile.masterVolume * volumeMult;
        musicBGM.setVolume(profile.musicVolume * volumeMult);

        if (sounds.isPlayingPooled(sounds.prestige))
            musicBGM.setVolume(0.f);

        playedUsAccumulator += playedClock.getElapsedTime().asMicroseconds();
        playedClock.restart();

        while (playedUsAccumulator > 1'000'000)
        {
            playedUsAccumulator -= 1'000'000;
            statSecondsPlayed();
        }

        // TODO:
        {
            const auto updateMilestone = [&](const char* name, U64& milestone, bool showTip = true)
            {
                const U64 oldMilestone = milestone;

                milestone = sf::base::min(milestone, game.statsTotal.secondsPlayed);

                if (showTip && milestone != oldMilestone)
                    doTip("Milestone reached!\n -> " + std::string{name}, /* maxPrestigeLevel */ UINT_MAX);
            };

            const auto nCatNormal = game.getCatCountByType(CatType::Normal);
            const auto nCatUni    = game.getCatCountByType(CatType::Uni);
            const auto nCatDevil  = game.getCatCountByType(CatType::Devil);
            const auto nCatAstro  = game.getCatCountByType(CatType::Astro);

            if (nCatNormal >= 1)
                updateMilestone("1st Cat", game.milestones.firstCat, /* showTip */ false);

            if (nCatUni >= 1)
                updateMilestone("1st Unicat", game.milestones.firstUnicat, /* showTip */ false);

            if (nCatDevil >= 1)
                updateMilestone("1st Devilcat", game.milestones.firstDevilcat, /* showTip */ false);

            if (nCatAstro >= 1)
                updateMilestone("1st Astrocat", game.milestones.firstAstrocat, /* showTip */ false);

            if (nCatNormal >= 5)
                updateMilestone("5th Cat", game.milestones.fiveCats);

            if (nCatUni >= 5)
                updateMilestone("5th Unicat", game.milestones.fiveUnicats);

            if (nCatDevil >= 5)
                updateMilestone("5th Devilcat", game.milestones.fiveDevilcats);

            if (nCatAstro >= 5)
                updateMilestone("5th Astrocat", game.milestones.fiveAstrocats);

            if (nCatNormal >= 10)
                updateMilestone("10th Cat", game.milestones.tenCats);

            if (nCatUni >= 10)
                updateMilestone("10th Unicat", game.milestones.tenUnicats);

            if (nCatDevil >= 10)
                updateMilestone("10th Devilcat", game.milestones.tenDevilcats);

            if (nCatAstro >= 10)
                updateMilestone("10th Astrocat", game.milestones.tenAstrocats);

            if (game.psvBubbleValue.nPurchases >= 1)
                updateMilestone("Prestige Level 1", game.milestones.prestigeLevel1);

            if (game.psvBubbleValue.nPurchases >= 2)
                updateMilestone("Prestige Level 2", game.milestones.prestigeLevel2);

            if (game.psvBubbleValue.nPurchases >= 3)
                updateMilestone("Prestige Level 3", game.milestones.prestigeLevel3);

            if (game.psvBubbleValue.nPurchases >= 4)
                updateMilestone("Prestige Level 4", game.milestones.prestigeLevel4);

            if (game.psvBubbleValue.nPurchases >= 5)
                updateMilestone("Prestige Level 5", game.milestones.prestigeLevel5);

            if (game.psvBubbleValue.nPurchases >= 10)
                updateMilestone("Prestige Level 10", game.milestones.prestigeLevel10);

            if (game.psvBubbleValue.nPurchases >= 15)
                updateMilestone("Prestige Level 15", game.milestones.prestigeLevel15);

            if (game.psvBubbleValue.nPurchases >= 20)
                updateMilestone("Prestige Level 20", game.milestones.prestigeLevel20);

            if (game.statsTotal.bubblesPoppedRevenue >= 10'000)
                updateMilestone("$10.000 Revenue", game.milestones.revenue10000);

            if (game.statsTotal.bubblesPoppedRevenue >= 100'000)
                updateMilestone("$100.000 Revenue", game.milestones.revenue100000);

            if (game.statsTotal.bubblesPoppedRevenue >= 1'000'000)
                updateMilestone("$1.000.000 Revenue", game.milestones.revenue1000000);

            if (game.statsTotal.bubblesPoppedRevenue >= 10'000'000)
                updateMilestone("$10.000.000 Revenue", game.milestones.revenue10000000);

            if (game.statsTotal.bubblesPoppedRevenue >= 100'000'000)
                updateMilestone("$100.000.000 Revenue", game.milestones.revenue100000000);

            if (game.statsTotal.bubblesPoppedRevenue >= 1'000'000'000)
                updateMilestone("$1.000.000.000 Revenue", game.milestones.revenue1000000000);
        }

        imGuiContext.update(window, deltaTime);

        const bool shouldDrawUI = inPrestigeTransition == 0 && splashCountdown.value <= 0.f;

        if (shouldDrawUI)
            drawUI(gameView);

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

        const sf::FloatRect* const normalCatTxr = game.geniusCatsPurchased  ? &txrGeniusCat
                                                  : game.smartCatsPurchased ? &txrSmartCat
                                                                            : &txrCat;

        const sf::FloatRect* const astroCatTxr = game.astroCatInspirePurchased ? &txrAstroCatWithFlag : &txrAstroCat;

        const sf::FloatRect* const catTxrsByType[nCatTypes]{
            normalCatTxr, // Normal
            &txrUniCat,   // Unicorn
            &txrDevilCat, // Devil
            &txrWitchCat, // Witch
            astroCatTxr,  // Astro
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
            if (!anyCatHovered && &cat != draggedCat && !cat.isAstroAndInFlight() &&
                (mousePos - cat.position).length() <= cat.getRadius() && !mBtnDown(sf::Mouse::Button::Left))
            {
                anyCatHovered = true;
                opacityMod    = 0.5f;
            }

            const auto& catTxr    = *catTxrsByType[static_cast<int>(cat.type)];
            const auto& catPawTxr = *catPawTxrsByType[static_cast<int>(cat.type)];

            const auto maxCooldown  = game.getComputedCooldownByCatType(cat.type);
            const auto cooldownDiff = (maxCooldown - cat.cooldownTimer.value);

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
                sf::Sprite{.position    = cat.getDrawPosition(),
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
                cat.cooldownTimer.value < 0.f ? static_cast<U8>(0u)
                                              : static_cast<U8>(cat.cooldownTimer.value / maxCooldown * 128.f)));
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
            tempSprite.scale *= bubble.type == BubbleType::Bomb ? 1.65f : 1.f;

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

        const float yBelowMinimap = game.mapPurchased ? (boundaries.y / profile.minimapScale) + 12.f : 0.f;

        moneyText.position.y = yBelowMinimap + 30.f;

        if (shouldDrawUI)
            window.draw(moneyText);

        if (game.comboPurchased)
        {
            comboText.setString("x" + std::to_string(combo + 1));

            comboTextShakeEffect.update(deltaTimeMs);
            comboTextShakeEffect.applyToText(comboText);

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
        if (game.mapPurchased)
            drawMinimap(profile.minimapScale, game.getMapLimit(), gameView, hudView, window, txBackground, cpuDrawableBatch, textureAtlas);

        //
        // UI
        imGuiContext.render(window);

        //
        // Splash screen
        if (splashCountdown.value > 0.f)
            drawSplashScreen(window, txLogo, splashCountdown);

        //
        // Tips
        if (tipTimer > 0.f)
        {
            tipTimer -= deltaTimeMs;

            if (profile.tipsEnabled)
            {
                float fade = 255.f;

                if (tipTimer > 4500.f)
                    fade = remap(tipTimer, 4500.f, 5000.f, 255.f, 0.f);
                else if (tipTimer < 500.f)
                    fade = remap(tipTimer, 0.f, 500.f, 0.f, 255.f);

                const auto alpha = static_cast<U8>(sf::base::clamp(fade, 0.f, 255.f));

                sounds.byteSpeak.setPitch(1.6f);

                sf::Text tipText{fontSuperBakery,
                                 {.position         = {},
                                  .string           = tipString.substr(0,
                                                             static_cast<SizeT>(
                                                                 sf::base::clamp((4100.f - tipTimer) / 25.f,
                                                                                 0.f,
                                                                                 static_cast<float>(tipString.size())))),
                                  .characterSize    = 29u,
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

                tipSprite.setBottomCenter({resolution.x / 2.f, resolution.y - 50.f});
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
        }

        //
        // Display
        window.display();
    }
}

// TODO IDEAS:
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
// - map expansion witches

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
