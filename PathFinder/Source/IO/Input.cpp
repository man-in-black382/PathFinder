#include "Input.hpp"

#include <type_traits>
#include <glm/gtc/constants.hpp>

#include <windows.h>
#include "../Foundation//StringUtils.hpp"

namespace PathFinder
{

    bool Input::IsKeyboardKeyPressed(KeyboardKey key, bool reportOnlyFreshPress) const
    {
        const auto &it = mCurrentFramePressedKeyboardKeys.find(key);
        bool pressedInCurrentFrame = it != mCurrentFramePressedKeyboardKeys.end();
        return reportOnlyFreshPress ? pressedInCurrentFrame && !WasKeyboardKeyPressedPrevously(key) : pressedInCurrentFrame;
    }

    bool Input::WasKeyboardKeyPressedPrevously(KeyboardKey key) const
    {
        const auto& it = mPreviousFramePressedKeyboardKeys.find(key);
        return it != mPreviousFramePressedKeyboardKeys.end();
    }

    bool Input::WasKeyboardKeyUnpressed(KeyboardKey key) const
    {
        return WasKeyboardKeyPressedPrevously(key) && !IsKeyboardKeyPressed(key, false);
    }

    bool Input::IsMouseButtonPressed(MouseButton button) const
    {
        return (1 << button) & mPressedMouseButtonsMask;
    }

    bool Input::IsAnyMouseButtonPressed() const
    {
        return mPressedMouseButtonsMask != 0;
    }

    bool Input::IsAnyKeyboardKeyPressed() const
    {
        return !mCurrentFramePressedKeyboardKeys.empty();
    }

    void Input::MouseDown(MouseButton buttonNumber)
    {
        mMouseDownTimeStamp = std::chrono::steady_clock::now();
        mPressedMouseButtonsMask |= 1 << buttonNumber;
    }

    void Input::MouseUp(MouseButton buttonNumber)
    {
        using namespace std::chrono;
        milliseconds clickDuration = duration_cast<milliseconds>(steady_clock::now() - mMouseDownTimeStamp);

        if (clickDuration < mClickDetectionTime)
        {
            ++mClickCountAccumulator;
        }
        else
        {
            mClickCountFinal = mClickCountAccumulator;
            mClickCountAccumulator = 0;
        }

        mPressedMouseButtonsMask &= ~(1 << buttonNumber);
    }

    void Input::SetMouseAbsolutePosition(const glm::vec2& position, bool originTopLeft)
    {
        mMousePosition = position;

        if (!originTopLeft)
        {
            mMousePosition.y = mWindowDimensions.Height - mMousePosition.y;
        }
    }

    void Input::SetScrollDelta(const glm::vec2& delta)
    {
        mScrollDelta = delta;
    }

    void Input::SetMouseDelta(const glm::vec2& delta)
    {
        mMouseDelta = delta;
        
        if (mInvertVerticalDelta)
        {
            mMouseDelta.y *= -1;
        }
    }

    void Input::SetInvertVerticalDelta(bool invert)
    {
        mInvertVerticalDelta = invert;
    }

    void Input::KeyboardKeyDown(KeyboardKey key)
    {
        mCurrentFramePressedKeyboardKeys.insert(key);

        if (!WasKeyboardKeyPressedPrevously(key))
        {
            mKeyDownEvent.Raise(key, this);
        }
    }

    void Input::KeyboardKeyUp(KeyboardKey key)
    {
        mCurrentFramePressedKeyboardKeys.erase(key);
        mKeyUpEvent.Raise(key, this);
    }

    void Input::BeginFrame()
    {
        mPreviousFramePressedKeyboardKeys = mCurrentFramePressedKeyboardKeys;
        mClickCountFinal = 0;
        mScrollDelta = glm::vec2{ 0.0f };
        mMouseDelta = glm::vec2{ 0.0f };
    }

    constexpr uint16_t RawKeyboardKey(KeyboardKey key)
    {
        return static_cast<std::underlying_type_t<KeyboardKey>>(key);
    }

}
