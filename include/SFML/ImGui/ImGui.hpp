#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/ImGui/Export.hpp"

#include "SFML/Graphics/Color.hpp"

#include "SFML/Window/JoystickAxis.hpp"

#include "SFML/System/Rect.hpp"
#include "SFML/System/Time.hpp"
#include "SFML/System/Vector2.hpp"

#include "SFML/Base/InPlacePImpl.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/Traits/IsSame.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace sf
{
class Event;
class GraphicsContext;
class RenderTarget;
class RenderTexture;
class RenderWindow;
struct Sprite;
class Texture;
class Window;
} // namespace sf

namespace sf::ImGui
{
class ImGuiContext;
} // namespace sf::ImGui


namespace sf::ImGui
{
////////////////////////////////////////////////////////////
/// \brief TODO P1: docs
///
////////////////////////////////////////////////////////////
template <typename TWindow>
class [[nodiscard]] SFML_IMGUI_API ImGuiWindowContext
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void activate();

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void update(Time dt) requires(SFML_BASE_IS_SAME(TWindow, RenderWindow));

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void update(RenderTarget& target, Time dt);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void processEvent(const Event& event);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void render() requires(SFML_BASE_IS_SAME(TWindow, RenderWindow));

private:
    friend ImGuiContext;

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    explicit ImGuiWindowContext(ImGuiContext& imGuiContext, TWindow& window);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    ImGuiContext& m_imGuiContext;
    TWindow&      m_window;
};

extern template class ImGuiWindowContext<Window>;
extern template class ImGuiWindowContext<RenderWindow>;

////////////////////////////////////////////////////////////
/// \brief TODO P1: docs
///
////////////////////////////////////////////////////////////
class [[nodiscard]] SFML_IMGUI_API ImGuiContext
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Constructor
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit ImGuiContext(GraphicsContext& graphicsContext);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~ImGuiContext();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext(const ImGuiContext&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext& operator=(const ImGuiContext&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext(ImGuiContext&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext& operator=(ImGuiContext&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    template <typename TWindow>
    [[nodiscard]] bool initImpl(TWindow& window, Vector2f displaySize, bool loadDefaultFont = true);

    [[nodiscard]] base::Optional<ImGuiWindowContext<RenderWindow>> init(RenderWindow& window, bool loadDefaultFont = true);

    [[nodiscard]] base::Optional<ImGuiWindowContext<Window>> init(Window&       window,
                                                                  RenderTarget& target,
                                                                  bool          loadDefaultFont = true);

    [[nodiscard]] base::Optional<ImGuiWindowContext<Window>> init(Window&  window,
                                                                  Vector2f displaySize,
                                                                  bool     loadDefaultFont = true);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void processEvent(const Window& window, const Event& event);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void update(RenderWindow& window, Time dt);
    void update(Window& window, RenderTarget& target, Time dt);
    void update(Vector2i mousePos, Vector2f displaySize, Time dt);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void render(RenderWindow& window);
    void render(RenderTarget& target);
    void render();

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void shutdown(const Window& window);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void setActiveJoystickId(unsigned int joystickId);
    void setJoystickDPadThreshold(float threshold);
    void setJoystickLStickThreshold(float threshold);
    void setJoystickRStickThreshold(float threshold);
    void setJoystickLTriggerThreshold(float threshold);
    void setJoystickRTriggerThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickMapping(int key, unsigned int joystickButton);
    void setDPadXAxis(Joystick::Axis dPadXAxis, bool inverted = false);
    void setDPadYAxis(Joystick::Axis dPadYAxis, bool inverted = false);
    void setLStickXAxis(Joystick::Axis lStickXAxis, bool inverted = false);
    void setLStickYAxis(Joystick::Axis lStickYAxis, bool inverted = false);
    void setRStickXAxis(Joystick::Axis rStickXAxis, bool inverted = false);
    void setRStickYAxis(Joystick::Axis rStickYAxis, bool inverted = false);
    void setLTriggerAxis(Joystick::Axis lTriggerAxis);
    void setRTriggerAxis(Joystick::Axis rTriggerAxis);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void setCurrentWindow(const Window& window);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void image(const Texture& texture, Color tintColor = Color::White, Color borderColor = Color::Transparent);
    void image(const Texture& texture, Vector2f size, Color tintColor = Color::White, Color borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void image(const RenderTexture& texture, Color tintColor = Color::White, Color borderColor = Color::Transparent);
    void image(const RenderTexture& texture, Vector2f size, Color tintColor = Color::White, Color borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void image(const Sprite&  sprite,
               const Texture& texture,
               Color          tintColor   = Color::White,
               Color          borderColor = Color::Transparent);

    void image(const Sprite&  sprite,
               const Texture& texture,
               Vector2f       size,
               Color          tintColor   = Color::White,
               Color          borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool imageButton(const char*    id,
                                   const Texture& texture,
                                   Vector2f       size,
                                   Color          bgColor   = Color::Transparent,
                                   Color          tintColor = Color::White);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool imageButton(const char*          id,
                                   const RenderTexture& texture,
                                   Vector2f             size,
                                   Color                bgColor   = Color::Transparent,
                                   Color                tintColor = Color::White);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool imageButton(const char*    id,
                                   const Sprite&  sprite,
                                   const Texture& texture,
                                   Vector2f       size,
                                   Color          bgColor   = Color::Transparent,
                                   Color          tintColor = Color::White);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void drawLine(Vector2f a, Vector2f b, Color col, float thickness = 1.0f);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void drawRect(const FloatRect& rect, Color color, float rounding = 0.0f, int roundingCorners = 0x0F, float thickness = 1.0f);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void drawRectFilled(const FloatRect& rect, Color color, float rounding = 0.0f, int roundingCorners = 0x0F);

private:
    // Shuts down all ImGui contexts
    void shutdown();

    struct Impl;
    base::InPlacePImpl<Impl, 128> m_impl; //!< Implementation details
};

} // namespace sf::ImGui
