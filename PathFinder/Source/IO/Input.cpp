#include "Input.hpp"

#include <type_traits>
#include <glm/gtc/constants.hpp>

static constexpr std::chrono::milliseconds ClickDetectionTime(200);

namespace PathFinder
{

    Input::SimpleMouseEvent &Input::GetSimpleMouseEvent()
    {
        return mSimpleMouseEvent;
    }

    Input::ScrollEvent &Input::GetScrollMouseEvent()
    {
        return mMouseScrollEvent;
    }


    Input::ClickEvent &Input::GetClickMouseEvent() {
        return mMouseClickEvent;
    }

    Input::KeyboardEvent &Input::GetKeyboardEvent() 
    {
        return mKeyboardEvent;
    }

    uint8_t Input::ClicksCount() const 
    {
        return mClickCount;
    }

    const glm::vec2 &Input::ScrollDelta() const 
    {
        return mScrollDelta;
    }

    const glm::vec2 &Input::MousePosition() const 
    {
        return mMousePosition;
    }

    const Input::KeyCode Input::PressedMouseButtonsMask() const 
    {
        return mPressedMouseButtonsMask;
    }

    const Input::KeySet &Input::PressedKeyboardButtons() const
    {
        return mPressedKeyboardKeys;
    }

    void Input::RegisterMouseAction(SimpleMouseAction action, const glm::vec2 &position, KeyCode keysMask)
    {
        using namespace std::chrono;

        auto now = steady_clock::now();
        milliseconds clickDuration = duration_cast<milliseconds>(now - mTimePoint);

        switch (action) {
            case SimpleMouseAction::PressDown: {
                if (clickDuration >= ClickDetectionTime) {
                    mClickCount = 0;
                }
                mTimePoint = steady_clock::now();
                break;
            }
            case SimpleMouseAction::PressUp: {
                if (clickDuration < ClickDetectionTime) {
                    mClickCount++;
                    mTimePoint = steady_clock::now();
                    mMouseClickEvent(this);
                }
                break;
            }

            default: {
                break;
            }
        }

        mMousePosition = position;
        mPressedMouseButtonsMask = keysMask;
        mSimpleMouseEvent[action](this);
    }

    void Input::RegisterMouseScroll(const glm::vec2 &delta) 
    {
        mScrollDelta = delta;
        mMouseScrollEvent(this);
    }

    void Input::RegisterKey(KeyCode code)
    {
        mPressedKeyboardKeys.insert(code);
        mKeyboardEvent[KeyboardAction::KeyDown](this);
    }

    void Input::UnregisterKey(KeyCode code)
    {
        mPressedKeyboardKeys.erase(code);
        mKeyboardEvent[KeyboardAction::KeyUp](this);
    }

    bool Input::IsKeyPressed(Key key) const 
    {
        using type = std::underlying_type<Key>::type;
        type rawKey = static_cast<type>(key);
        const auto &it = mPressedKeyboardKeys.find(rawKey);
        return it != mPressedKeyboardKeys.end();
    }

    bool Input::IsMouseButtonPressed(uint8_t button) const
    {
        return (1 << button) & mPressedMouseButtonsMask;
    }

}
