#pragma once

#include <glm/vec2.hpp>

#include <unordered_set>
#include <string>
#include <type_traits>
#include <chrono>

#include "../Foundation/Event.hpp"

namespace PathFinder
{

    class Input 
    {
    public:
        enum class KeyboardAction
        {
            KeyDown, KeyUp
        };

        enum class SimpleMouseAction
        {
            PressDown, PressUp, Drag, Move
        };

        using KeyCode = uint16_t;
        using KeySet = std::unordered_set<KeyCode>;
        using KeyboardEvent = Foundation::MultiEvent<Input, KeyboardAction, std::string, void(const Input*)>;
        using SimpleMouseEvent = Foundation::MultiEvent<Input, SimpleMouseAction, std::string, void(const Input*)>;
        using ScrollEvent = Foundation::Event<Input, std::string, void(const Input*)>;
        using ClickEvent = Foundation::Event<Input, std::string, void(const Input*)>;

        enum class Key : KeyCode
        {
            W = 13, S = 1, A = 0, D = 2
        };

        Input() = default;
        ~Input() = default;
        Input(Input&& that) = default;
        Input& operator=(Input&& rhs) = default;
        Input(const Input& that) = delete;
        Input& operator=(const Input& rhs) = delete;

        SimpleMouseEvent& GetSimpleMouseEvent();
        ScrollEvent& GetScrollMouseEvent();
        ClickEvent& GetClickMouseEvent();
        KeyboardEvent& GetKeyboardEvent();

        uint8_t ClicksCount() const;
        const glm::vec2& ScrollDelta() const;
        const glm::vec2& MousePosition() const;
        const KeyCode PressedMouseButtonsMask() const;
        const KeySet& PressedKeyboardButtons() const;
        void RegisterMouseAction(SimpleMouseAction action, const glm::vec2& position, KeyCode keysMask);
        void RegisterMouseScroll(const glm::vec2& delta);
        void RegisterKey(KeyCode code);
        void UnregisterKey(KeyCode code);
        bool IsKeyPressed(Key key) const;
        bool IsMouseButtonPressed(uint8_t button) const;

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
    };

}
