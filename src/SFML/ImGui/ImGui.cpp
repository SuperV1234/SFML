#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/ImGui/Backend.hpp"
#include "SFML/ImGui/ImGui.hpp"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/RenderTexture.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "SFML/Window/Clipboard.hpp"
#include "SFML/Window/Cursor.hpp"
#include "SFML/Window/Event.hpp"
#include "SFML/Window/Joystick.hpp"
#include "SFML/Window/Touch.hpp"
#include "SFML/Window/Window.hpp"

#include "SFML/System/Err.hpp"
#include "SFML/System/String.hpp"
#include "SFML/System/StringUtfUtils.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Memcpy.hpp"
#include "SFML/Base/Builtins/Strlen.hpp"
#include "SFML/Base/Math/Fabs.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/UniquePtr.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <string>
#include <vector>

#if defined(__APPLE__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef ANDROID
#ifdef USE_JNI

#include "SFML/System/NativeActivity.hpp"

#include <android/native_activity.h>
#include <jni.h>


////////////////////////////////////////////////////////////
[[nodiscard]] int keyboardIMEImpl(int value)
{
    ANativeActivity* activity = sf::getNativeActivity();

    JavaVM* vm  = activity->vm;
    JNIEnv* env = activity->env;

    JavaVMAttachArgs attachargs;
    attachargs.version = JNI_VERSION_1_6;
    attachargs.name    = "NativeThread";
    attachargs.group   = nullptr;

    jint res = vm->AttachCurrentThread(&env, &attachargs);
    if (res == JNI_ERR)
        return EXIT_FAILURE;

    jclass natact  = env->FindClass("android/app/NativeActivity");
    jclass context = env->FindClass("android/content/Context");

    jfieldID fid    = env->GetStaticFieldID(context, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject  svcstr = env->GetStaticObjectField(context, fid);

    jmethodID getss   = env->GetMethodID(natact, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject   imm_obj = env->CallObjectMethod(activity->clazz, getss, svcstr);

    jclass    imm_cls         = env->GetObjectClass(imm_obj);
    jmethodID toggleSoftInput = env->GetMethodID(imm_cls, "toggleSoftInput", "(II)V");

    env->CallVoidMethod(imm_obj, toggleSoftInput, value, 0);

    env->DeleteLocalRef(imm_obj);
    env->DeleteLocalRef(imm_cls);
    env->DeleteLocalRef(svcstr);
    env->DeleteLocalRef(context);
    env->DeleteLocalRef(natact);

    vm->DetachCurrentThread();

    return EXIT_SUCCESS;
}


////////////////////////////////////////////////////////////
[[nodiscard]] int openKeyboardIME()
{
    return keyboardIMEImpl(2);
}


////////////////////////////////////////////////////////////
[[nodiscard]] int closeKeyboardIME()
{
    return keyboardIMEImpl(1);
}

#endif
#endif

// TODO P0: cleanup and rename funcs, etc

static_assert(sizeof(unsigned int) <= sizeof(ImTextureID), "ImTextureID is not large enough to fit unsigned int.");

namespace sf::ImGui
{

namespace
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr ImColor toImColor(Color c)
{
    return {static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b), static_cast<int>(c.a)};
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr ImVec2 toImVec2(Vector2f v)
{
    return {v.x, v.y};
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr Vector2f toSfVector2f(ImVec2 v)
{
    return {v.x, v.y};
}


////////////////////////////////////////////////////////////
[[nodiscard]] ImVec2 getTopLeftAbsolute(const FloatRect& rect)
{
    return toImVec2(toSfVector2f(::ImGui::GetCursorScreenPos()) + rect.position);
}


////////////////////////////////////////////////////////////
[[nodiscard]] ImVec2 getDownRightAbsolute(const FloatRect& rect)
{
    return toImVec2(toSfVector2f(::ImGui::GetCursorScreenPos()) + rect.position + rect.size);
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr ImGuiKey keycodeToImGuiKey(Keyboard::Key code)
{
    // clang-format off
    switch (code)
    {
        case Keyboard::Key::Tab:        return ImGuiKey_Tab;
        case Keyboard::Key::Left:       return ImGuiKey_LeftArrow;
        case Keyboard::Key::Right:      return ImGuiKey_RightArrow;
        case Keyboard::Key::Up:         return ImGuiKey_UpArrow;
        case Keyboard::Key::Down:       return ImGuiKey_DownArrow;
        case Keyboard::Key::PageUp:     return ImGuiKey_PageUp;
        case Keyboard::Key::PageDown:   return ImGuiKey_PageDown;
        case Keyboard::Key::Home:       return ImGuiKey_Home;
        case Keyboard::Key::End:        return ImGuiKey_End;
        case Keyboard::Key::Insert:     return ImGuiKey_Insert;
        case Keyboard::Key::Delete:     return ImGuiKey_Delete;
        case Keyboard::Key::Backspace:  return ImGuiKey_Backspace;
        case Keyboard::Key::Space:      return ImGuiKey_Space;
        case Keyboard::Key::Enter:      return ImGuiKey_Enter;
        case Keyboard::Key::Escape:     return ImGuiKey_Escape;
        case Keyboard::Key::Apostrophe: return ImGuiKey_Apostrophe;
        case Keyboard::Key::Comma:      return ImGuiKey_Comma;
        case Keyboard::Key::Hyphen:     return ImGuiKey_Minus;
        case Keyboard::Key::Period:     return ImGuiKey_Period;
        case Keyboard::Key::Slash:      return ImGuiKey_Slash;
        case Keyboard::Key::Semicolon:  return ImGuiKey_Semicolon;
        case Keyboard::Key::Equal:      return ImGuiKey_Equal;
        case Keyboard::Key::LBracket:   return ImGuiKey_LeftBracket;
        case Keyboard::Key::Backslash:  return ImGuiKey_Backslash;
        case Keyboard::Key::RBracket:   return ImGuiKey_RightBracket;
        case Keyboard::Key::Grave:      return ImGuiKey_GraveAccent;
        // case :                           return ImGuiKey_CapsLock;
        // case :                           return ImGuiKey_ScrollLock;
        // case :                           return ImGuiKey_NumLock;
        // case :                           return ImGuiKey_PrintScreen;
        case Keyboard::Key::Pause:      return ImGuiKey_Pause;
        case Keyboard::Key::Numpad0:    return ImGuiKey_Keypad0;
        case Keyboard::Key::Numpad1:    return ImGuiKey_Keypad1;
        case Keyboard::Key::Numpad2:    return ImGuiKey_Keypad2;
        case Keyboard::Key::Numpad3:    return ImGuiKey_Keypad3;
        case Keyboard::Key::Numpad4:    return ImGuiKey_Keypad4;
        case Keyboard::Key::Numpad5:    return ImGuiKey_Keypad5;
        case Keyboard::Key::Numpad6:    return ImGuiKey_Keypad6;
        case Keyboard::Key::Numpad7:    return ImGuiKey_Keypad7;
        case Keyboard::Key::Numpad8:    return ImGuiKey_Keypad8;
        case Keyboard::Key::Numpad9:    return ImGuiKey_Keypad9;
        // case :                           return ImGuiKey_KeypadDecimal;
        case Keyboard::Key::Divide:     return ImGuiKey_KeypadDivide;
        case Keyboard::Key::Multiply:   return ImGuiKey_KeypadMultiply;
        case Keyboard::Key::Subtract:   return ImGuiKey_KeypadSubtract;
        case Keyboard::Key::Add:        return ImGuiKey_KeypadAdd;
        // case :                           return ImGuiKey_KeypadEnter;
        // case :                           return ImGuiKey_KeypadEqual;
        case Keyboard::Key::LControl:   return ImGuiKey_LeftCtrl;
        case Keyboard::Key::LShift:     return ImGuiKey_LeftShift;
        case Keyboard::Key::LAlt:       return ImGuiKey_LeftAlt;
        case Keyboard::Key::LSystem:    return ImGuiKey_LeftSuper;
        case Keyboard::Key::RControl:   return ImGuiKey_RightCtrl;
        case Keyboard::Key::RShift:     return ImGuiKey_RightShift;
        case Keyboard::Key::RAlt:       return ImGuiKey_RightAlt;
        case Keyboard::Key::RSystem:    return ImGuiKey_RightSuper;
        case Keyboard::Key::Menu:       return ImGuiKey_Menu;
        case Keyboard::Key::Num0:       return ImGuiKey_0;
        case Keyboard::Key::Num1:       return ImGuiKey_1;
        case Keyboard::Key::Num2:       return ImGuiKey_2;
        case Keyboard::Key::Num3:       return ImGuiKey_3;
        case Keyboard::Key::Num4:       return ImGuiKey_4;
        case Keyboard::Key::Num5:       return ImGuiKey_5;
        case Keyboard::Key::Num6:       return ImGuiKey_6;
        case Keyboard::Key::Num7:       return ImGuiKey_7;
        case Keyboard::Key::Num8:       return ImGuiKey_8;
        case Keyboard::Key::Num9:       return ImGuiKey_9;
        case Keyboard::Key::A:          return ImGuiKey_A;
        case Keyboard::Key::B:          return ImGuiKey_B;
        case Keyboard::Key::C:          return ImGuiKey_C;
        case Keyboard::Key::D:          return ImGuiKey_D;
        case Keyboard::Key::E:          return ImGuiKey_E;
        case Keyboard::Key::F:          return ImGuiKey_F;
        case Keyboard::Key::G:          return ImGuiKey_G;
        case Keyboard::Key::H:          return ImGuiKey_H;
        case Keyboard::Key::I:          return ImGuiKey_I;
        case Keyboard::Key::J:          return ImGuiKey_J;
        case Keyboard::Key::K:          return ImGuiKey_K;
        case Keyboard::Key::L:          return ImGuiKey_L;
        case Keyboard::Key::M:          return ImGuiKey_M;
        case Keyboard::Key::N:          return ImGuiKey_N;
        case Keyboard::Key::O:          return ImGuiKey_O;
        case Keyboard::Key::P:          return ImGuiKey_P;
        case Keyboard::Key::Q:          return ImGuiKey_Q;
        case Keyboard::Key::R:          return ImGuiKey_R;
        case Keyboard::Key::S:          return ImGuiKey_S;
        case Keyboard::Key::T:          return ImGuiKey_T;
        case Keyboard::Key::U:          return ImGuiKey_U;
        case Keyboard::Key::V:          return ImGuiKey_V;
        case Keyboard::Key::W:          return ImGuiKey_W;
        case Keyboard::Key::X:          return ImGuiKey_X;
        case Keyboard::Key::Y:          return ImGuiKey_Y;
        case Keyboard::Key::Z:          return ImGuiKey_Z;
        case Keyboard::Key::F1:         return ImGuiKey_F1;
        case Keyboard::Key::F2:         return ImGuiKey_F2;
        case Keyboard::Key::F3:         return ImGuiKey_F3;
        case Keyboard::Key::F4:         return ImGuiKey_F4;
        case Keyboard::Key::F5:         return ImGuiKey_F5;
        case Keyboard::Key::F6:         return ImGuiKey_F6;
        case Keyboard::Key::F7:         return ImGuiKey_F7;
        case Keyboard::Key::F8:         return ImGuiKey_F8;
        case Keyboard::Key::F9:         return ImGuiKey_F9;
        case Keyboard::Key::F10:        return ImGuiKey_F10;
        case Keyboard::Key::F11:        return ImGuiKey_F11;
        case Keyboard::Key::F12:        return ImGuiKey_F12;
        default:
            break;
    }
    // clang-format on

    return ImGuiKey_None;
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr ImGuiKey keycodeToImGuiMod(Keyboard::Key code)
{
    switch (code)
    {
        case Keyboard::Key::LControl:
        case Keyboard::Key::RControl:
            return ImGuiKey_ModCtrl;
        case Keyboard::Key::LShift:
        case Keyboard::Key::RShift:
            return ImGuiKey_ModShift;
        case Keyboard::Key::LAlt:
        case Keyboard::Key::RAlt:
            return ImGuiKey_ModAlt;
        case Keyboard::Key::LSystem:
        case Keyboard::Key::RSystem:
            return ImGuiKey_ModSuper;
        default:
            break;
    }

    return ImGuiKey_None;
}


////////////////////////////////////////////////////////////
[[nodiscard]] ImTextureID convertGLTextureHandleToImTextureID(unsigned int glTextureHandle)
{
    ImTextureID textureID{};
    SFML_BASE_MEMCPY(&textureID, &glTextureHandle, sizeof(unsigned int));
    return textureID;
}


////////////////////////////////////////////////////////////
[[nodiscard]] base::Optional<Texture> createImGuiDefaultFontTexture()
{
    ImGuiIO&       io     = ::ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int            width  = 0;
    int            height = 0;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    auto newTexture = Texture::create({static_cast<unsigned int>(width), static_cast<unsigned int>(height)});

    if (!newTexture.hasValue())
    {
        sf::priv::err() << "Failed to create default ImGui font texture";
        return newTexture; // Empty optional
    }

    newTexture->update(pixels);
    return newTexture;
}


////////////////////////////////////////////////////////////
struct [[nodiscard]] StickInfo
{
    Joystick::Axis xAxis{Joystick::Axis::X};
    Joystick::Axis yAxis{Joystick::Axis::Y};

    bool xInverted{false};
    bool yInverted{false};

    float threshold{15};
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] TriggerInfo
{
    Joystick::Axis axis{Joystick::Axis::Z};
    float          threshold{0};
};


////////////////////////////////////////////////////////////
constexpr unsigned int nullJoystickId = Joystick::MaxCount;


////////////////////////////////////////////////////////////
[[nodiscard]] unsigned int getConnectedJoystickId()
{
    for (unsigned int i = 0; i < static_cast<unsigned int>(Joystick::MaxCount); ++i)
        if (Joystick::query(i).hasValue())
            return i;

    return nullJoystickId;
}


////////////////////////////////////////////////////////////
struct [[nodiscard]] ImGuiPerWindowContext
{
    const Window*   window;
    ::ImGuiContext* imContext{::ImGui::CreateContext()};

    base::Optional<Texture> fontTexture; // internal font atlas which is used if user doesn't set a custom Texture.

    bool             windowHasFocus;
    bool             mouseMoved{false};
    bool             mousePressed[3]{};
    ImGuiMouseCursor lastCursor{ImGuiMouseCursor_COUNT};

    bool     lastInputSourceWasTouch{};
    bool     touchDown[3]{};
    Vector2i touchPositions[3]{};
    Vector2i lastTouchPos;

    unsigned int joystickId;
    ImGuiKey     joystickMapping[Joystick::ButtonCount]{ImGuiKey_None};
    StickInfo    dPadInfo;
    StickInfo    lStickInfo;
    StickInfo    rStickInfo;
    TriggerInfo  lTriggerInfo;
    TriggerInfo  rTriggerInfo;

    base::Optional<Cursor> mouseCursors[ImGuiMouseCursor_COUNT];

    [[nodiscard, gnu::always_inline, gnu::pure]] base::Optional<Cursor>& getMouseCursor(ImGuiMouseCursor i)
    {
        SFML_BASE_ASSERT(i < ImGuiMouseCursor_COUNT);
        return mouseCursors[i];
    }

    [[nodiscard, gnu::always_inline, gnu::pure]] const base::Optional<Cursor>& getMouseCursor(ImGuiMouseCursor i) const
    {
        SFML_BASE_ASSERT(i < ImGuiMouseCursor_COUNT);
        return mouseCursors[i];
    }

#ifdef ANDROID
#ifdef USE_JNI
    bool wantTextInput{false};
#endif
#endif

    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit ImGuiPerWindowContext(const Window& theWindow) :
    window(&theWindow),
    windowHasFocus(theWindow.hasFocus()),
    joystickId{getConnectedJoystickId()}
    {
    }

    ////////////////////////////////////////////////////////////
    ~ImGuiPerWindowContext()
    {
        ::ImGui::SetCurrentContext(imContext);
        priv::ImGui_ImplOpenGL3_Shutdown();
        ::ImGui::DestroyContext(imContext);
    }

    ////////////////////////////////////////////////////////////
    ImGuiPerWindowContext(const ImGuiPerWindowContext&)            = delete;
    ImGuiPerWindowContext& operator=(const ImGuiPerWindowContext&) = delete;

    ////////////////////////////////////////////////////////////
    using SetClipboardTextFn = void (*)(void*, const char*);
    using GetClipboardTextFn = const char* (*)(void*);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool init(Vector2f           displaySize,
                            bool               loadDefaultFont,
                            SetClipboardTextFn setClipboardTextFn,
                            GetClipboardTextFn getClipboardTextFn)
    {
        ImGuiIO& io = ::ImGui::GetIO();

        // tell ImGui which features we support
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_HasMouseCursors |
                           ImGuiBackendFlags_HasSetMousePos;

        io.BackendPlatformName = "imgui_impl_sfml";

        joystickId = getConnectedJoystickId();
        initDefaultJoystickMapping();

        // init rendering
        io.DisplaySize = toImVec2(displaySize);

        // clipboard
        io.SetClipboardTextFn = setClipboardTextFn;
        io.GetClipboardTextFn = getClipboardTextFn;

        // load mouse cursors
        loadMouseCursor(ImGuiMouseCursor_Arrow, Cursor::Type::Arrow);
        loadMouseCursor(ImGuiMouseCursor_TextInput, Cursor::Type::Text);
        loadMouseCursor(ImGuiMouseCursor_ResizeAll, Cursor::Type::SizeAll);
        loadMouseCursor(ImGuiMouseCursor_ResizeNS, Cursor::Type::SizeVertical);
        loadMouseCursor(ImGuiMouseCursor_ResizeEW, Cursor::Type::SizeHorizontal);
        loadMouseCursor(ImGuiMouseCursor_ResizeNESW, Cursor::Type::SizeBottomLeftTopRight);
        loadMouseCursor(ImGuiMouseCursor_ResizeNWSE, Cursor::Type::SizeTopLeftBottomRight);
        loadMouseCursor(ImGuiMouseCursor_Hand, Cursor::Type::Hand);

        if (loadDefaultFont)
        {
            // this will load default font automatically
            // No need to call AddDefaultFont
            if (!updateFontTexture())
                return false;
        }

        ::ImGui::SetCurrentContext(imContext);
        priv::ImGui_ImplOpenGL3_Init(nullptr);

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateFontTexture()
    {
        base::Optional<Texture> newTexture = createImGuiDefaultFontTexture();
        if (!newTexture.hasValue())
        {
            sf::priv::err() << "Failed to create default ImGui font texture";
            return false;
        }

        ::ImGui::GetIO().Fonts->SetTexID(convertGLTextureHandleToImTextureID(newTexture->getNativeHandle()));
        fontTexture = std::move(newTexture);

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] base::Optional<Texture>& getFontTexture()
    {
        return fontTexture;
    }


    ////////////////////////////////////////////////////////////
    void updateJoystickButtonState(ImGuiIO& io) const
    {
        for (int i = 0; i < static_cast<int>(Joystick::ButtonCount); ++i)
        {
            const ImGuiKey key = joystickMapping[i];

            if (key == ImGuiKey_None)
                continue;

            const bool isPressed = Joystick::query(joystickId)->isButtonPressed(static_cast<unsigned>(i));

            if (windowHasFocus || !isPressed)
                io.AddKeyEvent(key, isPressed);
        }
    }

    ////////////////////////////////////////////////////////////
    void updateJoystickAxis(ImGuiIO& io, ImGuiKey key, Joystick::Axis axis, float threshold, float maxThreshold, bool inverted = false) const
    {
        float pos = Joystick::query(joystickId)->getAxisPosition(axis);
        if (inverted)
            pos = -pos;

        const bool passedThreshold            = (pos > threshold) == (maxThreshold > threshold);
        const bool passedThresholdAndHasFocus = passedThreshold && windowHasFocus;

        io.AddKeyAnalogEvent(key, passedThresholdAndHasFocus, passedThresholdAndHasFocus ? base::fabs(pos / 100.f) : 0.f);
    }


    ////////////////////////////////////////////////////////////
    void updateJoystickAxisPair(ImGuiIO& io, ImGuiKey key1, ImGuiKey key2, Joystick::Axis axis, float threshold, bool inverted = false) const
    {
        updateJoystickAxis(io, key1, axis, -threshold, -100, inverted);
        updateJoystickAxis(io, key2, axis, threshold, 100, inverted);
    }


    ////////////////////////////////////////////////////////////
    void updateJoystickDPadState(ImGuiIO& io) const
    {
        updateJoystickAxisPair(io,
                               ImGuiKey_GamepadDpadLeft,
                               ImGuiKey_GamepadDpadRight,
                               dPadInfo.xAxis,
                               dPadInfo.threshold,
                               dPadInfo.xInverted);

        updateJoystickAxisPair(io,
                               ImGuiKey_GamepadDpadUp,
                               ImGuiKey_GamepadDpadDown,
                               dPadInfo.yAxis,
                               dPadInfo.threshold,
                               dPadInfo.yInverted);
    }


    ////////////////////////////////////////////////////////////
    void updateJoystickAxisState(ImGuiIO& io) const
    {
        updateJoystickAxisPair(io,
                               ImGuiKey_GamepadLStickLeft,
                               ImGuiKey_GamepadLStickRight,
                               lStickInfo.xAxis,
                               lStickInfo.threshold,
                               lStickInfo.xInverted);

        updateJoystickAxisPair(io,
                               ImGuiKey_GamepadLStickUp,
                               ImGuiKey_GamepadLStickDown,
                               lStickInfo.yAxis,
                               lStickInfo.threshold,
                               lStickInfo.yInverted);

        updateJoystickAxisPair(io,
                               ImGuiKey_GamepadRStickLeft,
                               ImGuiKey_GamepadRStickRight,
                               rStickInfo.xAxis,
                               rStickInfo.threshold,
                               rStickInfo.xInverted);

        updateJoystickAxisPair(io,
                               ImGuiKey_GamepadRStickUp,
                               ImGuiKey_GamepadRStickDown,
                               rStickInfo.yAxis,
                               rStickInfo.threshold,
                               rStickInfo.yInverted);

        updateJoystickAxis(io, ImGuiKey_GamepadL2, lTriggerInfo.axis, lTriggerInfo.threshold, 100, false);
        updateJoystickAxis(io, ImGuiKey_GamepadR2, rTriggerInfo.axis, rTriggerInfo.threshold, 100, false);
    }


    ////////////////////////////////////////////////////////////
    void loadMouseCursor(ImGuiMouseCursor imguiCursorType, Cursor::Type sfmlCursorType)
    {
        getMouseCursor(imguiCursorType) = Cursor::loadFromSystem(sfmlCursorType);
    }


    ////////////////////////////////////////////////////////////
    void processEvent(const Event& event)
    {
        ImGuiIO& io = ::ImGui::GetIO();

        if (!windowHasFocus && window->hasFocus())
        {
            io.AddFocusEvent(true);
            windowHasFocus = true;
        }
        else if (windowHasFocus && !window->hasFocus())
        {
            io.AddFocusEvent(false);
            windowHasFocus = false;
        }

        if (!windowHasFocus && event.is<Event::FocusGained>())
        {
            io.AddFocusEvent(true);
            windowHasFocus = true;
            return;
        }

        if (!windowHasFocus)
            return;

        if (windowHasFocus && event.is<Event::FocusLost>())
        {
            io.AddFocusEvent(false);
            windowHasFocus = false;
        }
        else if (const auto* resized = event.getIf<Event::Resized>())
        {
            io.DisplaySize = ImVec2(static_cast<float>(resized->size.x), static_cast<float>(resized->size.y));
        }
        else if (const auto* eMouseMoved = event.getIf<Event::MouseMoved>())
        {
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMousePosEvent(static_cast<float>(eMouseMoved->position.x), static_cast<float>(eMouseMoved->position.y));

            mouseMoved = true;
        }
        else if (const auto* mouseButtonPressed = event.getIf<Event::MouseButtonPressed>())
        {
            const int button = static_cast<int>(mouseButtonPressed->button);
            if (button >= 0 && button < 3)
            {
                mousePressed[static_cast<int>(mouseButtonPressed->button)] = true;

                io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
                io.AddMouseButtonEvent(button, true);
            }
        }
        else if (const auto* mouseButtonReleased = event.getIf<Event::MouseButtonReleased>())
        {
            const int button = static_cast<int>(mouseButtonReleased->button);
            if (button >= 0 && button < 3)
            {
                io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
                io.AddMouseButtonEvent(button, false);
            }
        }
        else if (const auto* eTouchMoved = event.getIf<Event::TouchMoved>())
        {
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(static_cast<float>(eTouchMoved->position.x), static_cast<float>(eTouchMoved->position.y));

            mouseMoved = false;
        }
        else if (const auto* touchBegan = event.getIf<Event::TouchBegan>())
        {
            mouseMoved                = false;
            const unsigned int button = touchBegan->finger;

            if (button < 3)
            {
                touchDown[button]      = true;
                touchPositions[button] = touchBegan->position;

                io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
                io.AddMouseButtonEvent(static_cast<int>(button), true);
            }
        }
        else if (const auto* touchEnded = event.getIf<Event::TouchEnded>())
        {
            mouseMoved                = false;
            const unsigned int button = touchEnded->finger;

            if (button < 3)
            {
                touchDown[button]      = false;
                touchPositions[button] = {};

                io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
                io.AddMouseButtonEvent(static_cast<int>(button), false);
            }
        }
        else if (const auto* mouseWheelScrolled = event.getIf<Event::MouseWheelScrolled>())
        {
            if (mouseWheelScrolled->wheel == Mouse::Wheel::Vertical ||
                (mouseWheelScrolled->wheel == Mouse::Wheel::Horizontal && io.KeyShift))
            {
                io.AddMouseWheelEvent(0, mouseWheelScrolled->delta);
            }
            else if (mouseWheelScrolled->wheel == Mouse::Wheel::Horizontal)
            {
                io.AddMouseWheelEvent(mouseWheelScrolled->delta, 0);
            }
        }

        const auto handleKeyChanged = [&io](const auto& keyChanged, bool down)
        {
            const ImGuiKey mod = keycodeToImGuiMod(keyChanged.code);
            // The modifier booleans are not reliable when it's the modifier
            // itself that's being pressed. Detect these presses directly.
            if (mod != ImGuiKey_None)
            {
                io.AddKeyEvent(mod, down);
            }
            else
            {
                io.AddKeyEvent(ImGuiKey_ModCtrl, keyChanged.control);
                io.AddKeyEvent(ImGuiKey_ModShift, keyChanged.shift);
                io.AddKeyEvent(ImGuiKey_ModAlt, keyChanged.alt);
                io.AddKeyEvent(ImGuiKey_ModSuper, keyChanged.system);
            }

            const ImGuiKey key = keycodeToImGuiKey(keyChanged.code);
            io.AddKeyEvent(key, down);
            io.SetKeyEventNativeData(key, static_cast<int>(keyChanged.code), -1);
        };

        if (const auto* keyPressed = event.getIf<Event::KeyPressed>())
        {
            handleKeyChanged(*keyPressed, true);
        }
        else if (const auto* keyReleased = event.getIf<Event::KeyReleased>())
        {
            handleKeyChanged(*keyReleased, false);
        }
        else if (const auto* textEntered = event.getIf<Event::TextEntered>())
        {
            // Don't handle the event for unprintable characters
            if (textEntered->unicode >= ' ' && textEntered->unicode != 127)
                io.AddInputCharacter(textEntered->unicode);
        }
        else if (const auto* joystickConnected = event.getIf<Event::JoystickConnected>())
        {
            if (joystickId == nullJoystickId)
                joystickId = joystickConnected->joystickId;
        }
        else if (const auto* joystickDisconnected = event.getIf<Event::JoystickDisconnected>())
        {
            if (joystickId == joystickDisconnected->joystickId)
            {
                // used gamepad was disconnected
                joystickId = getConnectedJoystickId();
            }
        }
    }


    ////////////////////////////////////////////////////////////
    void updateMouseCursor(Window& theWindow, const ImGuiMouseCursor cursor) const
    {
        if (!getMouseCursor(ImGuiMouseCursor_Arrow).hasValue())
        {
            theWindow.setMouseCursorVisible(false);
            return;
        }

        theWindow.setMouseCursorVisible(true);
        theWindow.setMouseCursor(
            getMouseCursor(cursor).hasValue() ? *getMouseCursor(cursor) : *getMouseCursor(ImGuiMouseCursor_Arrow));
    }


    ////////////////////////////////////////////////////////////
    void update(Window& theWindow, RenderTarget& target, Time dt)
    {
        // update OS/hardware mouse cursor if imgui isn't drawing a software cursor
        const ImGuiMouseCursor mouseCursor = ::ImGui::GetIO().MouseDrawCursor ? ImGuiMouseCursor_None
                                                                              : ::ImGui::GetMouseCursor();

        if (lastCursor != mouseCursor)
        {
            lastCursor = mouseCursor;
            updateMouseCursor(theWindow, mouseCursor);
        }

        if (!mouseMoved) // TODO P1: needed?
        {
            if (Touch::isDown(0))
                lastTouchPos = Touch::getPosition(0, theWindow);
            else if (touchDown[0])
                lastTouchPos = touchPositions[0];

            lastInputSourceWasTouch = true;
            update(lastTouchPos, target.getSize().toVector2f(), dt);
        }
        else
        {
            lastInputSourceWasTouch = false;
            update(Mouse::getPosition(theWindow), target.getSize().toVector2f(), dt);
        }
    }


    ////////////////////////////////////////////////////////////
    void update(Vector2i mousePos, Vector2f displaySize, Time dt)
    {
        ImGuiIO& io    = ::ImGui::GetIO();
        io.DisplaySize = toImVec2(displaySize);
        io.DeltaTime   = dt.asSeconds();

        if (windowHasFocus)
        {
            if (io.WantSetMousePos)
            {
                Mouse::setPosition({static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y)});
            }
            else
            {
                io.MousePos = ImVec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
            }

            for (unsigned int i = 0; i < 3; i++)
            {
                io.MouseDown[i] = Touch::isDown(i) || touchDown[i] || mousePressed[i] ||
                                  Mouse::isButtonPressed(static_cast<Mouse::Button>(i));

                mousePressed[i] = false;
                touchDown[i]    = false;
            }
        }

#ifdef ANDROID
#ifdef USE_JNI
        if (io.WantTextInput && !s_currWindowCtx->wantTextInput)
        {
            openKeyboardIME();
            s_currWindowCtx->wantTextInput = true;
        }

        if (!io.WantTextInput && s_currWindowCtx->wantTextInput)
        {
            closeKeyboardIME();
            s_currWindowCtx->wantTextInput = false;
        }
#endif
#endif

        SFML_BASE_ASSERT(io.Fonts->Fonts.Size > 0); // You forgot to create and set up font atlas (see createFontTexture)

        // gamepad navigation
        if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) && joystickId != nullJoystickId)
        {
            updateJoystickButtonState(io);
            updateJoystickDPadState(io);
            updateJoystickAxisState(io);
        }

        priv::ImGui_ImplOpenGL3_NewFrame();
        ::ImGui::NewFrame();
    }


    ////////////////////////////////////////////////////////////
    void setActiveJoystickId(unsigned int newJoystickId)
    {
        SFML_BASE_ASSERT(newJoystickId < Joystick::MaxCount);
        joystickId = newJoystickId;
    }


    ////////////////////////////////////////////////////////////
    void setJoystickDPadThreshold(float threshold)
    {
        SFML_BASE_ASSERT(threshold >= 0.f && threshold <= 100.f);
        dPadInfo.threshold = threshold;
    }


    ////////////////////////////////////////////////////////////
    void setJoystickLStickThreshold(float threshold)
    {
        SFML_BASE_ASSERT(threshold >= 0.f && threshold <= 100.f);
        lStickInfo.threshold = threshold;
    }


    ////////////////////////////////////////////////////////////
    void setJoystickRStickThreshold(float threshold)
    {
        SFML_BASE_ASSERT(threshold >= 0.f && threshold <= 100.f);
        rStickInfo.threshold = threshold;
    }


    ////////////////////////////////////////////////////////////
    void setJoystickLTriggerThreshold(float threshold)
    {
        SFML_BASE_ASSERT(threshold >= -100.f && threshold <= 100.f);
        lTriggerInfo.threshold = threshold;
    }


    ////////////////////////////////////////////////////////////
    void setJoystickRTriggerThreshold(float threshold)
    {
        SFML_BASE_ASSERT(threshold >= -100.f && threshold <= 100.f);
        rTriggerInfo.threshold = threshold;
    }


    ////////////////////////////////////////////////////////////
    void setJoystickMapping(int key, unsigned int joystickButton)
    {
        SFML_BASE_ASSERT(joystickButton < Joystick::ButtonCount);

        // This function now expects ImGuiKey_* values.
        // For partial backwards compatibility, also expect some ImGuiNavInput_* values.
        ImGuiKey finalKey{};
        switch (key)
        {
            case ImGuiKey_GamepadFaceDown:
                finalKey = ImGuiKey_GamepadFaceDown;
                break;
            case ImGuiKey_GamepadFaceRight:
                finalKey = ImGuiKey_GamepadFaceRight;
                break;
            case ImGuiKey_GamepadFaceUp:
                finalKey = ImGuiKey_GamepadFaceUp;
                break;
            case ImGuiKey_GamepadFaceLeft:
                finalKey = ImGuiKey_GamepadFaceLeft;
                break;
            case ImGuiKey_GamepadL1:
                finalKey = ImGuiKey_GamepadL1;
                break;
            case ImGuiKey_GamepadR1:
                finalKey = ImGuiKey_GamepadR1;
                break;
            default:
                SFML_BASE_ASSERT(key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END);
                finalKey = static_cast<ImGuiKey>(key);
        }

        joystickMapping[joystickButton] = finalKey;
    }


    ////////////////////////////////////////////////////////////
    void setDPadXAxis(Joystick::Axis dPadXAxis, bool inverted = false)
    {
        dPadInfo.xAxis     = dPadXAxis;
        dPadInfo.xInverted = inverted;
    }


    ////////////////////////////////////////////////////////////
    void setDPadYAxis(Joystick::Axis dPadYAxis, bool inverted = false)
    {
        dPadInfo.yAxis     = dPadYAxis;
        dPadInfo.yInverted = inverted;
    }


    ////////////////////////////////////////////////////////////
    void setLStickXAxis(Joystick::Axis lStickXAxis, bool inverted = false)
    {
        lStickInfo.xAxis     = lStickXAxis;
        lStickInfo.xInverted = inverted;
    }


    ////////////////////////////////////////////////////////////
    void setLStickYAxis(Joystick::Axis lStickYAxis, bool inverted = false)
    {
        lStickInfo.yAxis     = lStickYAxis;
        lStickInfo.yInverted = inverted;
    }


    ////////////////////////////////////////////////////////////
    void setRStickXAxis(Joystick::Axis rStickXAxis, bool inverted = false)
    {
        rStickInfo.xAxis     = rStickXAxis;
        rStickInfo.xInverted = inverted;
    }


    ////////////////////////////////////////////////////////////
    void setRStickYAxis(Joystick::Axis rStickYAxis, bool inverted = false)
    {
        rStickInfo.yAxis     = rStickYAxis;
        rStickInfo.yInverted = inverted;
    }


    ////////////////////////////////////////////////////////////
    void setLTriggerAxis(Joystick::Axis lTriggerAxis)
    {
        rTriggerInfo.axis = lTriggerAxis;
    }


    ////////////////////////////////////////////////////////////
    void setRTriggerAxis(Joystick::Axis rTriggerAxis)
    {
        rTriggerInfo.axis = rTriggerAxis;
    }


    ////////////////////////////////////////////////////////////
    void initDefaultJoystickMapping()
    {
        setJoystickMapping(ImGuiKey_GamepadFaceDown, 0);
        setJoystickMapping(ImGuiKey_GamepadFaceRight, 1);
        setJoystickMapping(ImGuiKey_GamepadFaceLeft, 2);
        setJoystickMapping(ImGuiKey_GamepadFaceUp, 3);
        setJoystickMapping(ImGuiKey_GamepadL1, 4);
        setJoystickMapping(ImGuiKey_GamepadR1, 5);
        setJoystickMapping(ImGuiKey_GamepadBack, 6);
        setJoystickMapping(ImGuiKey_GamepadStart, 7);
        setJoystickMapping(ImGuiKey_GamepadL3, 9);
        setJoystickMapping(ImGuiKey_GamepadR3, 10);

        setDPadXAxis(Joystick::Axis::PovX);
        // D-pad Y axis is inverted on Windows
#ifdef _WIN32
        setDPadYAxis(Joystick::Axis::PovY, true);
#else
        setDPadYAxis(Joystick::Axis::PovY);
#endif

        setLStickXAxis(Joystick::Axis::X);
        setLStickYAxis(Joystick::Axis::Y);
        setRStickXAxis(Joystick::Axis::U);
        setRStickYAxis(Joystick::Axis::V);
        setLTriggerAxis(Joystick::Axis::Z);
        setRTriggerAxis(Joystick::Axis::R);

        setJoystickDPadThreshold(5.f);
        setJoystickLStickThreshold(5.f);
        setJoystickRStickThreshold(15.f);
        setJoystickLTriggerThreshold(0.f);
        setJoystickRTriggerThreshold(0.f);
    }
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] SpriteTextureData
{
    ImVec2      uv0;
    ImVec2      uv1;
    ImTextureID textureID{};
};


////////////////////////////////////////////////////////////
[[nodiscard]] SpriteTextureData getSpriteTextureData(const Sprite& sprite, const Texture& texture)
{
    const auto textureSize(texture.getSize().toVector2f());
    const auto& [txrPosition, txrSize](sprite.textureRect);

    return {toImVec2(txrPosition.componentWiseDiv(textureSize)),
            toImVec2((txrPosition + txrSize).componentWiseDiv(textureSize)),
            convertGLTextureHandleToImTextureID(texture.getNativeHandle())};
}

} // namespace


////////////////////////////////////////////////////////////
struct ImGuiContext::Impl
{
    std::vector<base::UniquePtr<ImGuiPerWindowContext>> perWindowContexts;

    ImGuiPerWindowContext* currentPerWindowContext = nullptr;
    std::string            clipboardText;


    ////////////////////////////////////////////////////////////
    void updateMouseCursor(Window& window) const
    {
        const ImGuiIO& io = ::ImGui::GetIO();
        if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != 0)
            return;

        const ImGuiMouseCursor cursor = ::ImGui::GetMouseCursor();
        if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None)
        {
            window.setMouseCursorVisible(false);
            return;
        }

        currentPerWindowContext->updateMouseCursor(window, cursor);
    }
};


////////////////////////////////////////////////////////////
ImGuiContext::ImGuiContext() = default;


////////////////////////////////////////////////////////////
ImGuiContext::~ImGuiContext()
{
    shutdown();
}


////////////////////////////////////////////////////////////
ImGuiContext::ImGuiContext(ImGuiContext&&) noexcept = default;


////////////////////////////////////////////////////////////
ImGuiContext& ImGuiContext::operator=(ImGuiContext&&) noexcept = default;


////////////////////////////////////////////////////////////
bool ImGuiContext::init(RenderWindow& window, bool loadDefaultFont)
{
    return init(window, window, loadDefaultFont);
}


////////////////////////////////////////////////////////////
bool ImGuiContext::init(Window& window, RenderTarget& target, bool loadDefaultFont)
{
    return init(window, target.getSize().toVector2f(), loadDefaultFont);
}


////////////////////////////////////////////////////////////
bool ImGuiContext::init(Window& window, Vector2f displaySize, bool loadDefaultFont)
{
    m_impl->perWindowContexts.emplace_back(base::makeUnique<ImGuiPerWindowContext>(window));

    m_impl->currentPerWindowContext = m_impl->perWindowContexts.back().get();
    ::ImGui::SetCurrentContext(m_impl->currentPerWindowContext->imContext);

    thread_local std::string* clipboardTextPtr;
    clipboardTextPtr = &m_impl->clipboardText;

    return m_impl->currentPerWindowContext->init(displaySize,
                                                 loadDefaultFont,

                                                 [](void* /*userData*/, const char* text)
    { Clipboard::setString(StringUtfUtils::fromUtf8(text, text + SFML_BASE_STRLEN(text))); },

                                                 [](void* /*userData*/)
    {
        auto tmp = Clipboard::getString().toUtf8<std::u8string>();
        clipboardTextPtr->assign(tmp.begin(), tmp.end());
        return clipboardTextPtr->c_str();
    });
}


////////////////////////////////////////////////////////////
void ImGuiContext::setCurrentWindow(const Window& window)
{
    auto found = base::findIf(m_impl->perWindowContexts.begin(),
                              m_impl->perWindowContexts.end(),
                              [&](base::UniquePtr<ImGuiPerWindowContext>& ctx)
    { return ctx->window->getNativeHandle() == window.getNativeHandle(); });

    SFML_BASE_ASSERT(found != m_impl->perWindowContexts.end() &&
                     "Failed to find the window. Forgot to call init for the window?");

    m_impl->currentPerWindowContext = found->get();
    ::ImGui::SetCurrentContext(m_impl->currentPerWindowContext->imContext);
}


////////////////////////////////////////////////////////////
void ImGuiContext::processEvent(const Window& window, const Event& event)
{
    setCurrentWindow(window);
    SFML_BASE_ASSERT(m_impl->currentPerWindowContext && "No current window is set - forgot to call init?");

    m_impl->currentPerWindowContext->processEvent(event);
}


////////////////////////////////////////////////////////////
bool ImGuiContext::wasLastInputTouch() const
{
    SFML_BASE_ASSERT(m_impl->currentPerWindowContext && "No current window is set - forgot to call init?");
    return m_impl->currentPerWindowContext->lastInputSourceWasTouch;
}


////////////////////////////////////////////////////////////
void ImGuiContext::update(RenderWindow& window, Time dt)
{
    update(window, window, dt);
}


////////////////////////////////////////////////////////////
void ImGuiContext::update(Window& window, RenderTarget& target, Time dt)
{
    setCurrentWindow(window);
    SFML_BASE_ASSERT(m_impl->currentPerWindowContext);

    m_impl->currentPerWindowContext->update(window, target, dt);
}


////////////////////////////////////////////////////////////
void ImGuiContext::update(Vector2i mousePos, Vector2f displaySize, Time dt)
{
    SFML_BASE_ASSERT(m_impl->currentPerWindowContext && "No current window is set - forgot to call init?");
    m_impl->currentPerWindowContext->update(mousePos, displaySize, dt);
}


////////////////////////////////////////////////////////////
void ImGuiContext::render(RenderWindow& window)
{
    setCurrentWindow(window);
    render(static_cast<RenderTarget&>(window));
}


////////////////////////////////////////////////////////////
void ImGuiContext::render(RenderTarget& target)
{
    target.resetGLStates();
    render();
}


////////////////////////////////////////////////////////////
void ImGuiContext::render()
{
    ::ImGui::Render();
    priv::ImGui_ImplOpenGL3_RenderDrawData(::ImGui::GetDrawData());
}


////////////////////////////////////////////////////////////
void ImGuiContext::shutdown(const Window& window)
{
    const bool needReplacement = (m_impl->currentPerWindowContext->window->getNativeHandle() == window.getNativeHandle());

    // remove window's context
    auto found = base::findIf(m_impl->perWindowContexts.begin(),
                              m_impl->perWindowContexts.end(),
                              [&](base::UniquePtr<ImGuiPerWindowContext>& ctx)
    { return ctx->window->getNativeHandle() == window.getNativeHandle(); });

    SFML_BASE_ASSERT(found != m_impl->perWindowContexts.end() &&
                     "Window wasn't inited properly: forgot to call init(window)?");

    m_impl->perWindowContexts.erase(found); // s_currWindowCtx can become invalid here!

    // set current context to some window for convenience if needed
    if (!needReplacement)
        return;

    auto it = m_impl->perWindowContexts.begin();
    if (it == m_impl->perWindowContexts.end())
    {
        // no alternatives...
        shutdown();
        return;
    }

    // set to some other window
    m_impl->currentPerWindowContext = it->get();
    ::ImGui::SetCurrentContext(m_impl->currentPerWindowContext->imContext);
}


////////////////////////////////////////////////////////////
void ImGuiContext::shutdown()
{
    m_impl->currentPerWindowContext = nullptr;
    ::ImGui::SetCurrentContext(nullptr);

    m_impl->perWindowContexts.clear();
}


////////////////////////////////////////////////////////////
void ImGuiContext::image(const Texture& texture, Color tintColor, Color borderColor)
{
    image(texture, texture.getSize().toVector2f(), tintColor, borderColor);
}


////////////////////////////////////////////////////////////
void ImGuiContext::image(const Texture& texture, Vector2f size, Color tintColor, Color borderColor)
{
    ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());

    ::ImGui::Image(textureID, toImVec2(size), ImVec2(0, 0), ImVec2(1, 1), toImColor(tintColor), toImColor(borderColor));
}


////////////////////////////////////////////////////////////
void ImGuiContext::image(const RenderTexture& texture, Color tintColor, Color borderColor)
{
    image(texture, texture.getSize().toVector2f(), tintColor, borderColor);
}


////////////////////////////////////////////////////////////
void ImGuiContext::image(const RenderTexture& texture, Vector2f size, Color tintColor, Color borderColor)
{
    ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getTexture().getNativeHandle());
    ::ImGui::Image(textureID, toImVec2(size), ImVec2(0, 0), ImVec2(1, 1), toImColor(tintColor), toImColor(borderColor));
}


////////////////////////////////////////////////////////////
void ImGuiContext::image(const Sprite& sprite, const Texture& texture, Color tintColor, Color borderColor)
{
    image(sprite, texture, sprite.getGlobalBounds().size, tintColor, borderColor);
}


////////////////////////////////////////////////////////////
void ImGuiContext::image(const Sprite& sprite, const Texture& texture, Vector2f size, Color tintColor, Color borderColor)
{
    const auto [uv0, uv1, textureID] = getSpriteTextureData(sprite, texture);
    ::ImGui::Image(textureID, toImVec2(size), uv0, uv1, toImColor(tintColor), toImColor(borderColor));
}


////////////////////////////////////////////////////////////
bool ImGuiContext::imageButton(const char* id, const Texture& texture, Vector2f size, Color bgColor, Color tintColor)
{
    const ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());
    return ::ImGui::ImageButton(id, textureID, toImVec2(size), ImVec2(0, 0), ImVec2(1, 1), toImColor(bgColor), toImColor(tintColor));
}


////////////////////////////////////////////////////////////
bool ImGuiContext::imageButton(const char* id, const RenderTexture& texture, Vector2f size, Color bgColor, Color tintColor)
{
    const ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getTexture().getNativeHandle());
    return ::ImGui::ImageButton(id, textureID, toImVec2(size), ImVec2(0, 0), ImVec2(1, 1), toImColor(bgColor), toImColor(tintColor));
}


////////////////////////////////////////////////////////////
bool ImGuiContext::imageButton(const char* id, const Sprite& sprite, const Texture& texture, Vector2f size, Color bgColor, Color tintColor)
{
    const auto [uv0, uv1, textureID] = getSpriteTextureData(sprite, texture);
    return ::ImGui::ImageButton(id, textureID, toImVec2(size), uv0, uv1, toImColor(bgColor), toImColor(tintColor));
}


////////////////////////////////////////////////////////////
void ImGuiContext::drawLine(Vector2f a, Vector2f b, Color color, float thickness)
{
    ImDrawList* const drawList = ::ImGui::GetWindowDrawList();
    SFML_BASE_ASSERT(drawList != nullptr);

    const ImVec2 pos = ::ImGui::GetCursorScreenPos();

    drawList->AddLine(ImVec2(a.x + pos.x, a.y + pos.y),
                      ImVec2(b.x + pos.x, b.y + pos.y),
                      ::ImGui::ColorConvertFloat4ToU32(toImColor(color)),
                      thickness);
}


////////////////////////////////////////////////////////////
void ImGuiContext::drawRect(const FloatRect& rect, Color color, float rounding, int roundingCorners, float thickness)
{
    ImDrawList* const drawList = ::ImGui::GetWindowDrawList();
    SFML_BASE_ASSERT(drawList != nullptr);

    drawList->AddRect(getTopLeftAbsolute(rect),
                      getDownRightAbsolute(rect),
                      ::ImGui::ColorConvertFloat4ToU32(toImColor(color)),
                      rounding,
                      roundingCorners,
                      thickness);
}


////////////////////////////////////////////////////////////
void ImGuiContext::drawRectFilled(const FloatRect& rect, Color color, float rounding, int roundingCorners)
{
    ImDrawList* const drawList = ::ImGui::GetWindowDrawList();
    SFML_BASE_ASSERT(drawList != nullptr);

    drawList->AddRectFilled(getTopLeftAbsolute(rect),
                            getDownRightAbsolute(rect),
                            ::ImGui::ColorConvertFloat4ToU32(toImColor(color)),
                            rounding,
                            roundingCorners);
}

} // namespace sf::ImGui
