#pragma once

#include <glm/vec2.hpp>

#include <unordered_set>
#include <string>
#include <type_traits>
#include <chrono>

#include "../Foundation/Event.hpp"
#include "../Geometry/Dimensions.hpp"

namespace PathFinder
{

    enum class KeyboardKey : uint16_t
    {
        A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Escape, Space, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        Tilde,
        Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
        MinusUnderscore, PlusEquals, Backspace, Semicolon, Tab,
        BracketSquaredLeft, BracketSquaredRight, Enter, CapsLock,
        Slash, Backslash, Shift, Ctrl, Alt, SuperLeft, SuperRight,
        NumPad0, NumPad1, NumPad2, NumPad3, NumPad4, NumPad5,
        NumPad6, NumPad7, NumPad8, NumPad9,
        Left, Right, Up, Down,
        Insert, Home, End, Delete, PageUp, PageDown,
        END, BEGIN = A
    };

    constexpr uint16_t RawKeyboardKey(KeyboardKey key);

    /// A platform-independent input. 
    /// Use to receive input events.
    class Input 
    {
    public:
        using MouseButton = uint16_t;
        using MouseButtonMask = uint16_t;
        using KeyboardKeySet = std::unordered_set<KeyboardKey>;
        using KeyboardEvent = Foundation::Event<Input, std::string, void(KeyboardKey, const Input*)>;

        Input() = default;
        ~Input() = default;
        Input(Input&& that) = default;
        Input& operator=(Input&& rhs) = default;
        Input(const Input& that) = delete;
        Input& operator=(const Input& rhs) = delete;

        bool IsKeyboardKeyPressed(KeyboardKey key, bool reportOnlyFreshPress = false) const;
        bool WasKeyboardKeyPressedPrevously(KeyboardKey key) const;
        bool WasKeyboardKeyUnpressed(KeyboardKey key) const;
        bool IsMouseButtonPressed(MouseButton button) const;
        bool IsAnyMouseButtonPressed() const;
        bool IsAnyKeyboardKeyPressed() const;

        // To be called from platform-specific input handlers
        // ------------------------------------------------ //
        void MouseDown(MouseButton buttonNumber);
        void MouseUp(MouseButton buttonNumber);
        void SetMouseAbsolutePosition(const glm::vec2& position, bool originTopLeft);
        void SetScrollDelta(const glm::vec2& delta);
        void SetMouseDelta(const glm::vec2& delta);
        void SetInvertVerticalDelta(bool invert);
        void KeyboardKeyDown(KeyboardKey key);
        void KeyboardKeyUp(KeyboardKey key);
        void BeginFrame();
        // ------------------------------------------------ //

    private:
        std::chrono::milliseconds mClickDetectionTime = std::chrono::milliseconds{ 200 };
        uint8_t mClickCountAccumulator = 0; // Accumulates clicks until no more clicks are performed in detection time
        uint8_t mClickCountFinal = 0; // Final detected amount of clicks
        glm::vec2 mScrollDelta;
        glm::vec2 mMouseDelta;
        glm::vec2 mMousePosition;
        KeyboardKeySet mCurrentFramePressedKeyboardKeys;
        KeyboardKeySet mPreviousFramePressedKeyboardKeys;
        MouseButtonMask mPressedMouseButtonsMask;
        bool mInvertVerticalDelta = false;
        std::chrono::time_point<std::chrono::steady_clock> mMouseDownTimeStamp;
        Geometry::Dimensions mWindowDimensions = { 1, 1 };

        KeyboardEvent mKeyDownEvent;
        KeyboardEvent mKeyUpEvent;

    public:
        inline auto CurrentClickCount() const { return mClickCountFinal; }
        inline const glm::vec2& ScrollDelta() const { return mScrollDelta; }
        inline const glm::vec2& MouseDelta() const { return mMouseDelta; }
        inline const glm::vec2& MousePosition() const { return mMousePosition; }
        inline MouseButtonMask PressedMouseButtonsMask() const { return mPressedMouseButtonsMask; }
        inline const KeyboardKeySet& PressedKeyboardButtons() const { return mCurrentFramePressedKeyboardKeys; }

        inline KeyboardEvent& KeyDownEvent() { return mKeyDownEvent; }
        inline KeyboardEvent& KeyUpEvent() { return mKeyUpEvent; }
    };

}
