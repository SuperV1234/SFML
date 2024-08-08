#pragma once
#include "imgui_internal.h"

#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics/Export.hpp>

#include <SFML/Graphics/Color.hpp>

#include <SFML/Window/Joystick.hpp>

#include <SFML/System/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <SFML/Base/InPlacePImpl.hpp>
#include <SFML/Base/Optional.hpp>

namespace sf
{
class Event;
class GraphicsContext;
class RenderTarget;
class RenderTexture;
class RenderWindow;
class Sprite;
class Texture;
class Window;
} // namespace sf


namespace sf::ImGui
{
class [[nodiscard]] ImGuiContext
{
public:
    [[nodiscard]] explicit ImGuiContext(GraphicsContext& graphicsContext);
    ~ImGuiContext();

    ImGuiContext(const ImGuiContext&)            = delete;
    ImGuiContext& operator=(const ImGuiContext&) = delete;

    ImGuiContext(ImGuiContext&&) noexcept;
    ImGuiContext& operator=(ImGuiContext&&) noexcept;

    void processEvent(const Event&);

private:
    struct Impl;
    base::InPlacePImpl<Impl, 16> m_impl; //!< Implementation details
};


[[nodiscard]] SFML_GRAPHICS_API bool Init(sf::GraphicsContext& graphicsContext,
                                          sf::RenderWindow&    window,
                                          bool                 loadDefaultFont = true);

[[nodiscard]] SFML_GRAPHICS_API bool Init(sf::GraphicsContext& graphicsContext,
                                          sf::Window&          window,
                                          sf::RenderTarget&    target,
                                          bool                 loadDefaultFont = true);

[[nodiscard]] SFML_GRAPHICS_API bool Init(sf::GraphicsContext& graphicsContext,
                                          sf::Window&          window,
                                          sf::Vector2f         displaySize,
                                          bool                 loadDefaultFont = true);

SFML_GRAPHICS_API void SetCurrentWindow(const sf::Window& window);
SFML_GRAPHICS_API void ProcessEvent(const sf::Window& window, const sf::Event& event);

SFML_GRAPHICS_API void Update(sf::RenderWindow& window, sf::Time dt);
SFML_GRAPHICS_API void Update(sf::Window& window, sf::RenderTarget& target, sf::Time dt);
SFML_GRAPHICS_API void Update(sf::Vector2i mousePos, sf::Vector2f displaySize, sf::Time dt);

SFML_GRAPHICS_API void Render(sf::RenderWindow& window);
SFML_GRAPHICS_API void Render(sf::RenderTarget& target);
SFML_GRAPHICS_API void Render();

SFML_GRAPHICS_API void Shutdown(const sf::Window& window);
// Shuts down all ImGui contexts
SFML_GRAPHICS_API void Shutdown();

[[nodiscard]] SFML_GRAPHICS_API bool UpdateFontTexture(sf::GraphicsContext& graphicsContext);
[[nodiscard]] SFML_GRAPHICS_API sf::base::Optional<sf::Texture>& GetFontTexture();

// joystick functions
SFML_GRAPHICS_API void SetActiveJoystickId(unsigned int joystickId);
SFML_GRAPHICS_API void SetJoystickDPadThreshold(float threshold);
SFML_GRAPHICS_API void SetJoystickLStickThreshold(float threshold);
SFML_GRAPHICS_API void SetJoystickRStickThreshold(float threshold);
SFML_GRAPHICS_API void SetJoystickLTriggerThreshold(float threshold);
SFML_GRAPHICS_API void SetJoystickRTriggerThreshold(float threshold);

SFML_GRAPHICS_API void SetJoystickMapping(int key, unsigned int joystickButton);
SFML_GRAPHICS_API void SetDPadXAxis(sf::Joystick::Axis dPadXAxis, bool inverted = false);
SFML_GRAPHICS_API void SetDPadYAxis(sf::Joystick::Axis dPadYAxis, bool inverted = false);
SFML_GRAPHICS_API void SetLStickXAxis(sf::Joystick::Axis lStickXAxis, bool inverted = false);
SFML_GRAPHICS_API void SetLStickYAxis(sf::Joystick::Axis lStickYAxis, bool inverted = false);
SFML_GRAPHICS_API void SetRStickXAxis(sf::Joystick::Axis rStickXAxis, bool inverted = false);
SFML_GRAPHICS_API void SetRStickYAxis(sf::Joystick::Axis rStickYAxis, bool inverted = false);
SFML_GRAPHICS_API void SetLTriggerAxis(sf::Joystick::Axis lTriggerAxis);
SFML_GRAPHICS_API void SetRTriggerAxis(sf::Joystick::Axis rTriggerAxis);

// custom SFML overloads for ImGui widgets

// Image overloads for sf::Texture
SFML_GRAPHICS_API void Image(const sf::Texture& texture,
                             sf::Color          tintColor   = sf::Color::White,
                             sf::Color          borderColor = sf::Color::Transparent);
SFML_GRAPHICS_API void Image(const sf::Texture& texture,
                             sf::Vector2f       size,
                             sf::Color          tintColor   = sf::Color::White,
                             sf::Color          borderColor = sf::Color::Transparent);

// Image overloads for sf::RenderTexture
SFML_GRAPHICS_API void Image(const sf::RenderTexture& texture,
                             sf::Color                tintColor   = sf::Color::White,
                             sf::Color                borderColor = sf::Color::Transparent);
SFML_GRAPHICS_API void Image(const sf::RenderTexture& texture,
                             sf::Vector2f             size,
                             sf::Color                tintColor   = sf::Color::White,
                             sf::Color                borderColor = sf::Color::Transparent);

// Image overloads for sf::Sprite
SFML_GRAPHICS_API void Image(const sf::Sprite&  sprite,
                             const sf::Texture& texture,
                             sf::Color          tintColor   = sf::Color::White,
                             sf::Color          borderColor = sf::Color::Transparent);
SFML_GRAPHICS_API void Image(const sf::Sprite&  sprite,
                             const sf::Texture& texture,
                             sf::Vector2f       size,
                             sf::Color          tintColor   = sf::Color::White,
                             sf::Color          borderColor = sf::Color::Transparent);

// ImageButton overloads for sf::Texture
[[nodiscard]] SFML_GRAPHICS_API bool ImageButton(
    const char*        id,
    const sf::Texture& texture,
    sf::Vector2f       size,
    sf::Color          bgColor   = sf::Color::Transparent,
    sf::Color          tintColor = sf::Color::White);

// ImageButton overloads for sf::RenderTexture
[[nodiscard]] SFML_GRAPHICS_API bool ImageButton(
    const char*              id,
    const sf::RenderTexture& texture,
    sf::Vector2f             size,
    sf::Color                bgColor   = sf::Color::Transparent,
    sf::Color                tintColor = sf::Color::White);

// ImageButton overloads for sf::Sprite
[[nodiscard]] SFML_GRAPHICS_API bool ImageButton(
    const char*        id,
    const sf::Sprite&  sprite,
    const sf::Texture& texture,
    sf::Vector2f       size,
    sf::Color          bgColor   = sf::Color::Transparent,
    sf::Color          tintColor = sf::Color::White);

// Draw_list overloads. All positions are in relative coordinates (relative to top-left of the
// current window)
SFML_GRAPHICS_API void DrawLine(sf::Vector2f a, sf::Vector2f b, sf::Color col, float thickness = 1.0f);
SFML_GRAPHICS_API void DrawRect(const sf::FloatRect& rect,
                                sf::Color            color,
                                float                rounding        = 0.0f,
                                int                  roundingCorners = 0x0F,
                                float                thickness       = 1.0f);
SFML_GRAPHICS_API void DrawRectFilled(const sf::FloatRect& rect, sf::Color color, float rounding = 0.0f, int roundingCorners = 0x0F);
} // namespace sf::ImGui
