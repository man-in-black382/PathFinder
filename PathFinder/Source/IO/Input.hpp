#pragma once

#include <glm/vec2.hpp>

#include <unordered_set>
#include <string>
#include <type_traits>
#include <chrono>

#include "../Foundation/Event.hpp"

namespace PathFinder {

    class Input {
    public:
        enum class KeyboardAction {
            KeyDown, KeyUp
        };

        enum class SimpleMouseAction {
            PressDown, PressUp, Drag, Move
        };

        using KeyCode = uint16_t;
        using KeySet = std::unordered_set<KeyCode>;
        using KeyboardEvent = Foundation::MultiEvent<Input, KeyboardAction, std::string, void(const Input *)>;
        using SimpleMouseEvent = Foundation::MultiEvent<Input, SimpleMouseAction, std::string, void(const Input *)>;
        using ScrollEvent = Foundation::Event<Input, std::string, void(const Input *)>;
        using ClickEvent = Foundation::Event<Input, std::string, void(const Input *)>;

        enum class Key : KeyCode {
            W = 13, S = 1, A = 0, D = 2
        };

    private:
        SimpleMouseEvent mSimpleMouseEvent;
        ScrollEvent mMouseScrollEvent;
        ClickEvent mMouseClickEvent;
        KeyboardEvent mKeyboardEvent;

        uint8_t mClickCount;
        glm::vec2 mScrollDelta;
        glm::vec2 mMousePosition;
        KeySet mPressedKeyboardKeys;
        KeyCode mPressedMouseButtonsMask;
        std::chrono::time_point<std::chrono::steady_clock> mTimePoint;

        Input() = default;

        ~Input() = default;

        Input(const Input &that) = delete;

        Input &operator=(const Input &rhs) = delete;

    public:
        static Input &shared();

        SimpleMouseEvent &simpleMouseEvent();

        ScrollEvent &scrollMouseEvent();

        ClickEvent &clickMouseEvent();

        KeyboardEvent &keyboardEvent();

        uint8_t clicksCount() const;

        const glm::vec2 &scrollDelta() const;

        const glm::vec2 &mousePosition() const;

        const KeyCode pressedMouseButtonsMask() const;

        const KeySet &pressedKeyboardButtons() const;

        void registerMouseAction(SimpleMouseAction action, const glm::vec2 &position, KeyCode keysMask);

        void registerMouseScroll(const glm::vec2 &delta);

        void registerKey(KeyCode code);

        void unregisterKey(KeyCode code);

        bool isKeyPressed(Key key) const;

        bool isMouseButtonPressed(uint8_t button) const;
    };

}
